/*
    Copyright (C) 2006 by OXIM TEAM

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/      


#include "oximint.h"
#include "gencin.h"

/*----------------------------------------------------------------------------

	Dynamic module loading:

	For ELF systems (Solaris, GNU/Linux, FreeBSD, HP-UX.11)
	For HP-UX PA32, PA64 systems.

----------------------------------------------------------------------------*/

#ifdef	HAVE_DLOPEN			/* ELF */
#include <dlfcn.h>
#else
#ifdef	HAVE_SHL_LOAD			/* HP-UX */
#include <dl.h>
#endif
#endif

#include <locale.h>

struct _mod_stack_s {
   void *ld;
   mod_header_t *modp;
   int ref;
   struct _mod_stack_s *next;
};

struct _mod_stack_s *mod_stack = NULL;

char *realCname(char *s, char *e);

static int virtual_keyboard = False;

void
set_virtual_keyboard(flag)
{
    virtual_keyboard = flag;
}

int
is_virtual_keyboard()
{
    return virtual_keyboard;
}

/*----------------------------------------------------------------------------

	Load and Unload modules.

----------------------------------------------------------------------------*/

static int
find_module(char *path, int path_size, char *sub_path)
{
    char fn[BUFFER_LEN];

    if (oxim_check_datafile(path, sub_path, fn, BUFFER_LEN) == False)
	return False;

    strcpy(path, fn);
    return oxim_check_file_exist(path, FTYPE_FILE);
}

mod_header_t *
load_module(char *modname, enum mtype mod_type, char *version, char *sub_path)
{
    struct _mod_stack_s *ms=mod_stack;
    char ldfn[BUFFER_LEN];
    mod_header_t *module;
    void *ld=NULL;
    int err=1;

    while (ms != NULL) {
	if (strcmp(modname, ms->modp->name) == 0) {
	    ms->ref ++;
	    return ms->modp;
	}
	ms = ms->next;
    }

    sprintf(ldfn, "%s.so", modname);
    if (find_module(ldfn, BUFFER_LEN, sub_path) == True &&
#ifdef	HAVE_DLOPEN
        (ld = dlopen(ldfn, RTLD_LAZY)) != NULL)
#else
#ifdef	HAVE_SHL_LOAD
        (ld = (void *)shl_load(ldfn, BIND_DEFERRED, 0L)) != NULL)
#endif
#endif
	err = 0;

    if (err) {
#ifdef	HAVE_DLOPEN
	char *errstr = dlerror();
        oxim_perr(OXIMMSG_IWARNING, N_("dlerror: %s\n"), errstr);
#endif
	ld = NULL;
    }

#ifdef	HAVE_DLOPEN
    if (!err && !(module = (mod_header_t *)dlsym(ld, "module_ptr")))
#else
#ifdef	HAVE_SHL_LOAD
    if (!err && shl_findsym((shl_t *)&ld, "module_ptr", TYPE_DATA, &module)!=0)
#endif
#endif
    {
	oxim_perr(OXIMMSG_IWARNING, N_("module symbol \"module_ptr\" not found.\n"));
	err = 1;
    }
    if (!err && module->module_type != mod_type) {
	oxim_perr(OXIMMSG_IWARNING,
	     N_("invalid module type, type %d required.\n"), mod_type);
	err = 1;
    }
    if (!err && strcmp(module->version, version) != 0) {
	oxim_perr(OXIMMSG_IWARNING,
	     N_("invalid module version: %s, version %s required.\n"),
	     module->version, version);
    }

    if (err) {
        oxim_perr(OXIMMSG_WARNING,
	     N_("cannot load module \"%s\", ignore.\n"), modname);
	if (ld)
#ifdef HAVE_DLOPEN
	    dlclose(ld);
#else
#ifdef HAVE_SHL_OPEN
	    shl_unload((shl_t)ld);
#endif
#endif
	return NULL;
    }
    else {
	ms = oxim_malloc(sizeof(struct _mod_stack_s), 0);
	ms->ld = ld;
	ms->modp = module;
	ms->ref = 1;
	ms->next = mod_stack;
	mod_stack = ms;
	return module;
    }
}

