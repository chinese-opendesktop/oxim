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

#ifndef  _WINLIST_H
#define  _WINLIST_H

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <X11/cursorfont.h>
#include "oximtool.h"
typedef struct gui_s gui_t;

/*
 *  GUI Window configuration.
 */
#define WMODE_MAP		1
#define	WMODE_EXIT		2

typedef struct winlist_s winlist_t;
struct winlist_s {
    Window window;
    unsigned int font_size;
    XftDraw *draw;
    xmode_t winmode;

    int pos_x, pos_y;
    unsigned int width, height;

    void *data;
    void (*win_draw_func)(winlist_t *);
    void (*win_event_func)(winlist_t *, XEvent *);
    winlist_t *next;
};

/*
 *  GUI global configuration.
 */
#define WIN_CHANGE_IM           0x0001
#define WIN_CHANGE_FOCUS        0x0002
#define WIN_CHANGE_REDRAW       0x000f

#define WIN_MONITOR_CLIENT	0x0001
#define WIN_MONITOR_FOCUS	0x0002
#define WIN_MONITOR_OVERSPOT	0x0004

enum color_set {
    NO_COLOR = -1,
    WINBOX_COLOR = 0,
    BORDER_COLOR,
    LIGHT_COLOR,
    DARK_COLOR,
    CURSOR_COLOR, 
    CURSORFONT_COLOR,
    FONT_COLOR,
    CONVERTNAME_COLOR,
    INPUTNAME_COLOR,
    UNDERLINE_COLOR,
    KEYSTROKE_COLOR,
    KEYSTROKEBOX_COLOR,
    SELECTFONT_COLOR,
    SELECTBOX_COLOR,
    MENU_BG_COLOR,
    MENU_FONT_COLOR,
    XCIN_BORDER_COLOR,
    XCIN_BG_COLOR,
    XCIN_FONT_COLOR,
    XCIN_STATUS_FG_COLOR,
    XCIN_STATUS_BG_COLOR,
    XCIN_CURSOR_FG_COLOR,
    XCIN_CURSOR_BG_COLOR,
    XCIN_UNDERLINE_COLOR,
    MSGBOX_BG_COLOR,
    MAX_COLORS
};

struct gui_s {
    Display *display;
//    int (*display_width)(void), (*display_height)(void);
    int display_width, display_height;
    int screen;
    Visual *visual;

    Window root;
    Colormap colormap;

    FcCharSet *missing_chars; /* 系統找不到的字元集合 */

    unsigned int default_fontsize;
    unsigned int num_fonts;
    XftFont **xftfonts;
    XftColor colors[MAX_COLORS];

    int xcin_style; /* 是否使用 XCIN 輸入風格 */
    int onspot_enabled; /* 使用 On The Spot 風格 */
    winlist_t *xcin_win;

    winlist_t *root_win, *status_win, *preedit_win, *select_win, *tray_win;
    winlist_t *menu_win, *symbol_win, *keyboard_win;
    winlist_t *msgbox_win, *select_menu_win;
};

extern gui_t *gui;
extern winlist_t *gui_search_win(Window window);
extern winlist_t *gui_new_win(void);
extern void gui_freewin(Window window);
extern void gui_winmap_change(winlist_t *win, int state);
extern int gui_check_window(Window window);

extern void gui_move_window(winlist_t *win);
extern unsigned int gui_Draw_String(winlist_t *win, int fgcolor_idx, int bgcolor_idx, int x, int y, char *string, int len);
extern unsigned int gui_TextEscapement(winlist_t *win, char *string, int len);
extern void gui_Draw_3D_Box(winlist_t *win, int x, int y, int width, int height, int fill_idx, int boxstyle);
/* 劃線 */
extern void gui_Draw_Line(winlist_t *win, int x, int y, int x1, int y1, int color_idx, int style);

extern unsigned long gui_Color(int index);
extern void gui_Draw_Image(winlist_t *win, int x, int y, char **xpm_data);
/* 送出模擬按鍵到指定的視窗 */
extern void gui_send_key(Window win, int state, KeySym keysym);
/* 取得 Focus Window (None : 沒有) */
extern Window gui_get_input_focus(void);
/* 取得 LED 狀態 */
extern int gui_get_led_state(KeySym keysym);

extern Cursor gui_cursor_hand;
extern Cursor gui_cursor_move;
extern Cursor gui_cursor_arrow;
extern int gui_menu_actived(void);
extern void gui_show_menu(winlist_t *parent_win);
extern void gui_hide_menu(void);

/* 切換 XCIN 輸入風格 */
extern void gui_xcin_style_switch(void);

extern void gui_switch_symbol(void);
extern int gui_symbol_actived(void);
extern int gui_show_symbol(void);
extern int gui_hide_symbol(void);

extern void gui_switch_keyboard(void);
extern int gui_keyboard_actived(void);
extern void gui_show_keyboard(void);
extern void gui_hide_keyboard(void);
#endif
