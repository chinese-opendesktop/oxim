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
typedef int (*func) (void);

#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/xpm.h>
#include <string.h>
#include "oximtool.h"
#include "oxim.h"
#include "imodule.h"

#define X_LEADING       2
#define Y_LEADING       2
#define KEY_SPACE	16

/*----------------------------------------------------------*/
static int menu_actived = False; /* 選單執行中 */
static unsigned int n_items  = 0; /* 選項數量 */
static unsigned int max_name = 0; /* 最長的選項寬度 */
static unsigned int max_key  = 0; /* 最長的按鍵寬度 */
static unsigned int full_length = 0; /* 最大 Item 寬度 */
static int select_item = -1; /* 選項指標 */

typedef struct menu_item {
	int ID;
        int x, y;
        int width;
        char *name;
	char *key;
	int (*exec_func)(int idx);
	int enabled;
} menu_item;
/*----------------------------------------------------------*/
static menu_item **items = NULL;
/*----------------------------------------------------------*/

/*
  計算選單的大小
*/
static void CalcMenuItem(void)
{
    int i;
    winlist_t *win = gui->menu_win;
    menu_item *mi;
    max_name = 0; /* 最長的選項寬度 */
    max_key  = 0; /* 最長的按鍵寬度 */

    unsigned int start_x  = X_LEADING * 3;
    unsigned int start_y  = Y_LEADING;

    for (i = 0 ; i < n_items ; i++)
    {
	mi = items[i];
	const unsigned int name_len = strlen(mi->name); /* 字串長度 */
	const unsigned int key_len  = strlen(mi->key);  /* 按鍵長度 */
	unsigned int name_w = 0; /* 選項寬度 */
	unsigned int key_w = 0; /* 按鍵寬度 */

	/* 是否是分隔線? */
	if (mi->ID < 0)
	{
	    start_y += 6;
	}
	else
	{
	    name_w = gui_TextEscapement(win, mi->name, name_len);
	    if (key_len)
		key_w = gui_TextEscapement(win, mi->key, key_len);

	    if (max_name < name_w)
		max_name = name_w;

	    if (max_key < key_w)
		max_key = key_w;

	    start_y += win->font_size + Y_LEADING;
	}
	mi->x = start_x;
	mi->y = start_y;
    }
    full_length = max_name + max_key + (max_key ? KEY_SPACE : 0);
}

static void ShowMenuItem(int nItem, int isFocus)
{
    if (nItem < 0 || nItem > n_items)
	return;

    winlist_t *win = gui->menu_win;
    char *name = items[nItem]->name;
    char *key  = items[nItem]->key;
    const unsigned int name_len = strlen(name); /* 字串長度 */
    const unsigned int key_len  = strlen(key);  /* 按鍵長度 */

    int x = items[nItem]->x;
    int y = items[nItem]->y;
    int key_color, font_color, background_color;

    if (items[nItem]->ID < 0)
    {
	gui_Draw_3D_Box(win, x, y - 1, full_length, 2, NO_COLOR, 0);
    }
    else
    {
	if (isFocus && items[nItem]->enabled)
	{
	    key_color = font_color = MENU_FONT_COLOR;
	    background_color = MENU_BG_COLOR;
	}
	else
	{
	    if (items[nItem]->enabled)
	    {
		key_color = KEYSTROKE_COLOR;
		font_color = FONT_COLOR;
	    }
	    else
	    {
		key_color = font_color = DARK_COLOR;
	    }
	    background_color = BORDER_COLOR;
	    
	}

	int top = y - win->font_size;
	XftDrawRect(win->draw, &gui->colors[background_color], x, top,
		full_length, win->font_size + Y_LEADING);

	if (!isFocus)
	    gui_Draw_String(win, LIGHT_COLOR, NO_COLOR, x+1, y+1, name, name_len);

	gui_Draw_String(win, font_color, NO_COLOR, x, y, name, name_len);

	if (key_len)
	{
	    int offset = max_key - gui_TextEscapement(win, key, key_len);
	    if (!isFocus)
		gui_Draw_String(win, LIGHT_COLOR, NO_COLOR, max_name + KEY_SPACE + offset + 1, y+1, key, key_len);
	    gui_Draw_String(win, key_color, NO_COLOR, max_name + KEY_SPACE + offset, y, key, key_len);
	}
    }
}

int gui_menu_actived(void)
{
    return menu_actived;
}