void
unload_module(mod_header_t *modp)
{
    struct _mod_stack_s *ms = mod_stack;

    while (ms != NULL) {
	if (modp == ms->modp) {
	    ms->ref --;
	    break;
	}
	ms = ms->next;
    }
    if (ms && ms->ref <= 0) {
#ifdef	HAVE_DLOPEN
	dlclose(ms->ld);
#else
#ifdef	HAVE_SHL_LOAD
	shl_unload((shl_t)(ms->ld));
#endif
#endif
	mod_stack = ms->next;
	free(ms);
    }
}

/*----------------------------------------------------------------------------

	Print the module comment.

----------------------------------------------------------------------------*/

void
module_comment(mod_header_t *modp, char *mod_name)
{
    if (modp) {
	oxim_perr(OXIMMSG_NORMAL, N_("module \"%s\":"), mod_name);
	if (modp->comments)
	    oxim_perr(OXIMMSG_EMPTY, "\n\n%s\n", N_(modp->comments));
	else
	    oxim_perr(OXIMMSG_EMPTY, N_("no comments available.\n"));
    }
}

/*---------------------------------------------------------------------------

        Load Module & Module Init.

---------------------------------------------------------------------------*/

static int
check_module(module_t *modp, char *objname)
{
    char *str = NULL, **objn;
    int check_ok=0;

    if (modp == NULL)
	return False;
/*
 *  Check for necessary symbols.
 */
    if (! modp->conf_size)
	str = "conf_size";
    else if (! modp->init)
	str = "init";
    else if (! modp->xim_init)
	str = "xim_init";
    else if (! modp->xim_end)
	str = "xim_end";
    else if (! modp->keystroke)
	str = "keystroke";
    else if (! modp->show_keystroke)
	str = "show_keystroke";
    if (str) {
	oxim_perr(OXIMMSG_IWARNING,
	    N_("undefined symbol \"%s\" in module \"%s\", ignore.\n"),
	    str, modp->module_header.name);
	return  False;
    }
/*
 *  Check for the valid objname.
 */
    objn = modp->valid_objname;
    if (!objn) {
	if (strcmp_wild(modp->module_header.name, objname) == 0)
	    check_ok = 1;
    }
    else {
	while (*objn) {
	    if(strcmp_wild(*objn, objname) == 0) {
		check_ok = 1;
		break;
	    }
	    objn ++;
	}
    }
    if (check_ok == 0) {
	oxim_perr(OXIMMSG_WARNING,
	    N_("invalid objname \"%s\" for module \"%s\", ignore.\n"),
	    objname, modp->module_header.name);
	return False;
    }
    return True;
}

static imodule_t *
creat_module(module_t *templet, char *objname)
{
    imodule_t *imodp;

    imodp = oxim_malloc(sizeof(imodule_t), 1);
    imodp->modp = (void *)templet;
    imodp->name = templet->module_header.name;
    imodp->version = templet->module_header.version;
    imodp->comments = templet->module_header.comments;
    imodp->module_type = templet->module_header.module_type;
    imodp->conf = oxim_malloc(templet->conf_size, 1);
    imodp->init = templet->init;
    imodp->xim_init = templet->xim_init;
    imodp->xim_end = templet->xim_end;
    imodp->keystroke = templet->keystroke;
    imodp->show_keystroke = templet->show_keystroke;
    imodp->terminate = templet->terminate;

    imodp->objname = (objname) ? (char *)strdup(objname) : imodp->name; 

    return imodp;
}

