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

#include <stdio.h>
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
#include "images/close.xpm"

#define X_LEADING       2
#define Y_LEADING       2
#define TITLE_HEIGHT    20
#define BUFLEN		1024

static int symbol_actived = False;

enum point_type {
	POINT_NONE=0, /* 不在任何控制區內 */
	POINT_TITLE,  /* 游標在標題列 */
	POINT_CLOSE,  /* 游標在關閉按鈕 */
	POINT_MENU,   /* 游標在選單 */
	POINT_SYMBOL  /* 游標在符號框 */
};

typedef struct {
	int type;
	int index;
} point_t;

typedef struct {
	uch_t sym_char;
} chars;

typedef struct {
	int x, y;
        char *name;
	chars list[100];
} symbols;

typedef struct {
	int char_w, char_h;
	int table_x, table_y;
	int table_w, table_h;
	int max_w;
	int x[100], y[100];
	int index;
	int char_idx;
	int NumberOfSymbol;
	symbols **sym_list;
} symbol_data;

/*----------------------------------------------------------*/
static symbol_data sym_data;
/*----------------------------------------------------------*/

static void SplitToList(char *str, chars *list)
{
    char *p = str;
    int len = strlen(str);
    int nbytes;
    unsigned int ucs4;
    int char_idx;

    /* 清除陣列 */
    for (char_idx = 0 ; char_idx < 100 ; char_idx ++)
    {
	list[char_idx].sym_char.uch = (uchar_t)0;
    }

    /* 塞進陣列 */
    char_idx = 0;
    while (char_idx < 100 && len &&
	 (nbytes = oxim_utf8_to_ucs4(p, &ucs4, len)) > 0)
    {
	oxim_ucs4_to_utf8(ucs4, (char *)list[char_idx].sym_char.s); 
	char_idx ++;
	p += nbytes;
	len -= nbytes;
    }
}

/* 讀取 symbol.list */
static int ReadSymbolFile(void)
{
    char *s, *buf = oxim_malloc(BUFLEN, False);
    char *true_fn = oxim_malloc(BUFLEN, False);
    char *sub_path = "tables";
    int ret = False;

    if (oxim_check_datafile("symbol.list", sub_path, true_fn, 256) == True)
    {
	gzFile *fp = NULL;
	fp = oxim_open_file(true_fn, "r", OXIMMSG_WARNING);
	if (fp)
	{
	    while (oxim_get_line(buf, BUFLEN, fp, NULL, "#\r\n"))
	    {
		s = buf;
		int line = 0;
		set_item_t *set_item = oxim_get_key_value(s);
		/* 找找看有沒有重複 */
		if (set_item)
		{
		    int i;
		    for (i = 0 ; i < sym_data.NumberOfSymbol ; i++)
		    {
			if (strcmp(sym_data.sym_list[i]->name, set_item->key) == 0)
			    break;
		    }
		    /* 沒有重複 */
		    if (i >= sym_data.NumberOfSymbol && sym_data.NumberOfSymbol < 10)
		    {
			symbols *sym = oxim_malloc(sizeof(symbols), True);
			sym_data.NumberOfSymbol++;
			if (sym_data.NumberOfSymbol == 1)
			    sym_data.sym_list = (symbols **)oxim_malloc(sizeof(symbols *), True);
			else
			    sym_data.sym_list = (symbols **)oxim_realloc(sym_data.sym_list, sym_data.NumberOfSymbol * sizeof(symbols *));
			sym_data.sym_list[sym_data.NumberOfSymbol - 1] = sym;
			sym->name = strdup(_(set_item->key));
			SplitToList(set_item->value, sym->list);
		    }
		}
		oxim_key_value_destroy(set_item);
	    }
	    gzclose(fp);
	    if (sym_data.NumberOfSymbol > 0)
	    {
		ret = True;
	    }
	}
    }

    free(true_fn);
    free(buf);
    return ret;
}

void gui_switch_symbol(void)
{
    if (symbol_actived)
	gui_hide_symbol();
    else
	gui_show_symbol();
}

int gui_symbol_actived(void)
{
    return symbol_actived;
}

