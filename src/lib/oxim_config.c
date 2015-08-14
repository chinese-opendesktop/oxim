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

static conf_rc oxim_conf[] =
{
    {DefaultFontName, "DefaultFontName", "AR PL New Sung", NULL},
    {DefaultFontSize, "DefaultFontSize", "13", NULL},
    {PreeditFontSize, "PreeditFontSize", "16", NULL},
    {SelectFontSize,  "SelectFontSize",  "16", NULL},
    {StatusFontSize,  "StatusFontSize",  "16", NULL},
    {MenuFontSize,    "MenuFontSize",    "13", NULL},
    {SymbolFontSize,  "SymbolFontSize",  "13", NULL},
    {XcinFontSize,    "XcinFontSize",    "16", NULL},
    /* 顏色 */
    {WinBoxColor,       "WinBoxColor",        "#737173", NULL},
    {BorderColor,       "BorderColor",        "#eeeee6", NULL},
    {LightColor,        "LightColor",         "#ffffff", NULL},
    {DarkColor,         "DarkColor",          "#b4b29c", NULL},
    {CursorColor,       "CursorColor",        "#7f0000", NULL},
    {CursorFontColor,   "CursorFontColor",    "#ffff00", NULL},
    {FontColor,         "FontColor",          "#000000", NULL},
    {ConvertNameColor,  "ConvertNameColor",   "#7f0000", NULL},
    {InputNameColor,    "InputNameColor",     "#00007f", NULL},
    {UnderlineColor,    "UnderlineColor",     "#000000", NULL},
    {KeystrokeColor,    "KeystrokeColor",     "#0000ff", NULL},
    {KeystrokeBoxColor, "KeystrokeBoxColor",  "#ffffff", NULL},
    {SelectFontColor,   "SelectFontColor",    "#ffffff", NULL},
    {SelectBoxColor,    "SelectBoxColor",     "#3179ac", NULL},
    {MenuBGColor,       "MenuBGColor",        "#4c59a6", NULL},
    {MenuFontColor,     "MenuFontColor",      "#ffffff", NULL},
    {XcinBorderColor,   "XcinBorderColor",    "#ffffff", NULL},
    {XcinBGColor,       "XcinBGColor",        "#0000ff", NULL},
    {XcinFontColor,     "XcinFontColor",      "#ffffff", NULL},
    {XcinStatusFGColor, "XcinStatusFGColor",  "#00ffff", NULL},
    {XcinStatusBGColor, "XcinStatusBGColor",  "#000000", NULL},
    {XcinCursorFGColor, "XcinCursorFGColor",  "#ffff00", NULL},
    {XcinCursorBGColor, "XcinCursorBGColor",  "#7f0000", NULL},
    {XcinUnderlineColor,"XcinUnderlineColor", "#00ffff", NULL},
    {MsgboxBGColor,	"MsgboxBGColor", "#eaeaf1", NULL},
    
    {MaxSelectKey,	"MaxSelectKey", "10", NULL},

    {DefauleInputMethod,"DefauleInputMethod", "chewing", NULL},
    {XcinStyleEnabled,"XcinStyleEnabled", "No", NULL},
    {OnSpotEnabled,"OnSpotEnabled", "Yes", NULL}
};

static int N_CONFIG = sizeof(oxim_conf)/sizeof(oxim_conf[0]);

OximConfig *_Config = NULL;

static void _oxim_read_core_config(void)
{
    settings_t *settings = oxim_get_settings("SystemSetting", NULL);
    /* User config file version too old! remove it. */
    if (!settings)
    {
	char *usrconf = oxim_malloc(BUFFER_LEN, False);
	sprintf(usrconf, "%s/%s", _Config->user_dir, OXIM_DEFAULT_RC);
	unlink(usrconf);
	free(usrconf);
	/* Reload system config file */
	settings = oxim_get_settings("SystemSetting", NULL);
	/* If not found. Use internal config setting */
	if (!settings)
	{
	    _Config->config = oxim_conf;
	    return;
	}
    }

    int i, j;
    for (i=0 ; i < N_CONFIG ; i++)
    {
	/* 比對是否有相同的設定 */
	for (j=0 ; j < settings->nsetting; j++)
	{
	    set_item_t *set_item = settings->setting[j];
    	    /* 有相同的 key name 但 value 不同的話，紀錄新值 */
	    if (strcasecmp(oxim_conf[i].key_name, set_item->key) == 0 &&
		strcasecmp(oxim_conf[i].default_value, set_item->value) != 0 )
	    {
		/* 後來的覆蓋前一次設定(if have) */
		if (oxim_conf[i].set_value)
		{
		    free(oxim_conf[i].set_value);
		}
		oxim_conf[i].set_value = strdup(set_item->value);
		break;
	    }
	}
    }
    oxim_settings_destroy(settings); /* 釋放記憶體 */
    _Config->config = oxim_conf;
}