static imodule_t *
IM_load(char *modname, char *objname)
{
    module_t *modp;
    imodule_t *imodp;
    int load_ok=1;

    modp = (module_t *)load_module(modname, MTYPE_IM,MODULE_VERSION, "modules");
    if (check_module(modp, objname) == False)
	load_ok = 0;
    else {
	imodp = creat_module(modp, objname);
	if (imodp->init(imodp->conf, objname) != True) {
	    free(imodp->conf);
	    free(imodp);
	    load_ok = 0;
	}
    }

    if (load_ok == 0) {
	oxim_perr(OXIMMSG_WARNING,
	    N_("cannot load IM: %s, ignore.\n"), objname);
	unload_module((mod_header_t *)modp);
	return NULL;
    }
    return imodp;
}

/*----------------------------------------------------------------------------

	Cinput structer manager.

----------------------------------------------------------------------------*/
static void OXIM_IMFree(int idx)
{
    im_rc *imlist = &(_Config->sys_imlist);

    if (idx < 0 || idx >= imlist->NumberOfIM)
	return;

    im_t *imp = imlist->IMList[idx];

    if (imp->inpname)
    {
	free(imp->inpname);
	imp->inpname = NULL;
    }
    if (imp->aliasname)
    {
	free(imp->aliasname);
	imp->aliasname = NULL;
    }
    if (imp->modname)
    {
	free(imp->modname);
	imp->modname = NULL;
    }
    if (imp->objname)
    {
	free(imp->objname);
	imp->objname = NULL;
    }

    if (imp->settings)
    {
	oxim_settings_destroy(imp->settings);
	imp->settings = NULL;
    }

    if (imp->imodp)
    {
	if (imp->imodp->terminate)
	    imp->imodp->terminate(imp->imodp->conf);
	if (imp->imodp->modp)
	    unload_module((mod_header_t *)imp->imodp->modp);
	if (imp->imodp->conf)
	    free(imp->imodp->conf);
	free(imp->imodp);
	imp->imodp = NULL;
    }
    imp->key = -1;
}

imodule_t *
OXIM_IMGet(int idx)
{
    im_rc imlist = _Config->sys_imlist;
    if (idx < 0 || idx >= imlist.NumberOfIM)
	return NULL;

    im_t *imp = imlist.IMList[idx];

    if (imp->modname && imp->objname && imp->imodp == NULL)
	imp->imodp = IM_load(imp->modname, imp->objname);

//     if (imp->imodp == NULL)
// 	OXIM_IMFree(idx);

    return imp->imodp;
}

imodule_t *
OXIM_IMGetNext(int idx, int *idx_ret)
{
    im_rc imlist = _Config->sys_imlist;

    if (idx < 0 || idx >= imlist.NumberOfIM)
	return NULL;

    int i, j;
    im_t *imp;

    *idx_ret = -1;

    for (j = 0, i = idx ; j < imlist.NumberOfIM ; j++, i++)
    {
	if (i >= imlist.NumberOfIM)
	{
	    i = 0;
	}
	imp = imlist.IMList[i];

	if (imp->modname && imp->objname)
	{
	    if (imp->imodp == NULL)
	    {
		imp->imodp = IM_load(imp->modname, imp->objname);
		if (imp->imodp != NULL )
		{
		    *idx_ret = i;
		    break;
		}
	    }
	}
    }
    if (imp->imodp == NULL)
	OXIM_IMFree(*idx_ret);
    return imp->imodp;
}

imodule_t *
OXIM_IMGetPrev(int idx, int *idx_ret)
{
    im_rc imlist = _Config->sys_imlist;

    if (idx < 0 || idx >= imlist.NumberOfIM)
	return NULL;

    int i, j;
    im_t *imp;
    *idx_ret = -1;

    for (j = 0, i = idx ; j < imlist.NumberOfIM ; j++, i--)
    {
	if (i < 0)
	{
	    i = imlist.NumberOfIM - 1;
	}
	imp = imlist.IMList[i];

	if (imp->modname && imp->objname) {
	    *idx_ret = i;
	    break;
	}
    }
    if (*idx_ret != -1 && imp->modname && imp->objname && imp->imodp == NULL)
	imp->imodp = IM_load(imp->modname, imp->objname);
    if (imp->imodp == NULL)
	OXIM_IMFree(*idx_ret);
    return imp->imodp;
}

