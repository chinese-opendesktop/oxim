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

#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/xpm.h>
#include <X11/extensions/shape.h>
#include "gui.h"
#include "oxim.h"
#include <X11/Xft/Xft.h>

#define BUFLEN		1024
#define WINPOS_FILE	"windows.position"

typedef struct {
    unsigned int key_id;
    unsigned int color_idx;
} color_t;

static color_t colors[] =
{
    {WinBoxColor, WINBOX_COLOR},
    {BorderColor, BORDER_COLOR},
    {LightColor, LIGHT_COLOR},
    {DarkColor, DARK_COLOR},
    {CursorColor, CURSOR_COLOR},
    {CursorFontColor, CURSORFONT_COLOR},
    {FontColor, FONT_COLOR},
    {ConvertNameColor, CONVERTNAME_COLOR},
    {InputNameColor, INPUTNAME_COLOR},
    {UnderlineColor, UNDERLINE_COLOR},
    {KeystrokeColor, KEYSTROKE_COLOR},
    {KeystrokeBoxColor, KEYSTROKEBOX_COLOR},
    {SelectFontColor, SELECTFONT_COLOR},
    {SelectBoxColor, SELECTBOX_COLOR},
    {MenuBGColor, MENU_BG_COLOR},
    {MenuFontColor, MENU_FONT_COLOR},
    {XcinBorderColor, XCIN_BORDER_COLOR},
    {XcinBGColor, XCIN_BG_COLOR},
    {XcinFontColor, XCIN_FONT_COLOR},
    {XcinStatusFGColor, XCIN_STATUS_FG_COLOR},
    {XcinStatusBGColor, XCIN_STATUS_BG_COLOR},
    {XcinCursorFGColor, XCIN_CURSOR_FG_COLOR},
    {XcinCursorBGColor, XCIN_CURSOR_BG_COLOR},
    {XcinUnderlineColor, XCIN_UNDERLINE_COLOR},
    {MsgboxBGColor, MSGBOX_BG_COLOR},
    {0, 0}
};

Cursor gui_cursor_hand;
Cursor gui_cursor_move;
Cursor gui_cursor_arrow;

void gui_root_init(void);
void gui_status_init(void);
void gui_preedit_init(void);
void gui_select_init(void);
void gui_tray_init(void);
void gui_menu_init(void);
void gui_symbol_init(void);
void gui_keyboard_init(void);
void gui_xcin_init(void);

void gui_reread_resolution(winlist_t *win);

int check_tray_manager(void);
void xim_terminate(void);

gui_t *gui;
/*----------------------------------------------------------------------------

	Basic WinList handling functions.

----------------------------------------------------------------------------*/

static winlist_t *winlist = NULL, *free_win = NULL;

static void
free_win_content(winlist_t *win)
{
    int i;

    if ((win->winmode & WMODE_EXIT))
	return;

    XUnmapWindow(gui->display, win->window);
    win->winmode &= ~WMODE_MAP;
    XftDrawDestroy(win->draw);
    XDestroyWindow(gui->display, win->window);
    win->winmode |= WMODE_EXIT;
}

winlist_t *
gui_search_win(Window window)
{
    winlist_t *w = winlist;

    while (w) {
	if (w->window == window)
	    return w;
	w = w->next;
    }
    return NULL;
}

winlist_t *
gui_new_win(void)
{
    winlist_t *win;

    if (free_win)
    {
	win = free_win;
	free_win = free_win->next;
	bzero(win, sizeof(winlist_t));
    }
    else
    {
	win = oxim_malloc(sizeof(winlist_t), True);
    }
    win->window = XCreateSimpleWindow(gui->display, gui->root,
		0, 0, 1, 1, 1, 0, 0);
		XSetWindowBackgroundPixmap(gui->display, win->window, ParentRelative);
    win->win_draw_func = NULL;
    win->win_event_func = NULL;
    win->next = winlist;
    winlist = win;

    return win;
}

winlist_t *
gui_new_win_tray(XVisualInfo *vi)
{
    winlist_t *win;

    if (free_win)
    {
	win = free_win;
	free_win = free_win->next;
	bzero(win, sizeof(winlist_t));
    }
    else
    {
	win = oxim_malloc(sizeof(winlist_t), True);
    }
    Colormap colormap = XCreateColormap(gui->display, gui->root, vi->visual, AllocNone);
    XSetWindowAttributes wsa;
    wsa.background_pixmap = 0;
    wsa.colormap = colormap;
    wsa.background_pixel = 0;
    wsa.border_pixel = 0;
//    win->window = None;
    win->window = XCreateWindow(gui->display, gui->root, -1, -1, 1, 1,
	    0, vi->depth, InputOutput, vi->visual,
	    CWBackPixmap|CWBackPixel|CWBorderPixel|CWColormap, &wsa);		
    win->win_draw_func = NULL;
    win->win_event_func = NULL;
    win->next = winlist;
    winlist = win;

    return win;
}

