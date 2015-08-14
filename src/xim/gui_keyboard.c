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
#include <signal.h>
#include "oximtool.h"
#include "gui.h"
#include "oxim.h"
#include "module.h"

#define X_LEADING       2
#define Y_LEADING       2
#define BUFLEN		1024

/*-------------------------------------------------------------------*/
enum point_type {
        POINT_NONE=0, /* 不在任何控制區內 */
        POINT_TITLE,  /* 游標在標題列 */
        POINT_CLOSE,  /* 游標在關閉按鈕 */
        POINT_I_SWITCH,  /* 游標在 I-switch */
        POINT_S_SWITCH,  /* 游標在 S-switch */
        POINT_H_SWITCH,  /* 游標在 H-switch */
	POINT_KEY,     /* 按鍵 */
	
	POINT_ON_LEFTKEY,
	POINT_ON_RIGHTKEY,
	POINT_ON_SELKEY,
};

typedef struct {
        int type;
        int index;
} point_t;

typedef struct {
	KeyCode  code;
	KeySym   sym;
	char     ascii;      /* 按鍵值 */
	int	 etymon_idx; /* 字根索引 */
	XSegment segment;    /* 按鈕座標 */
	unsigned int pressed;
	unsigned int delay;
	char keyname[64];
} btn_t;

typedef struct {
	XSegment title;  /* 標題列座標 */
	XSegment close;  /* 關閉按鈕的座標 */
	XSegment I_switch;  /* I-switch */
	XSegment S_switch;  /* S-switch */
	XSegment H_switch;  /* H-switch */
	unsigned int modifiter;
	unsigned int NumberOfKey;
	btn_t **btns;
} key_map;

#define SELKEY_IDX 10
typedef struct {
    XSegment keys[SELKEY_IDX];
    XSegment left, right;
} sel_keys;
static sel_keys selkeys;
static int current_width = 0;
static int is_iqqi = False;

static void draw_funkeys(void);

/*-------------------------------------------------------------------*/
static int keyboard_actived = False;
static key_map keymap;
static uch_t *save_etymon_list = NULL;
/*-------------------------------------------------------------------*/

im_t * GetCurrentIM(IC *ic)
{
    if(!ic)
	return NULL;

    IM_Context_t *imc = ic->imc;
    if ((imc->inp_state & IM_CINPUT))
    {
	im_t *im = oxim_get_IMByIndex(imc->inp_num);
        if (im)
	    return im;
    }
    return NULL;
}

/*
* an overloading funciton 'alarm'
* for advance assign micro-second
*/
/*static */unsigned int
call_alarm (unsigned int seconds, unsigned int mseconds)
{
    struct itimerval old, new;
    new.it_interval.tv_usec = 0;
    new.it_interval.tv_sec = 0;
    new.it_value.tv_usec = (long int) mseconds;
    new.it_value.tv_sec = (long int) seconds;
    if (setitimer (ITIMER_REAL, &new, &old) < 0)
	return 0;
    else
	return old.it_value.tv_sec;
}

int
IsIqqi(void)
{
    return is_iqqi;
}

static int
SearchButton(char *key)
{
    int i;
    
    for(i=0; i<keymap.NumberOfKey; i++)
    {
	if( 0 == strcasecmp(key, keymap.btns[i]->keyname) )
	    return i;
    }
    return 0;
}

static int ConvertSegment(char *value, XSegment *seg)
{
    char cmd[BUFLEN];
    int idx = 0;
    int valid = True;

    bzero(seg, sizeof(XSegment));
    while(valid && oxim_get_word(&value, cmd, BUFLEN, ","))
    {
	if (cmd[0] != ',' && idx < 4)
	{
	    switch (idx)
	    {
		case 0: /* x1 */
		    seg->x1 = atoi(cmd);
		    break;
		case 1: /* y1 */
		    seg->y1 = atoi(cmd);
		    break;
		case 2: /* x2 */
		    seg->x2 = atoi(cmd);
		    break;
		case 3: /* y2 */
		    seg->y2 = atoi(cmd);
		    break;
	    }
	    idx ++;
	}

    }

    if (seg->x1 > seg->x2 || seg->y1 > seg->y2)
	valid = False;

    if (seg->x1 == seg->y1 == seg->x2 == seg->y2)
	valid = False;

    return valid;
}

