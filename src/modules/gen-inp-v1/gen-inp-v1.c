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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include "oximtool.h"
#include "module.h"
#include "gen-inp-v1.h"

static char *bopomofo[] = {"ㄅㄆㄇㄈㄉㄊㄋㄌㄍㄎㄏㄐㄑㄒㄓㄔㄕㄖㄗㄘㄙ", /* 聲母 */
		      "ㄧㄨㄩ",
		      "ㄚㄛㄜㄝㄞㄟㄠㄡㄢㄣㄤㄥㄦ" /* 韻母 */};

static int fillpage(gen_inp_conf_t *cf, inpinfo_t *inpinfo, gen_inp_iccf_t *iccf, byte_t dir);

static void gen_inp_resource(gen_inp_conf_t *cf, settings_t *im_settings);
/*----------------------------------------------------------------------------

        gen_inp_init()

----------------------------------------------------------------------------*/

static int
loadtab(gen_inp_conf_t *cf, char *objname)
{
    table_prefix_t tp;
    unsigned int size = MODULE_ID_SIZE;
    cf->memory_usage = sizeof(gen_inp_conf_t);

    if (gzread(cf->zfp , &tp, size) != size || strcmp(tp.prefix, "gencin") || tp.version != 1)
    {
	oxim_perr(OXIMMSG_WARNING, N_("gen_inp: %s: invalid tab file.\n"),cf->tabfn);
	return False;
    }

    size = sizeof(cintab_head_v1_t);
    cf->memory_usage += size;
    if (gzread(cf->zfp, &(cf->header), size) != size)
    {
	oxim_perr(OXIMMSG_WARNING, N_("gen_inp: %s: reading error.\n"), cf->tabfn);
	return False;
    }

    unsigned int checksum = crc32(0L, (char *)&cf->header, sizeof(cintab_head_v1_t) - sizeof(unsigned int));
    if (cf->header.chksum != checksum)
    {
	oxim_perr(OXIMMSG_WARNING, N_("gen_inp: %s: checksum error.\n"), cf->tabfn);
	return False;
    }

    settings_t *im_settings;
    /* 讀取輸入法表格預設的定義 */
    im_settings = oxim_get_default_settings(objname, True);
    if (im_settings)
    {
	gen_inp_resource(cf, im_settings);
	oxim_settings_destroy(im_settings);
    }

    /* 讀取使用者的定義 */
    im_settings = oxim_get_im_settings(objname);
    if (im_settings)
    {
	gen_inp_resource(cf, im_settings);
	oxim_settings_destroy(im_settings);
    }
    return True;
}

static void
setup_kremap(gen_inp_conf_t *cf, char *value)
{
    int i=0;
    unsigned int num;
    char *s = value, *sp;

    while (*s)
    {
        if (*s == ';')
	{
            i++;
	}
        s++;
    }

    if (!i) return;

    cf->n_kremap = i;
    cf->kremap = oxim_malloc(sizeof(kremap_t) * i, True);

    s = sp = value;
    for (i=0; i<cf->n_kremap; i++)
    {
        while (*s != ':')
	{
            s++;
	}
        *s = '\0';
        cf->kremap[i].keystroke = strdup(sp);

        s++;
        sp = s;
        while (*s != ';')
            s++;
        *s = '\0';
        strncpy((char *)cf->kremap[i].uch.s, sp, UCH_SIZE);
        s++;
        sp = s;
    }
}

static void
gen_inp_resource(gen_inp_conf_t *cf, settings_t *im_settings)
{
    int boolean;

    if (oxim_setting_GetBoolean(im_settings, "AutoCompose", &boolean))
	oxim_setflag(&cf->mode, INP_MODE_AUTOCOMPOSE, boolean);

    if (oxim_setting_GetBoolean(im_settings, "AutoUpChar", &boolean))
	oxim_setflag(&cf->mode, INP_MODE_AUTOUPCHAR, boolean);

    if (oxim_setting_GetBoolean(im_settings, "AutoFullUp", &boolean))
	oxim_setflag(&cf->mode, INP_MODE_AUTOFULLUP, boolean);

    if (oxim_setting_GetBoolean(im_settings, "SpaceAutoUp", &boolean))
	oxim_setflag(&cf->mode, INP_MODE_SPACEAUTOUP, boolean);

    if (oxim_setting_GetBoolean(im_settings, "SelectKeyShift", &boolean))
	oxim_setflag(&cf->mode, INP_MODE_SELKEYSHIFT, boolean);

    if (oxim_setting_GetBoolean(im_settings, "SpaceIgnore", &boolean))
	oxim_setflag(&cf->mode, INP_MODE_SPACEIGNOR, boolean);

    if (oxim_setting_GetBoolean(im_settings, "SpaceReset", &boolean))
	oxim_setflag(&cf->mode, INP_MODE_SPACERESET, boolean);

    if (oxim_setting_GetBoolean(im_settings, "AutoReset", &boolean))
	oxim_setflag(&cf->mode, INP_MODE_AUTORESET, boolean);

    if (oxim_setting_GetBoolean(im_settings, "WildEnable", &boolean))
	oxim_setflag(&cf->mode, INP_MODE_WILDON, boolean);

    if (oxim_setting_GetBoolean(im_settings, "EndKey", &boolean))
	oxim_setflag(&cf->mode, INP_MODE_ENDKEY, boolean);

    /* 使用注音符號字根排列規則 */
    if (oxim_setting_GetBoolean(im_settings, "BoPoMoFoCheck", &boolean))
	oxim_setflag(&cf->mode, INP_MODE_BOPOMOFOCHK, boolean);

    char *string;
    bzero(cf->disable_sel_list, N_KEYS);
    if (oxim_setting_GetString(im_settings, "DisableSelectList", &string))
    {
	if (strlen(string) && strcasecmp("NONE", string) != 0)
	{
	    strcpy(cf->disable_sel_list, string);
	}
    }

    if (oxim_setting_GetString(im_settings, "Remap", &string))
    {
	if (cf->kremap)
	{
	    int i;
	    for (i=0; i < cf->n_kremap; i++)
	    {
		free(cf->kremap[i].keystroke);
	    }
	    free(cf->kremap);
	    cf->n_kremap = 0;
	    cf->kremap = NULL;
	}

	if (strlen(string) && strcasecmp(string, "NONE") != 0)
	{
	    setup_kremap(cf, string);
	}
    }

    /* 有結束鍵設定，就自動設定按結束鍵自動上字 */
    if (cf->header.n_end_key)
    {
	cf->mode |= INP_MODE_ENDKEY;
    }
}