imodule_t *
OXIM_IMSearch(char *objname, int *idx_ret)
{
    int i;
    im_rc imlist = _Config->sys_imlist;
    im_t *imp;

    *idx_ret = -1;
    for (i = 0 ; i < imlist.NumberOfIM; i++)
    {
	imp = imlist.IMList[i];
	if (imp->objname && strcmp(imp->objname, objname) == 0)
	{
	    *idx_ret = i;
	    break;
	}
    }

    if (*idx_ret != -1 && imp->modname && imp->objname && imp->imodp == NULL)
	imp->imodp = IM_load(imp->modname, imp->objname);
    if (imp->imodp == NULL)
	OXIM_IMFree(*idx_ret);
    return imp->imodp;
}

void
OXIM_IMFreeAll(void)
{
    im_rc *imlist = &(_Config->sys_imlist);
    if (!imlist->IMList)
	return;
 
    int i;
    for (i = 0; i < imlist->NumberOfIM ; i++)
    {
	OXIM_IMFree(i);
	free(imlist->IMList[i]);
    }

    free(imlist->IMList);
    imlist->NumberOfIM = 0;
    imlist->IMList = NULL;
}

/*-------------------------------------------------------------------
  掃描使用者自訂之輸入法表格與輸入法模組
  順序為 使用者 > 系統
  Add By Firefly(firefly@opendesktop.org.tw)
-------------------------------------------------------------------*/
static void OXIM_IMRegister(im_rc *imlist, char *modname, char *objname, char *cname, char *path)
{
    int i;
    im_t *im = NULL;

    for (i = 0; i < imlist->NumberOfIM ; i++)
    {
	/* 找出第一個可用的位置 */
	if (im == NULL && !imlist->IMList[i]->objname)
	    im = imlist->IMList[i];

	/* 重複就不紀錄了 */
	if (strcmp(imlist->IMList[i]->objname, objname) == 0)
	    return;
    }

    /* 沒有可用位置, 那就配置一個空間 */
    if (im == NULL)
    {
        imlist->NumberOfIM++;
	im = (im_t *)oxim_malloc(sizeof(im_t), True);
	if (imlist->NumberOfIM == 1)
	{
	    imlist->IMList = (im_t **)oxim_malloc(sizeof(im_t *), True);
	}
	else
	{
	    imlist->IMList = (im_t **)oxim_realloc(imlist->IMList, imlist->NumberOfIM * sizeof(im_t *));
	}
	imlist->IMList[imlist->NumberOfIM - 1] = im;
    }
    im->key = -1;
    im->circular = True; /* 預設加入輪切 */
    im->inpname = strdup((cname ? cname : "No Name"));
    im->aliasname = NULL;
    im->modname = strdup(modname);
    im->objname = strdup(objname);
    int user_dir_len = strlen(_Config->user_dir);
    im->inuserdir = (strncmp(path, _Config->user_dir, user_dir_len) == 0);

    settings_t *im_settings = oxim_get_im_settings(objname);
    if (!im_settings)
	return;

    /* 將該輸入法表格的其餘相關設定寫入記憶體相應位置(im->settings) */
    oxim_set_IMSettings(imlist->NumberOfIM - 1, im_settings);

    int setkey;
    if (oxim_setting_GetInteger(im_settings, "SetKey", &setkey))
    {
	if (setkey >= 1 && setkey <= 9)
	    im->key = setkey;
	else if (setkey == 0)
	    im->key = 10;
	else
	    im->key = -1;
    }

    int circular;
    if (oxim_setting_GetBoolean(im_settings, "Circular", &circular))
    {
	im->circular = circular;
    }

    char *AliasName;
    if (oxim_setting_GetString(im_settings, "AliasName", &AliasName))
    {
	if (strlen(AliasName))
	    im->aliasname = strdup(AliasName);
    }
    oxim_settings_destroy(im_settings);
}