void
gui_freewin(Window window)
{
    winlist_t *w = winlist, *wp = NULL;

    while (w) {
	if (w->window == window) {
	    free_win_content(w);
	    if (wp == NULL)
		winlist = w->next;
	    else
		wp->next = w->next;
	    w->next = free_win;
	    free_win = w;
	    return;
	}
	wp = w;
	w  = w->next;
    }
}

void
gui_winmap_change(winlist_t *win, int state)
{
    if ((win->winmode & WMODE_EXIT))
	return;

    if (state && !(win->winmode & WMODE_MAP))
    {
	XMapRaised(gui->display, win->window);
	win->winmode |= WMODE_MAP;
    }
    else if (!state && (win->winmode & WMODE_MAP))
    {
	XUnmapWindow(gui->display, win->window);
	win->winmode &= ~WMODE_MAP;
    }
}

int
gui_check_window(Window window)
{
    Window root;
    int x, y;
    unsigned width, height, bw, depth;

    XGetGeometry(gui->display, window,
		&root, &x, &y, &width, &height, &bw, &depth);
    XSync(gui->display, False);
    if (errstatus == 0)
	return True;
    else {
	errstatus = 0;
	return False;
    }
}

int
gui_check_input_focus(xccore_t *xccore, Window win)
{
    Window w;
    int revert_to_return;

    XGetInputFocus(gui->display, &w, &revert_to_return);
    return (w == win) ? True : False;
}

Window gui_get_input_focus(void)
{
    Window w;
    int rt;
    XGetInputFocus(gui->display, &w, &rt);
    return w;
}

/* 取得 LED 狀態 */
int gui_get_led_state(KeySym keysym)
{
    KeyCode keycode = XKeysymToKeycode(gui->display, keysym);

    if (keycode == NoSymbol)
	return 0;

    int keymask;
    Window w1, w2;
    int n1,n2,n3,n4;
    unsigned int imask;
    XModifierKeymap *map = XGetModifierMapping(gui->display);
    int i;
    for (i = 0 ; i < 8 ; i++)
    {
	if (map->modifiermap[map->max_keypermod * i] == keycode)
	{
	    keymask = 1 << i;
	}
    }

    XQueryPointer(gui->display, gui->root, &w1, &w2,
			&n1, &n2, &n3, &n4, &imask);

    XFreeModifiermap(map);
    return ((keymask & imask) != 0);
}

/*----------------------------------------------------------------------------

	GUI Initialization & Main loop

----------------------------------------------------------------------------*/

static int errhandle(Display *disp, XErrorEvent *xevent)
{
    errstatus = xevent->error_code;
    return 1;
}

static void gui_save_window_pos(void)
{
    char *buf = oxim_malloc(BUFLEN, False);
    sprintf(buf, "%s/%s", oxim_user_dir(), WINPOS_FILE);

    gzFile *fp = oxim_open_file(buf, "w",  OXIMMSG_EMPTY);

    if (fp)
    {
	gzprintf(fp, "# 這是紀錄各個視窗最後座標的檔案，每次移動視窗都會更新\n");
	gzprintf(fp, "# 格式： 視窗名稱 = \"X座標,Y座標\"\n\n");
	gzprintf(fp, "root_win = \"%d,%d\"\n", gui->root_win->pos_x, gui->root_win->pos_y);
	gzprintf(fp, "xcin_win = \"%d,%d\"\n", gui->xcin_win->pos_x, gui->xcin_win->pos_y);
	gzprintf(fp, "status_win = \"%d,%d\"\n", gui->status_win->pos_x, gui->status_win->pos_y);
	gzprintf(fp, "preedit_win = \"%d,%d\"\n", gui->preedit_win->pos_x, gui->preedit_win->pos_y);
	gzprintf(fp, "select_win = \"%d,%d\"\n", gui->select_win->pos_x, gui->select_win->pos_y);
	gzprintf(fp, "symbol_win = \"%d,%d\"\n", gui->symbol_win->pos_x, gui->symbol_win->pos_y);
	gzprintf(fp, "keyboard_win = \"%d,%d\"\n", gui->keyboard_win->pos_x, gui->keyboard_win->pos_y);
	gzprintf(fp, "\n#--- End of file.\n");
	gzclose(fp);
    }
    free(buf);
}

