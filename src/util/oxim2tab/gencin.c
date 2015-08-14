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
#include "oxim2tab.h"
static char validkey[128];

static cintab_head_t th;
static ichar_t *ichar = NULL;
static icode_t *icode1 = NULL;
static icode_t *icode2 = NULL;


/* 所有詞的字元數目 */
static unsigned int n_word = 0;
static unsigned int table_size = 0;
static char *word_table = NULL;
static char *default_selection_keys = "1234567890";

/*--------------------------------------------------------------------------

	Normal cin2tab functions.

--------------------------------------------------------------------------*/

static void
cin_ename(char *arg, cintab_t *cintab)
{
    if (! arg[0])
	oxim_perr(OXIMMSG_ERROR, N_("%s(%d): arguement expected.\n"), 
		cintab->fname_cin, cintab->lineno);
    strncpy(th.ename, arg, CIN_ENAME_LENGTH);
    th.ename[CIN_ENAME_LENGTH - 1] = '\0';
}

static void
cin_cname(char *arg, cintab_t *cintab)
{
    if (! arg[0])
	oxim_perr(OXIMMSG_ERROR, N_("%s(%d): arguement expected.\n"),
		cintab->fname_cin, cintab->lineno);
    strncpy(th.cname, arg, CIN_CNAME_LENGTH);
    th.cname[CIN_CNAME_LENGTH - 1] = '\0';
}

