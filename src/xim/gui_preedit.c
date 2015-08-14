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

#define X_LEADING	2
#define Y_LEADING	2

#define DRAW_EMPTY	1
#define	DRAW_MCCH	2
#define	DRAW_PRE_MCCH	3
#define DRAW_LCCH	4

#define XIMPreeditMask	0x00ffL
#define XIMStatusMask	0xff00L

#define FLAG_KEYSTROKE_ONSPOT	0x0001L
#define FLAG_KEYSTROKE_OVERSPOT	0x0002L
#define FLAG_KEYSTROKE_MASK	0x000fL

#define FLAG_LCCH_ONSPOT	0x0010L
#define FLAG_LCCH_OVERSPOT	0x0020L
#define FLAG_LCCH_MASK		0x00f0L

#define FLAG_MCCH_ONBOX		0x0100L
#define FLAG_MCCH_MASK		0x0f00L

#define BUFSIZE 256

extern XIMS ims;
#define SELKEY_IDX 10
typedef struct {
    XSegment keys[SELKEY_IDX];
    XSegment left, right;
} sel_keys;
static sel_keys selkeys;

enum point_type {
    POINT_NOTGETIN = -3,
    POINT_ON_LEFTKEY,
    POINT_ON_RIGHTKEY,
    POINT_ON_SELKEY = 0,
};

#define IF_CHECKPOINT(m_x, m_y, x1, y1, x2, y2)	if( (m_x) > (x1) && (m_x) < (x2) && (m_y) > (y1) && (m_y) < (y2) )
/*
* check mouse point at the preedit menu
* return checked state( or type)
*/
static int 
CheckPoint(XEvent *event)
{
    unsigned int mouse_x = event->xbutton.x;
    unsigned int mouse_y = event->xbutton.y;
    unsigned int i;

    /* on selection key? */
    for(i=0; i<SELKEY_IDX; i++)
    {
	IF_CHECKPOINT( mouse_x, mouse_y,
		       selkeys.keys[i].x1, selkeys.keys[i].y1,
		       selkeys.keys[i].x2, selkeys.keys[i].y2 )

	    return (POINT_ON_SELKEY + i);
    }

    /* on left or right arrow ? */
//     if( CheckPoint(event->xbutton.x, event->xbutton.y, selkeys.left.x1, selkeys.left.y1, selkeys.left.x2, selkeys.left.y2)
    IF_CHECKPOINT( mouse_x, mouse_y, 
		   selkeys.left.x1, selkeys.left.y1, 
		   selkeys.left.x2, selkeys.left.y2 )
		   
	return POINT_ON_LEFTKEY;

    IF_CHECKPOINT( mouse_x, mouse_y, 
		   selkeys.right.x1, selkeys.right.y1, 
		   selkeys.right.x2, selkeys.right.y2 )
		   
	return POINT_ON_RIGHTKEY;

    return POINT_NOTGETIN;
}

static void draw_Win_3D_Border(winlist_t *win)
{
    gui_Draw_3D_Box(win, 0, 0, win->width, win->height, NO_COLOR, 1);
}