static int ParseSetting(char *s)
{
    winlist_t *win = gui->keyboard_win;
    char cmd[BUFLEN];
    int  idx = 0;
    int  isok = 0;
    XSegment seg;
    KeySym ksym;

//    keymap.modifier = 0;
    set_item_t *set_item = oxim_get_key_value(s);
    if (set_item)
    {
	if (strcasecmp(set_item->key, "font-size") == 0) /* 設定字型大小 */
	{
	    int size = atoi(set_item->value);
	    if (size != 0)
		win->font_size = size;
	    isok = 1;
	}
	else if (strcasecmp(set_item->key, "title-area") == 0) /* 標題區塊 */
	{
	    if ((isok = ConvertSegment(set_item->value, &seg)))
	    {
		keymap.title.x1 = seg.x1;
		keymap.title.y1 = seg.y1;
		keymap.title.x2 = seg.x2;
		keymap.title.y2 = seg.y2;
	    }
	}
	else if (strcasecmp(set_item->key, "close-button") == 0) /* 關閉按鈕區塊 */
	{
	    if ((isok = ConvertSegment(set_item->value, &seg)))
	    {
		keymap.close.x1 = seg.x1;
		keymap.close.y1 = seg.y1;
		keymap.close.x2 = seg.x2;
		keymap.close.y2 = seg.y2;
	    }
	}
	else if (strcasecmp(set_item->key, "I-switch") == 0) /* 切換輸入法按鈕區塊 */
	{
	    if ((isok = ConvertSegment(set_item->value, &seg)))
	    {
		keymap.I_switch.x1 = seg.x1;
		keymap.I_switch.y1 = seg.y1;
		keymap.I_switch.x2 = seg.x2;
		keymap.I_switch.y2 = seg.y2;
	    }
	}
	else if (strcasecmp(set_item->key, "S-switch") == 0) /* 補碼按鈕區塊 */
	{
	    if ((isok = ConvertSegment(set_item->value, &seg)))
	    {
		keymap.S_switch.x1 = seg.x1;
		keymap.S_switch.y1 = seg.y1;
		keymap.S_switch.x2 = seg.x2;
		keymap.S_switch.y2 = seg.y2;
	    }
	}
	else if (strcasecmp(set_item->key, "H-switch") == 0) /* 手寫按鈕區塊 */
	{
	    if ((isok = ConvertSegment(set_item->value, &seg)))
	    {
		keymap.H_switch.x1 = seg.x1;
		keymap.H_switch.y1 = seg.y1;
		keymap.H_switch.x2 = seg.x2;
		keymap.H_switch.y2 = seg.y2;
	    }
	}
	else if ((ksym = searchKeySymByName(set_item->key)) != 0) /* 按鍵區塊 */
	{
	    /* 
	     * ksym=0:  字串定義
	     * ksym!=0: 按鍵定義
	     */
	    ksym = searchKeySymByName(set_item->key);
	    btn_t *btn = oxim_malloc(sizeof(btn_t), True);
	    /* 轉換座標 */
	    isok = ConvertSegment(set_item->value, &seg);
	    if (!isok)
	    {
		free(btn);
	    }
	    else
	    {
		keymap.NumberOfKey ++;
		if (keymap.NumberOfKey == 1)
		{
		    keymap.btns = (btn_t **)oxim_malloc(sizeof(btn_t *), False);
		}
		else
		{
		    keymap.btns = (btn_t **)oxim_realloc(keymap.btns, sizeof(btn_t *) * keymap.NumberOfKey);
		}
		keymap.btns[keymap.NumberOfKey-1] = btn;
		btn->sym = ksym;
		btn->code = XKeysymToKeycode(gui->display, ksym);
		btn->segment.x1 = seg.x1;
		btn->segment.y1 = seg.y1;
		btn->segment.x2 = seg.x2;
		btn->segment.y2 = seg.y2;
		btn->pressed = 0;
		btn->delay = 0;
		strcpy(btn->keyname, set_item->key);

		if (strlen(set_item->key) == 1)
		{
		    btn->ascii = set_item->key[0];
		    btn->etymon_idx = oxim_key2code(set_item->key[0]);
		}
	    }
	}
	else
	{
	    printf(N_("Unknown tag : '%s'\n"), set_item->key);
	    isok = False;
	}
	oxim_key_value_destroy(set_item);
    }

    return isok;
}

/* 
 * let window manager know how to translate key/mouse input
 */
static 
void SetWindowInputHint(Window w)
{
    XWMHints wmhints;
    wmhints.flags = InputHint;
    wmhints.input = 0;
    XSetWMHints(gui->display, w, &wmhints);
}

/*
 * set window name property
 */
static 
void SetWindowName(Window w)
{
    XTextProperty window_name_property;
    char *window_name = "OXIMKeyboard";
    int rc = XStringListToTextProperty(&window_name,   
	    1, &window_name_property);  
  
    if (rc == 0)
    {
        oxim_perr(OXIMMSG_IERROR, "XStringListToTextProperty - out of memory\n");
        exit(1);
    }  
    XSetWMName(gui->display, w, &window_name_property);
}

/*
 * set position and size of window
 */
static 
void SetWindowSizeHint(winlist_t *win)
{
    XSizeHints size_hints;

    size_hints.x = win->pos_x;
    size_hints.y = win->pos_y;
    size_hints.width = win->width;
    size_hints.height = win->height;

    size_hints.flags = PPosition | PMaxSize | PMinSize | PSize;
    size_hints.min_width = size_hints.max_width = win->width;
    size_hints.min_height = size_hints.max_height = win->height;
    XSetWMNormalHints(gui->display, win->window, &size_hints);
}

/*
 * define the bound of dock window
 */
static 
void ChangeDockStructPartial(Window w, long *mapping)
{
    Atom atom = XInternAtom(gui->display, "_NET_WM_STRUT", False);
    XChangeProperty(gui->display, w, atom, XA_CARDINAL,
			32, PropModeReplace, (unsigned char *)mapping, 4);
    atom = XInternAtom(gui->display, "_NET_WM_STRUT_PARTIAL", False);
    XChangeProperty(gui->display, w, atom, XA_CARDINAL,
			32, PropModeReplace, (unsigned char *)mapping, 12);
}

/*
 * set attributes of dock window
 */
static 
void SetDockWindow(winlist_t *win, int x, int y, int w, int h)
{
    SetWindowName(win->window);
    SetWindowInputHint(win->window);

    /* place window on it's position */
    SetWindowSizeHint(win);

    long mapping[12] = {0};
    mapping[3] = gui->display_height  -  ((y+h) - win->height); /* bottom */
    mapping[4] = mapping[3];/* left_start_y */
    mapping[5] = gui->display_height;/* left_end_y */
    mapping[6] = mapping[4];/* right_start_y */
    mapping[7] = mapping[5];/* right_end_y */
    mapping[11] = gui->display_width;
    ChangeDockStructPartial(win->window, mapping);

    Atom type = XInternAtom(gui->display, "_NET_WM_WINDOW_TYPE", False);
    Atom at = XInternAtom(gui->display, "_NET_WM_WINDOW_TYPE_DOCK", False);
    XChangeProperty(gui->display, win->window, type, XA_ATOM, 32,
			    PropModeReplace, (unsigned char*)&at, 1);

    XSetWindowAttributes win_attr;
    win_attr.override_redirect = False;
    XChangeWindowAttributes(gui->display, win->window,
			CWOverrideRedirect, &win_attr);
}