static int gui_restore_window_pos(void)
{
    int success = False;
    char *buf = oxim_malloc(BUFLEN, False);
    sprintf(buf, "%s/%s", oxim_user_dir(), WINPOS_FILE);

    gzFile *fp = oxim_open_file(buf, "r",  OXIMMSG_EMPTY);
    if (fp)
    {
	while (oxim_get_line(buf, BUFLEN, fp, NULL, "#\n"))
	{
	    char *s = buf;
	    set_item_t *set_item = oxim_get_key_value(s);
	    if (set_item)
	    {
		int x, y;
		char *commapos = index(set_item->value, ',');
		if (commapos)
		{
		    *commapos = (char)0;
		    x = atoi(set_item->value);
		    y = atoi(commapos+1);
		    if (x >= gui->display_width || y >= gui->display_height)
		    {
			continue;
		    }
		    if (strcasecmp("xcin_win", set_item->key) == 0)
		    {
			gui->xcin_win->pos_x = x;
			gui->xcin_win->pos_y = y;
		    }
		    else if (strcasecmp("root_win", set_item->key) == 0)
		    {
			gui->root_win->pos_x = x;
			gui->root_win->pos_y = y;
		    }
		    else if (strcasecmp("status_win", set_item->key) == 0)
		    {
			gui->status_win->pos_x = x;
			gui->status_win->pos_y = y;
		    }
		    else if (strcasecmp("preedit_win", set_item->key) == 0)
		    {
			gui->preedit_win->pos_x = x;
			gui->preedit_win->pos_y = y;
		    }
		    else if (strcasecmp("select_win", set_item->key) == 0)
		    {
			gui->select_win->pos_x = x;
			gui->select_win->pos_y = y;
		    }
		    else if (strcasecmp("symbol_win", set_item->key) == 0)
		    {
			gui->symbol_win->pos_x = x;
			gui->symbol_win->pos_y = y;
		    }
/*		    else if (strcasecmp("keyboard_win", set_item->key) == 0)
		    {
			gui->keyboard_win->pos_x = x;
			gui->keyboard_win->pos_y = y;
		    }*/
		}
		oxim_key_value_destroy(set_item);
	    }
	}
	gzclose(fp);
	success = True;
    }
    free(buf);
    return success;
}

void gui_uncheck_tray()
{
    alarm(0);
}

/* 偵測 Tray Manager */
static void gui_tray_monitor(int sig)
{
    if (!check_tray_manager())
    {
	    alarm(1);
    }
}

static void MakeDefaultFont(winlist_t *win)
{
    int font_size = win->font_size;

    if (font_size <= 0)
	font_size = gui->default_fontsize;

    int i;
    double chk_size;
    /* 找找看有沒有相同的 size */
    for (i = 0 ; i < gui->num_fonts; i++)
    {
	/* 有的話，就結束 */
	if (FcPatternGetDouble(gui->xftfonts[i]->pattern, XFT_PIXEL_SIZE, 0, &chk_size) == FcResultMatch && (int)chk_size == font_size)
	    return;
    }

    gui->num_fonts ++;
    if (gui->num_fonts == 1)
    {
	gui->xftfonts = (XftFont **)oxim_malloc(sizeof(XftFont *), False);
    }
    else
    {
	gui->xftfonts = (XftFont **)oxim_realloc(gui->xftfonts, gui->num_fonts * sizeof(XftFont *));
    }
    gui->xftfonts[gui->num_fonts -1] = XftFontOpen(gui->display, gui->screen,
	XFT_FAMILY, XftTypeString, oxim_get_config(DefaultFontName),
	XFT_PIXEL_SIZE, XftTypeDouble, (double)font_size,
	NULL);
}

static void gui_init_xft(void)
{
    int i;

    /* 釋放原有的字型 */
    if (gui->xftfonts)
    {
	for (i=0 ; i < gui->num_fonts ; i++)
	{
	    XftFontClose(gui->display, gui->xftfonts[i]);
	}
	free(gui->xftfonts);
    }
    gui->num_fonts = 0;
    gui->xftfonts = NULL;

    /* 釋放缺字表 */
    if (gui->missing_chars)
    {
	FcCharSetDestroy(gui->missing_chars);
    }
    gui->missing_chars = FcCharSetCreate();

    /* */
    for (i = 0 ; i < MAX_COLORS ; i++)
    {
	XftColorFree(gui->display, gui->visual, gui->colormap, &gui->colors[i]);

	XftColorAllocName(gui->display, gui->visual, gui->colormap,
	oxim_get_config(colors[i].key_id), &gui->colors[colors[i].color_idx]);
    }

    /* 紀錄預設字型大小 */
    unsigned int default_fontsize = atoi(oxim_get_config(DefaultFontSize));

    if (default_fontsize < 12 || default_fontsize > 48)
	default_fontsize = 16;

    gui->default_fontsize = default_fontsize;

}

static void InitWindows(xccore_t *xccore)
{
    gui_status_init();
    gui_root_init();
    gui_preedit_init();
    gui_select_init();
    gui_tray_init();
    gui_menu_init();
    gui_symbol_init();
    gui_keyboard_init();
    gui_xcin_init();
    gui_msgbox_init();
    gui_selectmenu_init();

    int isok = gui_restore_window_pos();

    winlist_t *win = winlist;

    while (win)
    {
	win->data = (void *)xccore;
	if (win->draw)
	{
	    XftDrawDestroy(win->draw);
	}

	win->draw = XftDrawCreate(gui->display, win->window, gui->visual, gui->colormap);

	XSelectInput(gui->display, win->window, (ExposureMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask|EnterWindowMask|LeaveWindowMask|StructureNotifyMask|KeyPressMask));
 	MakeDefaultFont(win); /* 建立預設字型 */
	XMoveResizeWindow(gui->display, win->window,
			win->pos_x, win->pos_y, win->width, win->height);
	win = win->next;
    }
}