static int gen_inp_init(void *conf, char *objname)
{
    gen_inp_conf_t *cf = (gen_inp_conf_t *)conf;
    char *s, value[128], truefn[256];
    unsigned int size;

/*
 *  Read the IM tab file.
 */
    sprintf(value, "%s.tab", objname);
    if (oxim_check_datafile(value, "tables", truefn, 256) == True)
    {
	if ((cf->zfp = gzopen(truefn, "rb")) == NULL)
	{
	    return False;
	}

	cf->tabfn = (char *)strdup(truefn);
	if (!loadtab(cf, objname))
	{
	    free(cf->tabfn);
	    gzclose(cf->zfp);
	    cf->zfp = NULL;
	    return False;
	}

	/* 決定檔案讀取模式 */
	/* 使用基本的檔案操作函數判斷此檔是否為未壓縮格式 */
	table_prefix_t tp;
	FILE *fp;
	/* 不必判斷 fopen 是否成功！因為 gzopen 若通過，fopen() 沒有理由不行 */
	fp = fopen(truefn, "rb");
	fread(&tp, 1, MODULE_ID_SIZE, fp);
	fclose(fp);
	cf->direct_mode = (memcmp(tp.prefix, "gencin", 6) == 0) ? True : False;
    }
    else
    {
	return False;
    }

    /*------------------------------------------------------
       如果是壓縮型態的輸入法參考檔，就把 offset table 與 input content
       統統讀進記憶體。
      ------------------------------------------------------*/
    if (!cf->direct_mode)
    {
	if (gzseek(cf->zfp, cf->header.offset_table_offset, SEEK_SET) < 0)
	{
	    return False;
	}

	size = (cf->header.n_input_content+1) * sizeof(unsigned int);
	cf->offset_tbl = oxim_malloc(size, False);
	if (gzread(cf->zfp, cf->offset_tbl, size) != size)
	{
	    free(cf->offset_tbl);
	    oxim_perr(OXIMMSG_WARNING, N_("gen_inp: %s: reading offset table error.\n"), cf->tabfn);
	    return False;
	}
	cf->offset_size = size;
	cf->memory_usage += size;

	/* 把輸入資料也讀進記憶體 */
	if (gzseek(cf->zfp, cf->header.input_content_offset, SEEK_SET) < 0)
	{
	    free(cf->offset_tbl);
	    cf->offset_tbl = NULL;
	    return False;
	}

	size = cf->offset_tbl[cf->header.n_input_content] - cf->offset_tbl[0];
	cf->input_content = oxim_malloc(size, False);
	if(gzread(cf->zfp, cf->input_content, size) != size)
	{
	    free(cf->offset_tbl);
	    cf->offset_tbl = NULL;
	    free(cf->input_content);
	    cf->input_content = NULL;
	    return False;
	}
	cf->input_content_size = size;
	cf->memory_usage += size;
    }
    /*------------------------------------------------------*/

    /* 產生給螢幕小鍵盤用的字根表 */
    int i;
    for (i = 0 ; i <  N_KEYS ; i++)
    {
	if (cf->header.keyname[i].uch)
	{
	    int idx = oxim_key2code(i);
	    if (idx)
	    {
		cf->etymon_list[idx].uch = cf->header.keyname[i].uch;
	    }
	}
    }
    return True;
}

/*----------------------------------------------------------------------------

        gen_inp_xim_init()

----------------------------------------------------------------------------*/

static int
gen_inp_xim_init(void *conf, inpinfo_t *inpinfo)
{
    gen_inp_conf_t *cf = (gen_inp_conf_t *)conf;
    int i;
    unsigned int size;

    inpinfo->iccf = oxim_malloc(sizeof(gen_inp_iccf_t), True);
    inpinfo->etymon_list = cf->etymon_list;
    inpinfo->guimode = 0;

    inpinfo->keystroke_len = 0;
    inpinfo->s_keystroke = oxim_malloc((N_NAME_LENGTH)*sizeof(uch_t), True);
    inpinfo->suggest_skeystroke = oxim_malloc((cf->header.n_max_keystroke+1)*sizeof(uch_t), True);

    if (! (cf->mode & INP_MODE_SELKEYSHIFT))
    {
	inpinfo->n_selkey = cf->header.n_selection_key;
	inpinfo->s_selkey = oxim_malloc(inpinfo->n_selkey*sizeof(uch_t), True);
	for (i=0; i<SELECT_KEY_LENGTH && i<cf->header.n_selection_key; i++)
	    inpinfo->s_selkey[i].s[0] = cf->header.selection_keys[i];
    }
    else
    {
	inpinfo->n_selkey = cf->header.n_selection_key+1;
	inpinfo->s_selkey = oxim_malloc(inpinfo->n_selkey*sizeof(uch_t), True);
	for (i=0; i<SELECT_KEY_LENGTH && i<cf->header.n_selection_key; i++)
	    inpinfo->s_selkey[i+1].s[0] = cf->header.selection_keys[i];
    }
    i = inpinfo->n_selkey;
    inpinfo->n_mcch = 0;
    inpinfo->mcch = NULL;
    inpinfo->mcch_grouping = oxim_malloc((i+1) * sizeof(uint_t), True);
    inpinfo->mcch_pgstate = MCCH_ONEPG;

    inpinfo->n_lcch = 0;
    inpinfo->lcch = NULL;
    inpinfo->lcch_grouping = NULL;
    inpinfo->cch_publish.uch = 0;

    return  True;
}

