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

set_item_t *oxim_get_key_value(char *line)
{
    set_item_t *set_item = oxim_malloc(sizeof(set_item_t), True);
    if (!set_item)
	return NULL;

    char cmd[BUFFER_LEN];
    int isValid = True;
    int idx = 0;

    while (isValid && oxim_get_word(&line, cmd, BUFFER_LEN, "="))
    {
	switch (idx)
	{
	    case 0: /* Key */
		set_item->key = strdup(cmd);
		break;
	    case 1: /* = */
		if (strcmp("=", cmd) != 0)
		{
		    isValid = False;
		}
		break;
	    case 2: /* Value */
		if (strlen(cmd))
		{
		    set_item->value = strdup(cmd);
		}
		else
		{
		    isValid = False;
		}
		break;
	    default:
		isValid = False;
	}
	idx ++;
    }

    if (!isValid || !set_item->key || !set_item->value)
    {
	oxim_key_value_destroy(set_item);
	set_item = NULL;
    }

    return set_item;
}

/* 建立設定群組 */
settings_t *oxim_settings_create(void)
{
    settings_t *settings = oxim_malloc(sizeof(settings_t), True);

    if (settings)
    {
	settings->nsetting = 0;
    }
    return settings;
}

/* 是否是合法的表格設定項目 */
int _is_global_setting(const char *key)
{
    if (strcasecmp(key, "SetKey") == 0 ||
	strcasecmp(key, "Circular") == 0 ||
	strcasecmp(key, "AliasName") == 0 )
    {
	return True;
    }
    return False;
}

int oxim_settings_add(settings_t *settings, set_item_t *set_item)
{
    if (settings && set_item)
    {
	settings->nsetting++;
	if (settings->nsetting == 1)
	{
	    settings->setting = (set_item_t **)oxim_malloc(sizeof(set_item_t *), True);
	}
	else
	{
	    settings->setting = (set_item_t **)oxim_realloc(settings->setting, sizeof(set_item_t *) * settings->nsetting);
	}
	settings->setting[settings->nsetting - 1] = set_item;
	return True;
    }
    return False;
}

/* 新增一項設定 */
int oxim_settings_add_key_value(settings_t *settings, const char *key, const char *value)
{
    if (settings && key && value)
    {
	set_item_t *set_item = oxim_malloc(sizeof(set_item_t), False);
	if (!set_item)
	{
	    return False;
	}

	set_item->key = strdup(key);
	set_item->value = strdup(value);

	settings->nsetting++;
	if (settings->nsetting == 1)
	{
	    settings->setting = (set_item_t **)oxim_malloc(sizeof(set_item_t *), True);
	}
	else
	{
	    settings->setting = (set_item_t **)oxim_realloc(settings->setting, sizeof(set_item_t *) * settings->nsetting);
	}
	settings->setting[settings->nsetting - 1] = set_item;
    }

    return True;
}

/* 替代一項設定 */
void oxim_settings_replace(settings_t *settings, const char *key, const char *value)
{
    unsigned int i;
    if (!settings)
    {
	return;
    }
    
    /* 找相同的 Key 且 Value 不同 */
    for (i=0 ; i < settings->nsetting ; i++)
    {
	if (strcasecmp(key, settings->setting[i]->key) == 0 &&
		strcasecmp(value, settings->setting[i]->value) != 0)
	{
	    unsigned int old_len = strlen(settings->setting[i]->value);
	    unsigned int new_len = strlen(value);
	    /* 新的字串長度沒有超過舊的字串長度，就直接取代 */
	    if (new_len <= old_len)
	    {
		strcpy(settings->setting[i]->value, value);
	    }
	    else /* 否則釋放舊的字串，重新配置 */
	    {
		free(settings->setting[i]->value);
		settings->setting[i]->value = strdup(value);
	    }
	    break;
	}
    }
}

