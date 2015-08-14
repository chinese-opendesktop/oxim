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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include "oximtool.h"
#include "oxim-conv.h"
static char validkey[128];

static cintab_head_v1_t th;
static cintab_locale_table *locale_table = NULL;
static cintab_setting_table *setting_table = NULL;


/* 所有詞的字元數目 */
static char *word_table = NULL;
static char *default_selection_keys = "1234567890";

/*--------------------------------------------------------------------------

	Normal cin2tab functions.

--------------------------------------------------------------------------*/

static void cin_keepcase(char *arg, cintab_t *cintab)
{
    th.keep_key_case = True;
}

static void
cin_ename(char *arg, cintab_t *cintab)
{
    if (!arg)
    {
	oxim_perr(OXIMMSG_WARNING, N_("%s(%d): arguement expected.\n"), 
		cintab->fname_cin, cintab->lineno);
	return;
    }
    strncpy(th.orig_name, arg, N_NAME_LENGTH);
    th.orig_name[N_NAME_LENGTH - 1] = '\0';
}

static void
cin_cname(char *arg, cintab_t *cintab)
{
    if (!arg)
    {
	oxim_perr(OXIMMSG_WARNING, N_("%s(%d): arguement expected.\n"),
                cintab->fname_cin, cintab->lineno);
	return;
    }
    if( (strlen(th.cname)+strlen(arg)) < N_NAME_LENGTH )
    	strcat(th.cname, arg);
}

static void
cin_cname_scim(char *arg, cintab_t *cintab)
{
    if (!arg)
    {
	oxim_perr(OXIMMSG_WARNING, N_("%s(%d): arguement expected.\n"),
                cintab->fname_cin, cintab->lineno);
	return;
    }
    if( (strlen(th.orig_name)+strlen(arg)) < N_NAME_LENGTH )
    	strcat(th.orig_name, arg);
}

static void
cin_name(char *arg, cintab_t *cintab)
{
    if (!arg)
    {
	oxim_perr(OXIMMSG_WARNING, N_("%s(%d): arguement expected.\n"),
		cintab->fname_cin, cintab->lineno);
	return;
    }

    th.n_locale ++;
    if (th.n_locale == 1)
    {
	locale_table = oxim_malloc(sizeof(cintab_locale_table), False);
    }
    else
    {
	locale_table = oxim_realloc(locale_table, th.n_locale * sizeof(cintab_locale_table));
    }

    strcpy(locale_table[th.n_locale - 1].locale, "zh_TW");
    strncpy(locale_table[th.n_locale - 1].name, arg, N_NAME_LENGTH);
    locale_table[th.n_locale - 1].name[N_NAME_LENGTH - 1] = '\0';
}

static void
cin_selkey(char *arg, cintab_t *cintab)
{
    if (!arg)
    {
	oxim_perr(OXIMMSG_WARNING, N_("%s(%d): arguement expected.\n"),
		cintab->fname_cin, cintab->lineno);
	return;
    }

    th.n_selection_key = strlen(arg);

    if (th.n_selection_key > N_KEYS)
    {
	oxim_perr(OXIMMSG_NORMAL, N_("%s(%d): too many selection keys defined.\n"), cintab->fname_cin, cintab->lineno);
	th.n_selection_key = 10;
	strcpy(th.selection_keys, "1234567890");
    }
    else
    {
	strcpy(th.selection_keys, arg);
    }
    /* 設定合法字元表 */
    char *s = th.selection_keys;
    for (; *s ; s++)
    {
	validkey[*s] = '\1';
    }
}