static unsigned int
gen_inp_xim_end(void *conf, inpinfo_t *inpinfo)
{
    gen_inp_conf_t *cf = (gen_inp_conf_t *)conf;
    gen_inp_iccf_t *iccf = (gen_inp_iccf_t *)inpinfo->iccf;

    if (iccf->mkey_list)
    {
	free(iccf->mkey_list);
	iccf->mkey_list = NULL;
    }

    free(inpinfo->iccf);
    inpinfo->iccf = NULL;

    if (inpinfo->mcch)
    {
	free(inpinfo->mcch);
	inpinfo->mcch = NULL;
    }

    free(inpinfo->s_keystroke);
    inpinfo->s_keystroke = NULL;

    free(inpinfo->suggest_skeystroke);
    inpinfo->suggest_skeystroke = NULL;

    free(inpinfo->s_selkey);
    inpinfo->s_selkey = NULL;

    free(inpinfo->mcch_grouping);
    inpinfo->mcch_grouping = NULL;

    return IMKEY_ABSORB;
}

/*----------------------------------------------------------------------------

	gen_inp_keystroke()

----------------------------------------------------------------------------*/

static unsigned int
return_wrong(gen_inp_conf_t *cf)
{
    return IMKEY_ABSORB;
}

static unsigned int
return_correct(gen_inp_conf_t *cf)
{
    return IMKEY_ABSORB;
}

static void
reset_keystroke(inpinfo_t *inpinfo, gen_inp_iccf_t *iccf)
{
    inpinfo->s_keystroke[0].uch = 0;
    inpinfo->keystroke_len = 0;
    inpinfo->n_mcch = 0;
    iccf->keystroke[0] = '\0';
    iccf->mode = 0;
    inpinfo->mcch_pgstate = MCCH_ONEPG;

    if (iccf->mkey_list)
    {
	free(iccf->mkey_list);
	iccf->mkey_list = NULL;
    }
}


/*------------------------------------------------------------------------*/

static void *get_input_content(gen_inp_conf_t *cf, unsigned int idx)
{
    if (idx >= cf->header.n_input_content)
    {
	return NULL;
    }

    unsigned int offset, offset_len;

    /* 完全使用磁碟 I/O */
    if (cf->direct_mode)
    {
	unsigned int ofs[2];
	/* 表格起始位置 ＋ (索引值 x 4) */
	unsigned int idx_offset = cf->header.offset_table_offset + (idx << 2);
	unsigned int offsetsize = sizeof(unsigned int) << 1;
	/* 移動讀取指標 */
	if (gzseek(cf->zfp, idx_offset, SEEK_SET) < 0)
	{
	    return NULL;
	}
	/* 讀入輸入資料偏移位置 */
	if (gzread(cf->zfp, &ofs, offsetsize) != offsetsize)
	{
	    return NULL;
	}

	offset = ofs[0];
	offset_len = ofs[1] - offset;

	if (gzseek(cf->zfp, offset, SEEK_SET) < 0)
	{
	    return NULL;
	}
    }
    else /* Offset Table 已經讀入記憶體內 */
    {
	offset = cf->offset_tbl[idx] - cf->offset_tbl[0];
	offset_len = cf->offset_tbl[idx+1] - offset - cf->offset_tbl[0];
    }

    /* 配置記憶體 */
    /* 讀入真正的輸入資料 */
    void *icr = oxim_malloc(offset_len, False);
    if (cf->direct_mode)
    {
	if (gzread(cf->zfp, icr, offset_len) != offset_len)
	{
	    free(icr);
	    icr = NULL;
	}
    }
    else
    {
	memcpy(icr, cf->input_content+offset, offset_len);
    }

    return icr;
}

static int
cmp_icvalue(gen_inp_conf_t *cf, char *keystroke, unsigned int idx, int wildcmp)
{
    static char buf[256];
    unsigned char keystroke_len = strlen(keystroke);
    void *icr = get_input_content(cf, idx);
    unsigned char key_len;
    int cmp;

    if (!icr)
    {
	return -1;
    }
    memcpy(&key_len, icr, 1);

    strncpy(buf, icr+2, key_len);
    buf[key_len] = '\0';
    if (wildcmp)
    {
	cmp = strcmp_wild(keystroke, buf);
    }
    else
    {
	cmp = strcmp(buf, keystroke);
    }

    free(icr);

    if (cmp > 0)
	cmp = 1;
    else if (cmp < 0)
	cmp = -1;

    return cmp;
}

static int sort_cache(const void *a, const void *b)
{
    cache_t *aa = (cache_t *)a, *bb = (cache_t *)b;
    int cmp = strcmp(aa->keystroke, bb->keystroke);
    if (cmp == 0)
	return 0;
    else if (cmp > 0)
	return 1;
    else
	return -1;
}

static int get_cache(gen_inp_conf_t *cf, char *keystroke, unsigned int *ret_start, unsigned int *ret_end)
{
    int low, middle, high;
    int cmp_value;

    if (cf->n_cache)
    {
	low = 0;
	high = cf->n_cache - 1;
	while (low <= high)
	{
	    middle = (low+high) >> 1;
	    cmp_value = strcmp(cf->cache[middle].keystroke, keystroke);
	    if (cmp_value == 0)
	    {
		break;
	    }
	    else if (cmp_value > 0)
	    {
		high = middle - 1;
	    }
	    else
	    {
		low = middle + 1;
	    }
	}
	if (cmp_value == 0)
	{
	    *ret_start = cf->cache[middle].start_idx;
	    *ret_end   = cf->cache[middle].end_idx;
	    return True;
	}
    }
    return False;
}

static void set_cache(gen_inp_conf_t *cf, char *keystroke, unsigned int start_idx, unsigned int end_idx)
{
    cf->n_cache ++;
    unsigned int cache_pos = cf->n_cache - 1;
    if (cf->n_cache == 1)
    {
	cf->cache = oxim_malloc(sizeof(cache_t), False);
    }
    else
    {
	cf->cache = oxim_realloc(cf->cache, sizeof(cache_t) * cf->n_cache);
    }
    cf->cache[cache_pos].keystroke = strdup(keystroke);
    cf->cache[cache_pos].start_idx = start_idx;
    cf->cache[cache_pos].end_idx = end_idx;
    stable_sort(cf->cache, cf->n_cache, sizeof(cache_t), sort_cache);
}