static void FreeMenu(void)
{
    if (!items)
	return;

    int i;
    for (i = 0 ; i < n_items ; i++)
    {
	if (items[i]->name)
	    free(items[i]->name);
	if (items[i]->key)
	    free(items[i]->key);

	free(items[i]);
    }
    free(items);
    n_items = 0;
    items = NULL;
}

static void AddItem(int item_id, char *item_name, char *hotkey, void *func, int enabled)
{
    n_items ++;

    menu_item *mi = oxim_malloc(sizeof(menu_item), True);
    if (n_items == 1)
	items = (menu_item **)oxim_malloc(sizeof(menu_item *), True);
    else
	items = (menu_item **)oxim_realloc(items, n_items * sizeof(menu_item *));
    mi->ID   = item_id;
    mi->name = strdup(item_name);
    mi->key  = strdup(hotkey);
    items[n_items - 1] = mi;
    mi->exec_func = func;
    mi->enabled = enabled;
}

static int ChangeToIM(int idx)
{
    winlist_t *win = gui->menu_win;
    xccore_t *xccore = (xccore_t *)win->data;
    IC *ic = xccore->ic;

    if (!ic)
	return True;

    int ID = items[idx]->ID;
    if (!(ic->imc->inp_state & IM_CINPUT))
    {
	ic->imc->inp_num = ID;
	Window w = gui_get_input_focus();
	gui_send_key(w, ControlMask, XK_space);
    }
    else
    {
	change_IM(ic, ID);
	xim_update_winlist();
    }
    return True;
}

/* 全形/半形模式 */
/*static */int ChangeTo2BSBMode(int idx)
{
    winlist_t *win = gui->menu_win;
    xccore_t *xccore = (xccore_t *)win->data;
    IC *ic = xccore->ic;

    if (!ic)
	return True;

    IM_Context_t *imc = ic->imc;
    inp_state_t inp_state = imc->inp_state;

    if ((imc->inp_state & IM_2BYTES)) {
	imc->inp_state &= ~(IM_2BYTES | IM_2BFOCUS);
	if ((xccore->oxim_mode & OXIM_IM_FOCUS))
	    xccore->oxim_mode &= ~OXIM_RUN_2B_FOCUS;
    }
    else {
	imc->inp_state |= (IM_2BYTES | IM_2BFOCUS);
	if ((xccore->oxim_mode & OXIM_IM_FOCUS))
	    xccore->oxim_mode |= OXIM_RUN_2B_FOCUS;
    }

    xim_update_winlist();
    return True;
}

/* 切換成 輸出過濾 模式 */
/*static */int ChangeToFilterMode(int idx)
{
    winlist_t *win = gui->menu_win;
    xccore_t *xccore = (xccore_t *)win->data;
    IC *ic = xccore->ic;

    if (!ic)
		return True;

	idx = (idx<0) ? -1 : 1;
	
    IM_Context_t *imc = ic->imc;
    inp_state_t inp_state = imc->inp_state;

    change_filter(idx, True, &ic->filter_current);
    if (!(imc->inp_state & IM_FILTER))
    {
	imc->inp_state |= IM_FILTER;
	
	if( 0 == strcmp((char *)change_filter(0, False, &ic->filter_current), "default"))
	    change_filter(idx, False, &ic->filter_current);
    }

    xim_update_winlist();
    return True;
}

/* 切換成簡體輸出模式 */
static int ChangeToSimpMode(int idx)
{
    winlist_t *win = gui->menu_win;
    xccore_t *xccore = (xccore_t *)win->data;
    IC *ic = xccore->ic;

    if (!ic)
	return True;

    IM_Context_t *imc = ic->imc;
    inp_state_t inp_state = imc->inp_state;

    if (!(inp_state & IM_OUTSIMP))
    {
	ic->imc->inp_state &= 0x0f;
	ic->imc->inp_state |= IM_OUTSIMP;
    }
    else
	ic->imc->inp_state &= 0x0f;

    xim_update_winlist();
    return True;
}

