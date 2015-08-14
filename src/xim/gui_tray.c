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
#include <X11/Xutil.h>
#include <X11/xpm.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include "oximtool.h"
#include "gui.h"
#include "oxim.h"

#ifdef ENABLE_EEEPC
#include "images/oxim_kbd.xpm"
#else
#include "images/oxim_tray.xpm"
#endif
static int tray_w, tray_h;

int check_tray_manager(void);

static int  tray_init = False;
//static char **icon = NULL;
static Atom selection_atom = None;
static Atom manager_atom = None;
static Atom system_tray_opcode_atom = None;
static Atom orientation_atom = None;
static Atom xembed_atom = None;
static Window manager_window = None;
static Window parent_window = None;
static unsigned int stamp = 0;

static XVisualInfo *visual = NULL;
static XImage *icon;
static Pixmap icon_mask, picon;


#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

static void send_message(
     Display* dpy, /* display */
     Window win,   /* sender (tray icon window) */
     long message, /* message opcode */
     long data1,   /* message data 1 */
     long data2,   /* message data 2 */
     long data3    /* message data 3 */
)

{
    XEvent ev;
  
//     bzero(&ev, sizeof(ev));
    memset (&ev, 0, sizeof (ev));
    ev.xclient.type = ClientMessage;
    ev.xclient.window = win;
    ev.xclient.message_type = system_tray_opcode_atom;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = CurrentTime;
    ev.xclient.data.l[1] = message;
    ev.xclient.data.l[2] = data1;
    ev.xclient.data.l[3] = data2;
    ev.xclient.data.l[4] = data3;

//    trap_errors();
    XSendEvent(dpy, manager_window, False, NoEventMask, &ev);
    XSync(dpy, False);
/*
    if (untrap_errors()) {
    }
*/
}

/* 傳送訊息 */
static void gui_send_message(winlist_t *win)
{
    unsigned int timeout = 30000; /* 3 秒 */
//     char *msg = _("OXIM Method"); /* message */ //"OXIM 輸入法"
    char *msg = "OXIM Method"; /* message */ //"OXIM 輸入法"
    int len = strlen(msg);

    stamp++;

    send_message(gui->display, /* display */
     		 manager_window,   /* sender (tray icon window) */
		 SYSTEM_TRAY_BEGIN_MESSAGE, /* message opcode */
		 timeout,   /* message data 1 */
		 len,   /* message data 2 */
		 stamp /* message data 3 */
		);
    while (len > 0)
    {
	XClientMessageEvent ev;
	ev.type = ClientMessage;
	ev.window = manager_window;
	ev.format = 8;
	ev.message_type = XInternAtom(gui->display,
			"_NET_SYSTEM_TRAY_MESSAGE_DATA", False);
	if (len > 20)
	{
	    memcpy(&ev.data, msg, 20);
	    len -= 20;
	    msg += 20;
	}
	else
	{
	    memcpy(&ev.data, msg, len);
	    len = 0;
	}
	XSendEvent(gui->display, manager_window, False, StructureNotifyMask, (XEvent *)&ev);
	XSync(gui->display, False);
    }
}