/* 釋放設定項目 */
void oxim_settings_destroy(settings_t *settings)
{
    if (settings)
    {
	int i;
	for (i=0 ; i < settings->nsetting ; i++)
	{
	    oxim_key_value_destroy(settings->setting[i]);
	}
	free(settings);
    }
}

/* 取預設的表格輸入法設定 */
settings_t *oxim_system_table_settings(void)
{
    /* 系統預設 */
    settings_t *sys_setting = oxim_settings_create();
    if (!sys_setting)
	return NULL;

    /* 自動顯示組字結果 */
    oxim_settings_add_key_value(sys_setting, AutoCompose, "Yes");
    /* 自動出字 */
    oxim_settings_add_key_value(sys_setting, AutoUpChar, "Yes");
    /* 滿字根自動出字 */
    oxim_settings_add_key_value(sys_setting, AutoFullUp, "No");
    /* 空白鍵自動出字 */
    oxim_settings_add_key_value(sys_setting, SpaceAutoUp, "No");
    /* 選字鍵位移 */
    oxim_settings_add_key_value(sys_setting, SelectKeyShift, "No");
    /* 滿字根出字時，忽略後續空白 */
    oxim_settings_add_key_value(sys_setting, SpaceIgnore, "Yes");
    /* 錯誤按空白鍵清除 */
    oxim_settings_add_key_value(sys_setting, SpaceReset, "Yes");
    /* 啟用萬用字元查詢 */
    oxim_settings_add_key_value(sys_setting, WildEnable, "Yes");
    /* 按下結束鍵出字 */
    oxim_settings_add_key_value(sys_setting, EndKey, "No");
    /* 特殊字根定義 */
    oxim_settings_add_key_value(sys_setting, DisableSelectList, "None");

    return (sys_setting);
}

/* 讀取輸入法表格內建的設定 */
settings_t *oxim_get_default_settings(const char *imname, const int force)
{
    if (!_Config)
    {
	oxim_init();
    }
    im_rc imlist = _Config->sys_imlist;

    settings_t *default_setting = NULL;
    int i, is_v1_table = False;
    im_t *imt;
    /* 查驗該輸入法是否是表格式輸入法，因為只有新版表格式輸入法有內建設定 */
    for (i=0 ; !force && i < imlist.NumberOfIM ; i++)
    {
	imt = imlist.IMList[i];
	if (strcmp(imt->modname, "gen-inp-v1") == 0 &&
		strcmp(imt->objname, imname) == 0)
	{
	    is_v1_table = True;
	    break;
	}
    }

    /* 是新版的表格輸入法，讀取內建設定 */
    if (is_v1_table || force)
    {
	char value[128], truefn[256];
	gzFile *zfp;
	sprintf(value, "%s.tab", imname);
	if (oxim_check_datafile(value, "tables", truefn, 256) == True)
	{
	    unsigned int size = sizeof(cintab_head_v1_t);
	    cintab_head_v1_t *header = oxim_malloc(size, False);
	    if ((zfp = gzopen(truefn, "rb")))
	    {
		/* 跳過 prefix header，因為 oxim_init() 已經檢查過了 */
		gzseek(zfp, MODULE_ID_SIZE, SEEK_SET);
		/* 讀表頭資料 */
		gzread(zfp, header, size);
		if (header->n_setting)
		{
		    default_setting = oxim_settings_create();
		    /* 讀取輸入法內建設定 */
		    size = sizeof(cintab_setting_table) * header->n_setting;
		    cintab_setting_table *cst = oxim_malloc(size, False);
		    gzseek(zfp, header->setting_table_offset, SEEK_SET);
		    gzread(zfp, cst, size);
		    for (i=0 ; i < header->n_setting ; i++)
		    {
			oxim_settings_add_key_value(default_setting, cst[i].key, cst[i].value);
		    }
		    free(cst);
		}
		gzclose(zfp);
	    }
	    free(header);
	}
    }

    return default_setting;
}

