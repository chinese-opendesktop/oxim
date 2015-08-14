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

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/xpm.h>
#include <string.h>
#include <glob.h>
#include "oximtool.h"
#include "gui.h"
#include "oxim.h"

#include "images/halfchar.xpm"
#include "images/fullchar.xpm"
/*#include "images/out_cn.xpm"
#include "images/out_tw.xpm"*/
#include "images/sideback_l.xpm"
#include "images/sideback.xpm"
#include "images/sideback_r.xpm"

#define XIMPreeditMask	0x00ffL
// #define	XIMPreeditMask	(XIMPreeditArea | XIMPreeditCallbacks | XIMPreeditPosition | XIMPreeditNothing | XIMPreeditNone)

#define XIMStatusMask	0xff00L
#define X_LEADING       2
#define Y_LEADING       2

#define IF_CHECKPOINT(m_x, m_y, x1, y1, x2, y2)	if( (m_x) > (x1) && (m_x) < (x2) && (m_y) > (y1) && (m_y) < (y2) )

typedef struct {
    XSegment inp; /* 全半形狀態 */
    XSegment inpname; /* input method name */
    XSegment filter; /* filter */
} action_area;
static action_area actionarea;

static char **last_inpb = NULL; /* 全半形狀態 */
// static char **last_inp_conv = NULL; /* 繁簡狀態 */
static char *last_filter = NULL;
static char last_inpn[100] = {'\0'};
char *sInsert = NULL, *sMode = NULL, *sPreedit = NULL;

static void 
win_draw(gui_t *gui, winlist_t *win, int isForce)
{
    xccore_t *xccore = (xccore_t *)win->data;
    IC *ic = xccore->ic;

    char *inpn=NULL;
    char **inpb=NULL/*, **inp_conv=NULL*/;
    char *s, buf[100];    
    char *filter = NULL;

    IM_Context_t *imc = ic->imc;
    int x, y, len, img_y;
    img_y = (win->height - 16) / 2;
    inpb = ((imc->inp_state & IM_2BYTES)) ? icon_full : icon_half;
/*
    if (imc->inp_state & IM_OUTSIMP)
	inp_conv = icon_cn;
    else if (imc->inp_state & IM_OUTTRAD)
	inp_conv = icon_tw;*/
    int ret=0;
    filter = (char *)change_filter(0, False, &ic->filter_current);

	// define input method name
    if ((imc->inp_state & IM_CINPUT))
    {
	im_t *im = oxim_get_IMByIndex(imc->inp_num);
        if (im)
		{
            strcpy(buf, (im->aliasname ? im->aliasname : im->inpname));
            inpn = buf;
        }
	else
	{
	    inpn = _("No Name");
	}
    }
    else
        inpn = _("En");
        
    /* 狀態沒有異動, 就不重繪 */
/*    if (!isForce && inpb == last_inpb && inp_conv == last_inp_conv &&
	strcmp(inpn, last_inpn) == 0)*/
    if (!isForce && inpb == last_inpb && (last_filter==filter) &&
	strcmp(inpn, last_inpn) == 0)
	return;
    
    len = strlen((char *)inpn);

    gui_winmap_change(win, 1);
    XClearWindow(gui->display, win->window);
    
    gui_Draw_Image(win, 0, 0, sideback_l);
    x = 15 ;
    y = win->height - ((win->height - win->font_size) / 2) - 1;

    int inpn_w = gui_TextEscapement(win, inpn, len);

	// draw input method name(1)
    gui_Draw_String(win, FONT_COLOR, NO_COLOR, x+1, y+1, inpn, len);
    
    // define action area: input method name(1)
	actionarea.inpname.x1 = x;
	actionarea.inpname.y1 = 0;
	
	// draw input method name(2)
    x += gui_Draw_String(win, LIGHT_COLOR, NO_COLOR, x, y, inpn, len);
    
	// define action area: input method name(2)
	actionarea.inpname.x2 = x;
	actionarea.inpname.y2 = y;

    x += (X_LEADING * 2) + 1;
    
    // define action area: input status(1)
	actionarea.inp.x1 = x;
	actionarea.inp.y1 = 0;

    // draw input status
    gui_Draw_Image(win, x, img_y, inpb);
    x += 16 + (X_LEADING * 2);

    // define action area: input status(2)
	actionarea.inp.x2 = x;
	actionarea.inp.y2 = y;

	actionarea.filter.x1 = x;
	actionarea.filter.y1 = 0;
	actionarea.filter.x2 = 0;
	actionarea.filter.y2 = 0;

//     if (inp_conv)
    if((imc->inp_state & IM_FILTER) &&
    	!is_filter_default(&ic->filter_current)
	)
    {
		// draw filter icon
		char tmp[1024];
		
		strcpy(tmp, (char *)change_filter(0, False, &ic->filter_current));
		strcat(tmp, ".xpm");
		
		while(!oxim_check_file_exist(tmp, FTYPE_FILE))
		{
			strcpy(tmp, (char *)change_filter(1, False, &ic->filter_current));
			strcat(tmp, ".xpm");
		}
	// 	gui_Draw_Image_fromfile(win, x, img_y, inp_conv);
		gui_Draw_Image_Fromfile(win, x, img_y, tmp);
		x += 20;
		
		actionarea.filter.x2 = x;
		actionarea.filter.y2 = y;
    }

    win->width = x + 5;
    gui_Draw_Image(win, win->width - 5, 0, sideback_r);

    if (win->width + win->pos_x > gui->display_width)
    {
	win->pos_x = gui->display_width - win->width;
	XMoveWindow(gui->display, win->window, win->pos_x, win->pos_y);
    }
    XResizeWindow(gui->display, win->window, win->width, win->height);

    /* 紀錄最後狀態 */
    last_inpb = inpb;
//     last_inp_conv = inp_conv;
    last_filter = filter;
    strcpy(last_inpn, inpn);
}