static void
draw_multich_onbox(IC *ic, int preview)
{
    int i, j, n_groups, n, x2, len=0, toggle_flag;
    uch_t *selkey, *cch;
    inpinfo_t inpinfo = ic->imc->inpinfo;
    winlist_t *select_win = gui->select_win;

    cch = inpinfo.mcch;
    selkey = inpinfo.s_selkey;
    if (!selkey || !inpinfo.n_mcch)
	return;

    if (!inpinfo.mcch_grouping || inpinfo.mcch_grouping[0]==0) {
	toggle_flag = 0;
	n_groups = inpinfo.n_mcch;
    }
    else {
	toggle_flag = 1;
	n_groups = inpinfo.mcch_grouping[0];
    }

    /* 計算總共有幾個字詞可選
	以及最寬的英文以及中文
    */
    int save_flag = toggle_flag;
    int n_items = 0;  /* 總共幾個項目可選 */
    int key_max_w = 0; /* 選擇鍵最大寬度 */
    int str_max_w = 0; /* 字詞最大寬度 */
    uch_t *w_selkey, *w_cch;
    w_cch = inpinfo.mcch;
    w_selkey = inpinfo.s_selkey;
    for (i=0; i<n_groups && toggle_flag!=-1; i++, w_selkey++)
    {
	n_items ++;
	/* 每個項目的字數 */
	n = (toggle_flag > 0) ? inpinfo.mcch_grouping[i+1] : 1;
	// 算選擇鍵最大寬度
	if (w_selkey->uch != (uchar_t)0)
	{
	    len = strlen((char *)w_selkey->s);
	    int key_w = gui_TextEscapement(select_win, (char *)w_selkey->s, len);
	    if (key_w > key_max_w)
		key_max_w = key_w;
	}
	// 算出字詞最大寬度
	int work_w = 0;
        for (j=0; j<n; j++, w_cch++) {
	    len = strlen((char *)w_cch->s);
	    if (w_cch->uch == (uchar_t)0) {
		toggle_flag = -1;
		break;
	    }
	    work_w += gui_TextEscapement(select_win, (char *)w_cch->s, len);
        }
	if (work_w > str_max_w)
	    str_max_w = work_w;
    }
    toggle_flag = save_flag;

    int item_width = X_LEADING + 5 + key_max_w + X_LEADING + str_max_w + 5 +X_LEADING;

    char *word_str = _("Selected words");	//候選詞
    char *char_str = _("Selected word");	//候選字
    char *select_str = toggle_flag ? word_str : char_str;
    len = strlen(select_str);
    int select_w = X_LEADING + gui_TextEscapement(select_win, select_str, len) + X_LEADING;
    if (!preview && select_w > item_width)
	item_width = select_w;

    int fontheight = select_win->font_size + 2;
    int selectbox_height = (n_items * fontheight) + (Y_LEADING * 6);
    int selectbox_width = item_width - (X_LEADING * 2);

    int width = item_width;
    int height = selectbox_height + (Y_LEADING * 2) + (!preview ? fontheight : 0) + ((inpinfo.mcch_pgstate && !preview) ? fontheight : 0);
    if (select_win->width != width || select_win->height != height)
    {
	select_win->width = width;
	select_win->height = height;
	XResizeWindow(gui->display, select_win->window, width, height);
    }

    /* 顯示候選字詞 */
    int x = X_LEADING;
    int y = select_win->font_size + 2;
    if (!preview)
    {
	gui_Draw_String(select_win, LIGHT_COLOR, BORDER_COLOR, x+1, y+1, select_str, len);
	gui_Draw_String(select_win, INPUTNAME_COLOR, NO_COLOR, x, y, select_str, len);
	y += Y_LEADING * 2;
    }
    else
    {
	y = Y_LEADING;
    }

    gui_Draw_3D_Box(select_win, X_LEADING, y, selectbox_width, selectbox_height, NO_COLOR, 0);
    gui_Draw_3D_Box(select_win, X_LEADING+2, y+2, selectbox_width-4, selectbox_height-4, SELECTBOX_COLOR, 1);
    
    y += (Y_LEADING * 2);
    int key_x = (toggle_flag) ? (X_LEADING*4) : (item_width - (key_max_w + X_LEADING + str_max_w))/2;

    for(i=0; i<SELKEY_IDX; i++)
    {
	// empty
	selkeys.keys[i].x1 = selkeys.keys[i].y1 = selkeys.keys[i].x2 = selkeys.keys[i].y2 = 0;
    }

    int selkeyspot_ex = inpinfo.guimode & GUIMOD_SELKEYSPOT_EX;
    for (i=0; i<n_groups && toggle_flag!=-1; i++, selkey++) {
	n = (toggle_flag > 0) ? inpinfo.mcch_grouping[i+1] : 1;
        y += fontheight;
	x = key_x;
	if (selkey->uch != (uchar_t)0) {
	    len = strlen((char *)selkey->s);
	    gui_Draw_String(select_win, (preview || selkeyspot_ex) ? KeystrokeBoxColor : SELECTFONT_COLOR, NO_COLOR, x, y, (char *)selkey->s, len);
	    selkeys.keys[i].x1 = x;
	    selkeys.keys[i].y1 = y - fontheight;
        }
	x = key_x + key_max_w + X_LEADING;
        for (j=0; j<n; j++, cch++) {
	    len = strlen((char *)cch->s);
	    if (cch->uch == (uchar_t)0) {
		toggle_flag = -1;
		break;
	    }
	    x2 = x + gui_Draw_String(select_win, SELECTFONT_COLOR, NO_COLOR, x, y, (char *)cch->s, len);
	    x = x2;
        }
	selkeys.keys[i].x2 = x2;
	selkeys.keys[i].y2 = y;
    }
    if (preview)
	return;

    #define LEFT_AR 0x10
    #define RIGHT_AR 0x01
    int pgstate = 0;
    switch (inpinfo.mcch_pgstate)
    {
    case MCCH_BEGIN:
	pgstate = RIGHT_AR;
	break;
    case MCCH_MIDDLE:
	pgstate = LEFT_AR | RIGHT_AR;
	break;
    case MCCH_END:
	pgstate = LEFT_AR;
	break;
    }

    if (pgstate)
    {
	char *left_ar  = "⇦";
	int left_ar_w  = gui_TextEscapement(select_win, left_ar, strlen(left_ar));
	char *right_ar = "⇨";
	int right_ar_w = gui_TextEscapement(select_win, right_ar, strlen(right_ar));
	y += fontheight + (Y_LEADING * 2);
	gui_Draw_String(select_win, LIGHT_COLOR, NO_COLOR, X_LEADING+1, y+1, left_ar, strlen(left_ar));
	gui_Draw_String(select_win, LIGHT_COLOR, NO_COLOR, width - right_ar_w - X_LEADING + 1, y+1, right_ar, strlen(right_ar));
	int left_color = (pgstate & LEFT_AR) ? INPUTNAME_COLOR : DARK_COLOR;
	int left_width = gui_Draw_String(select_win, left_color, NO_COLOR, X_LEADING, y, left_ar, strlen(left_ar));
	int right_color = (pgstate & RIGHT_AR) ? INPUTNAME_COLOR : DARK_COLOR;
	int right_width = gui_Draw_String(select_win, right_color, NO_COLOR, width - right_ar_w - X_LEADING, y, right_ar, strlen(right_ar));
	
	selkeys.left.x1 = X_LEADING;
	selkeys.left.y1 = y - fontheight + (Y_LEADING * 2);
	selkeys.left.x2 = X_LEADING + left_width;
	selkeys.left.y2 = y;
	
	selkeys.right.x1 = width - right_ar_w - X_LEADING;
	selkeys.right.y1 = y - fontheight + (Y_LEADING * 2);
	selkeys.right.x2 = width - right_ar_w - X_LEADING + right_width;
	selkeys.right.y2 = y;
    }
}