/*
    讀取指定 Tag 設定
*/
/* 使用者定義 > 輸入法定義 > 系統預設 */
settings_t *oxim_get_im_settings(const char *imname)
{
    if (!_Config)
    {
	oxim_init();
    }

    im_rc imlist = _Config->sys_imlist;
    int i, is_v1_table = False, is_module = False, found = False;
    im_t *imt;

    /* 讀取設定檔定義內容 */
    settings_t *user_setting = oxim_get_settings("InputMethod", imname);

    /* 查驗該輸入法是否是表格式輸入法，因為只有新版表格式輸入法有內建設定 */
    for (i=0 ; i < imlist.NumberOfIM ; i++)
    {
	imt = imlist.IMList[i];
	if (strcmp(imt->objname, imname) == 0)
	{
	    found = True;
	    if (strcmp(imt->modname, "gen-inp-v1") == 0)
	    {
		is_v1_table = True;
	    }
	    else if (strncmp(imt->modname, "gen-inp", 7) != 0)
	    {
		is_module = True;
	    }
	    break;
	}
    }

    if (!found)
    {
	return NULL;
    }

    /* 模組的話，直接傳回使用者設定 */
    if (is_module && user_setting)
    {
	return user_setting;
    }

    /* 系統預設 */
    settings_t *sys_setting = oxim_system_table_settings();

    /* 如果有讀到使用者自訂的話，合併系統預設 */
    if (user_setting)
    {
	char *string;
	for (i=0 ; i < user_setting->nsetting ; i++)
	{
	    if (oxim_setting_GetString(sys_setting, user_setting->setting[i]->key, &string))
	    {
		oxim_settings_replace(sys_setting, user_setting->setting[i]->key, user_setting->setting[i]->value);
	    }
	    else
	    {
		oxim_settings_add_key_value(sys_setting, user_setting->setting[i]->key, user_setting->setting[i]->value);
	    }
	}
	oxim_settings_destroy(user_setting);
	/* 傳回合併結果 */
	return sys_setting;
    }
    /* 新版的表格輸入法 */
    settings_t *default_setting = NULL;
    if (is_v1_table)
    {
	default_setting = oxim_get_default_settings(imname, True);
	if (default_setting)
	{
	    char *string;
	    for (i=0 ; i < default_setting->nsetting ; i++)
	    {
	        if (oxim_setting_GetString(sys_setting, default_setting->setting[i]->key, &string))
		{
		    oxim_settings_replace(sys_setting, default_setting->setting[i]->key, default_setting->setting[i]->value);
		}
	    }
	    oxim_settings_destroy(default_setting);
	}
    }
    return (sys_setting);
}

