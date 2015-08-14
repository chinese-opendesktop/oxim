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
#include <X11/Xlocale.h>
#include <X11/Xatom.h>
#include "oximtool.h"
#include "oxim.h"
#include "gui.h"

int verbose, errstatus;

static xccore_t oxim_core;		/* OXIM kernel data. */
static Window win;
static Display *dpy;

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
	"Usage:  oxim-agent [-h] [-r] [-i] [-I] [-m] [-s] [-k] [-e symbol|keyboard|tegaki] [-x]\n"
	"Options:  -h: print this message.\n"
	"          -r: reload oxim.\n"
	"          -i: external input string.\n"
	"          -I: external input string (pass through filter).\n"
	"          -m: model window string.\n"
	"          -s: selection candidate strings.\n"
	"          -k: simulate keyboard string.\n"
	"          -e: show symbol/keyboard/tegaki window.\n"
	"          -x: tell oxim to reload screen's width and height.\n\n"));
}

// init the oxim atom and Window 
static Atom
init_xenv(void)
{
    Atom oxim_atom;
    
    if (! (dpy = XOpenDisplay(oxim_core.display_name)))
    {
	oxim_perr(OXIMMSG_ERROR, N_("cannot open display: %s\n"),
			oxim_core.display_name);
    }
    if( None == (oxim_atom = XInternAtom(dpy, OXIM_ATOM, True)) )
    {
	return None;
    }
    if( None == (win = XGetSelectionOwner(dpy, oxim_atom)) )
    {
	return None;
    }
    return oxim_atom;
}

static void 
send_event_by_cmd(long data1, long data2)
{
    Atom oxim_atom;
    XClientMessageEvent event;
    
    if( None != (oxim_atom = init_xenv()) )
    {
	event.type = ClientMessage;
	event.window = win;
	event.message_type = oxim_atom;
	event.format = 32;
	event.data.l[0] = data1;
	event.data.l[1] = data2;
	
	XSendEvent(dpy, win, False, 0, (XEvent *)&event);
	XSync(dpy, False);
    }
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
    Atom oxim_atom;
    init_xenv();

    while ((rev = getopt(argc, argv, "hri:k:m:e:xTt:sI:")) != EOF) {
        switch (rev) {
        case 'h':
	    // display help message
            print_usage();
            exit(0);
	    break;
	    
	case 's':
	{
	  int i;
	  size_t len = sizeof(char);
	  
	  optarg = (char *)oxim_malloc(sizeof(char), True);
	  for(i=2; i<argc; i++)
	  {
	    len += strlen(argv[i]) + sizeof(char);
	    optarg = oxim_realloc(optarg, len);
	    //sprintf(optarg, "%s%s\n\0", optarg, argv[i]);
	    //optarg = strcat(optarg, argv[i]);
	    
	    //optarg[strlen(optarg)] = '\n';
	    
	    char buffer[len];
	    sprintf(buffer, "%s%s\n\0", optarg, argv[i]);
	    optarg = strcpy(optarg, buffer);
	  }
	}
	case 'i':
	case 'I':
	case 'm':
	    // if argument data is empty then hide the model window.
	    if('\0' == optarg[0])
	    {
		send_event_by_cmd(OXIM_CMD_CMDLINE_HIDEMODWIN, 0);
	    }
	    else if ( ( None != (oxim_atom = init_xenv()) ) )
	    {
		char *p = optarg;
		unsigned int ucs4;
		int len = strlen(optarg);
		int nbytes;

		XClientMessageEvent event;
		event.type = ClientMessage;
		event.window = win;
		event.message_type = oxim_atom;
		event.format = 8;

		while (len && (nbytes = oxim_utf8_to_ucs4(p, &ucs4, len)) > 0)
		{
		    strncpy(event.data.b, p, nbytes);
		    event.data.b[nbytes] = '\0';
		    event.data.b[19] = rev;
		    XSendEvent(dpy, win, False, 0, (XEvent *)&event);
		    p += nbytes;
		    len -= nbytes;
		}

		event.format = 32;
		memset(event.data.b, '\0', sizeof(event.data.b));
		if(rev=='i')
		    event.data.l[0] = OXIM_CMD_CMDLINE_INSERT;
		if(rev=='m')
		    event.data.l[0] = OXIM_CMD_CMDLINE_SHOWTOMODWIN;
		if(rev=='s')
		    event.data.l[0] = OXIM_CMD_CMDLINE_PREEDITLIST;
		if(rev=='I')
		    event.data.l[0] = OXIM_CMD_CMDLINE_INSERT_BEFORE_FILTER;
		XSendEvent(dpy, win, False, 0, (XEvent *)&event);
    		XSync(dpy, False);
	    }
	    if('s' == rev)
	      free(optarg);
	    exit(0);
	    break;
	    
	case 'r':
	    // reload oxim
	    send_event_by_cmd(OXIM_CMD_RELOAD, 0);
	    exit(0);
	    break;
#if 0
	case 'T':
	    // get xim list
	    send_event_by_cmd(OXIM_CMD_CMDLINE_IMLIST, 0);
	    exit(0);
	    break;
#endif
	case 'k':
	    // send x11 keyboard key
	    send_event_by_cmd(OXIM_CMD_CMDLINE_KEYPRESS, XKeysymToKeycode(dpy, XStringToKeysym(optarg)));
	    exit(0);
	    break;
	    
	case 'e':
	{
	    // open the assigned application from agrument
	    if(0 == strcasecmp(optarg, "symbol"))
	    {
		send_event_by_cmd(OXIM_CMD_CMDLINE_SHOWSYMBOLWIN, 0);
		exit(0);
	    }
	    if(0 == strcasecmp(optarg, "keyboard_show"))
	    {
		send_event_by_cmd(OXIM_CMD_CMDLINE_SHOWKEYBOARDWIN, 0);
		exit(0);
	    }
	    if(0 == strcasecmp(optarg, "keyboard_hide"))
	    {
		send_event_by_cmd(OXIM_CMD_CMDLINE_HIDEKEYBOARDWIN, 0);
		exit(0);
	    }
	    if(0 == strcasecmp(optarg, "tegaki"))
	    {
		send_event_by_cmd(OXIM_CMD_CMDLINE_SHOWTEGAKI, 0);
		exit(0);
	    }
	    if(0 == strcasecmp(optarg, "keyboard"))
	    {
		send_event_by_cmd(OXIM_CMD_CMDLINE_SWITCHKEYBOARDWIN, 0);
		exit(0);
	    }
	    
	}
	    break;
	    
	case 'x':
	    // tell oxim : screen had been refresh
	    send_event_by_cmd(OXIM_CMD_CMDLINE_REFESHSCREEN, 0);
	    exit(0);
	    break;

        case '?':
            oxim_perr(OXIMMSG_ERROR, N_("unknown option  -%c.\n"), optopt);
            break;
        }
    }
    XCloseDisplay(dpy);
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
int
main(int argc, char **argv)
{
    command_switch(argc, argv);
    
    /*int ret=0;
    ((char *)change_filter(1, False, &ret));
            printf("ret=%d\n", ret);*/
    /*((char *)change_filter(1, False, &ret));
    printf("ret=%d\n", ret);
    ((char *)change_filter(1, False, &ret));
    printf("ret=%d\n", ret);*/
//     puts(oxim_sys_default_dir());
    return 0;
}