void draw_preedit_onspot(IC *ic, int isFake, int isvkb)
{
    if(isvkb)
	return;
    char buf[BUFSIZE] = {'\0'}; /* 預編輯文字緩衝區 */
    char keystroke_buf[BUFSIZE] = {'\0'}; /* 字根緩衝區 */
    int  keystroke_len = 0;
    XIMFeedback feedback[BUFSIZE];
    int show_key = ic->has_set_spot ? False : True;

    inpinfo_t inpinfo = ic->imc->inpinfo;

    if (!isFake)
    {
	wchs_to_mbs(buf, inpinfo.lcch, BUFSIZE);
    }

    int lcch_len = oxim_utf8len(buf);

    if (show_key && inpinfo.keystroke_len)
    {
	wchs_to_mbs(keystroke_buf, inpinfo.s_keystroke, BUFSIZE);
	keystroke_len = oxim_utf8len(keystroke_buf);
	strcat(buf, keystroke_buf);
    }

    /* 預編輯區字數(預編文字 + 字根) */
    int spot_length = lcch_len + keystroke_len;

    int i;
    /* 預編輯文字加底線 */
    for (i=0 ; i < lcch_len ; i++)
    {
	feedback[i] = XIMUnderline;
    }

    /* 字根反白 */
    for (i=lcch_len ; show_key && i < spot_length ; i++)
    {
	feedback[i] = XIMReverse;
    }

    /* 預編文字游標位置反白 */
    if (inpinfo.n_lcch && inpinfo.edit_pos < inpinfo.n_lcch)
    {
	feedback[inpinfo.edit_pos] |= XIMReverse;
    }

    XIMText text;
    IMPreeditCBStruct data;
    char *tmp;

    data.major_code = XIM_PREEDIT_DRAW;
    data.connect_id = ic->connect_id;
    data.icid = ic->id;
    data.todo.draw.chg_first = 0;
    data.todo.draw.chg_length = ic->preedit_length;
    data.todo.draw.text = &text;
    text.encoding_is_wchar = False;
    text.feedback = feedback;

    if (!spot_length)
    {
	xim_preeditcallback_done(ic);
    }
    else
    {
	tmp = (char *)xim_fix_big5_bug(gui->display, buf);
	text.string.multi_byte = tmp;
	text.length = strlen(tmp);
        data.todo.draw.caret = (inpinfo.edit_pos < inpinfo.n_lcch) ? inpinfo.edit_pos : inpinfo.n_lcch;
	IMCallCallback(ims, (XPointer)&data);
	XFree(tmp);
	ic->preedit_length = spot_length;
    }
}