static void DrawTray(winlist_t *win)
{
    XpmAttributes attributes;
    Pixmap tray, mask, shapemask;
    GC shapegc, traygc;
    attributes.valuemask = 0;
    XpmCreatePixmapFromData(gui->display, win->window,
			oxim_tray, &tray, &mask, &attributes);

    tray_w = attributes.width;
    tray_h = attributes.height;

    shapemask = XCreatePixmap(gui->display, win->window,
			win->width , win->height , 1);

    traygc = XCreateGC(gui->display, tray, 0, 0);
    shapegc = XCreateGC(gui->display, shapemask, 0, 0);

    XFillRectangle(gui->display, shapemask, shapegc, 0, 0,
		win->width, win->height);

    int sw = (win->width - tray_w) / 2;
    int sh = (win->height - tray_h) / 2;
    int new_x = (sw > 0) ? sw : 0;
    int new_y = (sh > 0) ? sh : 0;

    if (mask)
    {
	XCopyArea(gui->display, mask, shapemask, shapegc, 0, 0,
		tray_w, tray_h, new_x, new_y);
	XFreePixmap(gui->display, mask);
    }
    else
    {
	XSetForeground(gui->display, shapegc, 1);
	XFillRectangle(gui->display, shapemask, shapegc, new_x, new_y,
		tray_w, tray_h);
    }

    XShapeCombineMask(gui->display, win->window, ShapeBounding,
		0, 0, shapemask, ShapeSet);
/*    XShapeCombineMask(gui->display, win->window, ShapeClip,
		0, 0, shapemask, ShapeSet);*/
    XCopyArea(gui->display, tray, win->window, traygc, 0, 0,
		win->width, win->height, new_x, new_y);

    XpmFreeAttributes(&attributes);
    XFreePixmap(gui->display, tray);
    XFreePixmap(gui->display, shapemask);
    XFreeGC(gui->display, shapegc);
    XFreeGC(gui->display, traygc);    
}

static void DrawTrayExtend(winlist_t *win)
{
    Pixmap tray, dummy;
    XpmAttributes attr;
    attr.valuemask = 0;
    XpmCreatePixmapFromData(gui->display, win->window,
			oxim_tray, &tray, &dummy, &attr);

    int tray_w = attr.width;
    int tray_h = attr.height;
    int w = win->width;
    int h = win->height;
    int x=0, y=0;
    int offset_y = h/2 - tray_h/2;
    
    Picture trayw, mask, icon;
    
    XWindowAttributes attrs;

    XGetWindowAttributes( gui->display, win->window , &attrs );
    XRenderPictFormat*  format = XRenderFindVisualFormat( gui->display, attrs.visual );

    XRenderPictFormat *pformat = XRenderFindStandardFormat( gui->display, PictStandardA1 );
    XRenderPictFormat *pformat2 = XRenderFindStandardFormat( gui->display, PictStandardRGB24 );

    XRenderPictureAttributes pattrs;

    trayw = XRenderCreatePicture(gui->display, win->window,
	    format, 0, NULL);

    mask = XRenderCreatePicture(gui->display, icon_mask,
	    pformat, 0, NULL);

    pattrs.alpha_map = mask;
    icon = XRenderCreatePicture(gui->display, picon,
	    pformat2, CPAlphaMap, &pattrs);

    XRenderComposite(gui->display, PictOpOver, icon, None, trayw, 
	    x,y-offset_y , x,y,x,y,w,h);
    XRenderFreePicture(gui->display,trayw);
    XRenderFreePicture(gui->display,mask);
    XRenderFreePicture(gui->display,icon);
    XFreePixmap(gui->display,tray);
    if(dummy)
	XFreePixmap(gui->display,dummy);
}

static void gui_tray_draw(winlist_t *win)
{
    if (win->winmode & WMODE_EXIT || !tray_init)
        return;
    if (visual && visual->visual)
	DrawTrayExtend(win);
    else
	DrawTray(win);
}
 
