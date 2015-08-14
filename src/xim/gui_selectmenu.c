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
#include <math.h>
#include "oximtool.h"
#include "oxim.h"

#define X_LEADING	2
#define Y_LEADING	2

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

/*-------------------------------------------------------------------*/
static int selectmenu_actived = False;
static int current_page = 0;
static int content_bound = 0;
static char **sContent = NULL;
static char *selKey_define[SELKEY_IDX] = 
    {"1\0","2\0","3\0","4\0","5\0","6\0","7\0","8\0","9\0","0\0"};
/*-------------------------------------------------------------------*/

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
    if (!sContent)
	return;
    
    double total_page = ceil(content_bound/SELKEY_IDX);
    int i, j, len, toggle_flag, x2, c;
    winlist_t *select_menu_win = gui->select_menu_win;
    int n_items = 0;  /* 總共幾個項目可選 */
    int key_max_w = 0; /* 選擇鍵最大寬度 */
    int str_max_w = 0; /* 字詞最大寬度 */
    
    toggle_flag = 0;
    /* 計算總共有幾個字詞可選
	以及最寬的英文以及中文
    */
    int save_flag = toggle_flag;
    for(n_items=current_page*SELKEY_IDX, i=0, c=current_page*SELKEY_IDX*BUFSIZE; (i<SELKEY_IDX) && n_items<content_bound; i++, c+=BUFSIZE, n_items++)
    {
	char *word = (char *)(sContent+c);
	
	/* 算選擇鍵最大寬度 */
	len = strlen(selKey_define[i]);
	int key_w = gui_TextEscapement(select_menu_win, (char *)selKey_define[i], len);
	if (key_w > key_max_w)
	    key_max_w = key_w;
	// 算出字詞最大寬度
	len = strlen(word);
	int work_w = 0;
	work_w = gui_TextEscapement(select_menu_win, word, len);
	if (work_w > str_max_w)
	    str_max_w = work_w;
    }
    toggle_flag = save_flag;
    n_items = i;
	
    int item_width = X_LEADING + 5 + key_max_w + X_LEADING + str_max_w + 5 +X_LEADING;

    char *word_str = _("Selected words");	//候選詞
    char *char_str = _("Selected word");	//候選字
    char *select_str = toggle_flag ? word_str : char_str;