static void
scim_selkey(char *arg, cintab_t *cintab)
{
    unsigned int i;
    unsigned int len = strlen(arg);
    char selection_keys[256];

    if (!arg)
    {
	oxim_perr(OXIMMSG_WARNING, N_("%s(%d): arguement expected.\n"),
		cintab->fname_cin, cintab->lineno);
	return;
    }

    bzero(selection_keys, 256);
    char *s = selection_keys;
    for (i=0 ; i < len; i++)
    {
	if (arg[i] != ',')
	{
	    *s = arg[i];
	    s++;
	}
    }

    th.n_selection_key = strlen(selection_keys);
    if (th.n_selection_key > N_KEYS)
    {
	oxim_perr(OXIMMSG_NORMAL, N_("SCIM table \"%s\" line %d: too many selection keys defined. Use default selection keys \"%s\".\n"), cintab->fname_cin, cintab->lineno, default_selection_keys);
	th.n_selection_key = strlen(default_selection_keys);
	strcpy(th.selection_keys, default_selection_keys);
    }
    else
    {
	strcpy(th.selection_keys, selection_keys);
    }
    /* 設定合法字元表 */
    s = th.selection_keys;
    for (; *s ; s++)
    {
	validkey[*s] = '\1';
    }
}

static void
cin_endkey(char *arg, cintab_t *cintab)
{
    if (!arg)
    {
	oxim_perr(OXIMMSG_WARNING, N_("%s(%d): arguement expected.\n"),
		cintab->fname_cin, cintab->lineno);
	return;
    }

    th.n_end_key = strlen(arg);
    if (th.n_end_key > N_KEYS)
    {
	oxim_perr(OXIMMSG_WARNING, N_("%s(%d): too many end keys defined.\n"),
	     cintab->fname_cin, cintab->lineno);
	return;
    }

    strcpy(th.end_keys, arg);
    /* 設定合法字元表 */
    char *s = th.end_keys;
    for (; *s ; s++)
    {
	validkey[*s] = '\1';
    }
}

static void
cin_setting(char *arg, cintab_t *cintab)
{
    char cmd1[N_SETTING_NAME_LENGTH], arg1[N_SETTING_VALUE_LENGTH];
    int i, argc;

    while (argc = cmd_arg(True, cmd1, N_SETTING_NAME_LENGTH, arg1, N_SETTING_VALUE_LENGTH, NULL))
    {
	if (strcasecmp("%setting", cmd1) == 0 && strcasecmp("end", arg1) == 0)
	{
	    break;
	}

	if (argc != 2)
	{
	    continue;
	}

	/* 檢查是否重複定義 */
	int isduplicate = False;
	for (i = 0 ; i < th.n_setting ; i++)
	{
	    if (strcasecmp(setting_table[i].key, cmd1) == 0)
	    {
		isduplicate = True;
		break;
	    }
	}

	if (isduplicate)
	{
	    oxim_perr(OXIMMSG_WARNING, N_("%s(%d): setting->'%s %s' is already defined, overwrite it.\n"), cintab->fname_cin, cintab->lineno, cmd1, arg1);
	    strcpy(setting_table[i].value, arg1);
	    continue;
	}

	th.n_setting ++;
	if (th.n_setting == 1)
	{
	    setting_table = oxim_malloc(sizeof(cintab_setting_table), False);
	}
	else
	{
	    setting_table = oxim_realloc(setting_table, sizeof(cintab_setting_table) * th.n_setting);
	}
	strcpy(setting_table[th.n_setting-1].key, cmd1);
	strcpy(setting_table[th.n_setting-1].value, arg1);
    }
}

static void
cin_keyname(char *arg, cintab_t *cintab)
{
    char cmd1[64], arg1[64];
    int k, argc;

    while (argc = cmd_arg(True, cmd1, 64, arg1, 64, NULL))
    {
	if ((strcasecmp("%keyname", cmd1) == 0 && strcasecmp("end", arg1) == 0) || strcasecmp("END_CHAR_PROMPTS_DEFINITION", cmd1) == 0)
	{
	    break;
	}

	if (argc != 2 || strlen(cmd1) != 1)
	{
	    continue;
	}

	k = cmd1[0];
	if (k <= 0x20 || k > 0x7e)
	{
	    oxim_perr(OXIMMSG_WARNING, N_("%s(%d): illegal key \"%c\" specified.\n"), cintab->fname_cin, cintab->lineno, k);
	    continue;
	}

	if (!th.keep_key_case && k >= 'A' && k <= 'Z')
	{
	    k = tolower(k);
	    cmd1[0] = k;
	}

	if (th.keyname[k].uch)
	{
	    oxim_perr(OXIMMSG_WARNING, N_("%s(%d): key \"%c\" is already in used, overwrite it.\n"), cintab->fname_cin, cintab->lineno, cmd1[0]);
	    strncpy((char *)th.keyname[k].s, arg1, UCH_SIZE);
	    continue;
	}
	strncpy((char *)th.keyname[k].s, arg1, UCH_SIZE);

	validkey[k] = '\1';
	th.n_key_name++;
    }
}