static int 
bsearch_keystroke(gen_inp_conf_t *cf, char *keystroke, int size, int *end_idx)
{
    int head, middle, end, start_idx, last_idx;
    int ret;

    /* 優先找 cache */
    /* 全部存放於記憶體，就不必找 cache */
    if (!cf->direct_mode)
    {
	unsigned int ret_start, ret_end;
	if (get_cache(cf, keystroke, &ret_start, &ret_end))
	{
	    *end_idx = ret_end;
	    return (ret_start);
	}
    }

    head   = 0;
    middle = size >> 1;
    end    = size;
    while ((ret=cmp_icvalue(cf, keystroke, middle, False)))
    {
        if (ret > 0)
            end = middle;
        else
            head = middle + 1;
        middle = (end + head) >> 1;
        if (middle == head && middle == end)
            break;
    }

    if (ret == 0)
    {
	start_idx = last_idx = middle;
	/* 找該組字的一筆索引 */
	while(start_idx > 0 &&
	      ! cmp_icvalue(cf, keystroke, start_idx-1, False)) 
	{
	    start_idx --;
	}

	/* 找最後一筆索引 */
	while(!cmp_icvalue(cf, keystroke, last_idx+1, False))
	{
	    last_idx++;
	}

	*end_idx = last_idx;

	/* 如果直接讀取磁碟，且同字根數量超過 50，就加入 cache 列表 */
	if (cf->direct_mode && (last_idx - start_idx) >= 50)
	{
	    set_cache(cf, keystroke, start_idx, last_idx);
	}
	return start_idx;
    }

    return -1;
}

static int
match_keystroke(gen_inp_conf_t *cf, inpinfo_t *inpinfo, gen_inp_iccf_t *iccf)
{
    int ret, start_idx, end_idx;
    char *keystroke = iccf->keystroke;

    inpinfo->n_mcch = 0;

    if (!(iccf->mode & INPINFO_MODE_INWILD))
    {
	start_idx = bsearch_keystroke(cf, keystroke, cf->header.n_input_content, &end_idx);
	if (start_idx < 0)
	{
	    return False;
	}
    }
    else
    {
	int i, cmp;
	/* 如果只有一個 '*' 或 '?' 沒啥意義，當作沒這回事 :-P */
	if (strlen(keystroke) == 1 && (keystroke[0]=='*' || keystroke[0]=='?'))
	{
	    return False;
	}

	/* 查 cache */
	int first = -1, last;
	int hit_cache = get_cache(cf, keystroke, &start_idx, &end_idx);
	if (!hit_cache)
	{
	    start_idx = 0;
	    end_idx = cf->header.n_input_content - 1;
	}

	if (iccf->mkey_list)
	{
	    free(iccf->mkey_list);
	    iccf->mkey_list = NULL;
	}
	iccf->n_mkey_list = 0;

	for (i = start_idx ; i < end_idx ; i++)
	{
	    cmp = cmp_icvalue(cf, keystroke, i, True);
	    if (cmp == 0)
	    {
		if (first < 0)
		{
		    first = i;
		}

		iccf->n_mkey_list ++;
		if (iccf->n_mkey_list == 1)
		{
		    iccf->mkey_list = oxim_malloc(sizeof(unsigned int), False);
		}
		else
		{
		    iccf->mkey_list = oxim_realloc(iccf->mkey_list, sizeof(unsigned int)*iccf->n_mkey_list);
		}
		iccf->mkey_list[iccf->n_mkey_list -1] = i;
		last = i;
	    }
	}

	if (!iccf->n_mkey_list)
	{
	    return False;
	}

	/* 如果沒有找到 cache, 就設定 cache */
	if (!hit_cache)
	{
	    set_cache(cf, keystroke, first, last);
	}

	start_idx = 0;
	end_idx = iccf->n_mkey_list - 1;
    }

    iccf->n_record = end_idx - start_idx + 1; /* 總共幾筆紀錄 */
    iccf->n_page = iccf->n_record / inpinfo->n_selkey +
		   ((iccf->n_record % inpinfo->n_selkey) ? 1 : 0);
    iccf->start_idx = start_idx;
    iccf->end_idx = end_idx;
    iccf->this_page = 0;

    ret = fillpage(cf, inpinfo, iccf, 0);
    if (inpinfo->n_mcch > 1 && (iccf->mode & INPINFO_MODE_SPACE))
    {
	iccf->mode &= ~(INPINFO_MODE_SPACE);
    }

    return ret;
}

/*------------------------------------------------------------------------*/

static void
commit_content(gen_inp_conf_t *cf, inpinfo_t *inpinfo, gen_inp_iccf_t *iccf, int idx)
{
    static char cch_s[1024];
    unsigned int n_char = inpinfo->mcch_grouping[idx+1]; /* 有幾個字? */
    unsigned int mcch_idx = 0;
    int i;

    for (i=0 ; i < idx ; i++)
    {
	mcch_idx += inpinfo->mcch_grouping[i+1];
    }

    cch_s[0] = '\0';
    while (n_char--)
    {
	strcat(cch_s, inpinfo->mcch[mcch_idx++].s);
    }

    inpinfo->cch = cch_s;
    inpinfo->cch_publish.uch = 0;
    /* 只有單一字元才需要提示字根 */
    if (inpinfo->mcch_grouping[idx+1] == 1)
    {
	/* 計算真正的索引位置 */
	/* idx 是目前頁的位置，所以要加上該筆劃的起始偏移值 */
	unsigned int new_idx = (iccf->this_page * inpinfo->n_selkey) + idx + iccf->start_idx;
	unsigned int true_idx = new_idx;
	/* 有萬用字元的話要取 mkey_list[new_idx] */
	if (strchr(iccf->keystroke,'*') || strchr(iccf->keystroke,'?'))
	{
	    true_idx = iccf->mkey_list[new_idx];
	}
	cintab_input_content cic;
	void *icr = get_input_content(cf, true_idx);
	/* 字根長度 */
	memcpy(&cic.n_keystroke, icr, 1);
	/* 字根內容(注意! 沒有包含 NULL 字元) */
	cic.keystroke = icr+2;
	/* 字元內容 */
	cic.content = icr + 2 + cic.n_keystroke;
	for (i = 0 ; i < cic.n_keystroke ; i++)
	{
	    int keycode = cic.keystroke[i];
	    inpinfo->suggest_skeystroke[i].uch =
		cf->header.keyname[keycode].uch;

	}
	oxim_ucs4_to_utf8(cic.content[0], inpinfo->cch_publish.s);
	free(icr);
    }

    if (inpinfo->mcch)
    {
	free(inpinfo->mcch);
	inpinfo->mcch = NULL;
	inpinfo->n_mcch = 0;
    }

    inpinfo->keystroke_len = 0;
    inpinfo->s_keystroke[0].uch = 0;

    inpinfo->mcch_pgstate = MCCH_ONEPG;

    iccf->mode &= ~(INPINFO_MODE_MCCH);
    iccf->mode &= ~(INPINFO_MODE_INWILD);
    inpinfo->guimode &= ~(GUIMOD_SELKEYSPOT);
}

