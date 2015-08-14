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

#ifndef	_IMODULE_H
#define	_IMODULE_H

#include "module.h"

extern imodule_t *OXIM_IMGet(int idx);
extern imodule_t *OXIM_IMGetNext(int idx, int *idx_ret);
extern imodule_t *OXIM_IMGetPrev(int idx, int *idx_ret);
extern imodule_t *OXIM_IMSearch(char *objname, int *idx_ret);

/* 以按鍵碼(0-9)取得輸入法索引值(-1:表示無按鍵指定) */
extern int oxim_get_IMIdxByKey(int key);
extern KeySym keysym_ascii(int ch);

#endif