/* 切換成繁體輸出模式 */
static int ChangeToTradMode(int idx)
{
    winlist_t *win = gui->menu_win;
    xccore_t *xccore = (xccore_t *)win->data;
    IC *ic = xccore->ic;

    if (!ic)
	return True;

    IM_Context_t *imc = ic->imc;
    inp_state_t inp_state = imc->inp_state;

    if (!(inp_state & IM_OUTTRAD))
    {
	ic->imc->inp_state &= 0x0f;
	ic->imc->inp_state |= IM_OUTTRAD;
    }
    else
	ic->imc->inp_state &= 0x0f;

    xim_update_winlist();
    return True;
}
/* CTRL+SHIFT切換*/
/*static */int SwitchCtrlShift(int idx)
{
    Window w = gui_get_input_focus();
    gui_send_key(w, ControlMask|ShiftMask, XK_Control_L);

    return True;
}
/*static */int SwitchShiftControl(int idx)
{
    Window w = gui_get_input_focus();
    gui_send_key(w, ControlMask|ShiftMask, XK_Shift_L);

    return True;
}
/* 切換成英數中文切換模式 */
/*static*/ int ChangeToEn_ChMode(int idx)
{
    winlist_t *win = gui->menu_win;
    xccore_t *xccore = (xccore_t *)win->data;
    IC *ic = xccore->ic;

    if (!ic)
	return True;

    IM_Context_t *imc = ic->imc;
    inp_state_t inp_state = imc->inp_state;
    Window w = gui_get_input_focus();

    if ((inp_state & IM_2BYTES))
	gui_send_key(w, ShiftMask, XK_space);

    gui_send_key(w, ControlMask, XK_space);

    return True;
}

/* XCIN 輸入風格切換 */
static int SwitchXCINStyle(int idx)
{
    gui_xcin_style_switch();
    xim_update_winlist();
    return True;
}

static int SwitchSymbol(int idx)
{
    gui_switch_symbol();
    return True;
}

static int SwitchKeyboard(int idx)
{
#if 0
    if( oxim_check_cmd_exist("oxim-touchboard") )
    {
// 	if ( !touchboard_isrunning )
	{
	    (void)system("oxim-touchboard&");
// 	    touchboard_isrunning = True;
	}
    }
    else
#endif
	gui_switch_keyboard();
    return True;
}


static int RunSetup(int idx)
{
    (void)system("oxim-setup&");
    return True;
}

int RunTegaki(int idx)
{
    (void)system("tegaki-oxim&");
    return True;
}

static void MakeMenu(void)
{
    int n_IM = oxim_get_NumberOfIM();
    int idx;
    winlist_t *win = gui->menu_win;
    xccore_t *xccore = (xccore_t *)win->data;
    IC *ic = xccore->ic;
    
    setlocale (LC_MESSAGES, "");
    IM_Context_t *imc = NULL;
    inp_state_t inp_state = 0;
    int inp_num = -1;
    if (ic)
    {
	imc = ic->imc;
	inp_state = imc->inp_state;
	inp_num = imc->inp_num;
    }
    int isCinput = (inp_state & IM_CINPUT);
    char key[2];

    key[1] = '\0';

    for (idx = 0 ; idx < n_IM ; idx ++)
    {
	im_t *imp = oxim_get_IMByIndex(idx);
	if (imp)
	{
	    char hotkey[16];
	    strcpy(hotkey, "Ctrl Alt ");
	    if (imp->key >= 0)
	    {
		key[0] = '0' + (imp->key == 10 ? 0 : (char)imp->key);
		strcat(hotkey, key);
	    }
	    else
		hotkey[0] = '\0';

	    if (imp->inpname)
		AddItem(idx, (imp->aliasname ? imp->aliasname : imp->inpname), hotkey, ChangeToIM, ( !isCinput || idx != inp_num));
	}
    }
    AddItem(-1, "", "", NULL, False);
    AddItem(900, _("Width Toggle"), "Shift Space", ChangeTo2BSBMode, True);	//全形/半形模式切換
/*    AddItem(901, _("Han-S Output"), "Ctrl Alt -", ChangeToSimpMode, isCinput);//简体输出
    AddItem(902, _("Han-T Output"), "Ctrl Alt =", ChangeToTradMode, isCinput);	//繁體輸出*/
    AddItem(903, _("Mode Toggle"), "Ctrl Space", ChangeToEn_ChMode, True);	//輸入模式切換
    AddItem(904, _("Rule Switch"), _("L Ctrl Shift"), SwitchCtrlShift, True);	//輸入法輪換
    AddItem(1101, _("Style Toggle"), _("L+R Shift"), SwitchXCINStyle, True);	//OXIM 輸入風格 <->XCIN輸入風格
    AddItem(-1, "", "", NULL, False);
    AddItem(1102, _("Symbol List"), "Ctrl Alt ,", SwitchSymbol, True);	//各類符號輸入表
    AddItem(1103, _("Keyboard"), "Ctrl Alt .", SwitchKeyboard, True);	//螢幕輸入小鍵盤
    if(get_filter_num() > 1)
	AddItem(1104, _("Output Filter Switch"), "Ctrl Alt \\", ChangeToFilterMode, True);	// 輸出過濾
    if(oxim_check_cmd_exist("tegaki-oxim"))
    {
	AddItem(1202, _("HWR Input"), "Ctrl Alt /", RunTegaki, True);	//手寫輸入
    }
    if(oxim_check_cmd_exist("oxim-setup"))
    {
	AddItem(-1, "", "", NULL, False);
	AddItem(1202, _("Setting..."), "", RunSetup, True);	//設定
    }
}