/* 計算符號輸入視窗所需之大小 */
static void CalcSymbol(void)
{
    winlist_t *win = gui->symbol_win;
    xccore_t *xccore = (xccore_t *)win->data;

    if( 0 != strcmp("default", xccore->keyboard_name) )
	win->font_size = 23;

    sym_data.char_w = X_LEADING + win->font_size + X_LEADING; /* 單一符號寬度 */
    sym_data.char_h = Y_LEADING + win->font_size + Y_LEADING; /* 單一符號高度 */
    sym_data.table_w  = (sym_data.char_w * 10) + 11; /* 符號表寬度 */
    sym_data.table_h  = (sym_data.char_h * 10) + 11; /* 符號表高度 */

    /* 計算符號名稱最長寬度 */
    int i;
    int start_x = (X_LEADING * 2) + sym_data.table_w + (X_LEADING * 3);
    int start_y = TITLE_HEIGHT;
    for (i = 0 ; i < sym_data.NumberOfSymbol; i++)
    {
	int name_w = gui_TextEscapement(win, sym_data.sym_list[i]->name,
			strlen(sym_data.sym_list[i]->name));
	if (name_w > sym_data.max_w)
	    sym_data.max_w = name_w;

	sym_data.sym_list[i]->x = start_x;
	sym_data.sym_list[i]->y = start_y + win->font_size;
	start_y += Y_LEADING + win->font_size;
    }

    sym_data.table_x = (X_LEADING * 2);
    sym_data.table_y = TITLE_HEIGHT;
    win->width = start_x + sym_data.max_w + (X_LEADING * 3);
    win->height = TITLE_HEIGHT + (Y_LEADING * 2) + sym_data.table_h + (Y_LEADING * 2);

    /* 計算各符號字元座標 */
    int row, col;
    for (i = 0 ; i < 100 ; i++)
    {
	row = (int)(i / 10);
	col = i % 10;

	sym_data.x[i] = (sym_data.table_x + 1) + (col * (sym_data.char_w+1));
	sym_data.y[i] = (sym_data.table_y + 1) + (row * (sym_data.char_h+1)); 
    }
}

/* 檢查滑鼠指標位置 */
static point_t CheckPoint(int x, int y)
{
    winlist_t *win = gui->symbol_win;
    symbols **sym_list = sym_data.sym_list;
    chars *list = sym_list[sym_data.index]->list;
    point_t pt;
    int i;
    int x1, y1, x2, y2;

    pt.type = POINT_NONE;

    /* 檢查是否在 Title bar ? */
    if (x > 8 && x < win->width - 25 && y < TITLE_HEIGHT)
    {
	pt.type = POINT_TITLE;
	return pt;
    }

    /* 是否在關閉按鈕上 ? */
    if (x >= win->width - 17 && x <= win->width -1 && y >= 1 && y <= 17)
    {
	pt.type = POINT_CLOSE;
	return pt;
    }

    /* 是否在符號選項上 */
    for (i = 0 ; i < sym_data.NumberOfSymbol ; i++)
    {
	x1 = sym_list[i]->x;
	x2 = sym_list[i]->x + sym_data.max_w;
	y1 = sym_list[i]->y - win->font_size;
	y2 = sym_list[i]->y + 2;
	if (x >= x1 && x <= x2 && y >= y1 && y <= y2)
	{
	    pt.type = POINT_MENU;
	    pt.index = i;
	    return pt;
	} 
    }

    /* 檢查是否在符號表中 ? */
    for (i = 0 ; i < 100 ; i++)
    {
	x1 = sym_data.x[i];
	y1 = sym_data.y[i];
	x2 = x1 + sym_data.char_w + 1;
	y2 = y1 + sym_data.char_h + 1;
	if (x >= x1 && x <= x2 && y >= y1 && y <= y2 &&
		list[i].sym_char.uch != (uchar_t)0)
	{
	    pt.type = POINT_SYMBOL;
	    pt.index = i;
	    return pt;
	}
    }

    return pt;
}