static void
gui_status_draw(winlist_t *win)
{
    xccore_t *xccore = (xccore_t *)win->data;
    if (xccore->virtual_keyboard)
    {
	return;
    }
    if (gui_keyboard_actived())
    {
	return;
    }
    IC *ic = xccore->ic;
    inp_state_t inp_state = (ic) ? ic->imc->inp_state : 0;

    if (((win->winmode & WMODE_EXIT)) || !ic || !(ic->ic_state & IC_FOCUS)||(!(inp_state & IM_CINPUT) && !(inp_state & IM_2BYTES) && (/*inp_state & IM_FILTER && */is_filter_default(&ic->filter_current))) || gui->xcin_style || (ic->ic_rec.input_style & XIMPreeditMask) == XIMPreeditNothing || (ic->ic_rec.input_style & XIMPreeditMask) == XIMPreeditArea)
    {
	gui_winmap_change(win, 0);
	last_inpb = /*last_inp_conv = */ NULL;
	last_filter = NULL;
	last_inpn[0] = '\0';
	return;
    }
    gui_uncheck_tray();
    win_draw(gui, win, (!(win->winmode & WMODE_MAP) ? True : False));
}

void gui_show_status(void)
{
    xccore_t *xccore = (xccore_t *)gui->status_win->data;
    IC *ic = xccore->ic;
    
    if(!ic || xccore->virtual_keyboard)
	return;

    inp_state_t inp_state = (ic) ? ic->imc->inp_state : 0;
    
    if ((ic->ic_state & IC_FOCUS)&&(inp_state & IM_CINPUT))
	gui_winmap_change(gui->status_win, 1);
}

void gui_hide_status(void)
{
    gui_winmap_change(gui->status_win, 0);
}

static void alarm_keyboard()
{
    gui_send_key(gui_get_input_focus(), ControlMask|Mod1Mask, XK_period);
    alarm(0);
}