/* 檢查 tab 檔是否合格 */
int oxim_CheckTable(char *path, char *name, char *ret_cname, int *ret_ver)
{
    char *fullpath = oxim_malloc(BUFFER_LEN, True);
    gzFile *fp;
    int valid = True;

    /* 檔案完整路徑 */
    sprintf(fullpath, "%s/%s", path, name);

    /* 開檔 */
    if ((fp = gzopen(fullpath, "rb")) != NULL)
    {
	table_prefix_t tp;
    	cintab_head_t tab_hd;
	cintab_head_v1_t tab_v1_hd;
	int read_size, hd_size;
	int size = sizeof(table_prefix_t);

	if (gzread(fp , &tp, size) == size && strcmp(tp.prefix, "gencin") == 0)
	{
	    switch (tp.version)
	    {
		case 0:
		    hd_size = sizeof(cintab_head_t);
		    read_size = gzread(fp, &tab_hd, hd_size);
		    break;
		case 1:
		    hd_size = sizeof(cintab_head_v1_t);
		    read_size = gzread(fp, &tab_v1_hd, hd_size);
		    break;
		default:
		    valid = False;
	    }

	    /* 檔頭 */
	    if (valid && read_size == hd_size)
	    {
		switch (tp.version)
		{
		    case 0:
			if (strcmp(GENCIN_VERSION, tab_hd.version) != 0)
			{
			    valid = False;
			}
			else
			{
			    if (ret_cname)
			    {
				strcpy(ret_cname, tab_hd.cname);
			    }
			    if (ret_ver)
			    {
				*ret_ver = 0;
			    }
			}
			break;
		    case 1:
			{
			    unsigned int checksum = crc32(0L,  (Bytef *)&tab_v1_hd, sizeof(cintab_head_v1_t) - sizeof(unsigned int));
			    if (checksum == tab_v1_hd.chksum)
			    {
				if (ret_cname)
				{
				    strcpy(ret_cname, realCname(tab_v1_hd.cname, tab_v1_hd.orig_name));
// 				    strcpy(ret_cname, tab_v1_hd.cname);
				}
				if (ret_ver)
				{
				    *ret_ver = 1;
				}
			    }
			    else
			    {
				valid = False;
			    }
			}
			break;
		    default:
			valid = False;
		}
	    }
	}
	else
	{
	    valid = False;
	}
	gzclose(fp);
    }
    else
	valid = False;

    free(fullpath);
    return valid;
}

/**/
char *realCname(char *c_name, char *e_name)
{
	char delims[] = ";";
	char *result = NULL;
	char loc[BUFFER_LEN];
	strcpy(loc, setlocale(LC_MESSAGES, ""));

	/*檢查有沒有包含冒號，若沒有應該是舊的中文名稱*/
	if( NULL == strchr(e_name, delims[0]) )
	{
		return 
			0==strncmp(loc, "zh_", 3) ?
				c_name :  /*若locale開頭=『zh_』則傳回中文名稱*/
				e_name;	/*否則傳回英文名稱*/
	}

	char loc_comp[3][BUFFER_LEN];
	char ename[BUFFER_LEN];
	char *ptr;
	/*將包含『.』以後的字串予以清除*/
	if( NULL != (ptr=strchr(loc, '.')) )
	{
		ptr[0] = '\0';
	}
	strcpy(loc_comp[0], loc);
	if( NULL != (ptr=strchr(loc, '_')) )	/*locale包含『_』才進行比對*/
	{
		ptr[0] = '\0';	/*清除包括『_』之後的字元*/
		strcpy(loc_comp[1], loc);
	}
	strcpy(loc_comp[2], "en");

	int i, j, k;
	char *tmp = NULL;
	char split[] = ":";
	char *saveptr1, *saveptr2;
	/*check for zh_TW、zh*/
	for( k=0; k<3; k++)
	{
		strcpy(ename, e_name);
		for( i=0, result = strtok_r( ename, delims, &saveptr1 ); result != NULL; i++, result = strtok_r( NULL, delims, &saveptr1 ) )
		{
			char *data[2];
			for( j=0, tmp = strtok_r( result, split, &saveptr2 ); tmp!=NULL; j++, tmp = strtok_r( NULL, split, &saveptr2 ) )
			{
				data[j] = tmp;
			}
			if( 0 == strcmp(data[1], loc_comp[k]) )
			{
				return data[0];
			}
		}
	}
	return c_name;
}