static char prebgm[BUFLEN] = {'\0'};
static int KeyboardInit(int force_dock)
{
    winlist_t *win = gui->keyboard_win;
    xccore_t *xccore = (xccore_t *)win->data;
    char *buf = oxim_malloc(BUFLEN, False);
    char *true_fn = oxim_malloc(BUFLEN, False);
    char *sub_path = "panels";
    int ret = False;
    int need_redraw = True;
    if(force_dock)
	is_iqqi = False;
    im_t *im = GetCurrentIM(xccore->ic);
    /* 檢查並載入鍵盤圖片與設定檔 */
    int display_width = gui->display_width;
    int success = False;
    char *env_layout = xccore->keyboard_name;
    size_t env_layout_len = strlen(env_layout);
    char checkdatafile[env_layout_len+100];

    if(im)
    {
	sprintf(checkdatafile, "%s(%s)-%d.xpm", env_layout, im->objname, display_width);
	success = oxim_check_datafile(checkdatafile, sub_path, true_fn, 256);
    }
    if(!success)
    {
	*checkdatafile = '\0';
	sprintf(checkdatafile, "%s-%d.xpm", env_layout, display_width);
	success = oxim_check_datafile(checkdatafile, sub_path, true_fn, 256);
    }
    if(!success)
    {
	if(im)
	{
	    *checkdatafile = '\0';
	    sprintf(checkdatafile, "%s(%s).xpm", env_layout, im->objname);
	    success = oxim_check_datafile(checkdatafile, sub_path, true_fn, 256);
	}
    }
    if(!success)
    {
	*checkdatafile = '\0';
	sprintf(checkdatafile, "%s.xpm", env_layout);
	success = oxim_check_datafile(checkdatafile, sub_path, true_fn, 256);
    }
    if(!success)
    {
	strcpy(checkdatafile, "default.xpm");
	success = oxim_check_datafile(checkdatafile, sub_path, true_fn, 256);
    }

    if(success)
    {
	need_redraw = 0!=strcmp(prebgm, checkdatafile);
	/* 讀進鍵盤圖片 */
	Pixmap keyboard, mask;
	XpmAttributes attributes;
	attributes.valuemask = 0;
	int status;
	status =XpmReadFileToPixmap(gui->display, win->window, true_fn,
				&keyboard, &mask, &attributes);
	/* 圖形載入成功 */
	if (status == 0 && need_redraw)
 	{
	    strcpy(prebgm, checkdatafile);
	    /* 設定長寬 */
	    win->width  = attributes.width;
	    win->height = attributes.height;

	    /* 設定鍵盤為視窗背景 */
	    XSetWindowBackgroundPixmap(gui->display, win->window, keyboard);
	    if (mask)
		XShapeCombineMask(gui->display, win->window,
			ShapeBounding, 0, 0, mask, ShapeSet);

	    /* 重設視窗大小 */
	    int x, y, w, h;
	    gui_get_workarea(&x, &y, &w, &h);
	    
	    current_width = gui->display_width;
	    if(force_dock)
	    XMoveResizeWindow(gui->display, win->window,
				gui->display_width/2 - win->width/2, 
				(y+h) - win->height, 
				win->width, 
				win->height
			    );
	    win->pos_x = gui->display_width/2 - win->width/2;
	    win->pos_y = (y+h) - win->height;
	    
	    /* Dock window */
	    if(xccore->keyboard_autodock && force_dock)
		SetDockWindow(win, x, y, w, h);

	    /* 釋放記憶體 */
	    XpmFreeAttributes(&attributes);
	    XFreePixmap(gui->display, keyboard);
	    if (mask)
	    {
		XFreePixmap(gui->display, mask);
	    }
	    ret = True;
	}
    }
    if(!need_redraw)
	return True;

    success = False;
    /* 圖片載入成功的話，繼續處理設定檔 */
    if(im)
    {
	*checkdatafile = '\0';
	sprintf(checkdatafile, "%s(%s)-%d.conf", env_layout, im->objname, display_width);
	is_iqqi = success = oxim_check_datafile(checkdatafile, sub_path, true_fn, 256);
    }
    if(!success)
    {
	*checkdatafile = '\0';
	sprintf(checkdatafile, "%s-%d.conf", env_layout, display_width);
	is_iqqi = success = oxim_check_datafile(checkdatafile, sub_path, true_fn, 256);
    }
    if(!success)
    {
	if(im)
	{
	    *checkdatafile = '\0';
	    sprintf(checkdatafile, "%s(%s).conf", env_layout, im->objname);
	    is_iqqi = success = oxim_check_datafile(checkdatafile, sub_path, true_fn, 256);
	}
    }
    if(!success)
    {
	*checkdatafile = '\0';
	sprintf(checkdatafile, "%s.conf", env_layout);
	is_iqqi = success = oxim_check_datafile(checkdatafile, sub_path, true_fn, 256);
	if(0 == strcasecmp(env_layout, "default"))
	    is_iqqi = False;
    }
    if(!success)
	success = oxim_check_datafile("default.conf", sub_path, true_fn, 256);
//puts(true_fn);
    if(success)
    {
     /*if (ret && oxim_check_datafile("defaultkeyboard.conf", sub_path, true_fn, 256) == True)
     {*/
	bzero(&keymap, sizeof(key_map));
	
	int line_no = 0;
	gzFile *fp = NULL;
	fp = oxim_open_file(true_fn, "r", OXIMMSG_WARNING);
	if (fp)
	{
	    /* 讀入設定檔並分析 */
	    while (oxim_get_line(buf, BUFLEN, fp, &line_no, "#\r\n"))
	    {
		if (!ParseSetting(buf))
		{
		    printf(N_("%s line %d invalid !\n"), true_fn, line_no, buf);
		}
	    }
	    gzclose(fp); /* 關閉檔案 */
	}
	else
	{
	    ret = False;
	}
    }
    else
    {
	ret = False;
    }
    free(true_fn);
    free(buf);
    
    return ret;
}

int gui_keyboard_actived(void)
{
    return keyboard_actived;
}

void gui_switch_keyboard(void)
{
    if (keyboard_actived)
	gui_hide_keyboard();
    else
	gui_show_keyboard();
}