static void DrawMenuItem(int idx, int isFocus)
{
    if (idx < 0 || idx > sym_data.NumberOfSymbol)
	return;

    winlist_t *win = gui->symbol_win;
    symbols **sym_list = sym_data.sym_list;
    int font_color, background_color;

    if (isFocus)
    {
	font_color = MENU_FONT_COLOR;
	background_color = MENU_BG_COLOR;
	sym_data.index = idx;
    }
    else
    {
	font_color = FONT_COLOR;
	background_color = BORDER_COLOR;
    }
    gui_Draw_String(win, font_color, background_color,
	sym_list[idx]->x, sym_list[idx]->y,
	sym_list[idx]->name, strlen(sym_list[idx]->name));
}

static void DrawSymbolChar(int idx, int isFocus)
{
    if (idx < 0 || idx > 99)
	return ;

    winlist_t *win = gui->symbol_win;
    chars *list = sym_data.sym_list[sym_data.index]->list;

    int font_color, background_color;
    if (list[idx].sym_char.uch == (uchar_t)0)
    {
	background_color = BORDER_COLOR;
    }
    else
    {
	if (isFocus)
	{
	    font_color = MENU_FONT_COLOR;
	    background_color = MENU_BG_COLOR;
	    sym_data.char_idx = idx;
	}
	else
	{
	    font_color = KEYSTROKE_COLOR;
	    background_color = KEYSTROKEBOX_COLOR;
	}
    }
    XftDrawRect(win->draw, &gui->colors[background_color], sym_data.x[idx],
		sym_data.y[idx], sym_data.char_w, sym_data.char_h);

    gui_Draw_String(win, font_color, NO_COLOR,
	sym_data.x[idx] + X_LEADING, sym_data.y[idx] + win->font_size,
	(char *)list[idx].sym_char.s, strlen((char *)list[idx].sym_char.s));
}

static void DrawSymbol(void)
{
    winlist_t *win = gui->symbol_win;
    symbols **sym_list = sym_data.sym_list;
    int i;

    sym_data.char_idx = -1; /* 沒有選擇 */
    if (sym_data.index < 0 || sym_data.index >= sym_data.NumberOfSymbol)
	sym_data.index = 0;

    /* 外框 */
    gui_Draw_3D_Box(win, 0, 0, win->width, win->height, NO_COLOR, 1);

    /* 移動橫條 */
    gui_Draw_3D_Box(win, 4, 2, win->width - 25, 2, NO_COLOR, 1);
    gui_Draw_3D_Box(win, 4, 8, win->width - 25, 2, NO_COLOR, 1);
    gui_Draw_3D_Box(win, 4, 14, win->width - 25, 2, NO_COLOR, 1);

    /* 關閉按鈕 */
    gui_Draw_Image(win, win->width - 17, 1, close_btn);

    /* 畫符號表 */
    gui_Draw_3D_Box(win, sym_data.table_x, sym_data.table_y,
		sym_data.table_w, sym_data.table_h, NO_COLOR, 0);

    int x, y, x1, y1;
    for (i = 1 ; i < 10 ; i++)
    {
	x  = x1 = sym_data.table_x + (i * (sym_data.char_w+1));
	y  = sym_data.table_y;
	y1 = sym_data.table_y + sym_data.table_h - 2;
	gui_Draw_Line(win, x, y, x1, y1, DARK_COLOR, 0);

	y = y1 = sym_data.table_y + (i * (sym_data.char_h+1));
	x  = sym_data.table_x;
	x1 = sym_data.table_x + sym_data.table_w - 2;
	gui_Draw_Line(win, x, y, x1, y1, DARK_COLOR, 0);
    }

    /* 顯示符號字元 */
    for (i = 0 ; i < 100 ; i++)
    {
	DrawSymbolChar(i, False);
    }

    /* 顯示符號選項 */
    for (i = 0 ; i < sym_data.NumberOfSymbol ; i++)
    {
	DrawMenuItem(i, (i==sym_data.index ? True : False));
    }
}