void
gui_init(xccore_t *xccore)
{
    gui = oxim_malloc(sizeof(gui_t), True);

    if (! (gui->display = XOpenDisplay(xccore->display_name)))
    {
	oxim_perr(OXIMMSG_ERROR, N_("cannot open display: %s\n"),
			    xccore->display_name);
    }
    Atom oxim_atom = XInternAtom(gui->display, OXIM_ATOM, True);
    if (oxim_atom != None && XGetSelectionOwner(gui->display, oxim_atom) != None)
    {
	oxim_perr(OXIMMSG_WARNING, N_("OXIM already running!\n"));
	exit(0);
    }

    (void) XSetErrorHandler(errhandle);
    gui->screen = DefaultScreen(gui->display);
    gui->visual = DefaultVisual(gui->display, gui->screen);
    gui->colormap = DefaultColormap(gui->display, gui->screen);
    gui->display_width = DisplayWidth(gui->display, gui->screen);
    gui->display_height = DisplayHeight(gui->display, gui->screen);
    gui->root = RootWindow(gui->display, gui->screen);

    gui_cursor_hand = XCreateFontCursor(gui->display, XC_hand2);
    gui_cursor_move = XCreateFontCursor(gui->display, XC_fleur);
    gui_cursor_arrow = XCreateFontCursor(gui->display, XC_left_ptr);

    gui_init_xft();
    InitWindows(xccore);

    gui->xcin_style = False;
    if (strcasecmp("yes", oxim_get_config(XcinStyleEnabled)) == 0)
    {
	gui->xcin_style = True;
    }

    gui->onspot_enabled = False;
    if (strcasecmp("yes", oxim_get_config(OnSpotEnabled)) == 0)
    {
	gui->onspot_enabled = True;
    }

    xccore->display = gui->display;
    xccore->window  = gui->status_win->window;

    /* 利用計時中斷，偵測 WM 的 Tray Manager 是否 Ready */
    if(!xccore->hide_tray)
    {
	signal(SIGALRM, gui_tray_monitor);
	gui_tray_monitor(SIGALRM);
    }

}

void
gui_loop(xccore_t *xccore)
{

    while (1)
    {

	XEvent event;
	winlist_t *win;

	if ((xccore->oxim_mode & OXIM_RUN_EXITALL)) {
	    xim_terminate();
	    exit(0);
	}

	XFlush(gui->display);

	if (XPending(gui->display))
	{
	    XNextEvent(gui->display, &event);

	    if (XFilterEvent(&event, None) != True)
	    {
		win = gui_search_win(event.xany.window);
		if (win && win->win_event_func)
		{
		    win->win_event_func(win, &(event));
		}
	    }
	    
	}
	else
	{
	    struct timeval tv;
	    fd_set readfds;
	    int fd = ConnectionNumber(gui->display);
	    FD_ZERO(&readfds);
	    FD_SET(fd, &readfds);
	    tv.tv_sec = 5;
	    tv.tv_usec = 0;
	    if (select(fd + 1, &readfds, NULL, NULL, &tv) == 0)
	    {
		continue;
	    }
	}
    }
}

/*----------------------------------------------------------------------------

	GUI update window list functions.

----------------------------------------------------------------------------*/
void gui_update_winlist(void)
{
    if (gui->xcin_style)
    {
	gui->xcin_win->win_draw_func(gui->xcin_win);
    }
    else
    {
	gui->root_win->win_draw_func(gui->root_win);
	gui->preedit_win->win_draw_func(gui->preedit_win);
	gui->status_win->win_draw_func(gui->status_win);
    }

    if (gui->keyboard_win->win_draw_func)
    {
	gui->keyboard_win->win_draw_func(gui->keyboard_win);
    }
}

/*----------------------------------------------------------------------------
    Mouse Event
----------------------------------------------------------------------------*/
void gui_get_workarea(int *ret_x, int *ret_y,
			unsigned int *ret_width, unsigned int *ret_height)
{
// 有些發行版回報不正確，在找到更好的方式之前，先暫時不用 :-(
#if 1
    Atom hints = XInternAtom (gui->display, "_NET_WORKAREA", True);
    unsigned long nitems, bytesLeft;
    Atom actualType;
    int actualFormat;
    long* workarea = 0;
    if (XGetWindowProperty (gui->display, gui->root, hints, 0, 4, False,
            XA_CARDINAL, &actualType, &actualFormat, &nitems, &bytesLeft,
            (unsigned char**) &workarea) == Success)
    {
	if (actualType == XA_CARDINAL && actualFormat == 32 && nitems == 4)
	{
	    *ret_x = workarea[0];
	    *ret_y = workarea[1];
	    *ret_width = workarea[2];
	    *ret_height = workarea[3];
	}

	if (workarea)
	{
	    XFree(workarea);
	}
    }
    else
#endif
    {
	*ret_x = 0;
	*ret_y = 0;
	*ret_width = gui->display_width;
	*ret_height = gui->display_height;
    }
}