static void
cin_selkey(char *arg, cintab_t *cintab)
{
    if (! arg[0])
	oxim_perr(OXIMMSG_ERROR, N_("%s(%d): arguement expected.\n"),
		cintab->fname_cin, cintab->lineno);
    th.n_selkey = strlen(arg);

    if (th.n_selkey > SELECT_KEY_LENGTH)
    {
	oxim_perr(OXIMMSG_NORMAL, N_("%s(%d): too many selection keys defined.\n"), cintab->fname_cin, cintab->lineno);
	th.n_selkey = 10;
	strcpy(th.selkey, "1234567890");
    }
    else
    {
	strcpy(th.selkey, arg);
    }
    /* 設定合法字元表 */
    char *s = th.selkey;
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

    if (!arg[0])
    {
	oxim_perr(OXIMMSG_ERROR, N_("%s(%d): arguement expected.\n"),
		cintab->fname_cin, cintab->lineno);
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

    th.n_selkey = strlen(selection_keys);
    if (th.n_selkey > SELECT_KEY_LENGTH)
    {
	oxim_perr(OXIMMSG_NORMAL, N_("SCIM table \"%s\" line %d: too many selection keys defined. Use default selection keys \"%s\".\n"), cintab->fname_cin, cintab->lineno, default_selection_keys);
	th.n_selkey = strlen(default_selection_keys);
	strcpy(th.selkey, default_selection_keys);
    }
    else
    {
	strcpy(th.selkey, selection_keys);
    }
    /* 設定合法字元表 */
    s = th.selkey;
    for (; *s ; s++)
    {
	validkey[*s] = '\1';
    }
}

static void
cin_endkey(char *arg, cintab_t *cintab)
{
    if (! arg[0])
	oxim_perr(OXIMMSG_ERROR, N_("%s(%d): arguement expected.\n"),
		cintab->fname_cin, cintab->lineno);
    th.n_endkey = strlen(arg);
    if (th.n_endkey > END_KEY_LENGTH)
	oxim_perr(OXIMMSG_ERROR, N_("%s(%d): too many end keys defined.\n"),
	     cintab->fname_cin, cintab->lineno);
    strcpy(th.endkey, arg);
    /* 設定合法字元表 */
    char *s = th.endkey;
    for (; *s ; s++)
    {
	validkey[*s] = '\1';
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

	cmd1[0] = tolower(cmd1[0]);
	if (! (k = oxim_key2code(cmd1[0])))
	{
	    oxim_perr(OXIMMSG_ERROR, N_("%s(%d): illegal key \"%c\" specified.\n"), cintab->fname_cin, cintab->lineno, cmd1[0]);
	}

	if (th.keyname[k].uch)
	{
	    oxim_perr(OXIMMSG_NORMAL, N_("%s(%d): key \"%c\" is already in used.overwrite it.\n"), cintab->fname_cin, cintab->lineno, cmd1[0]);
	    strncpy((char *)th.keyname[k].s, arg1, UCH_SIZE);
	    continue;
	}

	validkey[cmd1[0]] = '\1';

	if (! read_hexwch(th.keyname[k].s, arg1))
	    strncpy((char *)th.keyname[k].s, arg1, UCH_SIZE);
	th.n_keyname++;
    }
}

/*------------------------------------------------------------------------*/

typedef struct {
    unsigned int key[2];
    unsigned int ucs4;
    unsigned int order;
    char *keystroke;
    unsigned short word_len;
    char *word;
} cin_char_t;

static cin_char_t *cchar = NULL;

static int
icode_cmp(const void *a, const void *b)
{
    cin_char_t *aa=(cin_char_t *)a, *bb=(cin_char_t *)b;

    if (aa->key[0] == bb->key[0]) {
	if (aa->key[1] == bb->key[1])
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
	else if (aa->key[1] > bb->key[1])
	{
	    return 1;
	}
	else
	{
	    return -1;
	}
    }
    else if (aa->key[0] > bb->key[0])
	return 1;
    else
	return -1;
}

static void
cin_chardef(char *arg, cintab_t *cintab)
{
    char cmd1[64], arg1[1024] = {'\0'}, arg2[32]= {'\0'};
    cin_char_t *cch=NULL;
    int len, ret, argc;
    unsigned int i, key, key_len;

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

	/* TODO : 字根目前限制不能超過 10 個(未來要改為不限制)*/
	key_len = strlen(cmd1);
	if (key_len > 10)
	{
	    continue;
	}

	/* 
	 * 看看字根中，是否有不合法的字元
	 * 所謂不合法，就是在 selkey、keyname、endkey 沒有定義的字元 
	 */
	int allkey_valid = True;
	for (i=0 ; i < key_len ; i++)
	{
	    cmd1[i] = tolower(cmd1[i]);
	    int kk = cmd1[i];
	    if (!validkey[kk])
	    {
		if (i || cmd1[i] != '#')
		{
		    oxim_perr(OXIMMSG_WARNING, N_("%s(%d): illegal key \"%c\" specified.\n"), cintab->fname_cin, cintab->lineno, cmd1[i]);
		}
		allkey_valid = False;
		break;
	    }
	}
	if (!allkey_valid)
	{
	    continue;
	}

	th.n_icode ++;
	if (th.n_icode == 1)
	{
	    cchar = oxim_malloc(sizeof(cin_char_t), True);
	}
	else
	{
	    cchar = oxim_realloc(cchar, (th.n_icode+1) * sizeof(cin_char_t));
	}

	cch = (cchar + th.n_icode) - 1;
 	bzero(cch, sizeof(cin_char_t));

	if ((cch->ucs4 = ccode_to_ucs4(arg1)) == 0)
	{
	    n_word ++;
	    cch->word_len = strlen(arg1);
	    cch->word = strdup(arg1);
	    table_size += cch->word_len + sizeof(cch->word_len);
	}

	if (arg2[0] != '\0')
	{
	    cch->order = atoi(arg2);
	}

	oxim_keys2codes(cch->key, 2, cmd1);
	cch->keystroke = strdup(cmd1);

	th.n_ichar ++;
	len = strlen(cmd1);
	if (th.n_max_keystroke < len)
	    th.n_max_keystroke = len;

	arg2[0] = '\0';
    }

    /*
     *  Determine the memory model.
     */
    ret = (th.n_max_keystroke <= 5) ? ICODE_MODE1 : ICODE_MODE2;
    th.icode_mode = ret;

    /*
     *  Fill in the ichar, icode_idx and icode1, icode2 arrays.
     */
    stable_sort(cchar, th.n_icode, sizeof(cin_char_t), icode_cmp);

    ichar = oxim_malloc(th.n_icode * sizeof(ichar_t), True);
    icode1 = oxim_malloc(th.n_icode*sizeof(icode_t), True);
    if (ret == ICODE_MODE2)
    {
        icode2 = oxim_malloc(th.n_icode*sizeof(icode_t), True);
    }

    if (n_word)
    {
	word_table = oxim_malloc(table_size, True);
    }

    unsigned int word_idx = 0;
    unsigned int offset = 0;
    unsigned int rec_len = 0;
    for (i=0, cch=cchar; i<th.n_icode; i++, cch++)
    {
	ichar[i] = cch->ucs4;
	/* ucs4 為 0 表示該筆紀錄是詞而不是字元 */
	if (!ichar[i])
	{
	    /* usc4 改成指向 offset_table 索引，再 or 0x80000000，讓程式可以判別這是字元或是指向 offset table */
	    /* 紀錄詞的偏移位置 */
	    ichar[i] = WORD_MASK | offset;
	    /* 計算下一個 offset */
	    rec_len = sizeof(cch->word_len) + cch->word_len;
	    memcpy(word_table + offset, &cch->word_len, sizeof(cch->word_len));
	    memcpy(word_table + offset + sizeof(cch->word_len), cch->word, cch->word_len);
	    offset += rec_len;
	    //free(cch->word);
	}
	icode1[i] = (icode_t)(cch->key[0]);
        if (ret == ICODE_MODE2)
	    icode2[i] = (icode_t)(cch->key[1]);
    }
    //free(cchar);
}


