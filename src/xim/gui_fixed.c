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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <string.h>
#include "oximtool.h"
#include "oxim.h"

#include "images/halfchar.xpm"
#include "images/fullchar.xpm"
#include "images/out_cn.xpm"
#include "images/out_tw.xpm"

#define XIMPreeditMask	0x00ffL
#define XIMStatusMask	0xff00L
#define X_LEADING	2
#define Y_LEADING	2

#define DRAW_EMPTY	1
#define	DRAW_MCCH	2
#define	DRAW_PRE_MCCH	3
#define DRAW_LCCH	4

#define BUFSIZE 256

static int draw_multich(IC *ic, int x, int y, int preview)
{
    int i, j, n_groups, n, x2, len=0, toggle_flag;
    inpinfo_t inpinfo = ic->imc->inpinfo;
    winlist_t *win = gui->root_win;
    uch_t *selkey = inpinfo.s_selkey;
    uch_t *cch = inpinfo.mcch;


    if (!selkey || !inpinfo.n_mcch)
    {
	return x;
    }

    if (!inpinfo.mcch_grouping || inpinfo.mcch_grouping[0]==0) {
	toggle_flag = 0;
	n_groups = inpinfo.n_mcch;
    }
    else {
	toggle_flag = 1;
	n_groups = inpinfo.mcch_grouping[0];
    }

    for (i=0; i<n_groups && toggle_flag!=-1; i++, selkey++)
    {
	n = (toggle_flag > 0) ? inpinfo.mcch_grouping[i+1] : 1;

	/* 顯示選擇按鍵 */
	if (selkey->uch != (uchar_t)0)
	{
	    len = strlen((char *)selkey->s);
	    x += gui_Draw_String(win, FONT_COLOR, BORDER_COLOR, x, y, (char *)selkey->s, len) + X_LEADING;
	}

	/* 顯示候選字詞 */
	for (j=0; j<n; j++, cch++)
	{
	    len = strlen((char *)cch->s);
	    if (cch->uch == (uchar_t)0)
	    {
		toggle_flag = -1;
		break;
	    }
	    x2 = x + gui_TextEscapement(win, (char *)cch->s, len);
	    if (inpinfo.mcch_hint && inpinfo.mcch_hint[(int)(cch-inpinfo.mcch)])
	    {
		gui_Draw_Line(win, x, y+2, x2, y+2, UNDERLINE_COLOR, 1);
	    }
	    gui_Draw_String(win, KEYSTROKE_COLOR, BORDER_COLOR, x, y, (char *)cch->s, len);
	    x = x2;
	}
	x += (X_LEADING * 2);
    }

    if (! (inpinfo.guimode & GUIMOD_SELKEYSPOT))
	return x;

    char *pgstate = NULL;
    switch (inpinfo.mcch_pgstate)
    {
	case MCCH_BEGIN:
	    pgstate = "←";
	    break;
	case MCCH_MIDDLE:
	    pgstate = "← →";
	    break;
	case MCCH_END:
	    pgstate = "→";
	    break;
    }

    if (pgstate)
    {
	x += gui_Draw_String(win, FONT_COLOR, BORDER_COLOR, x, y, pgstate, strlen(pgstate)) + (X_LEADING * 2);
    }

    return x;
}