static void gui_draw_menu(winlist_t *win)
{
    gui_winmap_change(win, 1);
    gui_Draw_3D_Box(win, 0, 0, win->width, win->height, NO_COLOR, 1);
    int i;
    for (i=0 ; i < n_items ; i++)
    {
	ShowMenuItem(i, 0);
    }
}


void gui_show_menu(winlist_t *parent_win)
{
    winlist_t *win = gui->menu_win;
    Window w = parent_win->window, junkwin;
    int ret_x, ret_y; /* 父視窗左上角座標 */
    XTranslateCoordinates(gui->display, w, gui->root, 0, 0, &ret_x, &ret_y, &junkwin);

    MakeMenu();
    CalcMenuItem();

    int width = full_length + (X_LEADING * 4);
    int height = items[n_items-1]->y + (Y_LEADING * 3);

    int new_x, new_y;

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

    XMoveResizeWindow(gui->display, win->window, 
			win->pos_x, win->pos_y,
			win->width, win->height);

    gui_draw_menu(win);
    select_item = -1;
    menu_actived = True;
}

void gui_hide_menu(void)
{
    gui_winmap_change(gui->menu_win, 0);
    FreeMenu();
    menu_actived = False;
}

static int GetMouseItemPos(int my)
{

    int font_size = gui->menu_win->font_size;
    int i;
    for (i = 0 ; i < n_items ; i++)
    {
	int y2 = items[i]->y;
	int y1 = y2 - font_size - Y_LEADING;
	if (my >= y1 && my <= y2)
	    return i;
    }
    return -1;
}

static void MenuEventProcess(winlist_t *win, XEvent *event)
{
    int idx;

    switch (event->type)
    {
	case Expose:
	    if (event->xexpose.count == 0)
	    {
		gui_draw_menu(win);
	    }
	    break;
	case MotionNotify: /* Mouse Move */
	    idx = GetMouseItemPos(event->xbutton.y);
	    if (idx >= 0 && items[idx]->enabled)
	    {
		XDefineCursor(gui->display, win->window, gui_cursor_hand);
		if (idx != select_item)
		{
		    ShowMenuItem(select_item, 0);
		    ShowMenuItem(idx, 1);
		    select_item = idx;
		}
	    }
	    else
	    {
		XDefineCursor(gui->display, win->window, gui_cursor_arrow);
	    }
	    break;
	case LeaveNotify: /* Mouse Out */
	    gui_hide_menu();
	    break;
	case ButtonPress:
	    {
		idx = GetMouseItemPos(event->xbutton.y);
		if (idx >= 0 && items[idx]->enabled && items[idx]->exec_func)
		{
		    if (items[idx]->exec_func(idx))
			gui_hide_menu();
		}
	    }
	    break;
    }
}


void gui_menu_init(void)
{
    winlist_t *win = NULL;

    if (gui->menu_win)
    {
	win = gui->menu_win;
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
    unsigned int font_size = atoi(oxim_get_config(MenuFontSize));
    if (font_size < 12 || font_size > 20)
	font_size = 13;

    win->font_size = font_size;

    win->width   = 1;
    win->height  = 1;
    win->pos_x = 0;
    win->pos_y = 0;

    win->win_event_func = MenuEventProcess;

    XSetWindowAttributes win_attr;
    win_attr.override_redirect = True;
    XChangeWindowAttributes(gui->display, win->window, 
			CWOverrideRedirect, &win_attr);

    gui->menu_win = win;
}
