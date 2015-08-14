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

int
wchs_to_mbs(char *mbs, uch_t *uchs, int size)
{
    int i=0, j;
    uch_t *uch = uchs;
    char  *ch = mbs;

    if (! uch)
	return 0;

    while (uch->uch && i < size-1) {
	for (j=0; j<UCH_SIZE && uch->s[j]; j++, ch++) {
	    *ch = uch->s[j];
	    i ++;
	}
	uch ++;
    }
    *ch = '\0';

    return i;
}

int
nwchs_to_mbs(char *mbs, uch_t *uchs, int n_uchs, int size)
{
    int i=0, j, n_uch=0;
    uch_t *uch = uchs;
    char  *ch = mbs;

    if (! uch)
	return 0;

    while (uch->uch && n_uch < n_uchs && i < size-1) {
	for (j=0; j<UCH_SIZE && uch->s[j]; j++, ch++) {
	    *ch = uch->s[j];
	    i ++;
	}
	uch ++;
	n_uch ++;
    }
    *ch = '\0';

    return i;
}
