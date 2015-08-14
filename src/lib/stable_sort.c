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

#ifndef HAVE_MERGESORT
#include <stdlib.h>
#include <string.h>
#include "oximtool.h"

static char *buf;

static void
swap(char *elm1, char *elm2, size_t size)
{
    memcpy(buf, elm1, size);
    memcpy(elm1, elm2, size);
    memcpy(elm2, buf, size);
}

static void
merge(char *base1, size_t nmemb1, char *base2, size_t nmemb2, size_t size, 
        int (*compar)(const void *, const void *))
{
    char *b1=base1, *b2=base2, *b3=buf;
    size_t i1=0, i2=0, i3=0;

    while (i1<nmemb1 && i2<nmemb2) {
        if (compar(b1, b2) > 0) {
            memcpy(b3, b2, size);
            b2 += size;
            i2 ++;
        }
        else {
            memcpy(b3, b1, size);
            b1 += size;
            i1 ++;
        }
        b3 += size;
        i3 ++;
    }
    if (i1 < nmemb1)
        memcpy(b3, b1, size*(nmemb1-i1));
    else if (i2 < nmemb2)
        memcpy(b3, b2, size*(nmemb2-i2));
    memcpy(base1, buf, size*(nmemb1+nmemb2));
}

static void
separate(void *base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *))
{
    if (nmemb == 1)
        return;

    else if (nmemb == 2) {
        char *b = (char *)base;
        if (compar(b, b+size) > 0)
            swap(b, b+size, size);
    }
    else {
        size_t nmemb1, nmemb2;
        char  *b1, *b2;

        nmemb1 = nmemb / 2;
        nmemb2 = nmemb - nmemb1;
        b1 = (char *)base;
        b2 = b1 + nmemb1 * size;
        separate((void *)b1, nmemb1, size, compar);
        separate((void *)b2, nmemb2, size, compar);
        merge(b1, nmemb1, b2, nmemb2, size, compar);
    }
}

void
oxim_mergesort(void *base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *))
{
    buf = oxim_malloc(nmemb*size, 0);    
    separate(base, nmemb, size, compar);
    free(buf);
}
#endif
