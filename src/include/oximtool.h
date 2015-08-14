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


#ifndef _OXIMTOOL_H
#define _OXIMTOOL_H

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <config.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Global status variables. */
extern int verbose, errstatus;

#if !defined(True) || !defined(False)
#undef  True
#undef  False
#define True  1
#define False 0
#endif

/* Integer types. */
typedef unsigned int	xmode_t;
typedef signed char	x_int8;
typedef unsigned char	x_uint8;

#if (SIZEOF_SHORT == 2)
typedef short		x_int16;
typedef unsigned short	x_uint16;
#else
#if (SIZEOF_INT == 2)
typedef int		x_int16;
typedef unsigned int	x_uint16;
#endif
#endif

#if (SIZEOF_INT == 4)
typedef int		x_int32;
typedef unsigned int	x_uint32;
#else
#if (SIZEOF_LONG == 4)
typedef long		x_int32;
typedef unsigned long	x_uint32;
#else
#if (SIZEOF_SHORT == 4)
typedef short		x_int32;
typedef unsigned short	x_uint32;
#endif
#endif
#endif

/* For general message level. */
#define OXIMMSG_NORMAL		 0		/* normal		*/
#define OXIMMSG_WARNING		 1		/* warning		*/
#define OXIMMSG_IWARNING	 2		/* internal warnning	*/
#define OXIMMSG_ERROR		-1		/* error		*/
#define OXIMMSG_IERROR		-2		/* internal error	*/
#define OXIMMSG_EMPTY		 3		/* pure message printed */

/* For international message output (gettext) */
#ifdef HAVE_LIBINTL_H
#  include <libintl.h>
#  define _(STRING) gettext(STRING)
#else
#  define _(STRING) STRING
#endif
#ifndef N_
#  define N_(STRING) STRING
#endif


#define N_ASCII                 95
#define UINT32BIT               0x80000000

#define OXIM_ATOM	"OXIM_STATUS"

enum cmd_id
{
    OXIM_CMD_RELOAD = 0, /* 重新讀取 config 設定 */
    OXIM_CMD_SET_LOCATION, /* 設定游標位置 */
    OXIM_CMD_CMDLINE_KEYPRESS, /* 輸出鍵盤按鍵 */
    OXIM_CMD_CMDLINE_INSERT,
    OXIM_CMD_CMDLINE_SHOWTOMODWIN,
    OXIM_CMD_CMDLINE_HIDEMODWIN,
    OXIM_CMD_CMDLINE_SHOWSYMBOLWIN,
    OXIM_CMD_CMDLINE_SWITCHKEYBOARDWIN,
    OXIM_CMD_CMDLINE_SHOWKEYBOARDWIN,
    OXIM_CMD_CMDLINE_HIDEKEYBOARDWIN,
    OXIM_CMD_CMDLINE_SHOWTEGAKI,
    OXIM_CMD_CMDLINE_REFESHSCREEN,
    OXIM_CMD_CMDLINE_PREEDITLIST,
    OXIM_CMD_CMDLINE_IMLIST,
    OXIM_CMD_CMDLINE_CHANGEIM,
    OXIM_CMD_CMDLINE_INSERT_BEFORE_FILTER,
    OXIM_INTERNAL_SHOWKEYBOARDWIN,
    OXIM_CMD_TEST
};

/* File type for check_file_exist(); */
enum ftype {
    FTYPE_FILE,			/* Regular file */
    FTYPE_DIR,			/* Directory */
    FTYPE_NONE
};

