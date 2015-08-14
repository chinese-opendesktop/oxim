/*
    Copyright (C) 1999 by  XCIN TEAM

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

#ifndef _GEN_INP_H
#define _GEN_INP_H

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

#define DEFAULT_INP_MODE (INP_MODE_AUTOCOMPOSE|INP_MODE_AUTOUPCHAR|INP_MODE_SPACERESET|INP_MODE_WILDON)

#define INPINFO_MODE_MCCH	0x0001
#define INPINFO_MODE_SPACE	0x0002
#define INPINFO_MODE_INWILD     0x0004
#define INPINFO_MODE_WRONG      0x0008

typedef struct {
    char *tabfn;		/* IM tab full path */
    unsigned int mode;		/* IM mode flag */
    cintab_head_t header;	/* cin-tab file header */
    ubyte_t modesc;		/* Modifier escape mode */
    char *disable_sel_list;	/* List of keys to disable selection keys */

    icode_t *ic1;		/* icode & idx for different memory models */
    icode_t *ic2;
    ichar_t *ichar;
    /*-----------------------------------------------------------------*/
    /* 詞的相關紀錄 						       */ 
    /*-----------------------------------------------------------------*/
    gzFile	*zfp; /* 有詞的存在不能 close 檔案, 這裡是紀錄 file handle */
    unsigned int word_start_pos; /* 詞在檔案中的起始位置 */
    /*-----------------------------------------------------------------*/
} gen_inp_conf_t;

#define HINTSZ	100

typedef struct
{
    uint_t	n_idx;
    ushort_t	n_word;
} word_group_t;

typedef struct {
    char keystroke[INP_CODE_LENGTH+1];
    unsigned short mode;
    uch_t *mcch_list;
    word_group_t *mcch_list_grouping;
    int *mkey_list;
    unsigned int n_mcch_list, mcch_hidx, mcch_eidx, n_mkey_list;
} gen_inp_iccf_t;


#endif
