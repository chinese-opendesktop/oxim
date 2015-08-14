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

#ifdef HPUX
#  define _INCLUDE_XOPEN_SOURCE
#endif

#include <locale.h>
#include "oximint.h"

#ifdef HAVE_LIBINTL_H
#  include <libintl.h>
#endif
#ifdef HAVE_LANGINFO_H
#  include <langinfo.h>
#endif

int
oxim_set_lc_ctype(char *loc_name, char *loc_return, int loc_size, 
	     char *enc_return, int enc_size, int exitcode)
{
    char *loc=NULL, *s;

    loc_return[0] = '\0';
    enc_return[0] = '\0';

    if (loc_name == NULL)
	loc_name = "";
    if (! (loc = setlocale(LC_CTYPE, loc_name))) {
	if (exitcode != 0) {
	    if (loc_name[0] != '\0')
		s = loc_name;
	    else {
		s = getenv("LC_ALL");
		if (! s)
		    s = getenv("LC_CTYPE");
		if (! s)
		    s = getenv("LANG");
		if (! s)
		    s = "(NULL)";
	    }
	    oxim_perr(exitcode, 
		 N_("C locale \"%s\" is not supported by your system.\n"), s);
	}
	setlocale(LC_CTYPE, "C");
	return False;
    }
    if (loc_return && loc_size > 0)
	strncpy(loc_return, loc, loc_size);

/* Determine the encoding */
    if (enc_return && enc_size > 0) {
#ifdef HAVE_LANGINFO_H
	if ((s = nl_langinfo(CODESET)))
	    strncpy(enc_return, s, enc_size);
#else
	if ((s = strrchr(loc, '.')))
	    strncpy(enc_return, s+1, enc_size);
#endif
	if (enc_return[0] != '\0') {
	    s = enc_return;
	    while (*s) {
		*s = (char)tolower(*s);
		s ++;
	    }
	}

	/* Kludge to deal with the change from BIG5HKSCS to BIG5-HKSCS */
	/* in glibc-2.2.4 */
	/* This should be fixed. -- by T.H.Hsieh */
	if (strncmp(enc_return, "big5-hkscs", 10) == 0)
	    strcpy(enc_return, "big5hkscs");
    }
    return True;
}

int
oxim_set_lc_messages(char *loc_name, char *loc_return, int loc_size)
{
    char *loc=NULL;

    if (! (loc = setlocale(LC_MESSAGES, loc_name)))
	return False;
    if (loc_return && loc_size > 0)
	strncpy(loc_return, loc, loc_size);
#ifdef HAVE_LIBINTL_H
    textdomain("oxim");
    bindtextdomain("oxim", NULL);
#endif
    return True;
}