static void gui_get_mouse_xy(int *x, int *y)
{
    Window ret_root, ret_child;
    int win_x, win_y, mx, my;
    unsigned int mask;

    XQueryPointer(gui->display, gui->root, &ret_root, &ret_child, &win_x, &win_y, &mx, &my, &mask);
    *x = mx;
    *y = my;
}

static void gui_draw_xor_box(GC gc, int x, int y, int width, int height)
{
    XSetForeground(gui->display, gc, WhitePixel(gui->display, gui->screen) ^ BlackPixel(gui->display, gui->screen));
    XDrawRectangle(gui->display, gui->root, gc, x, y, width, height);
}

void
gui_move_window(winlist_t *win)
{
    xccore_t *xccore = (xccore_t *)win->data;
    int event_x, event_y;
    int offset_x, offset_y;
    int move_x, move_y;
    GC  moveGC;
    int draw_flag = False;
    int workarea_x, workarea_y;
    int workarea_x2, workarea_y2;
    unsigned int workarea_width, workarea_height;

    gui_get_workarea(&workarea_x, &workarea_y,
			&workarea_width, &workarea_height);

    workarea_x2 = workarea_x + workarea_width;
    workarea_y2 = workarea_y + workarea_height;

    gui_get_mouse_xy(&event_x, &event_y);
    offset_x = event_x - win->pos_x;
    offset_y = event_y - win->pos_y;

    moveGC = XCreateGC(gui->display, gui->root, 0, NULL);
    XSetSubwindowMode(gui->display, moveGC, IncludeInferiors);
    XSetForeground(gui->display, moveGC, BlackPixel(gui->display, gui->screen));
    XSetFunction(gui->display, moveGC, GXxor);

    XChangeActivePointerGrab(gui->display,
                PointerMotionMask | ButtonMotionMask | ButtonReleaseMask |
                OwnerGrabButtonMask, None, CurrentTime);

    XGrabServer(gui->display);

    XEvent myevent;
    while(1)
    {
	XNextEvent(gui->display, &myevent);
	switch(myevent.type)
	{
	    case ButtonRelease:
		if(myevent.xbutton.button == Button1)
		{
		    if (draw_flag)
		    {
			gui_draw_xor_box(moveGC, move_x - offset_x, move_y - offset_y, win->width, win->height);
		    }
		    XFreeGC(gui->display, moveGC);
		    gui_get_mouse_xy(&move_x, &move_y);
		    win->pos_x = move_x - offset_x;
		    win->pos_y = move_y - offset_y;

		    /* */
		    if (win->pos_x < workarea_x)
			win->pos_x = workarea_x;
		    if (win->pos_y < workarea_y)
			win->pos_y = workarea_y;
		    if (win->pos_x + win->width > workarea_x2)
			win->pos_x = workarea_x2 - win->width - 2;
		    if (win->pos_y + win->height > workarea_y2)
			win->pos_y = workarea_y2 - win->height - 2;

		    XMoveWindow(gui->display, win->window,
				 win->pos_x, win->pos_y);
		    XUngrabServer(gui->display);
		    gui_save_window_pos(); /* 儲存視窗位置 */
		    return;
		}
		break;

	    case MotionNotify:
		if (draw_flag)
		{
		    gui_draw_xor_box(moveGC, move_x - offset_x, move_y - offset_y, win->width, win->height);
		}
		gui_get_mouse_xy(&move_x, &move_y);
		gui_draw_xor_box(moveGC, move_x - offset_x, move_y - offset_y, win->width, win->height);
		draw_flag = True;
		break;
	    default:
		break;
	}
    }
}

/*
* moving more windows' location
*/
void 
gui_move_windows(void)
{
    winlist_t *winlists[] = {gui->symbol_win, gui->keyboard_win, NULL};
    unsigned int i;
    
    gui_reread_resolution(gui->root_win);
    for(i = 0; winlists[i]!=NULL; i++)
    {
	winlist_t *win = winlists[i];
	
	if (win->pos_x >= gui->display_width)
	    win->pos_x = gui->display_width - win->width - 2;
	if(win->pos_y >= gui->display_height)
	    win->pos_y = gui->display_height - win->height - 2;
	
	XMoveWindow(gui->display, win->window,
				win->pos_x, win->pos_y);
    }

// 	    gui_save_window_pos(); /* 儲存視窗位置 */
}

/******************************************************************
*
*
******************************************************************/

/* 計算字串繪圖寬度 */
static unsigned int Utf8_Char_Escapement(XftDraw *draw, XftFont *font, char *Utf8Char, int Utf8len)
{
    XGlyphInfo GlyphInfo;
    XftTextExtentsUtf8(XftDrawDisplay(draw), font, (FcChar8 *)Utf8Char, Utf8len,  &GlyphInfo);
    return GlyphInfo.xOff;
}