void gui_hide_keyboard(void)
{
//    keyboard_actived = False;
    int i;
    gui_show_status();

    gui_winmap_change(gui->keyboard_win, 0);

    //int ret = XUnmapWindow(gui->display, gui->keyboard_win->window);    
    keyboard_actived = False;

    if (keymap.btns)
    {
	for (i = 0 ; i < keymap.NumberOfKey; i++)
	{
	    free(keymap.btns[i]);
	}
	free(keymap.btns);
	keymap.btns = NULL;
    }
    gui_preedit_draw(gui->preedit_win);
    //alarm(0);
    winlist_t *win = gui->keyboard_win;
    xccore_t *xccore = (xccore_t *)win->data;
    if(xccore->virtual_keyboard)
    {
	if(xccore->ic)
	    change_IM(xccore->ic, -1);
    }
}

/*
 *  for gui_show_keyboard()
 */
static 
void OpenIM(xccore_t *xccore)
{
    if(xccore->ic)
    {
	int n_IM = oxim_get_NumberOfIM();
	IM_Context_t *imc = xccore->ic->imc;
	inp_state_t inp_state = imc->inp_state;
	//int inp_num = imc->inp_num;
	int idx;
	
	if (inp_state & IM_CINPUT)
	    return;
	
	for (idx = 0 ; idx < n_IM ; idx ++)
	{
	    im_t *imp = oxim_get_IMByIndex(idx);
	    if(0==strcmp(xccore->input_method, imp->objname))
	    {
		xccore->ic->imc->inp_num = idx;
		xim_connect(xccore->ic);
		change_IM(xccore->ic, xccore->ic->imc->inp_num);
		break;
	    }
	}
    }
}

void gui_show_keyboard(void)
{
    gui_uncheck_tray();
    
    gui_hide_status();
    gui_winmap_change(gui->select_win, 0);
    if (keyboard_actived)
	return;

    winlist_t *win = gui->keyboard_win;
#ifdef ENABLE_DEVICE
    // only used for tablet pc 
    (void)system("/usr/bin/tegaki-oxim -d");
#endif
    xccore_t *xccore = win->data;
    if(xccore->keyboard_autodock) 
    {
	long mapping[12] = {0};
	ChangeDockStructPartial(win->window, mapping);
    }
    *prebgm = '\0';
    OpenIM(win->data);
    bzero(&keymap, sizeof(key_map));
    if (!KeyboardInit(True))
    {
	printf(_("Read keyboard setting error!!\n"));
	return;
    }

    gui_winmap_change(win, 1);
    keyboard_actived = True;
    //alarm(0);
}

#define IF_CHECKPOINT(m_x, m_y, x1, y1, x2, y2)	if( (m_x) > (x1) && (m_x) < (x2) && (m_y) > (y1) && (m_y) < (y2) )
static point_t CheckPoint(int x, int y)
{
    winlist_t *win = gui->symbol_win;
    xccore_t *xccore = win->data;
    btn_t **btns = keymap.btns;
    point_t pt;

    pt.type = POINT_NONE;

// TODO: same function from gui_preedit.c
// they need to merge to one function.
{
    unsigned int i;

    /* on selection key? */
    for(i=0; i<SELKEY_IDX; i++)
    {
	IF_CHECKPOINT( x, y,
		       selkeys.keys[i].x1, selkeys.keys[i].y1,
		       selkeys.keys[i].x2, selkeys.keys[i].y2 )
	{
	    pt.type = POINT_ON_SELKEY;
	    pt.index = i;
	    return pt;
	}
    }

    /* on left or right arrow ? */
    IF_CHECKPOINT( x, y, 
		   selkeys.left.x1, selkeys.left.y1, 
		   selkeys.left.x2, selkeys.left.y2 )
    {
	pt.type = POINT_ON_LEFTKEY;
	return pt;
    }

    IF_CHECKPOINT( x, y, 
		   selkeys.right.x1, selkeys.right.y1, 
		   selkeys.right.x2, selkeys.right.y2 )
    {
	pt.type = POINT_ON_RIGHTKEY;
	return pt;
    }
}

    /* 是否在 Title bar */
    if (x > keymap.title.x1 && x < keymap.title.x2 &&
	y > keymap.title.y1 && y < keymap.title.y2 &&
	!xccore->keyboard_autodock)
    {
	pt.type = POINT_TITLE;
	return pt;
    }
    /* 是否在關閉按鈕 ? */
    if (x > keymap.close.x1 && x < keymap.close.x2 &&
	y > keymap.close.y1 && y < keymap.close.y2)
    {
	pt.type = POINT_CLOSE;
	return pt;
    }
    /* 是否在 I-switch ? */
    if (x > keymap.I_switch.x1 && x < keymap.I_switch.x2 &&
	y > keymap.I_switch.y1 && y < keymap.I_switch.y2)
    {
	pt.type = POINT_I_SWITCH;
	return pt;
    }
    /* 是否在 S-switch ? */
    if (x > keymap.S_switch.x1 && x < keymap.S_switch.x2 &&
	y > keymap.S_switch.y1 && y < keymap.S_switch.y2)
    {
	pt.type = POINT_S_SWITCH;
	return pt;
    }
    /* 是否在 H-switch ? */
    if (x > keymap.H_switch.x1 && x < keymap.H_switch.x2 &&
	y > keymap.H_switch.y1 && y < keymap.H_switch.y2)
    {
	pt.type = POINT_H_SWITCH;
	return pt;
    }

    /* 是否在按鈕上 ? */
    int i;
    for (i = 0 ; i < keymap.NumberOfKey ; i++)
    {
	if (x >= btns[i]->segment.x1 && x <= btns[i]->segment.x2 &&
	    y >= btns[i]->segment.y1 && y <= btns[i]->segment.y2)
	{
	    pt.type = POINT_KEY;
	    pt.index = i;
	    return pt;
	}
    }

    return pt;
}


