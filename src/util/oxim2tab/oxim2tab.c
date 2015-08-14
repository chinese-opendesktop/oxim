/*
    Copyright (C) 1999 by  XCIN TEAM

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <unistd.h>
#include "oximtool.h"
#include "module.h"
#include "oxim2tab.h"

cintab_t cintab;

/*----------------------------------------------------------------------------

	All the converting procedure functions should registered here.

----------------------------------------------------------------------------*/

extern void gencin(cintab_t *cintab);

/*----------------------------------------------------------------------------

	Cin Reading Functions.

----------------------------------------------------------------------------*/

int cmd_arg(int ignoreremork, char *cmd, int cmdlen, ...)
{
    char line[1024], *s=line, *arg;
    int arglen, n_read=1;
    va_list list;

    va_start(list, cmdlen);
    if (! oxim_get_line(line, 1024, cintab.fr, &(cintab.lineno), (ignoreremork ?"\n\r":"#\n\r")))
    {
	return 0;
    }

    cmd[0] = '\0';
    oxim_get_word(&s, cmd, cmdlen, NULL);

    while ((arg = va_arg(list, char *))) {
	arglen = va_arg(list, int);
	if (!oxim_get_word(&s, arg, arglen, NULL))
	{
	    break;
	}
	n_read ++;
    }
    return n_read;
}

int
read_hexwch(unsigned char *uch_str, char *arg)
{
    if (arg[0] == '0' && (arg[1] == 'x' || arg[1] == 'X')) {
	char *s = arg+2, tmp[3];
	int i;

	while (*s && isxdigit(*s))
	    s ++;
	if (*s)
	    return 0;

	tmp[2] = '\0';
	for (i=0, s=arg+2; i<UCH_SIZE; i++, s+=2) {
	    if (*s) {
	        tmp[0] = *s;
	        tmp[1] = *(s+1);
	        uch_str[i] = (unsigned char)strtoul(tmp, NULL, 16);
	    }
	    else
		uch_str[i] = (unsigned char)0;
	}
	return 1;
    }
    return 0;
}

char *
turncat_fn(char *fn, char *ext_rm, char *ext_add)
{
    char fn_tmp[256], *s=NULL;

    strncpy(fn_tmp, fn, 254-strlen(ext_add));
    if ((s = strrchr(fn_tmp, '.')) && ! strcmp(s+1, ext_rm))
    {
	*s = '\0';
        sprintf(s, ".%s", ext_add);
    }
    else if (! s || strcmp(s+1, ext_add)) {
	strcat(fn_tmp, ".");
        strcat(fn_tmp, ext_add);
    }

    return (char *)strdup(fn_tmp);
}

/*----------------------------------------------------------------------------

	Main Functions.

----------------------------------------------------------------------------*/

int
cin2tab(void)
{
    int i;
    char cmd[64], arg[64];
    table_prefix_t tp;
    size_t ret;
    int ret_status = 0;

    bzero(&tp, sizeof(table_prefix_t));
    strcpy(tp.prefix, "gencin");
    tp.version = 0;

    if (! (cintab.fr = oxim_open_file(cintab.fname_cin, "r", OXIMMSG_EMPTY)))
    {
	cintab.fr = oxim_open_file(cintab.fname, "r", OXIMMSG_ERROR);
	free(cintab.fname_cin);
	cintab.fname_cin = cintab.fname;
    }

    if (!cintab.fname_outcin)
    {
	cintab.fw = gzopen(cintab.fname_tab, "wb");
    }
    else
    {
	cintab.cinfp = fopen(cintab.fname_outcin, "w");
	if (!cintab.cinfp)
	{
	    oxim_perr(OXIMMSG_ERROR, N_("Can not write to file '%s'.\n"), cintab.fname_outcin);
	}
    }

    oxim_perr(OXIMMSG_NORMAL, 
	N_("cin file: %s, version %s.\n"), cintab.fname_cin, GENCIN_VERSION);

    if (!cintab.fname_outcin)
    {
	ret = gzwrite(cintab.fw, &tp, sizeof(table_prefix_t));
    }

    gencin(&cintab);
    gzclose(cintab.fr);

    if (!cintab.fname_outcin)
    {
	gzclose(cintab.fw);
    }
    else
    {
	fclose(cintab.cinfp);
    }

    return ret_status;
}

static void
print_usage(void)
{
    int i;

    if (strcmp(cintab.cmd, "oxim2cin") == 0)
    {
	oxim_perr(OXIMMSG_EMPTY, 
	N_("OXIM convert table tool. Version (oxim %s)\n"
	"Usage: %s <-c new cin file> <.cin file|.txt.in(SCIM table source)>\n\n"), PACKAGE_VERSION, cintab.cmd);
    }
    else
    {
	oxim_perr(OXIMMSG_EMPTY, 
	N_("OXIM convert table tool. Version (oxim %s)\n"
	"Usage: %s [-o output] <.cin file|.txt.in(SCIM table source)> [-c new cin file]\n\n"), PACKAGE_VERSION, cintab.cmd);
    }
}

static void
cin2tab_setlocale()
{
    char loc_return[128], enc_return[128];
    int ret;

    oxim_set_lc_ctype("", loc_return, 128, enc_return, 128, OXIMMSG_WARNING);
    cintab.lc_ctype = (char *)strdup(loc_return);
}

int
main(int argc, char **argv)
{
    char *s;
    int rev;
#ifdef HPUX
    extern char *optarg;
    extern int opterr, optopt, optind;
#endif

    bzero(&cintab, sizeof(cintab));
    oxim_set_perr("oxim2tab");

    cintab.cmd = strdup(basename(argv[0]));

    oxim_set_lc_messages("", NULL, 0);
    if (argc < 2) {
	print_usage();
        exit(1);
    }

    opterr = 0;
    while ((rev = getopt(argc, argv, "h:o:c:")) != EOF) {
        switch (rev) {
	case 'h':
	    print_usage();
	    exit(0);
	case 'o':
	    cintab.fname_tab = strdup(optarg);
	    break;
	case 'c':
	    cintab.fname_outcin = strdup(optarg);
	    break;
        case '?':
            oxim_perr(OXIMMSG_ERROR, N_("unknown option  -%c.\n"), optopt);
            break;
        }
    }
    cin2tab_setlocale();

    oxim_perr(OXIMMSG_EMPTY, N_("OXIM convert table tool. Version (oxim %s)\n"), PACKAGE_VERSION);

    if (! argv[optind])
	oxim_perr(OXIMMSG_ERROR, N_("no cin file specified.\n"));
    cintab.fname = (char *)strdup(argv[optind]);
    cintab.fname_cin = turncat_fn(argv[optind], "cin", "cin");
    if (! cintab.fname_tab)
	cintab.fname_tab = turncat_fn(argv[optind], "cin", "tab");

    exit(cin2tab());
}