static int
draw_preedit_overspot(winlist_t *win, int x, int y)
{
    //int y = win->height - (Y_LEADING * 2) - 1;
    xccore_t *xccore = (xccore_t *)win->data;
    IC *ic = xccore->ic;
    int len;
    char buf[BUFSIZE];
    inpinfo_t inpinfo = ic->imc->inpinfo;

    if (!inpinfo.lcch || inpinfo.n_lcch == 0)
	return x;

    /* 字詞群組 */
    if (inpinfo.lcch_grouping)
    {
	int i, x2, y2, n_cch=0, n_seg, save_x = x;
	ubyte_t *glist = inpinfo.lcch_grouping+1;
	char str[128];
	for (i = 0; i < inpinfo.lcch_grouping[0]; i++)
	{
	    n_seg = glist[i];
	    len = nwchs_to_mbs(str, inpinfo.lcch+n_cch, n_seg, 128);
	    x2 = x + gui_TextEscapement(win, str, len);
	    if (n_seg > 1)
	    {
		int gx1 = x + X_LEADING;
		int gy1 = y + 3;
		int gx2 = x2 -X_LEADING;
		int gy2 = y + 5;
		gui_Draw_Line(win, gx1, gy1, gx1, gy2, UNDERLINE_COLOR, 1);
		gui_Draw_Line(win, gx1, gy2, gx2, gy2, UNDERLINE_COLOR, 0);
		gui_Draw_Line(win, gx2, gy1, gx2, gy2+1, UNDERLINE_COLOR, 1);
	    }
	    x = x2;
	    n_cch += n_seg;
	}
	x = save_x;
    }

    len = wchs_to_mbs(buf, inpinfo.lcch, BUFSIZE);
    int lcch_w = gui_TextEscapement(win, buf, len);
    int cursor_w = gui_TextEscapement(win, "  ", 2);

    gui_Draw_3D_Box(win, x-X_LEADING, 2, lcch_w + (inpinfo.edit_pos < inpinfo.n_lcch ? 0: cursor_w) + (X_LEADING*2), win->font_size + (Y_LEADING*2), KEYSTROKEBOX_COLOR, 0);

    if (inpinfo.edit_pos < inpinfo.n_lcch) {
	uch_t *tmp = inpinfo.lcch + inpinfo.edit_pos;

	if (inpinfo.edit_pos > 0) {
	    len = nwchs_to_mbs(buf, inpinfo.lcch, inpinfo.edit_pos, BUFSIZE);
	    x += gui_Draw_String(win, FONT_COLOR, NO_COLOR, x, y, buf, len);
	}
	len = strlen((char *)tmp->s);
	x += gui_Draw_String(win, CURSORFONT_COLOR, CURSOR_COLOR, x, y, (char *)tmp->s, len);
        if (inpinfo.edit_pos < inpinfo.n_lcch - 1) {
            len = wchs_to_mbs(buf, inpinfo.lcch + inpinfo.edit_pos+1, BUFSIZE);
	    x += gui_Draw_String(win, FONT_COLOR, NO_COLOR, x, y, buf, len);
        }
    }
    else {
        len = wchs_to_mbs(buf, inpinfo.lcch, BUFSIZE);
        if (len)
	    x += gui_Draw_String(win, FONT_COLOR, NO_COLOR, x, y, buf, len);
        else
            x = X_LEADING;
	x += gui_Draw_String(win, FONT_COLOR, CURSOR_COLOR, x, y, "  ", 2);
    }
    return x + (X_LEADING*2);
}

static int
draw_keystroke_overspot(winlist_t *win, int x, int y)
{
    xccore_t *xccore = (xccore_t *)win->data;
    IC *ic = xccore->ic;
    uch_t *keystroke = ic->imc->inpinfo.s_keystroke;

    if (!keystroke || keystroke[0].uch==(uchar_t)0)
	return x;

    char buf[256];
    int len = wchs_to_mbs(buf, keystroke, 255);
    buf[len] = '\0';

    int key_w = gui_TextEscapement(win, buf, len);
    gui_Draw_3D_Box(win, x-X_LEADING, 2, key_w + (X_LEADING*2), win->font_size + (Y_LEADING*2), KEYSTROKEBOX_COLOR, 0);
    x += gui_Draw_String(win, KEYSTROKE_COLOR, KEYSTROKEBOX_COLOR, x, y, buf, len);
    x += X_LEADING * 2;

    return x;
}