static void _oxim_chk_path(void)
{
    char *usrhome = getenv("HOME"); /* 使用者家目錄 */
    char *user_dir = oxim_malloc(BUFFER_LEN, False);
    char *tables_dir = oxim_malloc(BUFFER_LEN, False);
    char *modules_dir = oxim_malloc(BUFFER_LEN, False);
    char *panels_dir = oxim_malloc(BUFFER_LEN, False);

    /* 沒有的話用小寫取得 */
    if (!usrhome)
	usrhome = getenv("home");
    /* 再沒的話預設為 /tmp */
    if (!usrhome)
	usrhome = "/tmp";

    /* 含中間的 '/' */
    sprintf(user_dir, "%s/%s", usrhome, OXIM_USER_DIR);

    _Config->rc_dir = OXIM_DEFAULT_RCDIR;
    _Config->default_dir = OXIM_DEFAULT_DIR;
    _Config->user_dir = strdup(user_dir);

    /* 家目錄存在 */
    if (oxim_check_file_exist(usrhome, FTYPE_DIR))
    {
	sprintf(tables_dir, "%s/%s", user_dir, "/tables");
	sprintf(modules_dir, "%s/%s", user_dir, "/modules");
	sprintf(panels_dir, "%s/%s", user_dir, "/panels");

	/* 在家目錄建立 oxim 資料目錄 */
	if (!oxim_check_file_exist(user_dir, FTYPE_DIR))
	    mkdir(user_dir, 0700);

	/* 在 oxim 目錄下建立 tables 目錄 */
	if (!oxim_check_file_exist(tables_dir, FTYPE_DIR))
	    mkdir(tables_dir, 0700);

	/* 在 oxim 目錄下建立 modules 目錄 */
	if (!oxim_check_file_exist(modules_dir, FTYPE_DIR))
	    mkdir(modules_dir, 0700);

	/* 在 oxim 目錄下建立 panels 目錄 */
	if (!oxim_check_file_exist(panels_dir, FTYPE_DIR))
	    mkdir(panels_dir, 0700);
    }

    free(user_dir);
    free(tables_dir);
    free(modules_dir);
    free(panels_dir);
}

void oxim_init(void)
{
    if (_Config)
	return;

    _Config = oxim_malloc(sizeof(OximConfig), True);

    /* 設定並檢查相關目錄 */
    _oxim_chk_path();
    /* 讀入設定檔 */
    _oxim_read_core_config();
    /* 檢查所有輸入法表格及模組 */
    OXIM_LoadIMList();
    /* 初始片語輸入 */
    oxim_qphrase_init();
}

void oxim_Reload(void)
{
    oxim_destroy();
    oxim_init();
}

void oxim_destroy(void)
{
    if (!_Config)
	return;

    /* 路徑設定 */
    free(_Config->user_dir);
    OXIM_IMFreeAll();
    /*------------------------------*/

    int i;
    for (i=0 ; i < N_CONFIG ; i++)
    {
	if (_Config->config[i].set_value)
	{
	    free(_Config->config[i].set_value);
	    _Config->config[i].set_value = NULL;
	}
    }

    free(_Config);
    _Config = NULL;
}

/*
  傳回系統設定檔案存放目錄
*/
char *oxim_sys_rcdir(void)
{
    if (!_Config)
	oxim_init();

    return _Config->rc_dir;
}

/*
  傳回系統預設目錄
*/
char *oxim_sys_default_dir(void)
{
    if (!_Config)
	oxim_init();

    return _Config->default_dir;
}

/*
  傳回 User 預設目錄
*/
char *oxim_user_dir(void)
{
    if (!_Config)
	oxim_init();

    return _Config->user_dir;
}

