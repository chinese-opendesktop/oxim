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

/* 如果傳回非 NULL，則呼叫的 AP 必須 free 傳回值 */
char *oxim_addslashes(char *str)
{
    unsigned int size = strlen(str) + 1;
    char *ret_str = NULL;
    if (size == 1)
    {
	return NULL;
    }
    else
    {
	ret_str = oxim_malloc(size, False);
    }

    unsigned int i, j;
    for (i=0, j=0 ; str[i] ; i++, j++)
    {
	/* 如果是 Double quote 就需要加大配置的記憶體 */
	if (str[i] == '\"')
	{
	    ret_str = oxim_realloc(ret_str, ++size); 
	    ret_str[j++] = '\\';
	}
	ret_str[j] = str[i];
    }
    ret_str[j] = '\0';

    if (i != j)
    {
	return ret_str;
    }

    free(ret_str);
    return NULL;
}