static unsigned int
commit_keystroke(gen_inp_conf_t *cf, inpinfo_t *inpinfo, gen_inp_iccf_t *iccf)
/* return: the IMKEY state */
{

    if (cf->n_kremap)
    {
	int i;
	for (i=0; i<cf->n_kremap; i++)
	{
	    if (strcmp(iccf->keystroke, cf->kremap[i].keystroke) == 0)
	    {
		inpinfo->cch = cf->kremap[i].uch.s;
		inpinfo->keystroke_len = 0;
		inpinfo->s_keystroke[0].uch = (uchar_t)0;
		inpinfo->n_mcch = 0;
		inpinfo->cch_publish.uch = 0;
		inpinfo->mcch_pgstate = MCCH_ONEPG;
		iccf->mode &= ~(INPINFO_MODE_MCCH);
		iccf->mode &= ~(INPINFO_MODE_INWILD);
		inpinfo->guimode &= ~(GUIMOD_SELKEYSPOT);
		return IMKEY_COMMIT;
	    }
	}
    }

    if (match_keystroke(cf, inpinfo, iccf))
    {
        if (inpinfo->n_mcch == 1)
	{
            commit_content(cf, inpinfo, iccf, 0);
            return IMKEY_COMMIT;
        }
        else
	{
            iccf->mode |= INPINFO_MODE_MCCH;
	    inpinfo->guimode |= GUIMOD_SELKEYSPOT;
            return return_correct(cf);
        }
    }
    else
    {
	if ((cf->mode & INP_MODE_AUTORESET))
	{
	    reset_keystroke(inpinfo, iccf);
	}
	else
	{
	    iccf->mode |= INPINFO_MODE_WRONG;
	}
        return return_wrong(cf);
    }
}


/*------------------------------------------------------------------------*/

static int
mcch_choosech(gen_inp_conf_t *cf, 
	      inpinfo_t *inpinfo, gen_inp_iccf_t *iccf, int idx)
{
    int min;

    if (inpinfo->n_mcch == 0 && ! match_keystroke(cf, inpinfo, iccf))
    {
        return 0;
    }

    if (idx < 0)		/* Always select the first. */
    {
	idx = 0;
    }
    else
    {
        if ((cf->mode & INP_MODE_SELKEYSHIFT))
	{
	    idx ++;
	}
        min = (inpinfo->n_selkey > inpinfo->n_mcch) ? 
                inpinfo->n_mcch : inpinfo->n_selkey;
        if (idx >= min)
	{
            return 0;
	}
    }

    commit_content(cf, inpinfo, iccf, idx);
    reset_keystroke(inpinfo, iccf);

    return 1;
}

static int
fillpage(gen_inp_conf_t *cf, inpinfo_t *inpinfo, 
	 gen_inp_iccf_t *iccf, byte_t dir)
{
    unsigned int n_pg = inpinfo->n_selkey; /* 每一頁顯示項目長度 */
    cintab_input_content cic;
    void *icr;
    unsigned int i, j, idx, item, have_word;

    switch (dir)
    {
	case 0:  /* 從第一頁開始 */
	    iccf->this_page = 0;
	    break;
	case 1:  /* 下一頁 */
	    if ((iccf->this_page + 1) >= iccf->n_page)
	    {
		return False;
	    }
	    iccf->this_page ++;
	    break;
	case -1: /* 前一頁 */
	    if ((iccf->this_page - 1) < 0)
	    {
		return False;
	    }
	    iccf->this_page --;
	    break;
    }

    /* 計算顯示頁的起始索引 */
    idx = (iccf->this_page * n_pg) + iccf->start_idx;

    /* 本頁筆數 */
    if ((iccf->end_idx - idx + 1) < n_pg)
    {
	n_pg = iccf->end_idx - idx + 1;
    }

    if (inpinfo->mcch)
    {
	free(inpinfo->mcch);
	inpinfo->mcch = NULL;
    }

    inpinfo->n_mcch = 0;
    inpinfo->mcch_grouping[0] = 0; /* 預設是沒有詞 */
    i = 0;

    have_word = False;
    for (item = 0 ; item < n_pg ; item++)
    {
	/* 讀取真正的輸入資料 */
	icr = get_input_content(cf, ((iccf->mode & INPINFO_MODE_INWILD) ? iccf->mkey_list[idx] : idx));

	/* 字根長度 */
	memcpy(&cic.n_keystroke, icr, 1);
	/* 字元數目 */
	memcpy(&cic.n_char, icr+1, 1);
	/* 字根內容(注意! 沒有包含 NULL 字元) */
	cic.keystroke = icr+2;
	/* 字元內容 */
	cic.content = icr + 2 + cic.n_keystroke;
	/* 加總字元數目 */
	inpinfo->n_mcch += cic.n_char;

	/* 配置所需的記憶體 */
	if (inpinfo->n_mcch == cic.n_char)
	{
	    inpinfo->mcch = oxim_malloc(sizeof(uch_t) * cic.n_char, False);
	}
	else
	{
	    inpinfo->mcch = oxim_realloc(inpinfo->mcch, sizeof(uch_t)*inpinfo->n_mcch);
	}

	/* i : inpinfo->mcch 的索引
	   j : 單一輸入資料字元索引 */
	for (j = 0 ; j < cic.n_char; j++, i++)
	{
	    oxim_ucs4_to_utf8(cic.content[j], inpinfo->mcch[i].s);
	}

	inpinfo->mcch_grouping[item+1] = cic.n_char; 
	/* 超過一個字，表示這是詞 */
	if (cic.n_char > 1)
	{
	    have_word = True;
	}

	idx ++;
	free(icr);
    }

    inpinfo->mcch_grouping[0] = have_word ? n_pg : 0;

    if (iccf->n_page == 1)
	inpinfo->mcch_pgstate = MCCH_ONEPG;
    else if (iccf->this_page == 0)
	inpinfo->mcch_pgstate = MCCH_BEGIN;
    else if ((iccf->this_page + 1) == iccf->n_page)
	inpinfo->mcch_pgstate = MCCH_END;
    else
	inpinfo->mcch_pgstate = MCCH_MIDDLE;

    return True;
}