/* 繪出Utf8字元 */
static unsigned int Draw_Utf8_Char(winlist_t *win, XftFont *font, int foreground_idx, int background_idx, int x, int y, char *Utf8Char, int Utf8len)
{
    unsigned int width = Utf8_Char_Escapement(win->draw, font, Utf8Char, Utf8len);
    
    if (background_idx >= 0 && background_idx <= MAX_COLORS)
    {
	XftDrawRect(win->draw, &gui->colors[background_idx], x, y - win->font_size+1, width, win->font_size);
    }

    XftDrawStringUtf8(win->draw, &gui->colors[foreground_idx], font, x, y, (FcChar8 *)Utf8Char, Utf8len);

    return width;
}

/* 繪出虛線方框 */
static unsigned int Draw_Missing_Char(winlist_t *win, int foreground_idx, int background_idx, int x, int y)
{
    int width = win->font_size;
    int x1, y1, x2, y2;

    x1 = x;
    y1 = y - win->font_size + 1;
    x2 = x1 + width - 1;
    y2 = y;

    if (background_idx >= 0 && background_idx <= MAX_COLORS)
    {
	XftDrawRect(win->draw, &gui->colors[background_idx], x1, y1, width, width);
    }
    x1++ ; y1++ ; x2 --; y2--;

    /* top */
    gui_Draw_Line(win, x1, y1, x2, y1, foreground_idx, False);
    /* right */
    gui_Draw_Line(win, x2, y1, x2, y2+1, foreground_idx, False);
    /* bottom */
    gui_Draw_Line(win, x1, y2, x2, y2, foreground_idx, False);
    /* left */
    gui_Draw_Line(win, x1, y1, x1, y2, foreground_idx, False);
    return win->font_size;
}
static XftFont *gui_find_font(winlist_t *win, FcChar32 ucs4)
{
    unsigned int i;
    int weight, slant, scalable;
    double font_size;
    FcFontSet *fontset;
    XftFont *font = NULL;

    /* 缺字列表有這個字，那就不用再找啦 */
    if (FcCharSetHasChar(gui->missing_chars, ucs4))
    {
	return NULL;
    }

    /* 找出 Cache 相符的字型 */
    for (i=0 ; i < gui->num_fonts ; i++)
    {
	XftPatternGetDouble(gui->xftfonts[i]->pattern, XFT_PIXEL_SIZE, 0, &font_size);
	if ((int)font_size == win->font_size &&
	    FcCharSetHasChar(gui->xftfonts[i]->charset, ucs4))
	{
	    return gui->xftfonts[i];
	}
    }

    /* 列出所有可能的字型 */
    FcObjectSet *os = FcObjectSetBuild(FC_FAMILY, FC_FILE, FC_INDEX, FC_CHARSET, NULL);
    /* 只要標準、非斜體、可縮放字型即可 */
    FcPattern *listpat = FcPatternBuild(NULL,
				FC_SLANT, FcTypeInteger, FC_SLANT_ROMAN,
				FC_SCALABLE, FcTypeBool, FcTrue,
				NULL);
    fontset = FcFontList(NULL, listpat, os);
    FcPatternDestroy(listpat);
    FcObjectSetDestroy(os);

    for (i=0; i< fontset->nfont; i++)
    {
	FcPattern *pat = fontset->fonts[i];
	FcCharSet *fcs = NULL;

	if (FcPatternGetCharSet(pat, FC_CHARSET, 0, &fcs) != FcResultMatch)
	    continue;

	if (!FcCharSetHasChar(fcs, ucs4))
	    continue;

	FcResult res;
	FcPattern *mpat = FcFontMatch(0, pat, &res);
	if (!mpat)
	    continue;

	XftPatternAddDouble(mpat, XFT_PIXEL_SIZE, (double)win->font_size);
	XftFont *chkfont = XftFontOpenPattern(gui->display, mpat);

        if (chkfont)
	{
	    gui->num_fonts ++;
	    gui->xftfonts = (XftFont **)oxim_realloc(gui->xftfonts, gui->num_fonts * sizeof(XftFont *));
	    if (!gui->xftfonts)
	    {
		FcPatternDestroy(mpat);
		continue;
	    }
	    gui->xftfonts[gui->num_fonts - 1] = chkfont;
	    font = chkfont;
	    break;
	}
	else
	{
	    FcPatternDestroy(mpat);
	}
    }

    FcFontSetDestroy(fontset);

    if (!font)
	FcCharSetAddChar(gui->missing_chars, ucs4);

    return font;
}

