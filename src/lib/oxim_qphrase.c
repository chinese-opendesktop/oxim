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
#include "module.h"

static char **qphr = NULL;

void oxim_qphrase_init(void)
{
    char *s, cmd[15], var[80], buf[256], true_fn[256];
    char *sub_path = "tables";
    int lineno=0, key;
    gzFile *fp=NULL;
   
    if (oxim_check_datafile("default.phr", sub_path, true_fn, 256) == True)
	fp = oxim_open_file(true_fn, "r", OXIMMSG_WARNING);
    if (! fp)
	return;

    if (!qphr)
    {
	qphr = oxim_malloc(sizeof(char *) * N_KEYCODE, True);
    }
    else
    {
	int i;
	for (i=0 ; i < N_KEYCODE ; i++)
	{
	    if (qphr[i])
	    {
		free(qphr[i]);
		qphr[i] = NULL;
	    }
	}
    }

    while (oxim_get_line(buf, 256, fp, &lineno, "#\n\r"))
    {
	s = buf;
	oxim_get_word(&s, cmd, 15, NULL);
	if (! (key = oxim_key2code(cmd[0])))
	    continue;
	if (! oxim_get_word(&s, var, 80, NULL))
	    continue;

	qphr[key] = strdup(var);
    }
    gzclose(fp);
}

char *
oxim_qphrase_str(int ch)
{
    if (!qphr)
	return NULL;

    int key;

    if ((key = oxim_key2code(ch)) && qphr[key])
	return qphr[key];
    else
	return NULL;
}

char *
oxim_get_qphrase_list(void)
{
    static char list[N_KEYCODE + 1];
    char *s=list;
    int i;

    for (i=0; qphr!=NULL, i<N_KEYCODE; i++)
    {
	if (qphr[i])
	{
	    *s = oxim_code2key(i);
	    s ++;
	}
    }
    *s = '\0';
    return list;
}