static int
mcch_nextpage(gen_inp_conf_t *cf, 
	      inpinfo_t *inpinfo, gen_inp_iccf_t *iccf, char key)
{
    int ret=0;

    switch (inpinfo->mcch_pgstate) {
    case MCCH_ONEPG:
	switch (key) {
	case ' ':
	    if ((cf->mode & INP_MODE_AUTOUPCHAR))
                ret = (mcch_choosech(cf, inpinfo, iccf, -1)) ?
                	IMKEY_COMMIT : return_wrong(cf);
            else
                ret = return_correct(cf);
	    break;
	case '<':
	case '>':
	    ret = return_correct(cf);
	    break;
	default:
	    ret = return_wrong(cf);
	    break;
	}
	break;

    case MCCH_END:
	switch (key) {
	case ' ':
	case '>':
	    ret = (fillpage(cf, inpinfo, iccf, 0)) ? 
			IMKEY_ABSORB : return_wrong(cf);
	    break;
	case '<':
	    ret = (fillpage(cf, inpinfo, iccf, -1)) ?
			IMKEY_ABSORB : return_wrong(cf);
	    break;
	default:
	    ret = return_wrong(cf);
	    break;
	}
	break;

    case MCCH_BEGIN:
	switch (key) {
	case ' ':
	case '>':
	    ret = (fillpage(cf, inpinfo, iccf, 1)) ? 
			IMKEY_ABSORB : return_wrong(cf);
	    break;
	case '<':
	    ret = return_correct(cf);
	    break;
	default:
	    ret = return_wrong(cf);
	    break;
	}
	break;

    default:
	switch (key) {
	case ' ':
	case '>':
	    ret = (fillpage(cf, inpinfo, iccf, 1)) ? 
			IMKEY_ABSORB : return_wrong(cf);
	    break;
	case '<':
	    ret = (fillpage(cf, inpinfo, iccf, -1)) ?
			IMKEY_ABSORB : return_wrong(cf);
	    break;
	default:
	    ret = return_wrong(cf);
	    break;
	}
	break;
    }
    return ret;
}

/*------------------------------------------------------------------------*/

static unsigned int
modifier_escape(gen_inp_conf_t *cf, int class)
{
    unsigned int ret=0;

    switch (class) {
    case QPHR_SHIFT:
	if ((cf->modesc & QPHR_SHIFT))
	    ret |= IMKEY_SHIFTPHR;
	ret |= IMKEY_SHIFTESC;
	break;
    case QPHR_CTRL:
	if ((cf->modesc & QPHR_CTRL))
	    ret |= IMKEY_CTRLPHR;
	break;
    case QPHR_ALT:
	if ((cf->modesc & QPHR_ALT))
	    ret |= IMKEY_ALTPHR;
	break;
    case QPHR_FALLBACK:
	if ((cf->modesc & QPHR_FALLBACK))
	    ret |= IMKEY_FALLBACKPHR;
	break;
    }
    return ret;
}

static int
get_bopomofo_index(uch_t uch)
{
    int idx;
    int uch_len = strlen((char *)uch.s);
    int isbopomofo = False;
    for (idx = 0; idx < 3 && uch_len; idx++)
    {
	if (strstr(bopomofo[idx], (char *)uch.s))
	    return idx;
    }
    return -1;
}