unsigned int gui_Draw_String(winlist_t *win, int foreground_idx, int background_idx, int x, int y, char *string, int len)
{
    char *p = string;
    unsigned int ucs4;
    unsigned int string_width = 0;
    int nbytes;

    while (len && (nbytes = oxim_utf8_to_ucs4(p, &ucs4, len)) > 0)
    {
        unsigned int width;

	XftFont *font = gui_find_font(win, ucs4);

	if (font)
	    width = Draw_Utf8_Char(win, font, foreground_idx, background_idx, x, y, p, nbytes);
	else
	    width = Draw_Missing_Char(win, foreground_idx, background_idx, x, y);

	x += width;
	string_width += width;
	p += nbytes;
	len -= nbytes;
    }
    return string_width;
}

unsigned int gui_TextEscapement(winlist_t *win, char *string, int len)
{
    char *p = string;
    unsigned int ucs4;
    unsigned int string_width = 0;
    int nbytes;

    while (len && (nbytes = oxim_utf8_to_ucs4(p, &ucs4, len)) > 0)
    {
	XftFont *font = gui_find_font(win, ucs4);

	unsigned int width = font ? Utf8_Char_Escapement(win->draw, font, p, nbytes) : win->font_size;

	string_width += width;
	p += nbytes;
	len -= nbytes;
    }

    return string_width;
}

void gui_Draw_3D_Box(winlist_t *win, int x, int y, int width, int height, int fill_idx, int boxstyle)
{
    Window w = win->window;
    GC lightGC = XCreateGC(gui->display, w, 0, NULL);
    GC darkGC  = XCreateGC(gui->display, w, 0, NULL);

    if (boxstyle)
    {
	XSetForeground(gui->display, lightGC, gui_Color(LIGHT_COLOR));
	XSetForeground(gui->display, darkGC, gui_Color(DARK_COLOR));
    }
    else
    {
	XSetForeground(gui->display, lightGC, gui_Color(DARK_COLOR));
	XSetForeground(gui->display, darkGC, gui_Color(LIGHT_COLOR));
    }
    XSetLineAttributes(gui->display, lightGC, 1, LineSolid, CapRound, JoinRound);
    XSetLineAttributes(gui->display, darkGC, 1, LineSolid, CapRound, JoinRound);

    int x2 = x + width - 1;
    int y2 = y + height - 1;
    XDrawLine(gui->display, w, lightGC, x, y, x2, y);
    XDrawLine(gui->display, w, lightGC, x, y, x, y2);
    XDrawLine(gui->display, w, darkGC, x, y2, x2, y2);
    XDrawLine(gui->display, w, darkGC, x2, y, x2, y2);
    if (fill_idx >= 0 && fill_idx <= MAX_COLORS)
    {
	XftDrawRect(win->draw, &gui->colors[fill_idx], x+1, y + 1, width-2, height-2);
    }
    XFreeGC(gui->display, lightGC);
    XFreeGC(gui->display, darkGC);
}

void gui_Draw_Line(winlist_t *win, int x, int y, int x1, int y1, int color_idx, int line_style)
{
    XGCValues gv;
    Window w = win->window;

    gv.line_width = 1;
    gv.dashes = 1;
    gv.line_style = (line_style) ? LineSolid : LineOnOffDash;

    GC lineGC = XCreateGC(gui->display, w, GCLineWidth|GCLineStyle|GCDashList, &gv);
    XSetForeground(gui->display, lineGC, gui_Color(color_idx));
    XDrawLine(gui->display, w, lineGC, x, y, x1, y1);
    XFreeGC(gui->display, lineGC);
}

unsigned long gui_Color(int color_index)
{
    return gui->colors[color_index].pixel;
}

void gui_Draw_Image(winlist_t *win, int x, int y, char **xpm_data)
{
    XImage *image, *mask;
    XpmAttributes attr;
    Window w = win->window;
    GC pgc = XCreateGC(gui->display, w, 0, NULL);
    unsigned int px, py;
    unsigned long mask_val, img_val;

    bzero(&attr, sizeof(attr));
    XpmCreateImageFromData(gui->display, xpm_data, &image, &mask, &attr);
    if (mask)
    {
	for (py=0 ; py < attr.height ; py++)
	{
	    for (px=0 ; px < attr.width ; px++)
	    {
		mask_val = XGetPixel(mask, px, py);
		/* 要顯示的點 */
		if (mask_val)
		{
		    img_val = XGetPixel(image, px, py);
        	    XSetForeground(gui->display, pgc, img_val);
		    XDrawPoint(gui->display, w, pgc, px+x, py+y);
		}
	    }
	}
    }
    else
    {
	XPutImage(gui->display, w, DefaultGC(gui->display, gui->screen),
                image, 0, 0, x, y, attr.width, attr.height);
    }
    XDestroyImage(image);

    if (mask)
    {
	XDestroyImage(mask);
    }
    XpmFreeAttributes(&attr);
    XFreeGC(gui->display, pgc);
}

void gui_Draw_Image_Fromfile(winlist_t *win, int x, int y, char *xpm_file)
{
    char **xpm_data;
    
    int ret = XpmReadFileToData(xpm_file, &xpm_data);
/*     oxim_perr(OXIMMSG_EMPTY, ("%d\n"), ret);*/
    if(0 == ret)
	gui_Draw_Image(win, x, y, xpm_data);
    XpmFree(xpm_data);
}