/*
* 
*/
enum keywords
{
    DefaultFontName = 0,	/* OXIM 預設字型名稱 */
    DefaultFontSize,		/* 預設字型大小 (pixel) */
    PreeditFontSize,		/* 組字區字型大小 (pixel) */
    SelectFontSize,		/* 候選字詞字型大小 (pixel) */
    StatusFontSize,		/* 狀態區字型大小 (pixel) */
    MenuFontSize,		/* 快速選單字型大小 (pixel) */
    SymbolFontSize,		/* 符號輸入表字型大小 (pixel) */
    XcinFontSize,		/* XCIN 風格字型大小 (pixel) */
    /* 各種顏色設定 */
    WinBoxColor,
    BorderColor,
    LightColor,
    DarkColor,
    CursorColor,
    CursorFontColor,
    FontColor,
    ConvertNameColor,
    InputNameColor,
    UnderlineColor,
    KeystrokeColor,
    KeystrokeBoxColor,
    SelectFontColor,
    SelectBoxColor,
    MenuBGColor,
    MenuFontColor,
    XcinBorderColor,
    XcinBGColor,
    XcinFontColor,
    XcinStatusFGColor,
    XcinStatusBGColor,
    XcinCursorFGColor,
    XcinCursorBGColor,
    XcinUnderlineColor,
    MsgboxBGColor,
    /* 預設輸入法 */
    DefauleInputMethod,
    /* XCIN 輸入風格開啟 */
    XcinStyleEnabled,
    /* On The Spot 輸入風格開啟 */
    OnSpotEnabled,
    MaxSelectKey,
    MaxConfig
};

/* 
 * General module definition.
 */
enum mtype {
    MTYPE_IM			/* IM module */
};

typedef struct {		/* common module header */
    enum mtype module_type;
    char *name;
    char *version;
    char *comments;
} mod_header_t;


/* 
 * Replacement of the standard libc functions.
 */
#ifdef HAVE_MERGESORT
#  define stable_sort  mergesort
#else
#  define stable_sort  oxim_mergesort
   extern void oxim_mergesort(void *base, size_t nmemb, size_t size,
            int (*compar)(const void *, const void *));
#endif

#define DebugLog(defverb, action) \
	if (defverb <= verbose) oxim_perr_debug action

/* General char type: mbs encoding
 *
 * Note: In Linux, if uch_t.s = "a1 a4", then uch_t.uch = 0xa4a1, i.e.,
 *       the order reversed. This might not be the general case for all
 *       plateforms.
 */
#ifndef UCH_SIZE

#if (SIZEOF_LONG_LONG == 8)
#define uchar_t long long
#else
#define uchar_t long
#endif

#define UCH_SIZE sizeof(uchar_t)
typedef union {
    unsigned char s[UCH_SIZE];
    uchar_t uch;
} uch_t;
#endif


typedef signed char byte_t;
typedef unsigned char ubyte_t;
typedef unsigned short xtype_t;
typedef unsigned short ushort_t;
typedef long unsigned int uint_t;
/*typedef unsigned int uint_t;*/

typedef struct
{
    char *key;
    char *value;
} set_item_t;

typedef struct
{
    unsigned int nsetting;
    set_item_t **setting;
} settings_t;

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
} iminfo_t;

/* 以索引取得輸入法顯示名稱與 key */
extern iminfo_t *oxim_get_IM(int idx);

/* oxim_settings.c */
extern settings_t *oxim_settings_create(void);

extern int 
oxim_settings_add(settings_t *settings, set_item_t *set_item);

extern int 
oxim_settings_add_key_value(settings_t *settings, const char *key, const char *value);

/* 讀取版本編號 */
extern char *oxim_version(void);
/* 設定/移除旗標 */
extern void oxim_setflag(void *ref, unsigned int flag, int mode);
/* 取得指定的字串值 */
extern int oxim_setting_GetString(settings_t *settings, char *key, char **ret);
/* 取得指定的數值 */
extern int oxim_setting_GetInteger(settings_t *settings, char *key, int *ret);
/* 取得指定的真偽值(Yes/True:1, Other:0) */
extern int oxim_setting_GetBoolean(settings_t *settings, char *key, int *ret);

extern void oxim_settings_destroy(settings_t *settings);


/* oximtool functions. */
extern void oxim_init(void);
extern void oxim_Reload(void);
extern void oxim_destroy(void);

/* 傳回系統設定檔案存放目錄 */
extern char *oxim_sys_rcdir(void);

/* 傳回系統預設目錄 */
extern char *oxim_sys_default_dir(void);

/* 傳回 User 預設目錄 */
extern char *oxim_user_dir(void);

/* 傳回下載站台列表 */
extern char *oxim_mirror_url(void);

/* 傳回自定的下載站台 */
extern char *oxim_external_url(void);

/* 傳回指定的系統參數 */
extern char *oxim_get_config(int key_id);

extern int oxim_set_config(int key_id, char *value);