static unsigned int
gen_inp_keystroke(void *conf, inpinfo_t *inpinfo, keyinfo_t *keyinfo)
{
    gen_inp_conf_t *cf = (gen_inp_conf_t *)conf;
    gen_inp_iccf_t *iccf = (gen_inp_iccf_t *)inpinfo->iccf;
    KeySym keysym = keyinfo->keysym;
    char *keystr = keyinfo->keystr;
    int len, max_len;
    char sp_ignore=0, inp_wrong=0;

    len = inpinfo->keystroke_len;
    max_len = cf->header.n_max_keystroke;

    if ((iccf->mode & INPINFO_MODE_SPACE))
    {
        sp_ignore = 1;
        iccf->mode &= ~(INPINFO_MODE_SPACE);
    }

    if ((iccf->mode & INPINFO_MODE_WRONG))
    {
	inp_wrong = 1;
	iccf->mode &= ~(INPINFO_MODE_WRONG);
    }

    if ((keysym == XK_BackSpace || keysym == XK_Delete) && len)
    {
        iccf->keystroke[len-1] = '\0';
        inpinfo->s_keystroke[len-1].uch = (uchar_t)0;
        inpinfo->keystroke_len --;
	inpinfo->n_mcch = 0;
	inpinfo->cch_publish.uch = (uchar_t)0;
	inpinfo->mcch_pgstate = MCCH_ONEPG;
	inpinfo->guimode &= ~(GUIMOD_SELKEYSPOT);
	iccf->mode = 0;
	if ((cf->mode & INP_MODE_WILDON) &&
	    (strchr(iccf->keystroke, '*') || strchr(iccf->keystroke, '?')))
	{
	    iccf->mode |= INPINFO_MODE_INWILD;
	}

	if (len-1 > 0 && (cf->mode & INP_MODE_AUTOCOMPOSE))
	{
	    match_keystroke(cf, inpinfo, iccf);
	}
        return IMKEY_ABSORB;
    }
    else if (keysym == XK_Escape && len)
    {
	reset_keystroke(inpinfo, iccf);
        inpinfo->cch_publish.uch = (uchar_t)0;
	inpinfo->mcch_pgstate = MCCH_ONEPG;
	inpinfo->guimode &= ~(GUIMOD_SELKEYSPOT);
        return IMKEY_ABSORB;
    }

    else if (keysym == XK_space) {
        inpinfo->cch_publish.uch = (uchar_t)0;

	if ((cf->mode & INP_MODE_SPACEAUTOUP) && 
/* avoid wild mode auto up */
	    (! (iccf->mode & INPINFO_MODE_INWILD) ||
	       (iccf->mode & INPINFO_MODE_MCCH)) &&
/* end of wild mod */
	    (inpinfo->n_mcch > 1 || inpinfo->mcch_pgstate != MCCH_ONEPG))
	{
            if ((mcch_choosech(cf, inpinfo, iccf, -1)))
	    {
                return IMKEY_COMMIT;
	    }
            else
	    {
                if ((cf->mode & INP_MODE_AUTORESET))
		{
                    reset_keystroke(inpinfo, iccf);
		}
                else
		{
                    iccf->mode |= INPINFO_MODE_WRONG;
		}
                return return_wrong(cf);
            }
	}
	else if ((iccf->mode & INPINFO_MODE_MCCH))
	{
	    return mcch_nextpage(cf, inpinfo, iccf, ' ');
	}
        else if ((cf->mode & INP_MODE_SPACERESET) && inp_wrong)
	{
            reset_keystroke(inpinfo, iccf);
            return IMKEY_ABSORB;
        }
        else if (sp_ignore)
	{
            return IMKEY_ABSORB;
	}
	else if (inpinfo->keystroke_len)
	{
	    return commit_keystroke(cf, inpinfo, iccf);
	}
    }
    else if ((keysym >= XK_KP_0 && keysym <= XK_KP_9) || 
	     (keysym >= XK_KP_Multiply && keysym <= XK_KP_Divide))
    {
	return IMKEY_IGNORE;
    }
    else if ((keysym == XK_Left || keysym == XK_Right) &&
		(inpinfo->guimode & GUIMOD_SELKEYSPOT))
    {
	char pg_key = (keysym == XK_Left) ? '<' : '>';
	return mcch_nextpage(cf, inpinfo, iccf, pg_key);
    }
    else if (keyinfo->keystr_len == 1)
    {
        unsigned int ret=IMKEY_ABSORB, ret1=0;
	int selkey_idx, endkey_pressed=0;
        char keycode, *s;
        uch_t uch;

        inpinfo->cch_publish.uch = (uchar_t)0;
	keycode = keystr[0];

	/* 不區分按鍵大小寫 */
	if (!cf->header.keep_key_case)
	{
	    /* 沒有按下 Shift 的話，統統轉換為小寫 */
 	    if (!(keyinfo->keystate & ShiftMask))
	    {
		keycode = tolower(keycode);
	    }
	    else /* 否則轉換為大寫 */
	    {
		keycode = toupper(keycode);
	    }
	    keystr[0] = keycode;
	}
	uch.uch = cf->header.keyname[(int)keycode].uch;

	selkey_idx = ((s = strchr(cf->header.selection_keys, keystr[0]))) ? 
		(int)(s - cf->header.selection_keys) : -1;
	if (cf->header.n_end_key && 
	    strchr(cf->header.end_keys, iccf->keystroke[len-1]))
	{
	    endkey_pressed = 1;
	}
	if (len && selkey_idx != -1 && (endkey_pressed || ! uch.uch))
	{
	    /* Don't enter the multi-cch selection, but selkey pressed. */
	    if (len==1 && strlen(cf->disable_sel_list) && 
		strchr(cf->disable_sel_list, iccf->keystroke[len-1]))
	    {
		uch.s[0] = keystr[0];
	    }
	    else
	    {
		return (mcch_choosech(cf, inpinfo, iccf, selkey_idx)) ?
			IMKEY_COMMIT : return_wrong(cf);
	    }
	}
	else if ((keystr[0]=='<' || keystr[0]=='>') &&
		 (inpinfo->guimode & GUIMOD_SELKEYSPOT))
	{
	    return mcch_nextpage(cf, inpinfo, iccf, keystr[0]);
	}
	else if ((iccf->mode & INPINFO_MODE_MCCH))
	{
	    /* Enter the multi-cch selection. */
	    if (selkey_idx != -1)
	    {
		return (mcch_choosech(cf, inpinfo, iccf, selkey_idx)) ?
			IMKEY_COMMIT : return_wrong(cf);
	    }
	    else if ((cf->mode & INP_MODE_AUTOUPCHAR))
	    {
		if (! mcch_choosech(cf, inpinfo, iccf, -1))
		{
		    return return_wrong(cf);
		}
		ret |= IMKEY_COMMIT;
	    }
	    else
	    {
		return return_wrong(cf);
	    }
	}

	/* 檢查所輸入的字根是否為注音符號 */
	int bopomofo_idx = (cf->mode & INP_MODE_BOPOMOFOCHK) ? get_bopomofo_index(uch) : -1;
	int isappend = True;
	int stroke_idx;

	/* 注音符號要特別處理 */
	if (bopomofo_idx >= 0)
	{
	    for (stroke_idx=0 ;stroke_idx < inpinfo->keystroke_len;stroke_idx++)
	    {
		/* 首先，注音符號不可能有重複 */
		if (strcmp((char *)uch.s, (char *)inpinfo->s_keystroke[stroke_idx].s) == 0)
		    return IMKEY_ABSORB;

		/* 字根性質相同嗎?是的話，直接取代 */
		if (get_bopomofo_index(inpinfo->s_keystroke[stroke_idx]) == bopomofo_idx)
		{
		    isappend = False;
		    break;
		}

	    }
	}

	/* The previous cch might be committed, so len might be 0 */
	len = inpinfo->keystroke_len;

	if (!uch.uch && (keyinfo->keystate & ShiftMask))
	{
	    if (! (cf->mode & INP_MODE_WILDON) || 
		(keystr[0] != '*' && keystr[0] != '?'))
	    {
		return (ret | modifier_escape(cf, QPHR_SHIFT));
	    }
	    else
	    {
	        iccf->mode |= INPINFO_MODE_INWILD;
	    }
	}
	else if ((keyinfo->keystate & ControlMask) &&
		 (ret1 = modifier_escape(cf, QPHR_CTRL)))
	{
	    return (ret | ret1);
	}
	else if ((keyinfo->keystate & Mod1Mask) &&
		 (ret1 = modifier_escape(cf, QPHR_ALT)))
	{
	    return (ret | ret1);
	}
	else if (!uch.uch)
	{
	    return (ret | IMKEY_IGNORE);
	}
        else if (len >= max_len)
	{
            return return_wrong(cf);
	}

	if (isappend)
	{
            iccf->keystroke[len] = keystr[0];
            iccf->keystroke[len+1] = '\0';
	    if (keystr[0] == '*' || keystr[0] == '?')
	    {
	        inpinfo->s_keystroke[len].s[0] = keystr[0];
	        inpinfo->s_keystroke[len].s[1] = ' ';
	    }
	    else
	    {
                inpinfo->s_keystroke[len].uch = uch.uch;
	    }
            inpinfo->s_keystroke[len+1].uch = (uchar_t)0;
            inpinfo->keystroke_len ++;
            len ++;
	}
	else
	{
	    iccf->keystroke[stroke_idx] = keystr[0];
	    inpinfo->s_keystroke[stroke_idx].uch = uch.uch;
	}

	/* 如果是注音輸入法的話，要依據注音符號規則排列 */
	if (bopomofo_idx >= 0)
	{
	    int idx;
	    int keystroke_len = inpinfo->keystroke_len;
	    int sort_tbl[4] = {-1, -1, -1, -1};
	    /* 兩個字根以上才需要檢查 */
	    if (keystroke_len > 1)
	    {
		/* 取得每個注音符號的性質位置*/
		for (idx=0 ; idx < keystroke_len ; idx++)
		{
		    int b_idx = get_bopomofo_index(inpinfo->s_keystroke[idx]);
		    sort_tbl[idx] = b_idx < 0 ? 9 : b_idx;
		}
		for (idx=0 ; idx < keystroke_len-1 ; idx++)
		{
		    int x;
		    for (x=idx+1 ; x < keystroke_len ; x++)
		    {
			if (sort_tbl[idx] > sort_tbl[x])
			{
			    int tmp = sort_tbl[idx];
			    sort_tbl[idx] = sort_tbl[x];
			    sort_tbl[x] = tmp;

			    uch_t tmp_uch = inpinfo->s_keystroke[idx];
			    inpinfo->s_keystroke[idx] = inpinfo->s_keystroke[x];
			    inpinfo->s_keystroke[x] = tmp_uch;

			    char tmp_ch = iccf->keystroke[idx];
			    iccf->keystroke[idx] = iccf->keystroke[x];
			    iccf->keystroke[x] = tmp_ch;
			}
		    }
		}
	    }
	}

	if ((cf->mode & INP_MODE_SPACEIGNOR) && len == max_len)
	{
	    iccf->mode |= INPINFO_MODE_SPACE;
	}
	if ((cf->mode & INP_MODE_ENDKEY) && len>1 &&
		 (s=strchr(cf->header.end_keys, keystr[0])))
	{
	    return commit_keystroke(cf, inpinfo, iccf);
	}
	else if ((cf->mode & INP_MODE_AUTOFULLUP) && len == max_len)
	{
	    return commit_keystroke(cf, inpinfo, iccf);
	}
	else if ((cf->mode & INP_MODE_AUTOCOMPOSE))
	{
	    match_keystroke(cf, inpinfo, iccf);
	    //iccf->mode |= INPINFO_MODE_MCCH;
	    /*if(is_virtual_keyboard())
	    {
		inpinfo->guimode |= GUIMOD_SELKEYSPOT;	// by wind
		return return_correct(cf);
	    }*/
	}
	return ret;
    }

    return IMKEY_IGNORE;
}