static void
draw_multich_onbox(IC *ic, int title_width, int preview)
{
    int i, j, n_groups, n, x2, len=0, toggle_flag;
    uch_t *selkey, *cch;
    
    inpinfo_t inpinfo = ic->imc->inpinfo;
    winlist_t *select_win = gui->keyboard_win;

    int n_selkey = inpinfo.n_selkey;
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

    /*int fontheight = select_win->font_size + 2;*/
    int fontheight = keymap.title.y2 - keymap.title.y1 - 3;
    int fontwidth  = select_win->font_size + 2;
    int selectbox_height = (n_items * fontheight) + (Y_LEADING * 6);
    int selectbox_width = item_width - (X_LEADING * 2);

    int width = item_width;
    int height = selectbox_height + (Y_LEADING * 2) + (!preview ? fontheight : 0) + ((inpinfo.mcch_pgstate && !preview) ? fontheight : 0);

    int x = X_LEADING;
    int y = select_win->font_size + 2;
    y = 0;
    y += (Y_LEADING * 2);
    int key_x = (toggle_flag) ? (X_LEADING*4) : (item_width - (key_max_w + X_LEADING + str_max_w))/2;

    if(title_width>0)
	title_width += 10;
    /* draw lines */
    Window w = select_win->window;
    GC darkGC  = XCreateGC(gui->display, w, 0, NULL);

    XSetForeground(gui->display, darkGC, gui_Color(FONT_COLOR));
    XSetLineAttributes(gui->display, darkGC, 
			2, LineSolid, CapRound, JoinRound);
    int tx1 = keymap.title.x1 + title_width;
    int tx2 = tx1;
    int ty1 = keymap.title.y1;
    int ty2 = keymap.title.y2;
    //XDrawLine(gui->display, w, darkGC, tx1, ty1, tx2, ty2);


    int avail_width = keymap.title.x2 - keymap.title.x1 - title_width;
    //int arrow_width = avail_width/(SELKEY_IDX)/2;
    int arrow_width = avail_width/(n_selkey+2)/2;
    //int seg_width = (avail_width - arrow_width*2)/SELKEY_IDX;
    int seg_width = (avail_width - arrow_width*2)/n_selkey;
    //x = key_x + X_LEADING + fontwidth + title_width;
    //x = fontwidth + title_width;
    //x = title_width + seg_width/2 - fontwidth/2;
    int draw_width = 0;
    //for (i=0; i<(SELKEY_IDX+1); i++)
    for (i=0; i<(n_selkey+1); i++)
    {
	int box_width = (0==i) ? arrow_width : seg_width;
	draw_width += box_width;
	
	int dx = title_width + draw_width/* * (i+1) */+ keymap.title.x1;
	XDrawLine(gui->display, w, darkGC, dx, ty1, dx, ty2);
    }
    draw_width = arrow_width;
    for (i=0; i<n_groups && toggle_flag!=-1; i++, selkey++) {
	n = (toggle_flag > 0) ? inpinfo.mcch_grouping[i+1] : 1;
        y = fontheight;
	
	//int box_width = (0==i) ? arrow_width : seg_width;
	int box_width = seg_width;
	draw_width += box_width;
	int dx = title_width + draw_width + keymap.title.x1;
	
	//x = dx - seg_width/2 - fontwidth/2*;
	x = dx - box_width + 10 - 5;
	selkeys.keys[i].x1 = x-10;
	//selkeys.keys[i].y1 = y - fontheight;
	selkeys.keys[i].y1 = keymap.title.y1;
        for (j=0; j<n; j++, cch++) {
	    len = strlen((char *)cch->s);
	    if (cch->uch == (uchar_t)0) {
		toggle_flag = -1;
		break;
	    }
	    x2 = x + gui_Draw_String(select_win, LIGHT_COLOR, NO_COLOR, x, y, (char *)cch->s, len);
	    x = x2;
        }
	selkeys.keys[i].x2 = selkeys.keys[i].x1+box_width;
	selkeys.keys[i].y2 = keymap.title.y2 ;
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
	char *right_ar = "⇨";
	//int left_ar_w  = gui_TextEscapement(select_win, left_ar, strlen(right_ar));
	int right_ar_w = gui_TextEscapement(select_win, right_ar, strlen(right_ar));
	//y += fontheight + (Y_LEADING * 2);
	int left_x = title_width /*+ arrow_width*/ + fontwidth/2 + keymap.title.x1;
	//int right_x = title_width + avail_width /*- seg_width*/ + keymap.title.x1;
	//int right_x = left_x + seg_width*10 + arrow_width - fontwidth/2;
	int right_x = left_x + seg_width*n_selkey + arrow_width - fontwidth/2;

	gui_Draw_String(select_win, LIGHT_COLOR, NO_COLOR, X_LEADING+1 +left_x, y+1, left_ar, strlen(left_ar));
	gui_Draw_String(select_win, LIGHT_COLOR, NO_COLOR, /*width -*/ right_ar_w/**1.5*/ - X_LEADING + 1 + right_x, y+1, right_ar, strlen(right_ar));
	int left_color = !(pgstate & LEFT_AR) ? INPUTNAME_COLOR : DARK_COLOR;
	int left_width = gui_Draw_String(select_win, left_color, NO_COLOR, X_LEADING +left_x, y, left_ar, strlen(left_ar));
	int right_color = !(pgstate & RIGHT_AR) ? INPUTNAME_COLOR : DARK_COLOR;
	int right_width = gui_Draw_String(select_win, right_color, NO_COLOR, /*width -*/ right_ar_w/**1.5*/ - X_LEADING + right_x, y, right_ar, strlen(right_ar));
	
	selkeys.left.x1 = X_LEADING + left_x - seg_width/2 + fontwidth/2;
	selkeys.left.y1 = y - fontheight + (Y_LEADING * 2);
	selkeys.left.x2 = X_LEADING + left_width + left_x + seg_width/2 + fontwidth/2;
	selkeys.left.y2 = y;
	
	//selkeys.right.x1 = width - right_ar_w - X_LEADING + right_x  - seg_width/2 + fontwidth/2;
	selkeys.right.x1 = right_x;
	selkeys.right.y1 = y - fontheight + (Y_LEADING * 2);
	selkeys.right.x2 = keymap.title.x2;
	selkeys.right.y2 = y;
    }
}