int gui_show_symbol(void)
{
    if (symbol_actived)
	return True;

    winlist_t *win = gui->symbol_win;
    bzero(&sym_data, sizeof(sym_data));
    if (!ReadSymbolFile())
    {
	printf(_("Read 'symbol.list' error!!\n"));
	return False;
    }
    
    CalcSymbol();
    XResizeWindow(gui->display, win->window, win->width, win->height);
    gui_winmap_change(win, 1);
    symbol_actived = True;
    return True;
}

/* 釋放符號表佔用空間 */
static void FreeSymbol(void)
{
    if (!sym_data.sym_list)
	return;

    int i;
    for (i = 0 ; i < sym_data.NumberOfSymbol ; i++)
    {
	free(sym_data.sym_list[i]->name);
	free(sym_data.sym_list[i]);
    }
    free(sym_data.sym_list);
    sym_data.sym_list = NULL;
    sym_data.NumberOfSymbol = 0;
}

int gui_hide_symbol(void)
{
    gui_winmap_change(gui->symbol_win, 0);
    FreeSymbol();
    symbol_actived = False;
    return 1;
}

static void SymbolEventProcess(winlist_t *win, XEvent *event)
{
    point_t pt;

    switch (event->type)
    {
        case Expose:
            if (event->xexpose.count == 0)
            {
		DrawSymbol();
            }
            break;
	case MotionNotify: /* Mouse Move */
	    pt = CheckPoint(event->xbutton.x, event->xbutton.y);
	    switch (pt.type)
	    {
		case POINT_TITLE: /* Title bar */
		    XDefineCursor(gui->display, win->window, gui_cursor_move);
		    break;
		case POINT_CLOSE:
		    XDefineCursor(gui->display, win->window, gui_cursor_hand);
		    break;
		case POINT_MENU:
		    XDefineCursor(gui->display, win->window, gui_cursor_hand);
		    break;
		case POINT_SYMBOL:
		    XDefineCursor(gui->display, win->window, gui_cursor_hand);
		    if (sym_data.char_idx != pt.index)
		    {
			 DrawSymbolChar(sym_data.char_idx, False);
			 DrawSymbolChar(pt.index, True);
		    }
		    break;
		default:
		    XDefineCursor(gui->display, win->window, gui_cursor_arrow);
	    }
	    break;
	case LeaveNotify: /* Mouse Out */
	    break;
	case ButtonPress:
            if (event->xbutton.button == Button1)
            {
		pt = CheckPoint(event->xbutton.x, event->xbutton.y);
		switch (pt.type)
		{
		    case POINT_TITLE: /* Title bar */
			gui_move_window(win);
			break;
		    case POINT_CLOSE: /* Close Symbol window */
			gui_hide_symbol();
			break;
		    case POINT_MENU: /* Change Symbol Item */
			if (sym_data.char_idx != pt.index)
			{
			    DrawMenuItem(sym_data.index, False);
			    DrawMenuItem(pt.index, True);
			    /* 顯示符號字元 */
			    int i;
			    for (i = 0 ; i < 100 ; i++)
			    {
				DrawSymbolChar(i, False);
			    }
			}
			break;
		    case POINT_SYMBOL:
		    {
			xccore_t *xccore = (xccore_t *)win->data;
			IC *ic = xccore->ic;
			if (ic)
			{
			    chars *list = sym_data.sym_list[sym_data.index]->list;
			    xim_commit_string(ic, (char *)list[sym_data.char_idx].sym_char.s);
			}
			break;
		    }
		}
            }
	    break;
    }
}


void gui_symbol_init(void)
{
    winlist_t *win = NULL;

    if (gui->symbol_win)
    {
	win = gui->symbol_win;
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
    unsigned int font_size = atoi(oxim_get_config(SymbolFontSize));
    if (font_size < 12 || font_size > 24)
        font_size = 13;

    win->font_size = font_size;

    win->width   = 1;
    win->height  = 1;
    win->pos_x = gui->display_width / 2;
    win->pos_y = gui->display_height / 2;

    win->win_event_func = SymbolEventProcess;

    XSetWindowAttributes win_attr;
    win_attr.override_redirect = True;
    XChangeWindowAttributes(gui->display, win->window, 
			CWOverrideRedirect, &win_attr);

    gui->symbol_win = win;
}