/*----------------------------------------------------------------------------

	gen_inp_show_keystroke()

----------------------------------------------------------------------------*/

static int 
gen_inp_show_keystroke(void *conf, simdinfo_t *simdinfo)
{
    simdinfo->s_keystroke = NULL;
    return False;
}

static int
gen_inp_terminate(void *conf)
{
    gen_inp_conf_t *cf = (gen_inp_conf_t *)conf;

    if (cf->tabfn)
    {
	free(cf->tabfn);
	cf->tabfn = NULL;
    }

    if (cf->offset_tbl)
    {
	free(cf->offset_tbl);
	cf->offset_tbl = NULL;
	cf->memory_usage -= cf->offset_size;
    }

    if (cf->input_content)
    {
	free(cf->input_content);
	cf->input_content = NULL;
	cf->memory_usage -= cf->input_content_size;
    }

    if (cf->kremap)
    {
	int i;
	for (i=0; i < cf->n_kremap; i++)
	{
	    free(cf->kremap[i].keystroke);
	}
	free(cf->kremap);
	cf->n_kremap = 0;
	cf->kremap = NULL;
    }

    /* 釋放 cache 記憶體 */
    if (cf->n_cache)
    {
	int i;
	for (i=0 ; i < cf->n_cache ; i++)
	{
	    free(cf->cache[i].keystroke);
	}
	free(cf->cache);
	cf->n_cache = 0;
	cf->cache = NULL;
    }

    if (cf->zfp)
    {
	gzclose(cf->zfp);
	cf->zfp = NULL;
    }

    return True;
}
/*----------------------------------------------------------------------------

        Definition of general input method module (template).

----------------------------------------------------------------------------*/

static char *gen_inp_valid_objname[] = { "*", NULL };

static char gen_inp_comments[] = N_(
	"This is the general input method module.\n");

module_t module_ptr = {
    { MTYPE_IM,					/* module_type */
      "gen_inp_v1",				/* name */
      MODULE_VERSION,				/* version */
      gen_inp_comments },			/* comments */
    gen_inp_valid_objname,			/* valid_objname */
    sizeof(gen_inp_conf_t),			/* conf_size */
    gen_inp_init,				/* init */
    gen_inp_xim_init,				/* xim_init */
    gen_inp_xim_end,				/* xim_end */
    gen_inp_keystroke,				/* keystroke */
    gen_inp_show_keystroke,			/* show_keystroke */
    gen_inp_terminate				/* terminate */
};
