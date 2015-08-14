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
oxim_get_line(char *str, int str_size, gzFile *f, int *lineno, char *ignore_ch)
/*   str=buffer,          str_size=buffer_size, 
 *  *lineno=line_#_of_f,  ignore_ch=ignore ch's 
 */
{
    char *s, *s_ignor;

    str[0] = '\0';
    while (!gzeof(f)) {
	s = gzgets(f, str, str_size);

	if (lineno)
	    (*lineno)++;

	if (ignore_ch) {
	    for (s_ignor=ignore_ch; *s_ignor != '\0'; s_ignor++)
	    {
		if ((s = strchr(str, *s_ignor)) != NULL)
		{
		    char *quote_s = NULL, *quote_e = NULL;
		    quote_s = strchr(str, '\"');
		    if (quote_s)
			quote_e = strrchr(quote_s, '\"');

		    if (quote_s && quote_e)
		    {
			if (s > quote_s && s < quote_e)
			    break;
		    }
		    *s = '\0';
		}
	    }
        }
	if (str[0] != '\0')
	    return True;
    }
    return False;
}
