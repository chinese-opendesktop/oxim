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

static char *errhead;

void
oxim_set_perr(char *error_head)
{
    errhead = (char *)strdup(error_head);
}

void
oxim_perr(int msgcode, const char *fmt,...)
{
    va_list ap;
    int exitcode=0;
    FILE *fout;

    if (! errhead)
	errhead = "perr()";
    fout = (msgcode==OXIMMSG_NORMAL || msgcode==OXIMMSG_EMPTY) ? 
		stdout : stderr;
    switch (msgcode) {
    case OXIMMSG_NORMAL:
	fprintf(fout, "%s: ", errhead);
	break;
    case OXIMMSG_WARNING:
	fprintf(fout, _("%s: warning: "), errhead);
	break;
    case OXIMMSG_IWARNING:
	fprintf(fout, _("%s internal: warning: "), errhead);
	break;
    case OXIMMSG_ERROR:
	fprintf(fout, _("%s: error: "), errhead);
	exitcode = msgcode;
	break;
    case OXIMMSG_IERROR:
	fprintf(fout, _("%s internal: error: "), errhead);
	exitcode = msgcode;
	break;
    }
    va_start(ap, fmt);
    vfprintf(fout, _(fmt), ap);
    va_end(ap);
    if (exitcode)
        exit(exitcode);
}

void
oxim_perr_debug(const char *fmt,...)
{
    va_list ap;
    fprintf(stderr, "DEBUG: ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}