/*------------------------------------------------------------------------*/

typedef struct {
    unsigned char key_len;
    char *keystroke;
    unsigned int order;
    unsigned char word_len;
    unsigned int *word;
} cin_char_t;

static cin_char_t *cchar = NULL;

static int
char_cmp(const void *a, const void *b)
{
    cintab_char_index *aa=(cintab_char_index *)a;
    cintab_char_index *bb=(cintab_char_index *)b;
    if (aa->ucs4 == bb->ucs4)
	return 0;
    else if (aa->ucs4 > bb->ucs4)
	return 1;
    else
	return -1;
}

static int
icode_cmp(const void *a, const void *b)
{
    cin_char_t *aa=(cin_char_t *)a, *bb=(cin_char_t *)b;
    int cmp_val = strcmp(aa->keystroke, bb->keystroke);

    if (cmp_val == 0)
    {
	    if (aa->order < bb->order)
	    {
		return 1;
	    }
	    else if (aa->order > bb->order)
	    {
		return -1;
	    }
	    else
	    {
		return 0;
	    }
    }
    else if (cmp_val > 0)
	return 1;
    else
	return -1;
}

static void
cin_chardef(char *arg, cintab_t *cintab)
{
    char cmd1[64], arg1[1024] = {'\0'}, arg2[32]= {'\0'};
    cin_char_t *cch=NULL;
    int argc;
    unsigned int i, key_len;
    unsigned int len;
    unsigned int ucs4[256];

    while ((argc=cmd_arg(True, cmd1, 64, arg1, 1024, arg2, 32, NULL)))
    {

	if ((strcasecmp("%chardef", cmd1) == 0 && strcasecmp("end", arg1) == 0) || strcasecmp("END_TABLE", cmd1) == 0)
	{
	    break;
	}

	if (argc < 2 || argc > 3)
	{
	    oxim_perr(OXIMMSG_WARNING, N_("%s(%d): arguement expected.\n"),
		cintab->fname_cin, cintab->lineno);
	    continue;
	}

	key_len = strlen(cmd1);

	/* 
	 * 看看字根中，是否有不合法的字元
	 * 所謂不合法，就是在 selkey、keyname、endkey 沒有定義的字元 
	 */
	int allkey_valid = True;
	for (i=0 ; i < key_len ; i++)
	{
	    unsigned int kk = cmd1[i];
	    if (!th.keep_key_case && kk >= 'A' && kk <= 'Z')
	    {
		cmd1[i] = tolower(cmd1[i]);
		kk = cmd1[i];
	    }

	    if (!validkey[kk])
	    {
#if 0
		if (i || cmd1[i] != '#')
		{
		    oxim_perr(OXIMMSG_WARNING, N_("%s(%d): illegal key \"%c\" specified.\n"), cintab->fname_cin, cintab->lineno, cmd1[i]);
		}
		allkey_valid = False;
		break;
#else
		if (i || cmd1[i] != '#')
		{
		    th.keyname[kk].s[0] = kk;
		    th.keyname[kk].s[1] = '\0';
		    validkey[kk] = '\1';
		    th.n_key_name++;
		}
#endif
	    }
	}

	if (!allkey_valid)
	{
	    continue;
	}

	th.n_input_content ++;
	if (th.n_input_content == 1)
	{
	    cchar = oxim_malloc(sizeof(cin_char_t), True);
	}
	else
	{
	    cchar = oxim_realloc(cchar, (th.n_input_content+1) * sizeof(cin_char_t));
	}

	cch = (cchar + th.n_input_content) - 1;
 	bzero(cch, sizeof(cin_char_t));
	cch->key_len = key_len;
	cch->keystroke = strdup(cmd1);

	len = strlen(arg1);
	int nbytes;
	char *p = arg1;
	unsigned int n_char = 0;
	while (len && (nbytes = oxim_utf8_to_ucs4(p, &ucs4[n_char], len)) > 0)
	{
	    p += nbytes;
	    len -= nbytes;
	    n_char ++;
	}
	cch->word_len = n_char;
	unsigned int size = sizeof(unsigned int) * n_char;
	cch->word = oxim_malloc(size, False);
	memcpy(cch->word, ucs4, size);

	/* 累進單一字元數目 */
	if (n_char == 1)
	{
	    th.n_char ++;
	}

	if (arg2[0] != '\0')
	{
	    cch->order = atoi(arg2);
	}

	if (th.n_max_keystroke < key_len)
	    th.n_max_keystroke = key_len;

	arg2[0] = '\0';
    }
}