static int draw_inpname(winlist_t *win, int x, int y)
{
    xccore_t *xccore = (xccore_t *)win->data;
    IC *ic = xccore->ic;

    char *inpn=NULL;
    char **inpb=NULL, **inp_conv=NULL;
    char *s, buf[100];

    IM_Context_t *imc = ic->imc;
    int len, img_y;
    img_y = y - win->font_size;
    inpb = ((imc->inp_state & IM_2BYTES)) ? icon_full : icon_half;

    if (imc->inp_state & IM_OUTSIMP)
	inp_conv = icon_cn;
    else if (imc->inp_state & IM_OUTTRAD)
	inp_conv = icon_tw;

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
	    inpn = "No Name";
	}
    }
    else
        inpn = "En";

    len = strlen((char *)inpn);

    int inpn_w = gui_TextEscapement(win, inpn, len);

    x += gui_Draw_String(win, FONT_COLOR, NO_COLOR, x, y, inpn, len);

    x += (X_LEADING * 2) + 1;
    gui_Draw_Image(win, x, img_y, inpb);
    x += 16 + (X_LEADING * 2);

    if (inp_conv)
    {
	gui_Draw_Image(win, x, img_y, inp_conv);
	x += 20;
    }
    return (x+4);
}

static void root_win_draw(winlist_t *win)
{
    xccore_t *xccore = (xccore_t *)win->data;
    IC *ic = xccore->ic;
    IM_Context_t *imc = ic->imc;

    int x, x1;
    x = x1 = X_LEADING * 2;
    int y = win->font_size + 3;
    int y1 = win->height - 8;

    XClearWindow(gui->display, win->window);
    x = draw_inpname(win, x, y);
    x = draw_preedit_overspot(win, x, y);
    x = draw_keystroke_overspot(win, x, y);
    x1 = draw_multich(ic, x1, y1, True);

    int max = (x > x1) ? x : x1;
    if (max > win->width)
    {
	win->width = max;
	XResizeWindow(gui->display, win->window, win->width, win->height);
    }

    gui_Draw_3D_Box(win, X_LEADING+1, y1 - win->font_size-2, win->width-7, win->font_size+6, NO_COLOR, 0);
    gui_Draw_3D_Box(win, 0, 0, win->width, win->height, NO_COLOR, 1);
    gui_Draw_3D_Box(win, 1, 1, win->width-2, win->height-2, NO_COLOR, 1);
}

static void gui_root_draw(winlist_t *win)
{
    xccore_t *xccore = (xccore_t *)win->data;
    IC *ic = xccore->ic;
    unsigned long flag = 0;

    if ((win->winmode & WMODE_EXIT) || ic == NULL)
	return;

    IM_Context_t *imc = ic->imc;

    if ((!(imc->inp_state & IM_XIMFOCUS) && !(imc->inp_state & IM_2BFOCUS)) ||
	((ic->ic_rec.input_style & XIMPreeditMask) == XIMPreeditPosition || (ic->ic_rec.input_style & XIMPreeditMask) == XIMPreeditCallbacks))
    {
	gui_winmap_change(win, False);
	return;
    }
    gui_uncheck_tray();
    gui_winmap_change(win, True);
    root_win_draw(win);
}

static void ROOTEventProcess(winlist_t *win, XEvent *event)
{
    XDefineCursor(gui->display, win->window, gui_cursor_move);
    switch (event->type)
    {
	case Expose:
	    if (event->xexpose.count == 0)
	    {
		gui_root_draw(win);
	    }
	    break;
	case ButtonPress:
	    gui_move_window(win);
	    break;
    }
}

/* 固定輸入視窗 */
void gui_root_init(void)
{
    winlist_t *win = NULL;

    if (gui->root_win)
    {
	win = gui->root_win;
    }
    else
    {
	win = gui_new_win();
    }

    /* 邊框顏色 */
    XSetWindowBorder(gui->display, win->window, gui_Color(WINBOX_COLOR));
    /* 背景顏色 */
    XSetWindowBackground(gui->display, win->window, gui_Color(BORDER_COLOR));

    /* 讀取字型大小 */
    unsigned int font_size = 16;

    win->font_size = font_size;
    win->pos_x = gui->display_width / 2;
    win->pos_y = gui->display_height / 2;
    win->width  = X_LEADING * 2 + (font_size * 26);
    win->height = (Y_LEADING * 4) + (font_size * 2) + 16;
    win->win_draw_func  = gui_root_draw;
    win->win_event_func = ROOTEventProcess;

    XSetWindowAttributes win_attr;
    win_attr.override_redirect = True;
    XChangeWindowAttributes(gui->display, win->window,
                        CWOverrideRedirect, &win_attr);

    gui->root_win = win;
}
