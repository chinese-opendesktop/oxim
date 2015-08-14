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

#ifdef HPUX
#  define _INCLUDE_POSIX_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <X11/Xlocale.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <signal.h>
#include "oximtool.h"
#include "oxim.h"
#include "fkey.h"

int verbose, errstatus;

static xccore_t oxim_core;		/* OXIM kernel data. */

void gui_init(xccore_t *xccore);
void gui_loop(xccore_t *xccore);
void xim_init(xccore_t *xccore);
void gui_init_xft(void);

/*----------------------------------------------------------------------------

        Initial Input.

----------------------------------------------------------------------------*/

static void
oxim_setlocale(void)
{
    char loc_return[128], enc_return[128];

    oxim_set_perr("oxim");
    oxim_set_lc_ctype("", loc_return, 128, enc_return, 128, OXIMMSG_ERROR);
    oxim_core.lc_ctype = (char *)strdup(loc_return);
    oxim_set_lc_messages("", loc_return, 128);

    if (XSupportsLocale() != True)
        oxim_perr(OXIMMSG_ERROR, 
	     N_("X locale \"%s\" is not supported by your system.\n"),
	     oxim_core.lc_ctype);
}

static void
print_usage(void)
{
    oxim_perr(OXIMMSG_EMPTY,
	N_("OXIM (Open X Input Method) version %s.\n" 
	   "(module ver: %s).\n"),
	PACKAGE_VERSION, MODULE_VERSION);

    oxim_perr(OXIMMSG_EMPTY,
     _("\n"
	"Usage:  oxim [-h] [-d DISPLAY] [-x XIM_name] [-v n]\n"
	"Options:  -h: print this message.\n"
	"          -d: set X Display.\n"
	"          -x: register XIM server name to Xlib.\n"
	"          -v: set debug level to n.\n\n"));
}

static void
command_switch(int argc, char **argv)
/* Command line arguements. */
{
    int  rev;

#ifdef HPUX
    extern char *optarg;
    extern int opterr, optopt;
#endif
/*
 *  Command line arguement & preparing for oxim_rc_t.
 */
    bzero(&oxim_core, sizeof(xccore_t));

    oxim_setlocale();
    opterr = 0;
    set_virtual_keyboard(0);
    while ((rev = getopt(argc, argv, "hkKtd:x:v:")) != EOF) {
        switch (rev) {
        case 'h':
            print_usage();
            exit(0);
#if 0
	case 'k':
	    oxim_core.virtual_keyboard = 1;
	    set_virtual_keyboard(1);
	    break;
	case 'K':
	    oxim_core.keyboard_autodock = 1;
	    break;
#endif
	case 't':
	    oxim_core.hide_tray = 1;
	    break;
	case 'd':
	    strncpy(oxim_core.display_name, optarg,
			sizeof(oxim_core.display_name));
	    break;
	case 'x':
	    strncpy(oxim_core.xim_name, optarg, 
			sizeof(oxim_core.xim_name));
	    break;
	case 'v':
	    verbose = atoi(optarg);
	    break;
        case '?':
            oxim_perr(OXIMMSG_ERROR, N_("unknown option  -%c.\n"), optopt);
            break;
        }
    }

/*
 *  OXIM perface.
 */
    oxim_perr(OXIMMSG_EMPTY,
	N_("OXIM (Open X Input Method) version %s.\n" 
           "(use \"-h\" option for help)\n"),
	PACKAGE_VERSION);
    oxim_perr(OXIMMSG_NORMAL, N_("locale \"%s\"\n"), oxim_core.lc_ctype);
}

/*----------------------------------------------------------------------------

        Main Program.

----------------------------------------------------------------------------*/

void xim_terminate(void);

void sighandler(int sig)
{
    if (sig == SIGQUIT) {
	DebugLog(1, ("catch signal: SIGQUIT\n"));
    }
    else if (sig == SIGTERM) {
	DebugLog(1, ("catch signal: SIGTERM\n"));
    }
    else {
	DebugLog(1, ("catch signal: SIGINT\n"));
    }
    if (sig == SIGQUIT)
	return;

    if (oxim_core.ic != NULL && (oxim_core.ic->ic_state & IC_FOCUS))
	oxim_core.oxim_mode |= OXIM_RUN_KILL;
    else {
	xim_terminate();
	exit(0);
    }
}

int
main(int argc, char **argv)
{
    command_switch(argc, argv);

    signal(SIGQUIT, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGINT,  sighandler);

    /* 初始化 OXIM */
    oxim_init();

    gui_init(&oxim_core);
    xim_init(&oxim_core);
    
    ReloadPanel(&oxim_core);
    gui_loop(&oxim_core);
    return 0;
}