/*----------------------------------------------------------------------------

	Main Functions.

----------------------------------------------------------------------------*/

struct genfunc_s {
    char *name;
    void (*func) (char *, cintab_t *);
    byte_t gotit;
};

static struct genfunc_s genfunc[] = {
    {"%ename", cin_ename, 1},
    {"NAME", cin_ename, 1},
    {"%cname", cin_cname, 1},
    {"%prompt", cin_cname, 1},
    {"%keyname", cin_keyname, 1},
    {"BEGIN_CHAR_PROMPTS_DEFINITION", cin_keyname, 1},
    {"%selkey", cin_selkey, 1},
    {"SELECT_KEYS", scim_selkey, 1},
    {"%endkey", cin_endkey, 1},
    {"KEY_END_CHARS", cin_endkey, 1},
    {"%chardef", cin_chardef, 1},
    {"BEGIN_TABLE", cin_chardef, 1},
    {NULL, NULL, 0}
};

void
gencin(cintab_t *cintab)
{
    int i;
    char cmd[64], arg1[64], arg2[64];
    size_t ret;

    bzero(validkey, 128);
    bzero(&th, sizeof(cintab_head_t));

    strncpy(th.version, GENCIN_VERSION, VERLEN);
    strncpy(th.encoding, "UTF-8", ENCLEN);

    while (cmd_arg(False, cmd, 64, arg1, 64, arg2, 64, NULL)) {
	for (i=0; genfunc[i].name != NULL; i++)
	{
	    if (strcasecmp(genfunc[i].name, cmd) == 0)
	    {
		if (strlen(arg1) == 1 && arg1[0] == '=')
		{
		    genfunc[i].func(arg2, cintab);
		}
		else
		{
		    genfunc[i].func(arg1, cintab);
		}
		genfunc[i].gotit = 1;
		break;
	    }
	    /* SCIM 的 Locale Name */
	    else if (strncasecmp("NAME.", cmd, 5) == 0)
	    {
		cin_cname(arg2, cintab);
	    }
	}
	if (genfunc[i].name == NULL)
	    oxim_perr(OXIMMSG_NORMAL, N_("%s(%d): unknown command: %s, ignore.\n"), cintab->fname_cin, cintab->lineno, cmd);
    }

    for (i=0; genfunc[i].name != NULL; i++) {
	if (genfunc[i].gotit == 0)
	    oxim_perr(OXIMMSG_ERROR, N_("%s: command \"%s\" not specified.\n"),
		 cintab->fname_cin, genfunc[i].name);
    }
    oxim_perr(OXIMMSG_NORMAL,
	N_("number of keyname: %d\n"), th.n_keyname);
    oxim_perr(OXIMMSG_NORMAL, 
	N_("max length of keystroke: %d\n"), th.n_max_keystroke);
    oxim_perr(OXIMMSG_NORMAL, 
	N_("number of keystroke code defined: %d\n"), th.n_icode);
    oxim_perr(OXIMMSG_NORMAL, 
	N_("number of char defined: %d\n"), th.n_icode - n_word);
    oxim_perr(OXIMMSG_NORMAL, 
	N_("number of word: %d\n"), n_word);
    oxim_perr(OXIMMSG_NORMAL, 
	N_("memory model: %d\n"), th.icode_mode);

    if (!cintab->fname_outcin)
    {
	ret = gzwrite(cintab->fw, &th, sizeof(cintab_head_t));
	ret = gzwrite(cintab->fw, icode1, sizeof(icode_t) * th.n_icode);
	ret = gzwrite(cintab->fw, ichar, sizeof(ichar_t) * th.n_icode);

	if (th.icode_mode == ICODE_MODE2)
	{
	    ret = gzwrite(cintab->fw, icode2, sizeof(icode_t) * th.n_icode);
	}

	if (n_word)
	{
	    ret = gzwrite(cintab->fw, word_table, table_size);
	}
	return;
    }

    FILE *fp = cintab->cinfp;

    /* 標準 cin 檔頭標記 */
    fprintf(fp, "%%gen_inp\n");

    /* English Name */
    if (strlen(th.ename))
    {
	fprintf(fp, "%%ename %s\n", th.ename);
    }

    /* Chinese Name */
    if (strlen(th.cname))
    {
	fprintf(fp, "%%cname %s\n", th.cname);
    }

    /* Selection keys */
    if (strlen(th.selkey))
    {
	fprintf(fp, "%%selkey %s\n", th.selkey);
    }

    /* End keys */
    if (strlen(th.endkey))
    {
	fprintf(fp, "%%endkey %s\n", th.endkey);
    }

    /* */
    if (th.n_keyname)
    {
	int k, kn;
	fprintf(fp, "%%keyname begin\n");
	for (k=0 ; k < N_KEYCODE ; k++)
	{
	    kn = oxim_code2key(k);
	    if (th.keyname[k].uch)
	    {
	        kn = oxim_code2key(k);
		fprintf(fp, "%c %s\n", (char)kn, (char *)th.keyname[k].s);
	    }
	}
	fprintf(fp, "%%keyname end\n");
    }

    /* 寫入字根與字元對照段落*/
    fprintf(fp, "%%chardef begin\n");
    char utf8[UCH_SIZE];
    cin_char_t *cch;
    for (i=0, cch = cchar ; i < th.n_icode ; i++, cch++)
    {
	if (ichar[i] & WORD_MASK)
	{
	    char *slash = oxim_addslashes(cch->word);
	    if (slash)
	    {
		fprintf(fp, "%s\t\"%s\"\n", cch->keystroke, slash);
		free(slash);
	    }
	    else
	    {
		fprintf(fp, "%s\t\"%s\"\n", cch->keystroke, cch->word);
	    }
	}
	else
	{
	    if (oxim_ucs4_to_utf8(ichar[i], utf8))
	    {
		switch (ichar[i])
		{
		    case ' ':
			fprintf(fp, "%s\t\" \"\n", cch->keystroke);
			break;
		    case '\"':
			fprintf(fp, "%s\t%s\n", cch->keystroke, "\\\"");
			break;
		    default:
			fprintf(fp, "%s\t%s\n", cch->keystroke, utf8);
			break;
		}
	    }
	}
    }
    fprintf(fp, "%%chardef end\n");
    return;
}
