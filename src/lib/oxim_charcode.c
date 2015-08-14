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

#include <string.h>

unsigned int
ccode_to_ucs4(char *utf8)
{
    char *p = utf8;
    unsigned int ucs4 = 0;
    unsigned int len = strlen(utf8);
    int nbytes;
    int nchars = 0;

    while (len && (nbytes = oxim_utf8_to_ucs4(p, &ucs4, len)) > 0)
    {
	nchars ++;
	p += nbytes;
	len -= nbytes;
    }
    
    return (nchars == 1) ? ucs4 : 0;
}

int
ccode_to_char(unsigned int ucs4, char *mbs)
{
    return (!oxim_ucs4_to_utf8(ucs4, mbs)) ? 0 : 1;
}