static void ScanDirRegisterIM(im_rc *imlist, char *path, char *extname)
{
    struct dirent **namelist;
    struct stat stat;
    const unsigned int ext_len = strlen(extname);
    int n;
    char cname[256];
    char pwd[256];
    
    getcwd(pwd, 256);

    if (chdir(path) == 0)
    {
	n = scandir(path, &namelist, 0, alphasort);
	if (n < 0) /* 錯誤 */
	{
	    oxim_perr(OXIMMSG_IWARNING, N_("Scan dir: \"%s\" No any files!\n"), path);
	}
	else
	{
	    while (n--)
	    {
		char *name = namelist[n]->d_name;
		unsigned int name_len;
		lstat(name, &stat);
		/* 一般檔案或 Symbolic link */
		if ((name_len = strlen(name)) > ext_len &&
		    strncmp(name, "gen-inp", 7) != 0 &&
		   (S_ISREG(stat.st_mode) || S_ISLNK(stat.st_mode)))
		{
		    /* 是否符合副檔名 */
		    if (strcmp(name + name_len - ext_len, extname) == 0)
		    {
			char objname[256];
			strcpy(objname, name);
			objname[name_len - ext_len] = '\0';

			/* 檢查表格或模組是否正確 */
			if (strcmp(extname, ".tab") == 0)
			{
			    int version = 0;
			    if (oxim_CheckTable(path, name, cname, &version))
			    {
				OXIM_IMRegister(imlist, (version == 1 ? "gen-inp-v1" : "gen-inp"), objname, cname, path);
			    }
			}

			if (strcmp(extname, ".so") == 0)
			{
			    module_t *modp = (module_t *)load_module(objname, MTYPE_IM, MODULE_VERSION, "modules");
			    if (modp)
			    {
				OXIM_IMRegister(imlist, objname, objname, gettext(modp->module_header.name), path);
				unload_module((mod_header_t *)modp);
			    }
			}
		    }
		}
		free(namelist[n]);
	    }
	    free(namelist);
	}
    }
    else
    {
	oxim_perr(OXIMMSG_IWARNING, N_("Can not change dir to \"%s\"\n"), path);
    }
    chdir(pwd);
}

/* 取得輸入法總數 */
int oxim_get_NumberOfIM(void)
{
    return _Config->sys_imlist.NumberOfIM;
}

/* 以索引取得輸入法顯示名稱與 key*/
iminfo_t *oxim_get_IM(int idx)
{
    if (idx < 0 || idx >= _Config->sys_imlist.NumberOfIM)
	return NULL;

    return (iminfo_t *)_Config->sys_imlist.IMList[idx];
}

/* 以索引取得輸入法顯示名稱與 key*/
im_t *oxim_get_IMByIndex(int idx)
{
    if (idx < 0 || idx >= _Config->sys_imlist.NumberOfIM)
	return NULL;

    return _Config->sys_imlist.IMList[idx];
}

/* 設定輸入法快速鍵 */
int oxim_set_IMKey(int idx, int key)
{
    im_rc *imlist = &_Config->sys_imlist;
    if (idx < 0 || idx >= imlist->NumberOfIM)
	return True;

    /* key < 1 表示取消快速鍵 */
    if (key < 0)
    {
	imlist->IMList[idx]->key = -1;
	return True;
    }

    /* 檢查有無重複 */
    int i;
    for (i=0 ; i < imlist->NumberOfIM ; i++)
    {
	if (imlist->IMList[i]->key == key && i != idx)
	    return False;
    }
    imlist->IMList[idx]->key = key;
    return True;
}