static void ClientMessageProcress(winlist_t *win, XEvent *event)
{
    int cmd = event->xclient.data.l[0];
    xccore_t *xccore = (xccore_t *)win->data;
    IC *ic = xccore->ic;
    
    if(8 == event->xclient.format)
    {
	char check_point = event->xclient.data.b[19];

	if(check_point=='i' || check_point=='I')
	{
	    if(!sInsert)
	    {
		sInsert = (char*)oxim_malloc( sizeof(char), False );
		strcpy(sInsert, event->xclient.data.b);
	    }
	    else
	    {
		sInsert = (char*)oxim_realloc( sInsert, sizeof(sInsert)*(strlen(sInsert)+1) );
		strcat(sInsert, event->xclient.data.b);
	    }
	}
	if(check_point=='m')
	{
	    if(!sMode)
	    {
		sMode = (char*)oxim_malloc( sizeof(char), False );
		strcpy(sMode, event->xclient.data.b);
	    }
	    else
	    {
		sMode = (char*)oxim_realloc( sMode, sizeof(sMode)*(strlen(sMode)+1) );
		strcat(sMode, event->xclient.data.b);
	    }
	}
	if(check_point=='s')
	{
	    if(!sPreedit)
	    {
		sPreedit = (char*)oxim_malloc( sizeof(char), False );
		strcpy(sPreedit, event->xclient.data.b);
	    }
	    else
	    {
		sPreedit = (char*)oxim_realloc( sPreedit, sizeof(sPreedit)*(strlen(sPreedit)+1) );
		strcat(sPreedit, event->xclient.data.b);
	    }
	}
	return;
    }
    switch (cmd)
    {
    case OXIM_CMD_CMDLINE_KEYPRESS: /* 設定鍵盤按鍵*/
	gui_send_key(gui_get_input_focus(), 0, XKeycodeToKeysym(gui->display, event->xclient.data.l[1], 0));
	break;
	
    case OXIM_CMD_RELOAD: /* 重新載入 config */
	{
	    oxim_reload();
	}
	break;

    case OXIM_CMD_SET_LOCATION: /* 設定游標位置 */
	{
	    Window w = event->xclient.data.l[1]; /* focus window */
	    int x = event->xclient.data.l[2]; /* X */
	    int y = event->xclient.data.l[3]; /* Y */
	    ic_set_location(w, x, y);
	}
	break;
	
    case OXIM_CMD_CMDLINE_INSERT:
	if (ic)
	{ 
	    xim_commit_string_raw(ic, sInsert);
	    sInsert = NULL;
	}
	break;

    case OXIM_CMD_CMDLINE_INSERT_BEFORE_FILTER:
	if (ic)
	{ 
	    xim_commit_string(ic, sInsert);
	    sInsert = NULL;
	}
	break;

    /* show model window */
    case OXIM_CMD_CMDLINE_SHOWTOMODWIN:
	{
	    if (gui_msgbox_actived())
	    {
		gui_hide_msgbox(win);
	    }
	    gui_show_msgbox(gui->tray_win, sMode);
	    sMode = NULL;
	}
	break;

    case OXIM_CMD_CMDLINE_PREEDITLIST:
	{
	    inp_state_t inp_state = (ic) ? ic->imc->inp_state : 0;
	    if (inp_state & IM_CINPUT)
	    {
		gui_show_selectmenu(gui->tray_win, sPreedit);
	    }
	    sPreedit = NULL;
	}
	break;

    /* hide model window */
    case OXIM_CMD_CMDLINE_HIDEMODWIN:
	{
	    if (gui_msgbox_actived())
		gui_hide_msgbox(win);
	    if (gui_selectmenu_actived())
		gui_hide_selectmenu(win);
	}
	break;
    
    /* show the symbol window */
    case OXIM_CMD_CMDLINE_SHOWSYMBOLWIN:
	if (!gui_symbol_actived())
	    gui_show_symbol();
	break;

    /* switch the keyboard window */
    case OXIM_CMD_CMDLINE_SWITCHKEYBOARDWIN:
    {
	/*signal(SIGALRM, alarm_keyboard);
	call_alarm(0, 300000);*/
//	alarm_keyboard();

	gui_switch_keyboard();

/*	if (!gui_keyboard_actived())
	    gui_show_keyboard();*/
    }
	break;
	
    /* switch the keyboard window */
    case OXIM_INTERNAL_SHOWKEYBOARDWIN:
    {
	Window w = gui_get_input_focus();
	gui_send_key(w, ControlMask|Mod1Mask, XK_Super_L);
    }
	break;
	
    /* show the keyboard window */
    case OXIM_CMD_CMDLINE_SHOWKEYBOARDWIN:
    {
	gui_show_keyboard();
    }
	break;

    /* hide the keyboard window */
    case OXIM_CMD_CMDLINE_HIDEKEYBOARDWIN:
    {
	gui_hide_keyboard();
    }
	break;
	
    /* execute tegaki-oxim */
    case OXIM_CMD_CMDLINE_SHOWTEGAKI:
    {
	RunTegaki(0);
    }
	break;
	
    /* the screen had been refreshed
     * so re-move chid-window
     */
    case OXIM_CMD_CMDLINE_REFESHSCREEN:
	{
	    gui_move_windows();
	}
	break;
    }
}

