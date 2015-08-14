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

#ifndef	_MODULE_H
#define	_MODULE_H

#include <X11/Xlib.h>
#include "oximtool.h"

#ifdef __cplusplus 
extern "C" { 
#endif 

#define  MODULE_VERSION         "20010918"

/*--------------------------------------------------------------------------

	Input Method Information.

--------------------------------------------------------------------------*/

#ifndef True
#  define True  1
#  define False 0
#endif

/* Max allowed # of selection keys */
#define SELECT_KEY_LENGTH       15

/* Returned value of keystroke() function. */
#define IMKEY_ABSORB    	0x00000000
#define IMKEY_COMMIT    	0x00000001
#define IMKEY_IGNORE    	0x00000002
#define IMKEY_SHIFTESC  	0x00000010
#define IMKEY_SHIFTPHR		0x00000020
#define IMKEY_CTRLPHR		0x00000040
#define IMKEY_ALTPHR		0x00000080
#define IMKEY_FALLBACKPHR	0x00000100

/* Represent Page State of multi-char selection. */
#define MCCH_ONEPG		0
#define MCCH_BEGIN		1
#define MCCH_MIDDLE		2
#define MCCH_END		3

/* Options to control the GUI mode. */
#define GUIMOD_SELKEYSPOT  	0x0001
#define GUIMOD_LISTCHAR    	0x0004
#define GUIMOD_SELKEYSPOT_EX    	0x0008

/*  Structure for IM in IC.  */
typedef struct {
    int imid;				/* ID of current IM Context */
    void *iccf;				/* Internal data of IM for each IC */

    uch_t *etymon_list;			/* 字根列表 */
    ubyte_t zh_ascii;			/* The zh_ascii mode */
    xmode_t guimode;			/* GUI mode flag */

    ubyte_t keystroke_len;		/* # chars of keystroke */
    uch_t *s_keystroke;			/* keystroke printed in area 3 */
    uch_t *suggest_skeystroke;		/* keystroke printed in area 3 */

    ubyte_t n_selkey;			/* # of selection keys */
    uch_t *s_selkey;			/* the displayed select keys */
    uint_t n_mcch;			/* # of chars with the same keystroke */
    uch_t *mcch;			/* multi-char list */
    uint_t *mcch_grouping;		/* grouping of mcch list */
    byte_t mcch_pgstate;		/* page state of multi-char */

    unsigned short n_lcch;		/* # of composed cch list. */
    uch_t *lcch;			/* composed cch list. */
    unsigned short edit_pos;		/* editing position in lcch list. */
    ubyte_t *lcch_grouping;		/* grouping of lcch list */

    uch_t cch_publish;			/* A published cch. */
    char *cch;				/* the string for commit. */
    char *mcch_hint;			/* mcch selection hint. */
} inpinfo_t;

typedef struct {
    int imid;				/* ID of current Input Context */
    xmode_t guimode;			/* GUI mode flag */
    uch_t cch_publish;			/* A published cch. */
    uch_t *s_keystroke;			/* keystroke of cch_publish returned */
} simdinfo_t;

typedef struct {
    unsigned int keytype;		/* X11 KeyPress/KeyRelease */
    unsigned int keystate;		/* X11 key state/modifiers */
    KeySym keysym;			/* X11 key code. */
    char keystr[16];			/* X11 key name */
    int keystr_len;			/* key name length */
} keyinfo_t;


/*---------------------------------------------------------------------------

	General module definition

---------------------------------------------------------------------------*/

typedef struct module_s  module_t;
struct module_s {
    mod_header_t module_header;
    char **valid_objname;

    int conf_size;
    int (*init) (void *conf, char *objname);
	/* called when IM first loaded & initialized. */
    int (*xim_init) (void *conf, inpinfo_t *inpinfo);
	/* called when trigger key occures to switch IM. */
    unsigned int (*xim_end) (void *conf, inpinfo_t *inpinfo);
	/* called just before xim_init() to leave IM, not necessary */
    unsigned int (*keystroke) (void *conf, inpinfo_t *inpinfo, keyinfo_t *keyinfo);
	/* called to input key code, and output chinese char. */
    int (*show_keystroke) (void *conf, simdinfo_t *simdinfo);
    	/* called to show the key stroke. */
    int (*terminate) (void *conf);
	/* called when oxim is going to exit. */
};

/*---------------------------------------------------------------------------

	IM Common Module.

---------------------------------------------------------------------------*/

#define  N_CCODE_RULE           5       /* # of rules of encoding */
#define  N_KEYCODE              50      /* # of valid keys 0-9, a-z, .... */
#define  N_ASCII_KEY            95      /* Num of printable ASCII char */

/* Define the qphrase classes. */
#define QPHR_TRIGGER		0
#define QPHR_SHIFT		1
#define QPHR_CTRL		2
#define QPHR_ALT		4
#define QPHR_FALLBACK		8

/* Key <-> Code convertion */
extern int oxim_keys2codes(unsigned int *klist, int klist_size, char *keystroke);
extern void oxim_codes2keys(unsigned int *klist, int n_klist, char *keystroke, int keystroke_len);

/* CharCode system */
extern unsigned int ccode_to_ucs4(char *utf8);
extern int ccode_to_char(unsigned int idx, char *mbs);

/* Wide char for ASCII */
extern char *fullchar_keystring(int ch);
extern char *fullchar_keystroke(inpinfo_t *inpinfo, KeySym keysym, int is_cinput);
extern char *fullchar_ascii(inpinfo_t *inpinfo, int mode, keyinfo_t *keyinfo, int is_cinput);
extern char *halfchar_ascii(inpinfo_t *inpinfo, int mode, keyinfo_t *keyinfo);

/*---------------------------------------------------------------------------

	General module definition (Internal).

---------------------------------------------------------------------------*/

typedef struct imodule_s  imodule_t;
struct imodule_s {
    void *modp;
    char *name;
    char *version;
    char *comments;
    char *objname;
    enum mtype module_type;

    void *conf;
    int (*init) (void *conf, char *objname);
    int (*xim_init) (void *conf, inpinfo_t *inpinfo);
    unsigned int (*xim_end) (void *conf, inpinfo_t *inpinfo);
    int (*switch_in) (void *conf, inpinfo_t *inpinfo);
    int (*switch_out) (void *conf, inpinfo_t *inpinfo);
    unsigned int (*keystroke) (void *conf, inpinfo_t *inpinfo, keyinfo_t *keyinfo);
    int (*show_keystroke) (void *conf, simdinfo_t *simdinfo);
    int (*terminate) (void *conf);
};

typedef struct
{
    int  key;	   /* Hot key */
    int  circular; /* Ctrl+Chift 輪切 */
    char *inpname; /* 輸入法名稱 */
    char *aliasname; /* 別名 */
    char *modname; /* 模組名稱 */
    char *objname;
    int  inuserdir; /* 是否存放於使用者目錄 */
    settings_t *settings; /* 使用者自訂的設定 */
    imodule_t *imodp;
} im_t;

/* 以索引取得輸入法顯示名稱與 key */
extern im_t *oxim_get_IMByIndex(int idx);

#ifdef __cplusplus 
} /* extern "C" */
#endif 

#endif