static void gui_keyboard_set(uch_t *etymon_list, int force)
{
//        bzero(&keymap, sizeof(key_map));

    if (!KeyboardInit(False))
    {
	printf(_("Read keyboard setting error!!\n"));
	return;
    }
    
    winlist_t *win = gui->keyboard_win;
    xccore_t *xccore = (xccore_t *)win->data;
    IC *ic = xccore->ic;
    
    btn_t **btns = keymap.btns;

    //char title[128];
//     strcpy(title,  _("Open X Input Method Mini Keyboard"));
    //title[0] = '\0';
    
    /* empty */
    unsigned int i;
    for(i=0; i<SELKEY_IDX; i++)
    {
	selkeys.keys[i].x1 = selkeys.keys[i].y1 = 
	selkeys.keys[i].x2 = selkeys.keys[i].y2 = 0;
    }
    selkeys.left.x1 = selkeys.left.y1 =
    selkeys.left.x2 = selkeys.left.y2 =
    selkeys.right.x1 = selkeys.right.y1 =
    selkeys.right.x2 = selkeys.right.y2 = 0;

    if (ic)
    {
	IM_Context_t *imc = ic->imc;
	inpinfo_t inpinfo = imc->inpinfo;
	//inp_state_t inp_state = imc->inp_state;
	//int inp_num = imc->inp_num;

	im_t *imp = GetCurrentIM(ic);
	//if(inp_state & IM_CINPUT)
	if(imp)
	{
	    char title[128];
	    //im_t *imp = oxim_get_IMByIndex(inp_num);
	    //if (imp && (!xccore->virtual_keyboard && !(IsIqqi() && gui_keyboard_actived())))
	    if(!((xccore->virtual_keyboard || (IsIqqi() && gui_keyboard_actived())) && inpinfo.n_mcch))
	    {
		strcpy(title, 
			(imp->aliasname ? 
			    imp->aliasname : imp->inpname)
		    );

	    }
	    else
		title[0] = '\0';

	    XClearArea(gui->display, win->window, 
		keymap.title.x1, keymap.title.y1,
		keymap.title.x2-keymap.title.x1,
		keymap.title.y2-keymap.title.y1+3,
		0);
		
	    int title_width = 0;
	    int fontheight = keymap.title.y2 - keymap.title.y1 - 6;

	    if(win->font_size > fontheight)
		fontheight = keymap.title.y2-3;
	    if(title)
		title_width = gui_Draw_String(win, LIGHT_COLOR, NO_COLOR, 
						    keymap.title.x1+3, 
						    //keymap.title.y2-3 -7, 
						    fontheight,
						    title, strlen(title)
						    );

	    if (xccore->virtual_keyboard || (IsIqqi() && gui_keyboard_actived()))
	    {
		int preview = (inpinfo.guimode & GUIMOD_SELKEYSPOT) ? 
			    False : True;
		draw_multich_onbox(ic, title_width, preview);
	    }
	}
    }

    if (etymon_list && etymon_list == save_etymon_list && !force)
    {
	return;
    }
    
    if(ic && !(ic->imc->inp_state & IM_CINPUT))
	XClearWindow(gui->display, win->window); /* 清除字根 */
    else
	if (etymon_list)
	{
	    int i, len, etymon_w, x, y;
	    uch_t etymon;
	    for (i=0; i < keymap.NumberOfKey ; i++)
	    {
		if(!btns)
		{
		    break;
		}

		/* 清除字根 */
		XClearArea(gui->display, win->window, 
		    btns[i]->segment.x1, btns[i]->segment.y1,
		    btns[i]->segment.x2-btns[i]->segment.x1,
		    btns[i]->segment.y2-btns[i]->segment.y1,
		    0);

		if (btns[i]->ascii != 0)
		{
		    etymon = etymon_list[btns[i]->etymon_idx];
		    len = strlen((char *)etymon.s);
		    if (len)
		    {
			etymon_w = gui_TextEscapement(win, (char *)etymon.s, len);
			x = btns[i]->segment.x2 - etymon_w - 1 -4;
			y = btns[i]->segment.y2 - 3 -2;
			gui_Draw_String(win, FONT_COLOR, NO_COLOR, x+1, y, (char *)etymon.s, len);
			gui_Draw_String(win, FONT_COLOR, NO_COLOR, x, y, (char *)etymon.s, len);
    // 		    gui_Draw_String(win, LIGHT_COLOR, NO_COLOR, x, y, (char *)etymon.s, len);
		    }
		}
	    }
	}
    save_etymon_list = etymon_list;
}

// a singal funciton for mouse-forcus event 
// to redraw the draw context
static void 
gui_keyboard_refresh(int sig)
{
    winlist_t *win = gui->keyboard_win;
    xccore_t *xccore = (xccore_t *)win->data;
    IC *ic = xccore->ic;
    inp_state_t inp_state = (ic) ? ic->imc->inp_state : 0;
    
    if (!keyboard_actived || (win->winmode & WMODE_EXIT)/* || !ic*/)
	return;
/*if (!keyboard_actived)
	puts("!keyboard_actived");
if(win->winmode & WMODE_EXIT)
	puts("win->winmode & WMODE_EXIT");
if(!ic)
	puts("!ic");*/
    if (!(inp_state & IM_CINPUT))
    {
	gui_keyboard_set(NULL, True);
}
    else
	gui_keyboard_set(ic->imc->inpinfo.etymon_list, True);
    draw_funkeys();
}

static void 
draw_key(btn_t *btn)
{
    winlist_t *win = gui->keyboard_win;
    GC gc = XCreateGC(gui->display, win->window, 0, NULL);
    int width = btn->segment.x2 - btn->segment.x1;
    int height = btn->segment.y2 - btn->segment.y1;
    int x = btn->segment.x1;
    int y = btn->segment.y1;

    /*XSetForeground(gui->display, gc, 0xCF2C2C);
    XSetPlaneMask(gui->display, gc, 0x555555);*/
    XSetForeground(gui->display, gc, 20);
    XSetPlaneMask(gui->display, gc, 255);
    
    XFillRectangle(gui->display, win->window, gc, x, y, width, height);
    XFreeGC(gui->display, gc);
}