//     char *select_str = "相關詞";
    len = strlen(select_str);
    int select_w = X_LEADING + gui_TextEscapement(select_menu_win, select_str, len) + X_LEADING;
    if (!preview && select_w > item_width)
	item_width = select_w;
    
    int fontheight = select_menu_win->font_size + 2;
    int selectbox_height = (n_items * fontheight) + (Y_LEADING * 6);
    int selectbox_width = item_width - (X_LEADING * 2);

    int width = item_width;
    int height = selectbox_height + (Y_LEADING * 2) + (!preview ? fontheight : 0) + ((total_page && !preview) ? fontheight : 0);
    if (select_menu_win->width != width || select_menu_win->height != height)
    {
	select_menu_win->width = width;
	select_menu_win->height = height;
	XResizeWindow(gui->display, select_menu_win->window, width, height);
    }

    /* 顯示候選字詞 */
    int x = X_LEADING;
    int y = select_menu_win->font_size + 2;
    if (!preview)
    {
	gui_Draw_String(select_menu_win, LIGHT_COLOR, BORDER_COLOR, x+1, y+1, select_str, len);
	gui_Draw_String(select_menu_win, INPUTNAME_COLOR, NO_COLOR, x, y, select_str, len);
	y += Y_LEADING * 2;
    }
    else
    {
	y = Y_LEADING;
    }

    gui_Draw_3D_Box(select_menu_win, X_LEADING, y, selectbox_width, selectbox_height, NO_COLOR, 0);
    gui_Draw_3D_Box(select_menu_win, X_LEADING+2, y+2, selectbox_width-4, selectbox_height-4, SELECTBOX_COLOR, 1);
    
    y += (Y_LEADING * 2);
    int key_x = (toggle_flag) ? (X_LEADING*4) : (item_width - (key_max_w + X_LEADING + str_max_w))/2;

    for(i=0; i<SELKEY_IDX; i++)
    {
	// empty
	selkeys.keys[i].x1 = selkeys.keys[i].y1 = selkeys.keys[i].x2 = selkeys.keys[i].y2 = 0;
    }

    for(n_items=current_page*SELKEY_IDX, i=0, c=current_page*SELKEY_IDX*BUFSIZE; (i<SELKEY_IDX) && n_items<content_bound; i++, c+=BUFSIZE, n_items++)
    {
	char *word = (char *)(sContent+c);
        y += fontheight;
	x = key_x;
	
	len = strlen(selKey_define[i]);
	gui_Draw_String(select_menu_win, KeystrokeBoxColor, NO_COLOR, x, y, (char *)selKey_define[i], len);
	selkeys.keys[i].x1 = x;
	selkeys.keys[i].y1 = y - fontheight;
	    
	x = key_x + key_max_w + X_LEADING;
	len = strlen(word);
	x2 = x + gui_Draw_String(select_menu_win, SELECTFONT_COLOR, NO_COLOR, x, y, (char *)word, len);
	x = x2;
	
	selkeys.keys[i].x2 = x2;
	selkeys.keys[i].y2 = y;
    }
    if (preview)
	return;
    
    #define LEFT_AR 0x10
    #define RIGHT_AR 0x01
    int pgstate = 0;
    
    if(0 == current_page)
	pgstate = RIGHT_AR;
    else
    if(current_page < total_page)
	pgstate = LEFT_AR | RIGHT_AR;
    else
    if(current_page == total_page)
	pgstate = LEFT_AR;
    if (pgstate)
    {
	char *left_ar  = "︽";
	int left_ar_w  = gui_TextEscapement(select_menu_win, left_ar, strlen(left_ar));
	char *right_ar = "︾";
	int right_ar_w = gui_TextEscapement(select_menu_win, right_ar, strlen(right_ar));
	y += fontheight + (Y_LEADING * 2);
	gui_Draw_String(select_menu_win, LIGHT_COLOR, NO_COLOR, X_LEADING+1, y+1, left_ar, strlen(left_ar));
	gui_Draw_String(select_menu_win, LIGHT_COLOR, NO_COLOR, width - right_ar_w - X_LEADING + 1, y+1, right_ar, strlen(right_ar));
	int left_color = (pgstate & LEFT_AR) ? INPUTNAME_COLOR : DARK_COLOR;
	int left_width = gui_Draw_String(select_menu_win, left_color, NO_COLOR, X_LEADING, y, left_ar, strlen(left_ar));
	int right_color = (pgstate & RIGHT_AR) ? INPUTNAME_COLOR : DARK_COLOR;
	int right_width = gui_Draw_String(select_menu_win, right_color, NO_COLOR, width - right_ar_w - X_LEADING, y, right_ar, strlen(right_ar));
	
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

static void
close_All_Win(void)
{
    gui_winmap_change(gui->select_menu_win, False);
    selectmenu_actived = False;
}

static void
gui_selectmenu_draw(winlist_t *win)
{
    xccore_t *xccore = (xccore_t *)win->data;
    IC *ic = xccore->ic;

//     IM_Context_t *imc = ic->imc;
//     if (!(imc->inp_state & IM_XIMFOCUS) || (ic->ic_rec.input_style & XIMPreeditMask) == XIMPreeditNothing || (ic->ic_rec.input_style & XIMPreeditMask) == XIMPreeditArea)
    if (!sContent)
    {
	close_All_Win();
	return;
    }

    winlist_t *select_menu_win = gui->select_menu_win;
    Window w, junkwin;
    int ret_x, ret_y;
    int x = X_LEADING * 2;
    w = (ic->ic_rec.focus_win) ? ic->ic_rec.focus_win : ic->ic_rec.client_win;
    XTranslateCoordinates(gui->display, w, gui->root, 0, 0, &ret_x, &ret_y, &junkwin);
    
    int new_x = ret_x + ic->ic_rec.pre_attr.spot_location.x + 2;
    int new_y = ret_y + ic->ic_rec.pre_attr.spot_location.y + 2;

    int preview = False;
    draw_multich_onbox(ic, preview);
    draw_Win_3D_Border(select_menu_win);
    gui_winmap_change(select_menu_win, True);

    if (new_x + select_menu_win->width > gui->display_width)
	new_x = gui->display_width - select_menu_win->width;

    if (True)
    {
	/* 候選字詞狀態 */
	if (new_y + select_menu_win->height > gui->display_height)
	    new_y = gui->display_height - select_menu_win->height;
    }
    else
    {
	new_x = select_menu_win->pos_x;
	new_y = select_menu_win->pos_y;
    }

    if (new_x != select_menu_win->pos_x || new_y != select_menu_win->pos_y)
    {
	select_menu_win->pos_x = new_x;
	select_menu_win->pos_y = new_y;
	XMoveWindow(gui->display, select_menu_win->window,
		select_menu_win->pos_x, select_menu_win->pos_y);
    }
}

void select_menu_selected(IC *ic, int keysym)
{
        char *pattern = "!@#$%^&*()";
	int i;
	
	for(i=0; pattern[i]; i++)
	{
	    if((int)pattern[i]==keysym)
		xim_commit_string_raw(ic, (char*)(sContent + (current_page*SELKEY_IDX+i)*BUFSIZE));
	}
	sContent = NULL;
	close_All_Win();
}

void select_menu_selectpage(IC *ic, int next)
{
    double total_page = ceil(content_bound/SELKEY_IDX);
    
    if(next==-1)
	current_page = (0 == current_page) ? total_page : current_page-1;
    else
	current_page = (total_page == current_page) ? 0 : current_page+1;
    gui_selectmenu_draw(gui->select_menu_win);
}

int gui_selectmenu_actived(void)
{
    return selectmenu_actived;
}

void gui_hide_selectmenu()
{
    gui_winmap_change(gui->select_menu_win, 0);
    selectmenu_actived = False;
}

void gui_show_selectmenu(winlist_t *parent_win, char *msg)
{
/*    sContent = oxim_realloc(sContent, sizeof(char)*strlen(msg));
    strcpy(sContent, msg);*/
    char delims[] = "\n";
    char *result = NULL;
    char *saveptr;
    char word[BUFSIZE];
    int i, j, c;
    
    for( i=0, c=0, result = strtok_r( msg, delims, &saveptr ); result != NULL; i++, c+=BUFSIZE, result = strtok_r( NULL, delims, &saveptr ) )
    {
//       for(i=0, c=0; oxim_get_word(&result, word, BUFSIZE, NULL); i++, c+=BUFSIZE)
      {
	  sContent = (char **)oxim_realloc(sContent, (i+1) * BUFSIZE * sizeof(char*));
	  strcpy((char *)&sContent[c], result);
      }
    }
    content_bound = i;
    current_page = 0;
    selectmenu_actived = True;
    //gui_selectmenu_draw(parent_win);
    gui_selectmenu_draw(gui->status_win);
}

static void SelectMenuEventProcess(winlist_t *win, XEvent *event)
{
    switch (event->type)
    {
	case Expose:
	    if (event->xexpose.count == 0)
	    {
		gui_selectmenu_draw(win);
	    }
	    break;
	    
	case MotionNotify: /* Mouse Move */
	{
	    XDefineCursor(gui->display, win->window, gui_cursor_move);
	    if( CheckPoint(event) > POINT_NOTGETIN )
		XDefineCursor(gui->display, win->window, gui_cursor_hand);
	}
	    break;

	case ButtonPress:
	{
	    double total_page = ceil(content_bound/SELKEY_IDX);
	    if (event->xbutton.button == Button1)
	    {
		int cp = CheckPoint(event);
		Window w = gui_get_input_focus();
		xccore_t *xccore = (xccore_t *)win->data;
		IC *ic = xccore->ic;

		if(w!=None)
		{
		    if( cp>=POINT_ON_SELKEY )
		    {
			xim_commit_string_raw(ic, (char*)(sContent + (current_page*SELKEY_IDX+cp)*BUFSIZE));
			sContent = NULL;
			close_All_Win();
		    }
		    else if( cp==POINT_ON_LEFTKEY )
		    {
			select_menu_selectpage(ic, -1);
		    }
		    else if( cp==POINT_ON_RIGHTKEY )
		    {
			select_menu_selectpage(ic, 1);
		    }
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

void gui_selectmenu_init(void)
{
    winlist_t *win=NULL;

    if (gui->select_menu_win)
    {
	win = gui->select_menu_win;
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
    win->win_draw_func  = gui_selectmenu_draw;
    win->win_event_func = SelectMenuEventProcess;

    XSetWindowAttributes win_attr;
    win_attr.override_redirect = True;
//     win_attr.do_not_propagate_mask = True;
    XChangeWindowAttributes(gui->display, win->window,
			CWOverrideRedirect, &win_attr);

    gui->select_menu_win = win;
}
