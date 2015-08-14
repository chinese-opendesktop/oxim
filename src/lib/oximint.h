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

#ifndef _OXIMINT_H_
#define _OXIMINT_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include <oximtool.h>
#include <imodule.h>
#include <gencin.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define BUFFER_LEN 1024

/* 系統參數資訊 */
typedef struct _conf_rc
{
    int key_id;
    char *key_name;
    char *default_value;
    char *set_value;
} conf_rc;

/* 輸入法資訊 */
typedef struct _im_rc
{
    unsigned short NumberOfIM;  /* 共有幾個輸入法 */
    im_t **IMList;		/* 輸入法資訊陣列 */
} im_rc;

typedef struct _OximConfig
{
    char	*rc_dir;	/* 系統設定檔案存放目錄 */
    char	*default_dir;	/* OXIM 存放 tables, modules, panels 目錄名稱 */
    /*
	以下需要配置記憶體(記得要釋放)
    */
    char	*user_dir;	/* 使用者存放資料目錄 */
    /*------------------------------------------------------------------------*/

    conf_rc	*config;	/* 系統設定 */

    im_rc	sys_imlist;	/* */


} OximConfig;

extern OximConfig *_Config;

extern int _is_global_setting(const char *key);

#endif /* _OXIMINT_H_ */