static void 
draw_funkeys()
{
    int i;

    // draw focus of key
//     printf("%d, %d\n", keymap.modifiter&ShiftMask, keymap.modifiter&ControlMask);
    for(i=0; i<keymap.NumberOfKey; i++)
    {
	if( keymap.btns[i]->delay )
	{
	    if( keymap.modifiter || gui_get_led_state(XK_Caps_Lock) )
	    {
		draw_key(keymap.btns[i]);
	    }
	    else
		keymap.btns[i]->delay = False;
	}
    }
}

static void 
draw_keys()
{
    int i;

    // draw focus of key
    for(i=0; i<keymap.NumberOfKey; i++)
    {
	if( keymap.btns[i]->pressed>0 )
	{
	    draw_key(keymap.btns[i]);
	    keymap.btns[i]->pressed = 0;
	}
    }

/*    alarm(0);
    signal(SIGALRM, gui_keyboard_refresh);
    call_alarm(0, 200000);*/
}

void SendIMSelected(xccore_t *xccore, Window w, int cp)
{
    IC *ic = xccore->ic;
    inpinfo_t inpinfo = ic->imc->inpinfo;
    uch_t *cch = inpinfo.mcch;

    // list: preview mode
    if(ic->imc->inpinfo.guimode & GUIMOD_SELKEYSPOT_EX)
    {
	char s[2];
	int sel_idx = ++cp==SELKEY_IDX ? 0 : cp;
	sprintf(s, "%d", sel_idx);
	gui_send_key( w, ShiftMask, XStringToKeysym(s) );
    }
#if 1 
    else if((inpinfo.guimode & GUIMOD_SELKEYSPOT) || ((inpinfo.guimode & GUIMOD_LISTCHAR)/*&&xccore->virtual_keyboard*/))
    {	// list: not in preview mode
	char s[2];
	sprintf(s, "%d", ++cp==SELKEY_IDX ? 0 : cp);
	gui_send_key( w, 0, XStringToKeysym(s) );
    }
#endif
    else
    {
	unsigned int i;
	cp++;
	for(i=0; i<cp; i++, cch++)	;
	cch--;
	xim_commit_string_raw(ic, (char *)cch->s);
	gui_send_key( w, 0, XStringToKeysym("Escape") );
	inpinfo.mcch = '\0';
	gui_keyboard_set(NULL, False);
    }
}

static void
gui_keyboard_draw(winlist_t *win)
{
    xccore_t *xccore = (xccore_t *)win->data;
    IC *ic = xccore->ic;
    inp_state_t inp_state = (ic) ? ic->imc->inp_state : 0;

    if (!keyboard_actived || (win->winmode & WMODE_EXIT) || !ic)
	return;

    /* 清除字根 */
    if (!(inp_state & IM_CINPUT))
    {
	gui_keyboard_set(NULL, False);
    }
    else
    {
	gui_keyboard_set(ic->imc->inpinfo.etymon_list, False);
    }
}

#define FUNKEY_CHECK( a,b,c,d ) \
    if( ksym==(a) ) \
    { \
	key = SearchButton(c); \
	if( key && keymap.btns[key]->delay ) \
	    keymap.btns[key]->delay = False; \
    } \
    if( ksym==(b) ) \
    { \
	key = SearchButton(d); \
	if( key && keymap.btns[key]->delay ) \
	    keymap.btns[key]->delay = False; \
    } \
    btn->delay = True;

static
void ActionISwitch(IC *ic)
{
#ifdef ENABLE_DEVICE
    SwitchCtrlShift(0);	// for device
#else
    circular_change_IM(ic, 1);
#endif
    gui_winmap_change(gui->preedit_win, 0);
    gui_hide_status();

    //if (! (ic->imc->inp_state & IM_CINPUT))
	//xim_disconnect(ic);

    keymap.modifiter = 0;    
}

static 
void ActionSSwitch(point_t pt, IC *ic, Window focusw)
{
    int caps_lock = gui_get_led_state(XK_Caps_Lock);
    if(!caps_lock && POINT_I_SWITCH==pt.type)
    {
	draw_funkeys();
	return;
    }
	
    im_t *im = GetCurrentIM(ic);
    if (im && 0 == strcmp("iqqien", im->objname))
    {
	KeySym ksym = XK_Caps_Lock;
	XTestFakeKeyEvent(gui->display, XKeysymToKeycode(gui->display, ksym), True, CurrentTime);
	XTestFakeKeyEvent(gui->display, XKeysymToKeycode(gui->display, ksym), False, CurrentTime);
	btn_t *btn = keymap.btns[pt.index];
	btn->pressed++;

	btn->delay = True;
	draw_keys();
	return;
    }
    btn_t *btn = keymap.btns[pt.index];
    KeySym ksym = btn->sym;
    char ascii = btn->ascii;
    //int clear_modifiter = False;
    btn->pressed++;
    draw_keys();
    if( POINT_S_SWITCH == pt.type )
    {
	/*Window focusw = gui_get_input_focus();*/
	if (focusw!=None)
	    gui_send_key(focusw, 0, XK_Super_L);
    }
}