/* 設定輸入法別名 */
int oxim_set_IMAliasName(int idx, const char *alias)
{
    im_rc *imlist = &_Config->sys_imlist;
    if (idx < 0 || idx >= imlist->NumberOfIM)
	return False;

    if (imlist->IMList[idx]->aliasname)
    {
	free(imlist->IMList[idx]->aliasname);
    }

    if (alias)
	imlist->IMList[idx]->aliasname = strdup(alias);
    else
	imlist->IMList[idx]->aliasname = NULL;

    return True;
}

/* 設定輸入法設定 */
int oxim_set_IMSettings(int idx, settings_t *settings)
{
    im_rc *imlist = &_Config->sys_imlist;
    if (idx < 0 || idx >= imlist->NumberOfIM || !settings)
	return False;

    if (imlist->IMList[idx]->settings)
    {
	oxim_settings_destroy(imlist->IMList[idx]->settings);
    }
    imlist->IMList[idx]->settings = oxim_settings_create();
    int i;
    for (i = 0 ; i < settings->nsetting ; i++)
    {
	oxim_settings_add_key_value(imlist->IMList[idx]->settings,
		settings->setting[i]->key, settings->setting[i]->value);
    }
    return True;
}

int oxim_set_IMCircular(int idx, int OnOff)
{
    im_rc *imlist = &_Config->sys_imlist;
    if (idx < 0 || idx >= imlist->NumberOfIM)
	return False;

    imlist->IMList[idx]->circular = OnOff;
    return True;
}

/* 查詢某輸入法是否可 Circular */
int oxim_IMisCircular(int idx)
{
    im_rc *imlist = &_Config->sys_imlist;
    if (idx < 0 || idx >= imlist->NumberOfIM)
	return False;

    return imlist->IMList[idx]->circular;
}

/* 以按鍵碼取得輸入法索引值 */
int oxim_get_IMIdxByKey(int key)
{
    int idx;
    for (idx = 0 ; idx < _Config->sys_imlist.NumberOfIM ; idx++)
    {
	if (_Config->sys_imlist.IMList[idx]->key == key)
	    return idx;
    }
    return -1;
}

/* 取得預設輸入法索引編號 */
int oxim_get_Default_IM(void)
{
    im_rc *imlist = &_Config->sys_imlist;

    /* 預設輸入法 */
    int i;
    char *defim = oxim_get_config(DefauleInputMethod);
    if (defim)
    {
	for (i = 0; i < imlist->NumberOfIM ; i++)
	{
	    if(!imlist->IMList[i]->objname)
		continue;
	    if (strcasecmp(imlist->IMList[i]->objname, defim) == 0)
	    {
		return i;
	    }
	}
    }
    return 0;
}

void OXIM_LoadIMList(void)
{
    im_rc *imlist = &_Config->sys_imlist;
    char *tmp = oxim_malloc(BUFFER_LEN, False);

    sprintf(tmp, "%s/tables", _Config->user_dir);
    ScanDirRegisterIM(imlist, tmp, ".tab");

    sprintf(tmp, "%s/tables", _Config->default_dir);
    ScanDirRegisterIM(imlist, tmp, ".tab");

    sprintf(tmp, "%s/modules", _Config->user_dir);
    ScanDirRegisterIM(imlist, tmp, ".so");

    sprintf(tmp, "%s/modules", _Config->default_dir);
    ScanDirRegisterIM(imlist, tmp, ".so");

    if (imlist->NumberOfIM > 1)
    {
	int i, j;
	im_t *imp;
	/* 依照按鍵順序，排序輸入法 */
	for (i = 0; i < imlist->NumberOfIM ; i++)
	{
	    for (j = i + 1 ; j < imlist->NumberOfIM ; j++)
	    {
		if (imlist->IMList[i]->key > imlist->IMList[j]->key)
		{
		    imp = imlist->IMList[i];
		    imlist->IMList[i] = imlist->IMList[j];
		    imlist->IMList[j] = imp;
		}
	    }
	}

    }
    free(tmp);
}