extern int oxim_make_config(void);

/* */
extern int oxim_check_file_exist(const char *path, const int type);
/* */
extern int oxim_check_datafile(char *fn, char *sub_path,
				char *true_fn, int true_fnsize);
extern int oxim_check_cmd_exist(char *cmd);

/* 讀取指定 tag subname=NULL  */
extern settings_t *oxim_get_settings(const char *tag, const char *subname);
/* 讀取系統預設的表格輸入法設定 */
extern settings_t *oxim_system_table_settings(void);
/* 讀取指定名稱的輸入法設定 */
extern settings_t *oxim_get_im_settings(const char *imname);
extern settings_t *oxim_get_default_settings(const char *imname, const int force);

/* 取得輸入法總數 */
extern int oxim_get_NumberOfIM(void);

/* 設定輸入法快速鍵
 *
 * Return : True - 成功
 *         False - 失敗 (有重複或參數傳遞不正確)
 */
extern int oxim_set_IMKey(int idx, int key);

/* 設定輸入法別名 */
extern int oxim_set_IMAliasName(int idx, const char *alias);

/* 設定輸入法輪換 */
extern int oxim_set_IMCircular(int idx, int OnOff);

/* 查詢輸入法是否可輪換 */
extern int oxim_IMisCircular(int idx);

/* 設定輸入法設定 */
extern int oxim_set_IMSettings(int idx, settings_t *settings);

/* Quick key phrase: %trigger, %shift, %ctrl, %alt  */
extern void oxim_qphrase_init(void);
extern char *oxim_qphrase_str(int ch);
extern char *oxim_get_qphrase_list(void);

extern int oxim_key2code(int key);
extern int oxim_code2key(int code);

extern gzFile *oxim_open_file(char *fn, char *md, int exitcode);

extern int
oxim_get_line(char *str, int str_size, gzFile *f, int *lineno, char *ignore_ch); 
extern int
oxim_get_word(char **line, char *word, int word_size, char *token);

extern set_item_t *oxim_get_key_value(char *line);
extern void oxim_key_value_destroy(set_item_t *set_item);

extern void oxim_set_perr(char *error_head);
extern void oxim_perr(int exitcode, const char *fmt, ...);
extern void oxim_perr_debug(const char *fmt, ...);
extern void *oxim_malloc(size_t n_bytes, int reset);
extern void *oxim_realloc(void *pt, size_t n_bytes);
extern int oxim_set_lc_ctype(char *loc_name, char *loc_return, int loc_size,
		char *enc_return, int enc_size, int exitcode);
extern int oxim_set_lc_messages(char *loc_name, char *loc_return, int loc_size);

extern mod_header_t *load_module(char *modname, enum mtype mod_type,
		char *version, char *sub_path);
extern void unload_module(mod_header_t *imodule);
extern void module_comment(mod_header_t *modp, char *mod_name);

extern int strcmp_wild(char *s1, char *s2);
extern int wchs_to_mbs(char *mbs, uch_t *wchs, int size);
extern int nwchs_to_mbs(char *mbs, uch_t *wchs, int n_wchs, int size);
extern int oxim_utf8_to_ucs4(const char *src_orig, unsigned int *ucs4, int len);
extern int oxim_utf8len(char *utf8str);
extern int oxim_ucs4_to_utf8(const unsigned int ucs4, char *dst);
extern char *oxim_output_filter(char *input, int is_t2s);

/* 取得預設輸入法索引編號 */
extern int oxim_get_Default_IM(void);

/* 檢查 tab 檔案是否合法
 *
 * path : tab 檔路徑
 * name : tab 檔名(含副檔名)
 * ret_cname : 傳回的輸入法名稱(NULL 表示不需要)
 * ret_ver : 傳回的輸入法 Version(NULL 表示不需要)
 *
 * return : True=合法, False=不合法
 */
extern int oxim_CheckTable(char *path, char *name, char *ret_cname, int *ret_ver);

/*
 * 替字串中的 " 前面加上 '\'
 * 如果傳回非 NULL，則呼叫的 AP 必須 free 傳回值
 */
extern char *oxim_addslashes(char *str);

#ifdef __cplusplus 
} /* extern "C" */
#endif 

#endif