/*
    讀取指定 Tag 設定
*/
settings_t *oxim_get_settings(const char *tag, const char *subname)
{
    char tmp[BUFFER_LEN];
    char buf[BUFFER_LEN];
    char *rc_name = OXIM_DEFAULT_RC;
    int chk_ok = False;
    char *s;

    if (!tag)
    {
	return NULL;
    }

    /* User: $HOME/.oxim/oxim.conf */
    sprintf(tmp, "%s/%s", _Config->user_dir, rc_name);
    if (oxim_check_file_exist(tmp, FTYPE_FILE))
	chk_ok = True;

    /* Default: /etc/oxim/oxim.conf */
    if (!chk_ok)
    {
	sprintf(tmp, "%s/%s", _Config->rc_dir, rc_name);
	if (oxim_check_file_exist(tmp, FTYPE_FILE))
	    chk_ok = True;
	
    }

    if (!chk_ok)
    {
	oxim_perr(OXIMMSG_ERROR, N_("'%s' not found.\n"), rc_name);
    }

    gzFile *fp = oxim_open_file(tmp, "r", OXIMMSG_WARNING);
    if (!fp)
	return NULL;

    settings_t *settings = oxim_settings_create();
    if (!settings)
	return NULL;

    enum
    {
	FIND_TAG,
	FIND_END
    };

    int find_mode = FIND_TAG;

    while (oxim_get_line(buf, BUFFER_LEN, fp, NULL, "#\r\n"))
    {
	s = buf;
	char *start_tag = index(s, '<');
	char *end_tag = rindex(s, '>');

	/* 找到起始邊界，但沒有結束邊界，不合法！忽略 */
	if (start_tag && !end_tag)
	   continue;

	/* 搜尋標籤狀態 */
	if (find_mode == FIND_TAG)
	{
	    /* 找不到起始邊界就忽略 */
	    if (!start_tag)
		continue;

	    /* 比對是否為指定標籤 */
	    int idx = 0;
	    int isok = True;
	    while (isok && oxim_get_word(&start_tag, tmp, BUFFER_LEN, "<>"))
	    {
		switch (idx)
		{
		    case 1: /* Tag Name */
			if (strcasecmp(tmp, tag) != 0)
			{
			    isok = False;
			}
			break;
		    case 2: /* Sub Name or '>' */
			/* 有指定 Sub Name */
			if (subname)
			{
			    if (strcasecmp(tmp, subname) != 0)
				isok = False;
			}
			else /* 沒指定 Sub Name */
			{
			    if (tmp[0] != '>')
				isok = False;
			}
			break;
		}
		idx ++;
	    }
	    if (isok)
		find_mode = FIND_END;
	}
	else /* 讀入設定，一直到遇到下一個起始邊界為止 */
	{
	    /* 遇到另一個起始邊界符號就停止 */
	    if (start_tag)
		break;

	    set_item_t *set_item = oxim_get_key_value(s);
	    if (set_item)
	    {
		if (!oxim_settings_add(settings, set_item))
		    oxim_key_value_destroy(set_item);
	    }
	}
    }
    gzclose(fp);

    if (!settings->nsetting)
    {
	oxim_settings_destroy(settings);
	settings = NULL;
    }

    return settings;
}

void oxim_key_value_destroy(set_item_t *set_item)
{
    if (set_item)
    {
	if (set_item->key)
	    free(set_item->key);
	if (set_item->value)
	    free(set_item->value);
	free(set_item);
    }
}

/* 取字串 */
int oxim_setting_GetString(settings_t *settings, char *key, char **ret)
{
    int match = False;
    int i;

    if (settings)
    {
	for (i=0; i < settings->nsetting ; i++)
	{
	    if (strcasecmp(key, settings->setting[i]->key) == 0)
	    {
		*ret = settings->setting[i]->value;
		match = True;
		break;
	    }
	}
    }

    return match;
}

/* 取整數值 */
int oxim_setting_GetInteger(settings_t *settings, char *key, int *ret)
{
    int match = False;
    int i;

    if (settings)
    {
	for (i=0; i < settings->nsetting ; i++)
	{
	    if (strcasecmp(key, settings->setting[i]->key) == 0)
	    {
		*ret = atoi(settings->setting[i]->value);
	        match = True;
	        break;
	    }
	}
    }

    return match;
}

/* 取真偽值 */
int oxim_setting_GetBoolean(settings_t *settings, char *key, int *ret)
{
    int match = False;
    int i;

    if (settings)
    {
	for (i=0; i < settings->nsetting ; i++)
	{
	    if (strcasecmp(key, settings->setting[i]->key) == 0)
	    {
		char *value = settings->setting[i]->value;
		if (strcasecmp("YES", value) == 0 ||
		    strcasecmp("TRUE", value) == 0)
		{
		    *ret = True;
		}
		else
		{
		    *ret = False;
		}
	        match = True;
	        break;
	    }
	}
    }

    return match;
}

/*
* 設定/移除旗標
*
* ref  : int 位址
* flag : 旗標
* mode : 1:設立, 0:移除
*/
void oxim_setflag(void *ref, unsigned int flag, int mode)
{
    if (mode)
	*(unsigned int *)ref |= flag;
    else
	*(unsigned int *)ref &= (~flag);
}