/*----------------------------------------------------------------------------

	Main Functions.

----------------------------------------------------------------------------*/

struct genfunc_s {
    char *name;
    void (*func) (char *, cintab_t *);
};

static struct genfunc_s genfunc[] = {
    {"%keep_key_case", cin_keepcase},

    {"%setting", cin_setting},

    {"%ename", cin_ename},
/*    {"NAME", cin_cname},*/

    {"%cname", cin_cname},
    {"%prompt", cin_cname},

    {"%keyname", cin_keyname},
    {"BEGIN_CHAR_PROMPTS_DEFINITION", cin_keyname},

    {"%selkey", cin_selkey},
    {"SELECT_KEYS", scim_selkey},

    {"%endkey", cin_endkey},
    {"KEY_END_CHARS", cin_endkey},

    {"%chardef", cin_chardef},
    {"BEGIN_TABLE", cin_chardef},
    {NULL, NULL}
};


void
gencin(cintab_t *cintab)
{
    unsigned int i, argc;
    char cmd[256], arg1[256], arg2[256];
    size_t ret;

    bzero(validkey, 128);
    bzero(&th, sizeof(cintab_head_v1_t));

    while (argc = cmd_arg(False, cmd, 256, arg1, 256, arg2, 256, NULL))
    {
	for (i=0; genfunc[i].name != NULL; i++)
	{
	    if (strcasecmp(genfunc[i].name, cmd) == 0)
	    {
		if (strlen(arg1) == 1 && arg1[0] == '=')
		{
		    genfunc[i].func(arg2, cintab);
		}
		else if (argc == 2)
		{
		    genfunc[i].func(arg1, cintab);
		}
		else
		{
		    genfunc[i].func(NULL, cintab);
		}
		break;
	    }
	    /* SCIM 的 Locale Name */
	    else if (strcasecmp("NAME", cmd) == 0)
	    {
	    	/* */
	        char tmp[128];
		bzero(tmp, sizeof(tmp));
		sprintf(tmp, "%s:en;", arg2);
		cin_cname_scim(tmp, cintab);
		break;
	    }
	    else if (strncasecmp("NAME.", cmd, 5) == 0)
	    {
	    	/* 以 XXX:zh_TW;YYY:zh_CN 的格式將輸入法表格名稱與locale一起加進去*/
	        char tmp[128];
		char *charset = cmd+5;
		bzero(tmp, sizeof(tmp));
		sprintf(tmp, "%s:%s;", arg2, charset);
		cin_cname_scim(tmp, cintab);
		break;
	    }
	}
	if (genfunc[i].name == NULL)
	    oxim_perr(OXIMMSG_NORMAL, N_("%s(%d): unknown command: %s, ignore.\n"), cintab->fname_cin, cintab->lineno, cmd);
    }


#if 1
    /* 對輸入內容排序 */
    stable_sort(cchar, th.n_input_content, sizeof(cin_char_t), icode_cmp);

    unsigned int prefix_size = sizeof(table_prefix_t); /* 檔案識別結構 size */
    unsigned int hdr_size = sizeof(cintab_head_v1_t); /* 表頭 size */
    /* 國家地區列表 size */
    unsigned int locale_size = sizeof(cintab_locale_table) * th.n_locale;
    /* 設定列表 size */
    unsigned int setting_size = sizeof(cintab_setting_table) * th.n_setting;
    /* 位置索引列表 size */
    unsigned int offset_size = sizeof(unsigned int) * (th.n_input_content + 1);
    unsigned int *offset_table = oxim_malloc(offset_size, False);

    /* 單字索引 size */
    unsigned int char_index_size = sizeof(cintab_char_index) * th.n_char;
    cintab_char_index *char_idx = NULL;
    if (th.n_char)
    {
	char_idx = oxim_malloc(char_index_size, False);
    }

    cin_char_t *cch;
    unsigned int idx = 0;
    unsigned int offset = prefix_size + hdr_size + locale_size + setting_size + offset_size + char_index_size;
    unsigned int rec_len = 0;
    for (i=0, cch = cchar ; i < th.n_input_content ; i++, cch++)
    {
	offset_table[i] = offset;
	/* 建立字元索引 */
	if (cch->word_len == 1)
	{
	    char_idx[idx].ucs4 = cch->word[0];
	    char_idx[idx].offset = offset;
	    idx ++;
	}
	/* 1 + 1 + 字根長度 + 字詞長度 */
	rec_len = sizeof(unsigned char) + sizeof(unsigned char) + cch->key_len + (cch->word_len * sizeof(unsigned int));
	offset += rec_len;
    }
    offset_table[i] = offset;

    /* 依據字元排序 */
    if (char_idx)
    {
	stable_sort(char_idx, th.n_char, sizeof(cintab_char_index), char_cmp);
    }

    /* locale table offset */
    th.locale_table_offset = prefix_size + hdr_size;
    /* setting table offset */
    th.setting_table_offset = th.locale_table_offset + locale_size;
    /* offset table offset */
    th.offset_table_offset = th.setting_table_offset + setting_size;
    /* char offset */
    th.char_offset = th.offset_table_offset + offset_size;
    /* input content offset */
    th.input_content_offset = th.char_offset + char_index_size;

    /* 計算 Checksum */
    th.chksum = crc32(th.chksum, (char *)&th, sizeof(cintab_head_v1_t) - sizeof(unsigned int));
    /* 寫入表頭 */
    gzwrite(cintab->fw, &th, hdr_size);
    /* 寫入國家地區列表 */
    if (th.n_locale)
    {
	gzwrite(cintab->fw, locale_table, locale_size);
    }
    /* 寫入設定列表 */
    if (th.n_setting)
    {
	gzwrite(cintab->fw, setting_table, setting_size);
    }
    /* 寫入位置索引列表 */
    gzwrite(cintab->fw, offset_table, offset_size);
    /* 寫入字元索引列表 */
    if (th.n_char)
    {
	gzwrite(cintab->fw, char_idx, char_index_size);
    }
    /* 最後寫入真正的資料 */
    for (i=0, cch = cchar ; i < th.n_input_content ; i++, cch++)
    {
	/* 字根長度 */
	gzwrite(cintab->fw, &cch->key_len, sizeof(unsigned char));
	/* 字詞長度 */
	gzwrite(cintab->fw, &cch->word_len, sizeof(unsigned char));
	/* 字根資料 */
	gzwrite(cintab->fw, cch->keystroke, cch->key_len);
	/* 字詞資料 */
	gzwrite(cintab->fw, cch->word, cch->word_len * sizeof(unsigned int));
    }

#endif
    oxim_perr(OXIMMSG_NORMAL,
	N_("number of locale: %d\n"), th.n_locale);
    oxim_perr(OXIMMSG_NORMAL,
	N_("number of settings: %d\n"), th.n_setting);
    oxim_perr(OXIMMSG_NORMAL,
	N_("number of keyname: %d\n"), th.n_key_name);
    oxim_perr(OXIMMSG_NORMAL, 
	N_("max length of keystroke: %d\n"), th.n_max_keystroke);
    oxim_perr(OXIMMSG_NORMAL, 
	N_("number of keystroke code defined: %d\n"), th.n_input_content);
    oxim_perr(OXIMMSG_NORMAL, 
	N_("number of char defined: %d\n"), th.n_char);
    oxim_perr(OXIMMSG_NORMAL, 
	N_("number of phrase define: %d\n"), th.n_input_content - th.n_char);
}