static void StatusEventProcess(winlist_t *win, XEvent *event)
{
    switch (event->type)
    {
	case Expose:
	    if (event->xexpose.count == 0)
	    {
		win_draw(gui, win, True); /* 強迫重繪 */
	    }
	    break;
	case MotionNotify: /* Mouse Move */
	    if (gui_msgbox_actived())
	    {
		gui_hide_msgbox(win);
	    }

	    if (event->xbutton.x < 12)
	    {
		XDefineCursor(gui->display, win->window, gui_cursor_move);
	    }
	    else
	    {
		XDefineCursor(gui->display, win->window, gui_cursor_arrow);
	    }
	    break;
	case ButtonPress:
	    if (event->xbutton.button == Button1 && event->xbutton.x < 12)
	    {
				gui_move_window(win);
		}
		else
		{
			unsigned int mouse_x = event->xbutton.x;
			unsigned int mouse_y = event->xbutton.y;
			IF_CHECKPOINT( mouse_x, mouse_y, 
				   actionarea.inpname.x1, 
				   actionarea.inpname.y1, 
				   actionarea.inpname.x2, 
				   actionarea.inpname.y2 )
			{
				if(event->xbutton.button == Button1)
					SwitchCtrlShift(0);
				if(event->xbutton.button == Button3)
					SwitchShiftControl(0);
			}
			IF_CHECKPOINT( mouse_x, mouse_y, 
				   actionarea.inp.x1, 
				   actionarea.inp.y1, 
				   actionarea.inp.x2, 
				   actionarea.inp.y2 )
			{
				if(event->xbutton.button == Button1)
					ChangeTo2BSBMode(0);
			}
			IF_CHECKPOINT( mouse_x, mouse_y, 
				   actionarea.filter.x1, 
				   actionarea.filter.y1, 
				   actionarea.filter.x2, 
				   actionarea.filter.y2 )
			{
				if(event->xbutton.button == Button1)
					ChangeToFilterMode(1);
				if(event->xbutton.button == Button3)
					ChangeToFilterMode(-1);
			}
		}
	    break;
	case ClientMessage:
	    ClientMessageProcress(win, event);
	    break;
    }
}

void gui_status_init(void)
{
    winlist_t *win = NULL;

    if (gui->status_win)
    {
	win = gui->status_win;
    }
    else
    {
	win = gui_new_win();
    }

    unsigned int font_size = atoi(oxim_get_config(StatusFontSize));

    if (font_size < 12 || font_size > 18)
        font_size = 16;

    win->width    = 1;
    win->height   = 24;
    win->font_size = font_size;
    win->pos_x = gui->display_width - 100;
    win->pos_y = gui->display_height - 64;

    win->win_draw_func = gui_status_draw;
    win->win_event_func = StatusEventProcess;

    Pixmap pix;
    XpmCreatePixmapFromData(gui->display, win->window, sideback, &pix, NULL, NULL);
    XSetWindowAttributes win_attr;
    win_attr.background_pixmap = pix;
    win_attr.override_redirect = True;
    XChangeWindowAttributes(gui->display, win->window,
			CWBackPixmap|CWOverrideRedirect, &win_attr);
    XFreePixmap(gui->display, pix);

    Atom atom = XInternAtom(gui->display, OXIM_ATOM, False);
    XSetSelectionOwner(gui->display, atom, win->window, CurrentTime);

    gui->status_win = win;
}