/* 送出模擬按鍵到指定的視窗 */
void gui_send_key(Window win, int state, KeySym keysym)
{
    XEvent Keyevent;

    Keyevent.type = KeyPress;
    Keyevent.xany.type = KeyPress;
    Keyevent.xany.display = gui->display;
    Keyevent.xkey.serial = 0L;
    /*Keyevent.xkey.send_event = True;*/
    Keyevent.xkey.send_event = False;
    Keyevent.xkey.window = win;
    Keyevent.xkey.subwindow = None;
    Keyevent.xkey.root = gui->root;
    Keyevent.xkey.time = CurrentTime;
    Keyevent.xkey.x = 0;
    Keyevent.xkey.y = 0;
    Keyevent.xkey.x_root = 0;
    Keyevent.xkey.y_root = 0;
    Keyevent.xkey.same_screen = True;
    Keyevent.xkey.keycode = XKeysymToKeycode(gui->display, keysym);
    Keyevent.xkey.state = state;

    /*XSendEvent(gui->display, win, False, KeyPressMask, (XEvent *)&Keyevent);*/
    XSendEvent(gui->display, win, False, 0, (XEvent *)&Keyevent);
}

void ReloadPanel(xccore_t *oxim_core)
{
    int success = 0;
    char *true_fn = oxim_malloc(BUFLEN, False);
    char *sub_path = "panels";
    
    strcpy(oxim_core->keyboard_name, "default");
    strcpy(oxim_core->input_method, "default");
    success = oxim_check_datafile("/etc/oxim/panel.conf", sub_path, true_fn, 256);
    if(success)
    {
	char *buf = oxim_malloc(BUFLEN, False);
	int line_no = 0;
	gzFile *fp = NULL;
	fp = oxim_open_file(true_fn, "r", OXIMMSG_WARNING);
	if (fp)
	{
	    /* 讀入設定檔並分析 */
	    while (oxim_get_line(buf, BUFLEN, fp, &line_no, "#\r\n"))
	    {
		set_item_t *set_item = oxim_get_key_value(buf);
		if(set_item)
		{
		    if(0==strcasecmp(set_item->key, "VIRTUAL_KEYBOARD"))
		    {
			strcpy(oxim_core->keyboard_name, set_item->value);
			if(0==strcmp(set_item->value, "keyboard"))
			{
			    oxim_core->virtual_keyboard = 1;
			    //set_virtual_keyboard(1);
			}
		    }
		    if(0==strcasecmp(set_item->key, "AUTO_DOCK"))
		    {
			if (strcasecmp("YES", set_item->value) == 0 ||
			    strcasecmp("TRUE", set_item->value) == 0)

			    oxim_core->keyboard_autodock = True;
			else
			    oxim_core->keyboard_autodock = False;
		    }
		    if(0==strcasecmp(set_item->key, "INPUT_METHOD"))
		    {
			strcpy(oxim_core->input_method, set_item->value);
		    }
		}
		oxim_key_value_destroy(set_item);
	    }
	    gzclose(fp); /* 關閉檔案 */
	}
    
	free(true_fn);
    }
}

void oxim_reload(void)
{
    winlist_t *win = gui->status_win;
    xccore_t *xccore = (xccore_t *)win->data;
    IC *ic = xccore->ic;

    /* 關掉所有視窗 */
    gui_hide_symbol();
    gui_hide_keyboard();
    gui_hide_menu();
    gui_hide_msgbox();
    gui_hide_selectmenu();
    reload_filter();

    /* 關掉輸入狀態，回復英數模式 */
    if (ic)
    {
	IM_Context_t *imc = ic->imc;
	inp_state_t inp_state = imc->inp_state;
	change_IM(ic, -1);
	if (inp_state & IM_2BYTES)
	{
	    imc->inp_state &= ~(IM_2BYTES|IM_2BFOCUS|IM_CINPUT);
	}
	gui_update_winlist();
    }

    /* 紀錄各個視窗位置 */
    gui_save_window_pos();

    /* 重新載入系統設定 */
    oxim_Reload();

    /* 重新初始化字型系統 */
    gui_init_xft();

    /* 重新初始化視窗 */
    InitWindows(xccore);

    /* 重設所有的 imc */
    imc_reset();

    /* 重設所有的 Trigger Keys */
    xim_set_trigger_keys();
    
    /* reload panel.conf */
    ReloadPanel(xccore);
}

void gui_reread_resolution(winlist_t *win)
{
//	static int res_checked = 0;
// 	if(!res_checked)
	{
	    xccore_t *xccore = win->data;
	    Display *display = XOpenDisplay(xccore->display_name);
	    int screen = DefaultScreen(display);

	    gui->display_width = DisplayWidth(display, screen);
	    gui->display_height = DisplayHeight(display, screen);
	    //	res_checked = 1;
	    XCloseDisplay(display);
	    gui_restore_window_pos();
	}
}