static int
draw_preedit_overspot(winlist_t *win, int x)
{
    int y = win->height - (Y_LEADING * 2) - 1;
    xccore_t *xccore = (xccore_t *)win->data;
    IC *ic = xccore->ic;
    int len;
    char buf[BUFSIZE];
    inpinfo_t inpinfo = ic->imc->inpinfo;

    if (inpinfo.n_lcch == 0)
	return x;

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
draw_keystroke_overspot(winlist_t *win, int x, int isvkb)
{
    if(isvkb)
	return x;
    int y = win->height - (Y_LEADING * 2) - 1;
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

static void
close_All_Win(void)
{
    gui_winmap_change(gui->preedit_win, False);
    gui_winmap_change(gui->select_win, False);
}

/*static*/ void
gui_preedit_draw(winlist_t *win)
{
    xccore_t *xccore = (xccore_t *)win->data;
    IC *ic = xccore->ic;
    unsigned long flag = 0;
    int iqqi = IsIqqi() && gui_keyboard_actived();
    //printf("iqqi=%d\n", iqqi);

    if ((win->winmode & WMODE_EXIT) || ic == NULL)
	return;

    IM_Context_t *imc = ic->imc;
    if (!(imc->inp_state & IM_XIMFOCUS) || (ic->ic_rec.input_style & XIMPreeditMask) == XIMPreeditNothing || (ic->ic_rec.input_style & XIMPreeditMask) == XIMPreeditArea)
    {
	close_All_Win();
	return;
    }

    winlist_t *preedit_win = gui->preedit_win;
    winlist_t *select_win = gui->select_win;
    inpinfo_t inpinfo = imc->inpinfo;

    Window w, junkwin;
    int ret_x, ret_y;
    int x = X_LEADING * 2;
    w = (ic->ic_rec.focus_win) ? ic->ic_rec.focus_win : ic->ic_rec.client_win;
    XTranslateCoordinates(gui->display, w, gui->root, 0, 0, &ret_x, &ret_y, &junkwin);

    switch (ic->ic_rec.input_style & XIMPreeditMask)
    {
	case XIMPreeditCallbacks: /* On The Spot */
	    if (inpinfo.keystroke_len)
	    {
		if (ic->has_set_spot)
		    flag |= FLAG_KEYSTROKE_OVERSPOT;
	    }

	    if (inpinfo.n_lcch || inpinfo.keystroke_len ||ic->preedit_length)
	    {
		flag |= FLAG_LCCH_ONSPOT;
		if (!gui->onspot_enabled)
		{
		    flag |= FLAG_LCCH_OVERSPOT;
		}
	    }

	    if (inpinfo.n_mcch)
	    {
		if (!(imc->inpinfo.guimode & GUIMOD_SELKEYSPOT) || !ic->has_set_spot)
		    flag |= FLAG_MCCH_ONBOX;
		else
		    flag = FLAG_MCCH_ONBOX;
	    }
	    break;
	case XIMPreeditPosition:  /* Over The Spot */
	    if (inpinfo.keystroke_len)
		flag |= FLAG_KEYSTROKE_OVERSPOT;
	    if (inpinfo.n_lcch)
		flag |= FLAG_LCCH_OVERSPOT;
	    if (inpinfo.n_mcch)
	    {
		if (imc->inpinfo.guimode & GUIMOD_SELKEYSPOT)
		    flag = FLAG_MCCH_ONBOX;
		else
		    flag |= FLAG_MCCH_ONBOX;
	    }
	    break;
    }

    int isvkb = xccore->virtual_keyboard;
//printf("isvkb=%d\n", isvkb);
    if (flag)
    {
	/* 判斷預編輯文字 */
	switch (flag & FLAG_LCCH_MASK)
	{
	    case FLAG_LCCH_ONSPOT|FLAG_LCCH_OVERSPOT:
		if(!iqqi)
		    draw_preedit_onspot(ic, True, isvkb);
		else
		    draw_preedit_onspot(ic, True, 0);	// for iqqi
		    
		x = draw_preedit_overspot(preedit_win, x);
		break;
	    case FLAG_LCCH_ONSPOT: /* 預編輯文字在游標之上 */
		if(!iqqi)
		    draw_preedit_onspot(ic, False, isvkb);
		else
		    draw_preedit_onspot(ic, False, 0);	// for iqqi
		break;
	    case FLAG_LCCH_OVERSPOT:
		x = draw_preedit_overspot(preedit_win, x);
		break;
	}

	if ((flag & FLAG_KEYSTROKE_MASK) == FLAG_KEYSTROKE_OVERSPOT ||
		!gui->onspot_enabled || inpinfo.keystroke_len || (imc->inpinfo.guimode & GUIMOD_SELKEYSPOT_EX))
	{
	    if(!iqqi)
		x = draw_keystroke_overspot(preedit_win, x, isvkb);
	    else
		x = draw_keystroke_overspot(preedit_win, x, 0);	// for iqqi
	}

    uch_t *keystroke = ic->imc->inpinfo.s_keystroke;
    int iqqi_empty = 0;
    if (!keystroke || keystroke[0].uch==(uchar_t)0)
	iqqi_empty = 1;

	int new_x = ret_x + ic->ic_rec.pre_attr.spot_location.x + 2;
	int new_y = ret_y + ic->ic_rec.pre_attr.spot_location.y + 2;
	if ((flag & FLAG_LCCH_OVERSPOT || flag & FLAG_KEYSTROKE_OVERSPOT/* || inpinfo.keystroke_len*/) && !isvkb /*&& iqqi*/ || (imc->inpinfo.guimode & GUIMOD_SELKEYSPOT_EX && !iqqi_empty))
	//if ((flag & FLAG_LCCH_OVERSPOT || flag & FLAG_KEYSTROKE_OVERSPOT/* || inpinfo.keystroke_len*/) && !isvkb)	// for iqqi
	{
	    gui_winmap_change(preedit_win, True);
	    if (new_x + x > gui->display_width)
	    {
		new_x = gui->display_width - x;
	    }

	    if (new_y + preedit_win->height > gui->display_height)
	    {
		new_y = gui->display_height - preedit_win->height;
	    }

	    if (new_x != preedit_win->pos_x || new_y != preedit_win->pos_y || x != preedit_win->width)
	    {
		preedit_win->pos_x = new_x;
		preedit_win->pos_y = new_y/* - 25*/;
		preedit_win->width = x;
		XMoveResizeWindow(gui->display, preedit_win->window,
			preedit_win->pos_x, preedit_win->pos_y,
			preedit_win->width, preedit_win->height);
	    }
	    draw_Win_3D_Border(preedit_win);
	}
	else
	{
	    gui_winmap_change(preedit_win, False);
	}

        if (flag & FLAG_MCCH_ONBOX && !isvkb && !iqqi)
	//if (flag & FLAG_MCCH_ONBOX && !isvkb &&0) // for iqqi
	{
	    int preview = (imc->inpinfo.guimode & GUIMOD_SELKEYSPOT) ? False : True;
	    draw_multich_onbox(ic, preview);
	    draw_Win_3D_Border(select_win);
	    gui_winmap_change(select_win, True);

	    if (new_x + select_win->width > gui->display_width)
		new_x = gui->display_width - select_win->width;

	    if (ic->has_set_spot)
	    {
		/* 預覽字詞狀態狀態 */
		if (preview)
		{
		    int bottom_h = gui->display_height - (preedit_win->pos_y + preedit_win->height);
		    if (bottom_h < select_win->height)
			new_y = preedit_win->pos_y - select_win->height;
		    else
			new_y = preedit_win->pos_y + preedit_win->height;
		}
		else /* 候選字詞狀態 */
		{
		    if (new_y + select_win->height > gui->display_height)
			new_y = gui->display_height - select_win->height;
		}
	    }
	    else
	    {
		new_x = select_win->pos_x;
		new_y = select_win->pos_y;
	    }

	    if (new_x != select_win->pos_x || new_y != select_win->pos_y)
	    {
		select_win->pos_x = new_x;
		select_win->pos_y = new_y;
		if(imc->inpinfo.guimode & GUIMOD_SELKEYSPOT_EX)
		    select_win->pos_y+=preedit_win->height;
		    
		XMoveWindow(gui->display, select_win->window,
			select_win->pos_x, select_win->pos_y);
	    }
	}
	else
	{
	    gui_winmap_change(select_win, False);
	}
    }
    else
    {
	close_All_Win();
    }
}

static void PreeditEventProcess(winlist_t *win, XEvent *event)
{
    switch (event->type)
    {
	case Expose:
	    if (event->xexpose.count == 0)
	    {
		gui_preedit_draw(win);
	    }
	    break;
	    
	case MotionNotify: /* Mouse Move */
	{
// 	    XDefineCursor(gui->display, win->window, gui_cursor_arrow);
// 	    if (imc->inpinfo.guimode & GUIMOD_SELKEYSPOT)
	    if (win == gui->select_win)
	    {
		XDefineCursor(gui->display, win->window, gui_cursor_move);
		if( CheckPoint(event) > POINT_NOTGETIN )
		    XDefineCursor(gui->display, win->window, gui_cursor_hand);
	    }
	}
	    break;
	    
	case ButtonPress:
	{
	    if (event->xbutton.button == Button1 && win == gui->select_win)
	    {
		int cp = CheckPoint(event);
		Window w = gui_get_input_focus();
		if (w!=None)
		{
		    if( cp>=POINT_ON_SELKEY )
		    {
			SendIMSelected(win->data, w, cp);	// from gui_keyboard.c
		    }
		    else if( cp==POINT_ON_LEFTKEY )
			gui_send_key( w, 0, XStringToKeysym("Left") );
		    else if( cp==POINT_ON_RIGHTKEY )
			gui_send_key( w, 0, XStringToKeysym("Right") );
		    else
			gui_move_window(win);
		}
		
	    }
	}
	    break;
    }
}

/*----------------------------------------------------------------------------

	OverTheSpot Candidate Window initialization.

----------------------------------------------------------------------------*/

void gui_preedit_init(void)
{
    winlist_t *win=NULL;

    if (gui->preedit_win)
    {
	win = gui->preedit_win;
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
    unsigned int font_size = atoi(oxim_get_config(PreeditFontSize));

    if (font_size < 12 || font_size > 48)
        font_size = 16;

    win->font_size = font_size;
    win->width    = 1;
    win->height   = win->font_size + (Y_LEADING*4);

    /* 初始座標在螢幕正中央 */
    win->pos_x = gui->display_width / 2;
    win->pos_y = gui->display_height / 2;
    win->win_draw_func  = gui_preedit_draw;
    win->win_event_func = PreeditEventProcess;

    XSetWindowAttributes win_attr;
    win_attr.override_redirect = True;
    XChangeWindowAttributes(gui->display, win->window,
			CWOverrideRedirect, &win_attr);

    gui->preedit_win = win;
}

void gui_select_init(void)
{
    winlist_t *win=NULL;

    if (gui->select_win)
    {
	win = gui->select_win;
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
    unsigned int font_size = atoi(oxim_get_config(PreeditFontSize));

    if (font_size < 12 || font_size > 48)
        font_size = 16;

    win->font_size = font_size;

    win->width    = 1;
    win->height   = 1;

    /* 初始座標在螢幕正中央 */
    win->pos_x = gui->display_width / 2;
    win->pos_y = gui->display_height / 2;
    win->win_draw_func  = gui_preedit_draw;
    win->win_event_func = PreeditEventProcess;

    XSetWindowAttributes win_attr;
    win_attr.override_redirect = True;
    XChangeWindowAttributes(gui->display, win->window,
			CWOverrideRedirect, &win_attr);

    gui->select_win = win;
}
