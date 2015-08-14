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

#ifndef _OXIM_CONV_H
#define _OXIM_CONV_H

#include <zlib.h>
#include <libgen.h>
#include "oximtool.h"
#include "gencin.h"

typedef struct {
    char *lc_ctype;
    char *cmd;
    char *fname;
    char *fname_cin;
    char *fname_tab;
    char *fname_outcin;
    gzFile *fr;
    gzFile *fw;
    FILE *cinfp;
    int lineno;
} cintab_t;

extern int cmd_arg(int ignoreremork, char *cmd, int cmdlen, ...);
extern int read_hexwch(unsigned char *wch_str, char *arg);

#endif