static
void ActionKey(Window focusw, point_t pt)
{
    if(None==focusw)
    {
	draw_funkeys();
	return;
    }
    
    /* 取得 LED 狀態 */
    int i, key;
    int capslock_and_shift = False;
    int caps_lock = gui_get_led_state(XK_Caps_Lock);
    btn_t *btn = keymap.btns[pt.index];
    KeySym ksym = btn->sym;
    char ascii = btn->ascii;
    int clear_modifiter = False;
    
    btn->pressed++;
    btn->delay = False;
    switch (ksym)
    {
	case XK_Shift_L:
	case XK_Shift_R:
	    FUNKEY_CHECK( XK_Shift_L, XK_Shift_R, "R-Shift", "L-Shift" );
	    if( keymap.modifiter & ShiftMask )
		keymap.modifiter ^= ShiftMask;
	    else
		keymap.modifiter |= ShiftMask;
	    break;
	case XK_Control_L:
	case XK_Control_R:
	    FUNKEY_CHECK( XK_Control_L, XK_Control_R, "R-Ctrl", "L-Ctrl" );
	    keymap.modifiter |= ControlMask;
	    break;
	case XK_Alt_L:
	case XK_Alt_R:
	    FUNKEY_CHECK( XK_Alt_L, XK_Alt_R, "R-Alt", "L-Alt" );
	    keymap.modifiter |= Mod1Mask;
	    break;
	case XK_Caps_Lock:
	    XTestFakeKeyEvent(gui->display, XKeysymToKeycode(gui->display, ksym), True, CurrentTime);
	    XTestFakeKeyEvent(gui->display, XKeysymToKeycode(gui->display, ksym), False, CurrentTime);

	    btn->delay = True;
	    draw_keys();
	    return;
	    break;
	default:
	{
	    clear_modifiter = True;
	    
	    if (caps_lock && keymap.modifiter & ShiftMask)
	    {
		//FUNKEY_CHECK( XK_Shift_L, XK_Shift_R, "R-Shift", "L-Shift" );
		//keymap.modifiter ^= ShiftMask;
		key = SearchButton("L-Shift"); 
		if( key ) 
		{
		    keymap.btns[key]->delay = 0;
		    keymap.modifiter = 0;
		    //keymap.modifiter |= ShiftMask;
		    capslock_and_shift = True;
		}
		clear_modifiter = True;
		//btn->delay = True;
		//draw_keys();
		//return;
	    }

	}
    }
    //else
    if (caps_lock && ascii >= 'a' && ascii <= 'z' && !capslock_and_shift)
	keymap.modifiter |= ShiftMask;
    
    draw_keys();
    gui_send_key(focusw, keymap.modifiter, ksym);
    if (clear_modifiter)
    {
	keymap.modifiter = 0;
    }
}

static
void OnKeyPress(winlist_t *win, XEvent *event)
{
    if (event->xbutton.button != Button1)
    {
	draw_funkeys();
	return;
    }
    
    xccore_t *xccore = (xccore_t *)win->data;
    IC *ic = xccore->ic;
    Window focusw = gui_get_input_focus();
    point_t pt = CheckPoint(event->xbutton.x, event->xbutton.y);
    switch (pt.type)
    {
	case POINT_ON_SELKEY:
	    SendIMSelected(xccore, focusw, pt.index);
	    break;
	case POINT_ON_LEFTKEY:
	    if (focusw!=None)
		gui_send_key( focusw, 0, XStringToKeysym("Left") );
	    break;
	case POINT_ON_RIGHTKEY:
	    if (focusw!=None)
		gui_send_key( focusw, 0, XStringToKeysym("Right") );
	    break;
	case POINT_TITLE:
	    if(!xccore->keyboard_autodock)
		gui_move_window(win);
	    break;
	case POINT_CLOSE:
	    gui_hide_keyboard();
	    //ChangeToEn_ChMode(0);	// test for device
	    return;
	    break;
	case POINT_I_SWITCH:
	    ActionISwitch(ic);
	    //break; 
	case POINT_S_SWITCH:
	    ActionSSwitch(pt, ic, focusw);
	    break;
	case POINT_H_SWITCH:
	    {
		gui_hide_keyboard();
		RunTegaki(0);
		return;
	    }
	    break;
	case POINT_KEY:
	    ActionKey(focusw, pt);
	    break;
    }
    draw_funkeys();
}

static void KeyboardEventProcess(winlist_t *win, XEvent *event)
{
    xccore_t *xccore = (xccore_t *)win->data;
    IC *ic = xccore->ic;
    inp_state_t inp_state = (ic) ? ic->imc->inp_state : 0;
    point_t pt;

    switch (event->type)
    {
	case Expose:
	    if (event->xexpose.count == 0)
	    {
		gui_reread_resolution(gui->root_win);
		if(current_width != gui->display_width)
		{
		    //gui_move_windows();
		    gui_hide_keyboard();
		    gui_show_keyboard();
		}
		if (!(inp_state & IM_CINPUT))
		    gui_keyboard_set(NULL, True);
		else
		    gui_keyboard_set(ic->imc->inpinfo.etymon_list, True);
	    }
	    break;
	case MotionNotify:
	    pt = CheckPoint(event->xbutton.x, event->xbutton.y);
	    switch (pt.type)
	    {
		case POINT_TITLE:
		    XDefineCursor(gui->display, win->window, gui_cursor_move);
		    break;
		case POINT_CLOSE:
		case POINT_I_SWITCH:
		case POINT_S_SWITCH:
		case POINT_H_SWITCH:
		case POINT_ON_LEFTKEY:
		case POINT_ON_RIGHTKEY:
		case POINT_ON_SELKEY:
		case POINT_KEY:
		    XDefineCursor(gui->display, win->window, gui_cursor_hand);
		    break;
		default:
		    XDefineCursor(gui->display, win->window, gui_cursor_arrow);
	    }
	    break;
	case ButtonRelease:
	    gui_keyboard_refresh(0);
	    break;
	case ButtonPress:
	    OnKeyPress(win, event);
	    break;
    }
}


void gui_keyboard_init(void)
{
    winlist_t *win = NULL;

    if (gui->keyboard_win)
    {
	win = gui->keyboard_win;
    }
    else
    {
	win = gui_new_win();
    }
    
    unsigned int font_size = atoi(oxim_get_config(StatusFontSize));
    
    win->pos_x = gui->display_width / 2;
    win->pos_y = gui->display_height / 2;
    win->width  = 1;
    win->height = 1;
    win->font_size = 18;

    win->win_draw_func  = gui_keyboard_draw;
    win->win_event_func = KeyboardEventProcess;
    XSetWindowAttributes win_attr;
    win_attr.override_redirect = True;
    XChangeWindowAttributes(gui->display, win->window,
			CWOverrideRedirect, &win_attr);
			
    gui->keyboard_win = win;
}