static void gui_focus_event(winlist_t *win, XEvent *event)
{
    switch (event->type)
    {
	case MapNotify:
	    DebugLog(1, ("MapNotify\n"));
	    if (!tray_init)
	    {
		XUnmapWindow(gui->display, win->window);
		alarm(1);
	    }
	    break;
	case UnmapNotify:
	    DebugLog(1, ("UnMapNotify\n"));
	    break;
	case ReparentNotify:
	    DebugLog(1, ("ReparentNotify Root(%d), Event_win(%d), Parent_Win(%d)\n", gui->root, event->xreparent.event, event->xreparent.parent));
	    parent_window = event->xreparent.parent;
	    if (parent_window == gui->root)
	    {
		DebugLog(1, ("Escape Manager Window\n"));
		tray_init = False;
	    }
	    break;
	case ConfigureNotify:
	case MotionNotify: /* Mouse Over */
	    {
	    XConfigureEvent *e = &event->xconfigure;
	    DebugLog(1, ("(my %d) ConfigureNotify manager=%d, Window=%d, x=%d, y=%d, w=%d, h=%d, border_w:%d, aboveWin=%d, override=%d\n",win->window, manager_window, e->window, e->x, e->y, e->width, e->height, e->border_width, e->above, e->override_redirect));
 		// width, height 會被改變，所以這裡要偵測大小是否被改變了
		// 有的話要重新把圖示放在新的範圍中間
		if (win->window == e->window && (win->width != e->width || win->height != e->height))
		{
		    DebugLog(1, ("Window size changed!\n"));
/*
		    int sw = (e->width - win->width) / 2;
		    int sh = (e->height - win->height) / 2;
		    int new_x = (sw > 1) ? sw : 0;
		    int new_y = (sh > 1) ? sh : 0;
		    XMoveWindow(gui->display, win->window, new_x, new_y);
*/
		    win->width = e->width;
		    win->height = e->height;
		}
	    gui_reread_resolution(win);
	    }
	    break;
//	case MotionNotify: /* Mouse Over */
//	    break;
	case EnterNotify: /* Mouse In */
	    DebugLog(1, ("EnterNotify\n"));
	    if (!gui_msgbox_actived() && !gui_menu_actived() )
	    {
		static char s[20];
		strcpy(s, "OXIM");
		strcat(s, " ");
		strcat(s, oxim_version());
		gui_show_msgbox(win, s);
	    }
	    break;
	case LeaveNotify: /* Mouse Out */
	    if (gui_msgbox_actived())
	    {
		gui_hide_msgbox(win);
	    }
	    break;
	case Expose:
	    DebugLog(1, ("Expose count=%d\n", event->xexpose.count));
	    if (event->xexpose.count == 0)
	    {
		gui_tray_draw(win);
	    }
	    break;
	case ButtonPress: /* 按下滑鼠按鍵 */
	    if (event->xbutton.button == Button1)
	    {
		if (gui_msgbox_actived())
		{
		    gui_hide_msgbox(win);
		}
		if (gui_menu_actived())
		{
		    gui_hide_menu();
		}
		else
		{
		    gui_show_menu(win);
		}
	    }
	    break;
	case ClientMessage: /* Client message */
	    if (event->xclient.message_type == xembed_atom)
	    {
		DebugLog(1, ("in tray %d\n", event->xclient.data.l[1]));
	    }
	    else
	    {
		DebugLog(1, ("Unknow Client message -> %d\n", event->xclient.message_type));
	    }
	    break;
	case PropertyNotify:
	    DebugLog(1, ("PropertyNotify\n"));
	    break;
	case DestroyNotify:
	    DebugLog(1, ("DestroyNotify\n"));
	    break;
	default:
	    DebugLog(1, ("tray win event %d\n", event->type));
    }
}

/*static XVisualInfo *
x11_screen_lookup_visual (VisualID xvisualid)
{
    XVisualInfo *vlist, vinfo_template, *v;
    int num_vis;

    vlist = XGetVisualInfo(gui->display, VisualNoMask, &vinfo_template, &num_vis);
    for (v = vlist; v < vlist + num_vis; v++) {
	if (v->visualid == xvisualid) {
 	    //visual = v->visual;

	    if (vlist[i].depth == 32 &&
	      (v->visual->red_mask   == 0xff0000 &&
	       v->visual->green_mask == 0x00ff00 &&
	       v->visual->blue_mask  == 0x0000ff))
	    {
		return v;
	      //screen_x11->rgba_visual = GDK_VISUAL (visuals[i]);
	    }

	    //printf("get visualid=%d, red_mask=%d, green_mask=%d, blue_mask=%d, depth=%d, class=%d\n", xvisualid, v->visual->red_mask, v->visual->green_mask, v->visual->blue_mask, vinfo_template.depth, vinfo_template.class);
	    //return v;
	}
    }
  return NULL;
}*/

