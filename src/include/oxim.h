/*
    Copyright (C) 1999 by  XCIN TEAM

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


#ifndef  _OXIM_H
#define  _OXIM_H

#include <X11/Xlib.h>
#include "gui.h"
#include "IC.h"

/*
 *  Flags for oxim_mode (for the global setting).
 */
#define OXIM_MODE_HIDE          0x00000001      /* hide oxim */
#define OXIM_SHOW_CINPUT        0x00000002      /* sinmd is set to be default */
#define OXIM_XKILL_OFF          0x00000004      /* disable xkill */
#define OXIM_IM_FOCUS           0x00000008      /* IM focus on */
#define OXIM_ICCHECK_OFF	0x00000010	/* disable IC check */
#define OXIM_SINGLE_IMC		0x00000020	/* single inpinfo for all IC */
#define OXIM_KEEP_POSITION	0x00000040	/* keep position enable */
#define OXIM_NO_WM_CTRL		0x00000080	/* disable WM contral */
#define OXIM_OVERSPOT_USRCOLOR	0x00000100	/* use client color setting */
#define OXIM_OVERSPOT_FONTSET	0x00000200	/* use user specified fontset */
#define OXIM_MAINWIN2		0x00000400	/* start mainwin 2 */
#define OXIM_OVERSPOT_WINONLY	0x00001000	/* only start OverSpot window */
#define OXIM_KEYBOARD_TRANS	0x00002000	/* translation keyboard layout*/
#define OXIM_RUN_IM_FOCUS       0x00100000      /* run time IM focus on */
#define OXIM_RUN_2B_FOCUS	0x00200000	/* run time 2B focus on */
#define OXIM_RUN_EXIT		0x00400000	/* oxim is now exiting */
#define OXIM_RUN_KILL		0x00800000	/* oxim gets killed & exiting */
#define OXIM_RUN_EXITALL	0x01000000	/* XIM terminated, exiting */
#define OXIM_RUN_INIT		0x02000000	/* oxim start to run */

/*
 *  OXIM core configuration.
 */
typedef struct {
    char *lc_ctype;
    Display *display;
    Window window;
    int virtual_keyboard; /* 自動虛擬鍵盤模式 */
    int keyboard_autodock; /* 自動虛擬鍵盤模式 */
    int hide_tray;

    /* XIM & Input Method configuration. */
    IC *ic, *icp;
    xmode_t oxim_mode;
    inp_state_t default_im;
    inp_state_t default_im_sinmd;
    inp_state_t im_focus;
    XIMStyles input_styles;

    /* Initialization structer. */
    char display_name[256];
    char xim_name[64];
    char keyboard_name[128];
    char input_method[64];
} xccore_t;

#endif
