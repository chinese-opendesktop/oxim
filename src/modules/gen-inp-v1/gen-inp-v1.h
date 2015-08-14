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

#ifndef _GEN_INP_V1_H
#define _GEN_INP_V1_H

#include <stdlib.h>
#include <zlib.h>
#include "gencin.h"

#define INP_MODE_AUTOSELECT  0x00000001 /* Auto-select mode on. */
#define INP_MODE_AUTOCOMPOSE 0x00000002 /* Auto-compose mode on. */
#define INP_MODE_AUTOUPCHAR  0x00000004 /* Auto-up-char mode on. */
#define INP_MODE_AUTOFULLUP  0x00000008 /* Auto-full-up mode on. */
#define INP_MODE_SPACEAUTOUP 0x00000010 /* Space key can auto-up-char */
#define INP_MODE_SELKEYSHIFT 0x00000020 /* selkey shift mode on. */
#define INP_MODE_SPACEIGNOR  0x00000040 /* Ignore the space after a char. */
#define INP_MODE_WILDON      0x00000080 /* Enable the wild mode. */
#define INP_MODE_ENDKEY      0x00000100 /* Enable the end key mode. */
#define INP_MODE_SPACERESET  0x00000400 /* Enable space reset error mode. */
#define INP_MODE_AUTORESET   0x00000800 /* Enable auto reset error mode. */
#define INP_MODE_BOPOMOFOCHK 0x00001000 /* Enable BoPoMoFo rules. */

#define DEFAULT_INP_MODE (INP_MODE_AUTOCOMPOSE|INP_MODE_AUTOUPCHAR|INP_MODE_SPACERESET|INP_MODE_WILDON)

#define INPINFO_MODE_MCCH	0x0001
#define INPINFO_MODE_SPACE	0x0002
#define INPINFO_MODE_INWILD     0x0004
#define INPINFO_MODE_WRONG      0x0008

typedef struct
{
    char *keystroke;
    unsigned int start_idx;
    unsigned int end_idx;
} cache_t;

typedef struct
{
    char *keystroke;
    uch_t uch;
} kremap_t;

typedef struct
{
    unsigned int memory_usage;  /* 耗用記憶體大小 (Bytes) */

    char *tabfn;		/* IM tab full path */
    unsigned int mode;		/* IM mode flag */
    cintab_head_v1_t header;	/* cin-tab version 1 file header */
    ubyte_t modesc;		/* Modifier escape mode */
    char disable_sel_list[N_KEYS]; /* List of keys to disable selection keys */
    uch_t etymon_list[N_KEYS];
    int n_kremap;               /* Number of keystroke remapping */
    kremap_t *kremap;           /* Keystroke remapping list */

    /* 檔案模式(在開啟檔案時判別) 
	True  : 未壓縮模式。使用直接磁碟讀取有資料。
	False : gzip 壓縮模式。在使用該輸入法時將 offset_tbl 與 input_content
		讀入記憶體，然後從記憶體中讀取，未使用時，則釋放掉。 
    */
    unsigned int direct_mode;

    unsigned int offset_size;   /* offset_table 的大小 (bytes) */
    unsigned int *offset_tbl;	/* offset table */

    unsigned int input_content_size; /* 輸入資料的大小 (bytes) */
    void *input_content;	/* 所有的輸入資料 */

    /* 當輸入法檔案並不是壓縮型態時，cache 才會使用 */
    /* 每一筆 cache 佔用 字根長度 + 1 + 8 */
    unsigned int n_cache; /* 有多少筆 cache 紀錄 */
    cache_t *cache;

    gzFile	*zfp; /* 這裡是紀錄 file handle */
} gen_inp_conf_t;

#define HINTSZ	100

typedef struct
{
    uint_t	n_idx;
    ushort_t	n_word;
} word_group_t;

typedef struct
{
    char keystroke[N_NAME_LENGTH+1]; /* 使用者輸入的字根 */

    unsigned int n_record;  /* 總筆數 */
    unsigned int n_page;    /* 總頁數 */
    unsigned int start_idx; /* 起始索引編號 */
    unsigned int end_idx;   /* 結束索引編號 */
    unsigned int this_page; /* 目前的顯示頁 */

    unsigned short mode;
    unsigned int n_mkey_list;
    unsigned int *mkey_list;
} gen_inp_iccf_t;

#endif