static 
Bool ProcessTrayIcon(Display *dpy, Window window, char** data, XImage** image, Pixmap* pixmapshape, Pixmap* pixmap)
{
	XImage* dummy = NULL;
	XpmAttributes attr;
	attr.valuemask = XpmColormap | XpmDepth | XpmCloseness;
	attr.colormap = DefaultColormap(dpy, DefaultScreen(dpy));
	attr.depth = DefaultDepth(dpy, DefaultScreen(dpy));
	attr.closeness = 40000;
	attr.exactColors = False;

	if (XpmCreateImageFromData(dpy, data, image, &dummy, &attr))
		return False;
	if (XpmCreatePixmapFromData(dpy, window,data, pixmap,
				pixmapshape, &attr))
		return False;
	if (dummy)
		XDestroyImage(dummy);

	return True;
}

static 
Window ManagerWindow(Display *dpy, int screen)
{
    char tray_mgr[32];
    sprintf(tray_mgr, "_NET_SYSTEM_TRAY_S%d", screen);
    Atom selection_atom = XInternAtom(dpy, tray_mgr, False);
    if (selection_atom == None)
    {
	//printf("[%d]%s: no selection atom\n", __LINE__, __FILE__);
	return;
    }
    Window managerwindow = XGetSelectionOwner(dpy, selection_atom);
    
    return managerwindow;
}
static Window aw;
Window GetTrayWindow(void)
{
    return aw;
}


winlist_t *
gui_new_win_tray(XVisualInfo *vi);
/*
    建立 Tray 
*/
void gui_tray_init(void)
{
    winlist_t *win = NULL;

    if (gui->tray_win)
    {
	//return;
	win = gui->tray_win;
    }
    else
    {
#if 0
	//manager_window = XGetSelectionOwner(gui->display, selection_atom);
	manager_window = ManagerWindow(gui->display, gui->screen);

	if (manager_window != None)
	{
	    Atom visual_atom = XInternAtom(gui->display, "_NET_SYSTEM_TRAY_VISUAL", False);

	    Atom type;
	    int format;
	    union {
		    unsigned long *prop;
		    unsigned char *prop_ch;
	    } prop = { NULL };
	    unsigned long nitems;
	    unsigned long bytes_after;
	    int result;

	    type = None;
	    result = XGetWindowProperty (gui->display,
					manager_window,
					visual_atom,
					0, sizeof(long), False,
					XA_VISUALID,
					&type, &format, &nitems,
					&bytes_after, &(prop.prop_ch));

	    Visual *default_xvisual;
	    
	    if (result == 0 &&
		type == XA_VISUALID && nitems == 1 && format == 32)
		{
		    VisualID visual_id = prop.prop[0];
		    visual = x11_screen_lookup_visual (visual_id);
		}
	}
#endif
#if 1
/*        Atom actual_type;
        int actual_format;
        unsigned long nitems, bytes_remaining;
        unsigned char *data = 0;
        int result = XGetWindowProperty(gui->display, manager_window, visual_atom, 0, 1,
                                        False, XA_VISUALID, &actual_type,
                                        &actual_format, &nitems, &bytes_remaining, &data);
        VisualID vid = 0;
        if (result == Success && data && actual_type == XA_VISUALID && actual_format == 32 &&
                nitems == 1 && bytes_remaining == 0)
            vid = *(VisualID*)data;
        if (data)
            XFree(data);
        if (vid == 0)
            return;
*/
        uint mask = VisualScreenMask;
        XVisualInfo *vi, rvi;
        int count=0, i;
        //rvi.visualid = vid;
	rvi.screen = gui->screen;
        vi = XGetVisualInfo(gui->display, mask, &rvi, &count);
	for(i=0; i<count; i++)
	{
	      if (vi[i].depth == 32 &&
		  (vi[i].red_mask   == 0xff0000 &&
		   vi[i].green_mask == 0x00ff00 &&
		   vi[i].blue_mask  == 0x0000ff))
		{
			//printf("rgba_visual=%d\n", vi[i].visual->visualid);
			visual = &vi[i];
			break;
		}
	}
        /*if (vi) {
            //visual = vi[0];
	    visual = &vi[0];
	    //return;
            //XFree((char*)vi);
        }*/
#endif

	if(visual && visual->visual)
	    win = gui_new_win_tray((XVisualInfo*)visual);
	else
	    win = gui_new_win();
    }

    /* Tray 不需要邊框 */
    XSetWindowBorderWidth(gui->display, win->window, 0);

    XSetWindowAttributes win_attr;
    win_attr.override_redirect = True;
    XChangeWindowAttributes(gui->display, win->window,
			CWOverrideRedirect, &win_attr);

    Pixmap tray, mask;
    XpmAttributes attributes;
    attributes.valuemask = 0;

    if (!ProcessTrayIcon(gui->display, win->window, oxim_tray, &icon, &icon_mask,
				&picon)) 
    {
	//fprintf(stderr, "failed to get inactive icon image\n");
	return;
    }
#if 1
    if(visual && visual->visual)
    {
	/* GCs for copy and drawing on mask */
	XGCValues gv;
	gv.foreground = BlackPixel(gui->display, gui->screen);
	gv.background = BlackPixel(gui->display, gui->screen);
	//GC gc = XCreateGC(gui->display, win->window, GCBackground|GCForeground, &gv);
	GC gc = XCreateGC(gui->display, aw, GCBackground|GCForeground, &gv);

	XSetClipMask(gui->display, gc, icon_mask);
    }
#endif
    XpmCreatePixmapFromData(gui->display, win->window,
		oxim_tray, &tray, &mask, &attributes);
    tray_w = attributes.width;
    tray_h = attributes.height;

    win->width  = tray_w;
    win->height = tray_h;

    XpmFreeAttributes(&attributes);
    XFreePixmap(gui->display, tray);
    if (mask)
    {
	XFreePixmap(gui->display, mask);
    }
    win->pos_x = 0;
    win->pos_y = 0;

    win->win_draw_func = NULL;
    win->win_event_func = gui_focus_event;

    XSizeHints *size_hints = XAllocSizeHints();
    size_hints->flags = PMinSize | PMaxSize;
    size_hints->min_width = win->width;
    size_hints->min_height = win->height;
    size_hints->max_width = win->width;
    size_hints->max_height = win->height;
    XSetWMNormalHints(gui->display, win->window, size_hints);
    XFree(size_hints);

    Atom xembed_info = XInternAtom(gui->display, "_XEMBED_INFO", False);
    unsigned int mapping[2];
    mapping[0] = 0; /* ver 0 */
    mapping[1] = 1; /* request mapping */
    XChangeProperty(gui->display, win->window, xembed_info, xembed_info,
			32, PropModeReplace, (unsigned char *)mapping, 2);
    gui->tray_win = win;
}

