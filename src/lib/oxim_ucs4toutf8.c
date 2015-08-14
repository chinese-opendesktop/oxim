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

int oxim_ucs4_to_utf8(unsigned int ucs4, char *mbs)
{
    int bits;
    char *d = mbs;

    bzero(mbs, UCH_SIZE);
    if (ucs4 < 0x80)
    { *d++= ucs4; bits= -6 ; }
    else if (ucs4 < 0x800)
    { *d++= ((ucs4 >>  6) & 0x1F) | 0xC0; bits=  0 ; }
    else if (ucs4 < 0x10000)
    { *d++= ((ucs4 >> 12) & 0x0F) | 0xE0; bits=  6 ; }
    else if (ucs4 < 0x200000)
    { *d++= ((ucs4 >> 18) & 0x07) | 0xF0; bits= 12 ; }
    else if (ucs4 < 0x4000000)
    { *d++= ((ucs4 >> 24) & 0x03) | 0xF8; bits= 18 ; }
    else if (ucs4 < 0x80000000)
    { *d++= ((ucs4 >> 30) & 0x01) | 0xFC; bits= 24 ; }
    else return 0;

    for ( ; bits >= 0; bits-= 6)
    {
        *d++= ((ucs4 >> bits) & 0x3F) | 0x80;
    }

    return d - mbs;
}