/* 傳回站台列表的 URL */
char *oxim_mirror_url(void)
{
    return OXIM_MIRROR_SITE_URL;
}

/* 傳回自定的下載站台 */
char *oxim_external_url(void)
{
    char *s = getenv("EXTERNAL_DOWNLOAD_URL");
    if( (NULL == s) || ('\0' == s[0]) )
    	return NULL;
    return s;
}

/* 傳回指定的系統參數 */
char *oxim_get_config(int key_id)
{
    if (!_Config)
	oxim_init();

    if (key_id < 0 || key_id >= N_CONFIG)
	return NULL;

    int i;
    for (i=0 ; i < N_CONFIG ; i++)
    {
	if (_Config->config[i].key_id == key_id)
	{
	    if (_Config->config[i].set_value)
		return _Config->config[i].set_value;
	    else
		return _Config->config[i].default_value;
	}
    }
    return NULL;
}

/* 指定新值 */
int oxim_set_config(int key_id, char *value)
{
    if ((key_id >= 0 || key_id < MaxConfig) && value)
    {
	int i;
	for (i=0 ; i < N_CONFIG ; i++)
	{
	    /* 有相同的 key_id */
	    if (oxim_conf[i].key_id == key_id)
	    {
		/* 已經紀錄過新值，就釋放掉 */
		if (oxim_conf[i].set_value)
		{
		    free(oxim_conf[i].set_value);
		    oxim_conf[i].set_value = NULL;
		}
		/* 跟預設值不同才需紀錄 */
		if (strcasecmp(oxim_conf[i].default_value, value) != 0)
		{
		    oxim_conf[i].set_value = strdup(value);
		}
		return True;
	    }
	}
    }
	return False;
}

/* 寫入 ~/.oxim/oxim.conf */
int oxim_make_config(void)
{
    char *buf = oxim_malloc(BUFFER_LEN, False);
    sprintf(buf, "%s/%s", oxim_user_dir(), OXIM_DEFAULT_RC);

    int i;

    FILE *fp = fopen(buf, "w");

    if (!fp)
	return False;

    /* 寫入系統設定 */
    fprintf(fp, "<SystemSetting>\n");
    for (i=0 ; i < N_CONFIG ; i++)
    {
	fprintf(fp, "\t%s = ", oxim_conf[i].key_name);
	if (oxim_conf[i].set_value)
	    fprintf(fp, "\"%s\"\n", oxim_conf[i].set_value);
	else
	    fprintf(fp, "\"%s\"\n", oxim_conf[i].default_value);
    }
    fprintf(fp, "</SystemSetting>\n");

    /* 寫入輸入法設定 */
    im_rc *imlist = &_Config->sys_imlist;
    for (i=0 ; i < imlist->NumberOfIM ; i++)
    {
	fprintf(fp, "\n#\n");
	fprintf(fp, "# %s\n", imlist->IMList[i]->inpname);
	fprintf(fp, "#\n");
	fprintf(fp, "<InputMethod \"%s\">\n", imlist->IMList[i]->objname);

	if (imlist->IMList[i]->key > 0 && imlist->IMList[i]->key <= 10)
	{
	    if (imlist->IMList[i]->key == 10)
	        fprintf(fp, "\tSetKey = 0\n");
	    else
	        fprintf(fp, "\tSetKey = %d\n", imlist->IMList[i]->key);
	}

	if (imlist->IMList[i]->aliasname)
	{
	    fprintf(fp, "\tAliasName = \"%s\"\n", imlist->IMList[i]->aliasname);
	}
	fprintf(fp, "\tCircular = %s\n", imlist->IMList[i]->circular ? "Yes":"No");

	/* 有無設定項目 */
	if (imlist->IMList[i]->settings)
	{
	    int j;
	    settings_t *settings = imlist->IMList[i]->settings;
	    for (j=0 ; j < settings->nsetting ; j++)
	    {
		if (!_is_global_setting(settings->setting[j]->key))
		{
		    fprintf(fp, "\t%s = \"%s\"\n", settings->setting[j]->key, settings->setting[j]->value);
		}
	    }
	}
	/*---------------*/
	fprintf(fp, "</InputMethod>\n");
    }

    fclose(fp);

    return True;
}

char *oxim_version(void)
{
    return (VERSION);
}