int check_tray_manager(void)
{
    if (tray_init)
	return True;

    xembed_atom = XInternAtom(gui->display, "_XEMBED", False);
    system_tray_opcode_atom = XInternAtom(gui->display, "_NET_SYSTEM_TRAY_OPCODE", False);
    orientation_atom = XInternAtom(gui->display, "_NET_SYSTEM_TRAY_ORIENTATION", False);
    manager_window = ManagerWindow(gui->display, gui->screen);

    XGrabServer(gui->display);

    if (manager_window != None)
    {
	XSelectInput(gui->display, manager_window, StructureNotifyMask|PropertyChangeMask);
	send_message(gui->display, manager_window, SYSTEM_TRAY_REQUEST_DOCK, gui->tray_win->window, 0, 0);
aw = gui->tray_win->window;
/*	XftDrawDestroy(gui->tray_win->draw);
	//XDestroyWindow(gui->display, gui->tray_win->window);
	//XUnmapWindow(gui->display, gui->tray_win->window);
	gui->tray_win = NULL;
	gui_tray_init();*/

//	send_message(gui->display, manager_window, SYSTEM_TRAY_REQUEST_DOCK, gui->tray_win->window, 0, 0);
	
	tray_init = True;
    }
    XUngrabServer(gui->display);
    XFlush(gui->display);

    return tray_init;
}
