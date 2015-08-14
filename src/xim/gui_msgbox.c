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
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/xpm.h>
#include <X11/Xatom.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/shape.h>
#include <string.h>
#include "oximtool.h"
#include "gui.h"
#include "oxim.h"
#include "module.h"

#define X_LEADING       5
#define Y_LEADING       5
#define BUFLEN		1024

/*-------------------------------------------------------------------*/
typedef struct {
        int type;
        int index;
} point_t;

/*-------------------------------------------------------------------*/
static int msgbox_actived = False;
char *sContent;
/*-------------------------------------------------------------------*/

int gui_msgbox_actived(void)
{
    return msgbox_actived;
}

void gui_hide_msgbox()
{
    gui_winmap_change(gui->msgbox_win, 0);
    msgbox_actived = False;
}

void gui_show_msgbox(winlist_t *parent_win, char *msg)
{
/*    sContent = oxim_realloc(sContent, sizeof(sContent)*strlen(msg));
    strcpy(sContent, msg);*/
    sContent = msg;

    winlist_t *win = gui->msgbox_win;
    //Window w = parent_win->window, junkwin;
    Window w = GetTrayWindow(), junkwin;
    xccore_t *xccore = (xccore_t *)win->data;

    int ret_x, ret_y; 
    XTranslateCoordinates(gui->display, w, gui->root, 0, 0, &ret_x, &ret_y, &junkwin);
    int width = X_LEADING * 3;
    int height = Y_LEADING * 4;

    int new_x, new_y;

    if(xccore->hide_tray)
    {
	ret_x = gui->display_width;
	ret_y = 0;
    }
    
    if ((ret_x + width + 2) > gui->display_width)
	new_x = gui->display_width - width - 2;
    else
	new_x = ret_x;

    new_y = ret_y - height;
    if (new_y < 0)
	new_y = ret_y + parent_win->height;

   
    win->pos_x = new_x;
    win->pos_y = new_y;
    win->width = width;
    win->height = height;
    gui_winmap_change(win, 1);

    msgbox_actived = True;
    
//     gui_keyboard_refresh(0);
    signal(SIGALRM, gui_hide_msgbox);
    alarm(5);
}

void gui_msgbox_set(winlist_t *parent_win, char *m)
{
    winlist_t *win = gui->msgbox_win;
    Window w = parent_win->window, junkwin;
//     int text_width = gui_TextEscapement(win, msg, strlen(msg));
    char *msg = strdup(m);
    int text_width = 0, text_height = 0;
    int i;
    char delims[] = "\n";
    char *text = NULL;
    
    XClearWindow(gui->display, win->window); /* 清除視窗 */
    
    for( i = 1, text=strtok(msg, delims); text!=NULL; i++, text=strtok(NULL, delims) )
    {
	int width = 0;
	text_height = win->font_size*i;
	
	width = gui_Draw_String(win, FONT_COLOR, NO_COLOR, X_LEADING, text_height, text, strlen(text));
	if(width>text_width)
	{
		text_width = width;
	}
    }
    
    int ret_x, ret_y; /* 父視窗左上角座標 */
    int height = Y_LEADING * 4;
    int new_x, new_y;
    int new_width, new_height;
    
    new_y = parent_win->pos_y;
    if( (new_y+text_height) > (parent_win->pos_y+parent_win->height) )
	new_y = parent_win->pos_y - text_height+parent_win->height - Y_LEADING;
    if(new_y<0)
	new_y = parent_win->pos_y;

    new_x = win->pos_x;    
    new_width = text_width+X_LEADING*2;
    new_height = text_height +3;
    if( (new_x+text_width) > gui->display_width )
	new_x = gui->display_width - new_width - X_LEADING*2;
    
    XMoveResizeWindow(gui->display, win->window, new_x, new_y, new_width, new_height);
//     XMoveResizeWindow(gui->display, win->window, 0, 500, gui->display_width, gui->display_height/2);
    
    
    free(msg);
}

static void KeyboardEventProcess(winlist_t *win, XEvent *event)
{
    xccore_t *xccore = (xccore_t *)win->data;
    point_t pt;
    switch (event->type)
    {
	case Expose:
	    if (event->xexpose.count == 0)
	    {
		gui_msgbox_set(win, sContent);
	    }
	    break;
	case ButtonPress:
	    gui_hide_msgbox();
	    break;
    }
}

void gui_msgbox_init(void)
{
    winlist_t *win = NULL;
//    sContent = oxim_malloc(sizeof(char), False);

    if (gui->msgbox_win)
    {
	win = gui->msgbox_win;
    }
    else
    {
	win = (winlist_t*)gui_new_win();
    }

    /* 邊框顏色 */
    XSetWindowBorder(gui->display, win->window, gui_Color(UNDERLINE_COLOR));
    /* 背景顏色 */
    XSetWindowBackground(gui->display, win->window, gui_Color(MSGBOX_BG_COLOR));

	
    win->font_size = 15;
    win->pos_x = gui->display_width / 2;
    win->pos_y = gui->display_height / 2;
    win->width  = 1;
    win->height = 1;

    win->win_event_func = KeyboardEventProcess;
    XSetWindowAttributes win_attr;
    win_attr.override_redirect = True;
    XChangeWindowAttributes(gui->display, win->window,
			CWOverrideRedirect, &win_attr);
			
    gui->msgbox_win = win;
}
