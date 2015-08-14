/* GTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <config.h>
#include <locale.h>
#include <string.h>
#include <stdlib.h>

#include <gtk/gtklabel.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtkversion.h>
#include "gtkintl.h"
#include "gtkimcontextoxim.h"
#include "../include/oximtool.h"


//#include "../include/gui.h"
//#include "../include/oxim.h"

typedef struct _GtkOXIMInfo GtkOXIMInfo;

struct _GtkIMContextOXIM
{
  GtkIMContext object;

  GtkOXIMInfo *im_info;

  gchar *locale;
  gchar *mb_charset;

  GdkWindow *client_window;
  GtkWidget *client_widget;

  gint preedit_size;
  gint preedit_length;
  gunichar *preedit_chars;
  XIMFeedback *feedbacks;

  gint preedit_cursor;

  XIMCallback preedit_start_callback;
  XIMCallback preedit_done_callback;
  XIMCallback preedit_draw_callback;
  XIMCallback preedit_caret_callback;

  XIMCallback status_start_callback;
  XIMCallback status_done_callback;
  XIMCallback status_draw_callback;

  XIMCallback string_conversion_callback;

  XIC ic;

  guint filter_key_release : 1;
  guint use_preedit : 1;
  guint finalizing : 1;
  guint in_toplevel : 1;
  guint has_focus : 1;
};

struct _GtkOXIMInfo
{
  GdkScreen *screen;
  XIM im;
  char *locale;
  XIMStyle preedit_style_setting;
  XIMStyle style;
  GtkSettings *settings;
  gulong status_set;
  gulong preedit_set;
  gulong display_closed_cb;
  XIMStyles *xim_styles;
  GSList *ics;

  guint reconnecting :1;
  guint supports_string_conversion;
};

static void     gtk_im_context_xim_class_init         (GtkIMContextOXIMClass  *class);
static void     gtk_im_context_xim_init               (GtkIMContextOXIM       *im_context_xim);
static void     gtk_im_context_xim_finalize           (GObject               *obj);
static void     gtk_im_context_xim_set_client_window  (GtkIMContext          *context,
						       GdkWindow             *client_window);
static gboolean gtk_im_context_xim_filter_keypress    (GtkIMContext          *context,
						       GdkEventKey           *key);
static void     gtk_im_context_xim_reset              (GtkIMContext          *context);
static void     gtk_im_context_xim_focus_in           (GtkIMContext          *context);
static void     gtk_im_context_xim_focus_out          (GtkIMContext          *context);
static void     gtk_im_context_xim_set_cursor_location (GtkIMContext          *context,
						       GdkRectangle		*area);
static void     gtk_im_context_xim_set_use_preedit    (GtkIMContext          *context,
						       gboolean		      use_preedit);
static void     gtk_im_context_xim_get_preedit_string (GtkIMContext          *context,
						       gchar                **str,
						       PangoAttrList        **attrs,
						       gint                  *cursor_pos);

static void reinitialize_ic      (GtkIMContextOXIM *context_xim);
static void set_ic_client_window (GtkIMContextOXIM *context_xim,
				  GdkWindow       *client_window);

static void setup_styles (GtkOXIMInfo *info);

static void update_client_widget   (GtkIMContextOXIM *context_xim);

static void xim_destroy_callback   (XIM      xim,
				    XPointer client_data,
				    XPointer call_data);

static XIC       gtk_im_context_xim_get_ic            (GtkIMContextOXIM *context_xim);
static void           xim_info_display_closed (GdkDisplay *display,
			                       gboolean    is_error,
			                       GtkOXIMInfo *info);

static GObjectClass *parent_class;

GType gtk_type_im_context_oxim = 0;

static GSList *open_ims = NULL;

void
gtk_im_context_xim_register_type (GTypeModule *type_module)
{
  static const GTypeInfo im_context_xim_info =
  {
    sizeof (GtkIMContextOXIMClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) gtk_im_context_xim_class_init,
    NULL,           /* class_finalize */
    NULL,           /* class_data */
    sizeof (GtkIMContextOXIM),
    0,
    (GInstanceInitFunc) gtk_im_context_xim_init,
  };

  gtk_type_im_context_oxim =
    g_type_module_register_type (type_module,
				 GTK_TYPE_IM_CONTEXT,
				 "GtkIMContextOXIM",
				 &im_context_xim_info, 0);
}

#define PREEDIT_MASK (XIMPreeditCallbacks | XIMPreeditPosition | \
		      XIMPreeditArea | XIMPreeditNothing | XIMPreeditNone)
#define STATUS_MASK (XIMStatusCallbacks | XIMStatusArea | \
		      XIMStatusNothing | XIMStatusNone)
#define ALLOWED_MASK (XIMPreeditCallbacks | XIMPreeditNothing | XIMPreeditNone | \
		      XIMStatusCallbacks | XIMStatusNothing | XIMStatusNone)

static XIMStyle
choose_better_style (XIMStyle style1, XIMStyle style2)
{
  XIMStyle s1, s2, u;

  if (style1 == 0) return style2;
  if (style2 == 0) return style1;
  if ((style1 & (PREEDIT_MASK | STATUS_MASK))
    	== (style2 & (PREEDIT_MASK | STATUS_MASK)))
    return style1;

  s1 = style1 & PREEDIT_MASK;
  s2 = style2 & PREEDIT_MASK;
  u = s1 | s2;
  if (s1 != s2) {
    if (u & XIMPreeditCallbacks)
      return (s1 == XIMPreeditCallbacks) ? style1 : style2;
    else if (u & XIMPreeditPosition)
      return (s1 == XIMPreeditPosition) ? style1 :style2;
    else if (u & XIMPreeditArea)
      return (s1 == XIMPreeditArea) ? style1 : style2;
    else if (u & XIMPreeditNothing)
      return (s1 == XIMPreeditNothing) ? style1 : style2;
    else if (u & XIMPreeditNone)
      return (s1 == XIMPreeditNone) ? style1 : style2;
  } else {
    s1 = style1 & STATUS_MASK;
    s2 = style2 & STATUS_MASK;
    u = s1 | s2;
    if (u & XIMStatusCallbacks)
      return (s1 == XIMStatusCallbacks) ? style1 : style2;
    else if (u & XIMStatusArea)
      return (s1 == XIMStatusArea) ? style1 : style2;
    else if (u & XIMStatusNothing)
      return (s1 == XIMStatusNothing) ? style1 : style2;
    else if (u & XIMStatusNone)
      return (s1 == XIMStatusNone) ? style1 : style2;
  }
  return 0; /* Get rid of stupid warning */
}

static void
reinitialize_all_ics (GtkOXIMInfo *info)
{
  GSList *tmp_list;

  for (tmp_list = info->ics; tmp_list; tmp_list = tmp_list->next)
    reinitialize_ic (tmp_list->data);
}

static void
preedit_style_change (GtkOXIMInfo *info)
{
  GtkIMPreeditStyle preedit_style;
  g_object_get (info->settings,
		"gtk-im-preedit-style", &preedit_style,
		NULL);

  if (preedit_style == GTK_IM_PREEDIT_CALLBACK)
    info->preedit_style_setting = XIMPreeditCallbacks;
  else if (preedit_style == GTK_IM_PREEDIT_NOTHING)
    info->preedit_style_setting = XIMPreeditNothing;
  else if (preedit_style == GTK_IM_PREEDIT_NONE)
    info->preedit_style_setting = XIMPreeditNone;
  else
    return;

  setup_styles (info);

  reinitialize_all_ics (info);
}

static void
setup_styles (GtkOXIMInfo *info)
{
  int i;
  unsigned long settings_preference;
  XIMStyles *xim_styles = info->xim_styles;

  settings_preference = info->preedit_style_setting;
  info->style = 0;
  if (xim_styles)
    {
      for (i = 0; i < xim_styles->count_styles; i++)
	if ((xim_styles->supported_styles[i] & ALLOWED_MASK) == xim_styles->supported_styles[i])
	  {
	    if (settings_preference == xim_styles->supported_styles[i])
	      {
		info->style = settings_preference;
		break;
	      }
	    info->style = choose_better_style (info->style,
					       xim_styles->supported_styles[i]);
	  }
    }
  if (info->style == 0)
    info->style = XIMPreeditNothing | XIMStatusNothing;
}

static void
setup_im (GtkOXIMInfo *info)
{
  XIMValuesList *ic_values = NULL;
  XIMCallback im_destroy_callback;
  GdkDisplay *display;

  if (info->im == NULL)
    return;

  im_destroy_callback.client_data = (XPointer)info;
  im_destroy_callback.callback = (XIMProc)xim_destroy_callback;
  XSetIMValues (info->im,
		XNDestroyCallback, &im_destroy_callback,
		NULL);

  XGetIMValues (info->im,
		XNQueryInputStyle, &info->xim_styles,
		XNQueryICValuesList, &ic_values,
		NULL);

  info->settings = gtk_settings_get_for_screen (info->screen);

  if (!g_object_class_find_property (G_OBJECT_GET_CLASS (info->settings),
				     "gtk-im-preedit-style"))
    gtk_settings_install_property (g_param_spec_enum ("gtk-im-preedit-style",
						      P_("IM Preedit style"),
						      P_("How to draw the input method preedit string"),
						      GTK_TYPE_IM_PREEDIT_STYLE,
						      GTK_IM_PREEDIT_CALLBACK,
						      G_PARAM_READWRITE));

  if (!g_object_class_find_property (G_OBJECT_GET_CLASS (info->settings),
				     "gtk-im-status-style"))
    gtk_settings_install_property (g_param_spec_enum ("gtk-im-status-style",
						      P_("IM Status style"),
						      P_("How to draw the input method statusbar"),
						      GTK_TYPE_IM_STATUS_STYLE,
						      GTK_IM_STATUS_CALLBACK,
						      G_PARAM_READWRITE));

  info->preedit_set = g_signal_connect_swapped (info->settings,
						"notify::gtk-im-preedit-style",
						G_CALLBACK (preedit_style_change),
						info);

  info->supports_string_conversion = FALSE;
  if (ic_values)
    {
      int i;

      for (i = 0; i < ic_values->count_values; i++)
	if (strcmp (ic_values->supported_values[i],
		    XNStringConversionCallback) == 0)
	  {
	    info->supports_string_conversion = TRUE;
	    break;
	  }

#if 0
      for (i = 0; i < ic_values->count_values; i++)
	g_print ("%s\n", ic_values->supported_values[i]);
      for (i = 0; i < xim_styles->count_styles; i++)
	g_print ("%#x\n", xim_styles->supported_styles[i]);
#endif

      XFree (ic_values);
    }

  preedit_style_change (info);
  
  display = gdk_screen_get_display (info->screen);
  info->display_closed_cb = g_signal_connect (display, "closed",
	                                      G_CALLBACK (xim_info_display_closed), info);
}

static void
xim_info_display_closed (GdkDisplay *display,
			 gboolean    is_error,
			 GtkOXIMInfo *info)
{
  GSList *ics, *tmp_list;

  open_ims = g_slist_remove (open_ims, info);

  ics = info->ics;
  info->ics = NULL;

  for (tmp_list = ics; tmp_list; tmp_list = tmp_list->next)
    set_ic_client_window (tmp_list->data, NULL);

  g_slist_free (ics);

  if (info->preedit_set)
    g_signal_handler_disconnect (info->settings, info->preedit_set);
  if (info->display_closed_cb)
    g_signal_handler_disconnect (display, info->display_closed_cb);

  if (info->xim_styles)
    XFree (info->xim_styles);
  g_free (info->locale);

  if (info->im)
    XCloseIM (info->im);

  g_free (info);
}

static void
xim_instantiate_callback (Display *display, XPointer client_data,
			  XPointer call_data)
{
  GtkOXIMInfo *info = (GtkOXIMInfo*)client_data;
  XIM im = NULL;

  im = XOpenIM (display, NULL, NULL, NULL);

  if (!im)
    return;

  info->im = im;
  setup_im (info);

  XUnregisterIMInstantiateCallback (display, NULL, NULL, NULL,
				    xim_instantiate_callback,
				    (XPointer)info);
  info->reconnecting = FALSE;
}

/* initialize info->im */
static void
xim_info_try_im (GtkOXIMInfo *info)
{
  GdkScreen *screen = info->screen;
  GdkDisplay *display = gdk_screen_get_display (screen);

  g_assert (info->im == NULL);
  if (info->reconnecting)
    return;

  if (XSupportsLocale ())
    {
      if (!XSetLocaleModifiers ("@im=oxim"))
	g_warning ("Unable to set locale modifiers with XSetLocaleModifiers()");
      info->im = XOpenIM (GDK_DISPLAY_XDISPLAY (display), NULL, NULL, NULL);
      if (!info->im)
	{
	  XRegisterIMInstantiateCallback (GDK_DISPLAY_XDISPLAY(display),
					  NULL, NULL, NULL,
					  xim_instantiate_callback,
					  (XPointer)info);
	  info->reconnecting = TRUE;
	  return;
	}
      setup_im (info);
    }
}

static void
xim_destroy_callback (XIM      xim,
		      XPointer client_data,
		      XPointer call_data)
{
  GtkOXIMInfo *info = (GtkOXIMInfo*)client_data;

  info->im = NULL;

  g_signal_handler_disconnect (info->settings, info->preedit_set);

  reinitialize_all_ics (info);
  xim_info_try_im (info);
  return;
}

static GtkOXIMInfo *
get_im (GdkWindow *client_window,
	const char *locale)
{
  GSList *tmp_list;
  GtkOXIMInfo *info;
  GdkScreen *screen = gdk_drawable_get_screen (client_window);

  info = NULL;
  tmp_list = open_ims;
  while (tmp_list)
    {
      GtkOXIMInfo *tmp_info = tmp_list->data;
      if (tmp_info->screen == screen &&
	  strcmp (tmp_info->locale, locale) == 0)
	{
	  if (tmp_info->im)
	    {
	      return tmp_info;
	    }
	  else
	    {
	      tmp_info = tmp_info;
	      break;
	    }
	}
      tmp_list = tmp_list->next;
    }

  if (info == NULL)
    {
      info = g_new (GtkOXIMInfo, 1);
      open_ims = g_slist_prepend (open_ims, info);

      info->screen = screen;
      info->locale = g_strdup (locale);
      info->xim_styles = NULL;
      info->preedit_style_setting = 0;
      info->settings = NULL;
      info->preedit_set = 0;
      info->display_closed_cb = 0;
      info->ics = NULL;
      info->reconnecting = FALSE;
      info->im = NULL;
    }

  xim_info_try_im (info);
  return info;
}

static void
gtk_im_context_xim_class_init (GtkIMContextOXIMClass *class)
{
  GtkIMContextClass *im_context_class = GTK_IM_CONTEXT_CLASS (class);
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  im_context_class->set_client_window = gtk_im_context_xim_set_client_window;
  im_context_class->filter_keypress = gtk_im_context_xim_filter_keypress;
  im_context_class->reset = gtk_im_context_xim_reset;
  im_context_class->get_preedit_string = gtk_im_context_xim_get_preedit_string;
  im_context_class->focus_in = gtk_im_context_xim_focus_in;
  im_context_class->focus_out = gtk_im_context_xim_focus_out;
  im_context_class->set_cursor_location = gtk_im_context_xim_set_cursor_location;
  im_context_class->set_use_preedit = gtk_im_context_xim_set_use_preedit;
  gobject_class->finalize = gtk_im_context_xim_finalize;
}

static void
gtk_im_context_xim_init (GtkIMContextOXIM *im_context_xim)
{
  im_context_xim->use_preedit = TRUE;
  im_context_xim->filter_key_release = FALSE;
  im_context_xim->finalizing = FALSE;
  im_context_xim->has_focus = FALSE;
  im_context_xim->in_toplevel = FALSE;
}

static void
gtk_im_context_xim_finalize (GObject *obj)
{
  GtkIMContextOXIM *context_xim = GTK_IM_CONTEXT_OXIM (obj);

  context_xim->finalizing = TRUE;

  if (context_xim->im_info && !context_xim->im_info->ics->next) 
    {
      if (context_xim->im_info->reconnecting)
	{
	  GdkDisplay *display;

	  display = gdk_screen_get_display (context_xim->im_info->screen);
	  XUnregisterIMInstantiateCallback (GDK_DISPLAY_XDISPLAY (display),
					    NULL, NULL, NULL,
					    xim_instantiate_callback,
					    (XPointer)context_xim->im_info);
	}
      else if (context_xim->im_info->im)
	{
	  XIMCallback im_destroy_callback;

	  im_destroy_callback.client_data = NULL;
	  im_destroy_callback.callback = NULL;
	  XSetIMValues (context_xim->im_info->im,
			XNDestroyCallback, &im_destroy_callback,
			NULL);
	}
    }

  set_ic_client_window (context_xim, NULL);

  g_free (context_xim->locale);
  g_free (context_xim->mb_charset);

  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
reinitialize_ic (GtkIMContextOXIM *context_xim)
{
  if (context_xim->ic)
    {
      XDestroyIC (context_xim->ic);
      context_xim->ic = NULL;

      if (context_xim->preedit_length)
	{
	  context_xim->preedit_length = 0;
	  if (!context_xim->finalizing)
	    g_signal_emit_by_name (context_xim, "preedit_changed");
	}
    }
  /*
     reset filter_key_release flag, otherwise keystrokes will be doubled
     until reconnecting to XIM.
  */
  context_xim->filter_key_release = FALSE;
}

static void
set_ic_client_window (GtkIMContextOXIM *context_xim,
		      GdkWindow       *client_window)
{
  reinitialize_ic (context_xim);
  if (context_xim->client_window)
    {
      context_xim->im_info->ics = g_slist_remove (context_xim->im_info->ics, context_xim);
      context_xim->im_info = NULL;
    }

  context_xim->client_window = client_window;

  if (context_xim->client_window)
    {
      context_xim->im_info = get_im (context_xim->client_window, context_xim->locale);
      context_xim->im_info->ics = g_slist_prepend (context_xim->im_info->ics, context_xim);
    }

// update_client_widget (context_xim);

}
#if 0
/* Called when context_xim->client_window changes; takes care of
 * removing and/or setting up our watches for the toplevel
 */
static void
update_client_widget (GtkIMContextOXIM *context_xim)
{
  GtkWidget *new_client_widget = widget_for_window (context_xim->client_window);

  if (new_client_widget != context_xim->client_widget)
    {
      if (context_xim->client_widget)
      {
        g_signal_handlers_disconnect_by_func (context_xim->client_widget,
                                    G_CALLBACK (on_client_widget_hierarchy_changed),
                                    context_xim);
      }
      context_xim->client_widget = new_client_widget;
      if (context_xim->client_widget)
      {
        g_signal_connect (context_xim->client_widget, "hierarchy-changed",
                      G_CALLBACK (on_client_widget_hierarchy_changed),
                      context_xim);
      }

//       update_in_toplevel (context_xim);
    }
}

/* Updates the in_toplevel flag for @context_xim
 */
static void
update_in_toplevel (GtkIMContextOXIM *context_xim)
{
  if (context_xim->client_widget)
    {
      GtkWidget *toplevel = gtk_widget_get_toplevel (context_xim->client_widget);
      
      context_xim->in_toplevel = (toplevel && GTK_WIDGET_TOPLEVEL (toplevel));
    }
  else
    context_xim->in_toplevel = FALSE;

  /* Some paranoia, in case we don't get a focus out */
  if (!context_xim->in_toplevel)
    context_xim->has_focus = FALSE;
  
//   update_status_window (context_xim);
}
#endif

static void
gtk_im_context_xim_set_client_window (GtkIMContext          *context,
				      GdkWindow             *client_window)
{
  GtkIMContextOXIM *context_xim = GTK_IM_CONTEXT_OXIM (context);

  set_ic_client_window (context_xim, client_window);
}

GtkIMContext *
gtk_im_context_xim_new (void)
{
  GtkIMContextOXIM *result;
  // 取得系統預設的語言與編碼
  const gchar *sys_locale = setlocale(LC_CTYPE, NULL);
  const gchar *charset;
  gboolean codesetIsUtf8 = (strcasecmp(nl_langinfo(CODESET), "UTF-8") == 0);
  gboolean use_fallback = !XSupportsLocale();

  result = g_object_new(GTK_TYPE_IM_CONTEXT_OXIM, NULL);

  if (use_fallback || !codesetIsUtf8)
  {
    g_warning ("Fallback to default locale : %s\n", FALLBACK_LOCALE);
    result->locale = g_strdup(setlocale(LC_CTYPE, FALLBACK_LOCALE));
  }
  else
  {
    result->locale = g_strdup(setlocale(LC_CTYPE, NULL));
  }

  g_get_charset (&charset);
  result->mb_charset = g_strdup (charset);

  return GTK_IM_CONTEXT (result);
}

static char *
mb_to_utf8 (GtkIMContextOXIM *context_xim,
	    const char      *str)
{
  GError *error = NULL;
  gchar *result;

  if (strcmp (context_xim->mb_charset, "UTF-8") == 0)
    result = g_strdup (str);
  else
    {
      result = g_convert (str, -1,
			  "UTF-8", context_xim->mb_charset,
			  NULL, NULL, &error);
      if (!result)
	{
	  g_warning ("Error converting text from IM to UTF-8: %s\n", error->message);
	  g_error_free (error);
	}
    }

  return result;
}

static gboolean
gtk_im_context_xim_filter_keypress (GtkIMContext *context,
				    GdkEventKey  *event)
{
  GtkIMContextOXIM *context_xim = GTK_IM_CONTEXT_OXIM (context);
  XIC ic = gtk_im_context_xim_get_ic (context_xim);
  gchar static_buffer[256];
  gchar *buffer = static_buffer;
  gint buffer_size = sizeof(static_buffer) - 1;
  gint num_bytes = 0;
  KeySym keysym;
  Status status;
  gboolean result = FALSE;
  GdkWindow *root_window = gdk_screen_get_root_window (gdk_drawable_get_screen (event->window));

  XKeyPressedEvent xevent;

  if (event->type == GDK_KEY_RELEASE && !context_xim->filter_key_release)
    return FALSE;

  xevent.type = (event->type == GDK_KEY_PRESS) ? KeyPress : KeyRelease;
  xevent.serial = 0;		/* hope it doesn't matter */
  xevent.send_event = event->send_event;
  xevent.display = GDK_DRAWABLE_XDISPLAY (event->window);
  xevent.window = GDK_DRAWABLE_XID (event->window);
  xevent.root = GDK_DRAWABLE_XID (root_window);
  xevent.subwindow = xevent.window;
  xevent.time = event->time;
  xevent.x = xevent.x_root = 0;
  xevent.y = xevent.y_root = 0;
  xevent.state = event->state;
  xevent.keycode = event->hardware_keycode;
  xevent.same_screen = True;

  if (XFilterEvent ((XEvent *)&xevent, GDK_DRAWABLE_XID (context_xim->client_window)))
    return TRUE;

  if (event->state &
      (gtk_accelerator_get_default_mod_mask () & ~(GDK_SHIFT_MASK | GDK_CONTROL_MASK))) 
    return FALSE;

 again:
  if (ic)
    num_bytes = XmbLookupString (ic, &xevent, buffer, buffer_size, &keysym, &status);
  else
    {
      num_bytes = XLookupString (&xevent, buffer, buffer_size, &keysym, NULL);
      status = XLookupBoth;
    }

  if (status == XBufferOverflow)
    {
      buffer_size = num_bytes;
      if (buffer != static_buffer)
	g_free (buffer);
      buffer = g_malloc (num_bytes + 1);
      goto again;
    }

  /* I don't know how we should properly handle XLookupKeysym or XLookupBoth
   * here ... do input methods actually change the keysym? we can't really
   * feed it back to accelerator processing at this point...
   */
  if (status == XLookupChars || status == XLookupBoth)
    {
      char *result_utf8;

      buffer[num_bytes] = '\0';

      result_utf8 = mb_to_utf8 (context_xim, buffer);
      if (result_utf8)
	{
	  if ((guchar)result_utf8[0] >= 0x20 &&
	      result_utf8[0] != 0x7f) /* Some IM have a nasty habit of converting
				       * control characters into strings
				       */
	    {
	      g_signal_emit_by_name (context, "commit", result_utf8);
	      result = TRUE;
	    }

	  g_free (result_utf8);
	}
    }

  if (buffer != static_buffer)
    g_free (buffer);

  return result;
}

static GtkWidget *
widget_for_window (GdkWindow *window);

static void
gtk_im_context_xim_focus_in (GtkIMContext *context)
{
  GtkIMContextOXIM *context_xim = GTK_IM_CONTEXT_OXIM (context);
#if 0  
//   display = gdk_screen_get_display (info->screen);
  Window w;
  int rt;
  XGetInputFocus(GDK_DISPLAY(), &w, &rt);
gboolean v = 0;
widget_for_window(context_xim->client_window);
GtkWidget *new_client_widget = widget_for_window (context_xim->client_window);
g_object_get(G_OBJECT(new_client_widget), "editable", &v, NULL);
#endif
  if (!context_xim->has_focus )//&& (/*RevertToPointerRoot == rt ||*/ RevertToParent == rt))
    {
      XIC ic = gtk_im_context_xim_get_ic (context_xim);

      context_xim->has_focus = TRUE;

      if (ic)
      {
	XSetICFocus (ic);
      }
    }
	//system("oxim-agent -ekeyboard_show");
  return;
}

static void
gtk_im_context_xim_focus_out (GtkIMContext *context)
{
  GtkIMContextOXIM *context_xim = GTK_IM_CONTEXT_OXIM (context);
#if 0
  Window w;
  int rt;
  XGetInputFocus(GDK_DISPLAY(), &w, &rt);
#endif
  if (context_xim->has_focus /*&& RevertToNone == rt*/)
    {
      XIC ic = gtk_im_context_xim_get_ic (context_xim);

      context_xim->has_focus = FALSE;

      if (ic)
	XUnsetICFocus (ic);
    }
//system("oxim-agent -ekeyboard_hide");
  return;
}

static void
gtk_im_context_xim_set_cursor_location (GtkIMContext *context,
					GdkRectangle *area)
{
  GtkIMContextOXIM *context_xim = GTK_IM_CONTEXT_OXIM (context);
  XIC ic = gtk_im_context_xim_get_ic (context_xim);

  XVaNestedList preedit_attr;
  XPoint          spot;

  if (!ic)
    return;

  spot.x = area->x + area->width;
  spot.y = area->y + area->height;
/*
  spot.x = area->x;
  spot.y = area->y;*/

  preedit_attr = XVaCreateNestedList (0, XNSpotLocation, &spot, NULL);
  if (XSetICValues(ic, XNPreeditAttributes, preedit_attr, NULL) != NULL)
  {
	GdkScreen *screen = gdk_drawable_get_screen(context_xim->client_window);
	GdkDisplay *display = gdk_screen_get_display (screen);
	Display *Xdisplay = GDK_DISPLAY_XDISPLAY (display);
	Window XWin = GDK_DRAWABLE_XID (context_xim->client_window);
	Atom oxim_atom = XInternAtom(Xdisplay, OXIM_ATOM, True);
	if (oxim_atom)
	{
	    Window config_win = XGetSelectionOwner(Xdisplay, oxim_atom);
	    XClientMessageEvent event;
	    event.type = ClientMessage;
	    event.window = config_win;
	    event.message_type = oxim_atom;
	    event.format = 32;
	    event.data.l[0] = OXIM_CMD_SET_LOCATION;
	    event.data.l[1] = XWin;
	    event.data.l[2] = spot.x;
	    event.data.l[3] = spot.y;
	    XSendEvent(Xdisplay, config_win, False, 0, (XEvent *)&event);
	}
  }
  XFree(preedit_attr);

  return;
}

static void
gtk_im_context_xim_set_use_preedit (GtkIMContext *context,
				    gboolean      use_preedit)
{
  GtkIMContextOXIM *context_xim = GTK_IM_CONTEXT_OXIM (context);

  use_preedit = use_preedit != FALSE;

  if (context_xim->use_preedit != use_preedit)
    {
      context_xim->use_preedit = use_preedit;
      reinitialize_ic (context_xim);
    }

  return;
}

static void
gtk_im_context_xim_reset (GtkIMContext *context)
{
  GtkIMContextOXIM *context_xim = GTK_IM_CONTEXT_OXIM (context);
  XIC ic = gtk_im_context_xim_get_ic (context_xim);
  gchar *result;

  /* restore conversion state after resetting ic later */
  XIMPreeditState preedit_state = XIMPreeditUnKnown;
  XVaNestedList preedit_attr;
  gboolean have_preedit_state = FALSE;

  if (!ic)
    return;


  if (context_xim->preedit_length == 0)
    return;

  preedit_attr = XVaCreateNestedList(0,
                                     XNPreeditState, &preedit_state,
                                     NULL);
  if (!XGetICValues(ic,
                    XNPreeditAttributes, preedit_attr,
                    NULL))
    have_preedit_state = TRUE;

  XFree(preedit_attr);

  result = XmbResetIC (ic);

  preedit_attr = XVaCreateNestedList(0,
                                     XNPreeditState, preedit_state,
                                     NULL);
  if (have_preedit_state)
    XSetICValues(ic,
		 XNPreeditAttributes, preedit_attr,
		 NULL);

  XFree(preedit_attr);

  if (result)
    {
      char *result_utf8 = mb_to_utf8 (context_xim, result);
      if (result_utf8)
	{
	  g_signal_emit_by_name (context, "commit", result_utf8);
	  g_free (result_utf8);
	}
    }

  if (context_xim->preedit_length)
    {
      context_xim->preedit_length = 0;
      g_signal_emit_by_name (context, "preedit_changed");
    }

  XFree (result);
}

/* Mask of feedback bits that we render
 */
#define FEEDBACK_MASK (XIMReverse | XIMUnderline)

static void
add_feedback_attr (PangoAttrList *attrs,
		   const gchar   *str,
		   XIMFeedback    feedback,
		   gint           start_pos,
		   gint           end_pos)
{
  PangoAttribute *attr;

  gint start_index = g_utf8_offset_to_pointer (str, start_pos) - str;
  gint end_index = g_utf8_offset_to_pointer (str, end_pos) - str;

  if (feedback & XIMUnderline)
    {
      attr = pango_attr_underline_new (PANGO_UNDERLINE_SINGLE);
      attr->start_index = start_index;
      attr->end_index = end_index;

      pango_attr_list_change (attrs, attr);
    }

  if (feedback & XIMReverse)
    {
      attr = pango_attr_foreground_new (0xffff, 0xffff, 0xffff);
      attr->start_index = start_index;
      attr->end_index = end_index;

      pango_attr_list_change (attrs, attr);

      attr = pango_attr_background_new (0, 0, 0);
      attr->start_index = start_index;
      attr->end_index = end_index;

      pango_attr_list_change (attrs, attr);
    }

  if (feedback & ~FEEDBACK_MASK)
    g_warning ("Unrendered feedback style: %#lx", feedback & ~FEEDBACK_MASK);
}

static void
gtk_im_context_xim_get_preedit_string (GtkIMContext   *context,
				       gchar         **str,
				       PangoAttrList **attrs,
				       gint           *cursor_pos)
{
  GtkIMContextOXIM *context_xim = GTK_IM_CONTEXT_OXIM (context);
  gchar *utf8 = g_ucs4_to_utf8 (context_xim->preedit_chars, context_xim->preedit_length, NULL, NULL, NULL);

  if (attrs)
    {
      int i;
      XIMFeedback last_feedback = 0;
      gint start = -1;

      *attrs = pango_attr_list_new ();

      for (i = 0; i < context_xim->preedit_length; i++)
	{
	  XIMFeedback new_feedback = context_xim->feedbacks[i] & FEEDBACK_MASK;
	  if (new_feedback != last_feedback)
	    {
	      if (start >= 0)
		add_feedback_attr (*attrs, utf8, last_feedback, start, i);

	      last_feedback = new_feedback;
	      start = i;
	    }
	}

      if (start >= 0)
	add_feedback_attr (*attrs, utf8, last_feedback, start, i);
    }

  if (str)
    *str = utf8;
  else
    g_free (utf8);

  if (cursor_pos)
    *cursor_pos = context_xim->preedit_cursor;
}

static int
preedit_start_callback (XIC      xic,
			XPointer client_data,
			XPointer call_data)
{
  GtkIMContext *context = GTK_IM_CONTEXT (client_data);
  GtkIMContextOXIM *context_xim = GTK_IM_CONTEXT_OXIM (context);

  if (!context_xim->finalizing)
    g_signal_emit_by_name (context, "preedit_start");

  return -1;			/* No length limit */
}

static void
preedit_done_callback (XIC      xic,
		     XPointer client_data,
		     XPointer call_data)
{
  GtkIMContext *context = GTK_IM_CONTEXT (client_data);
  GtkIMContextOXIM *context_xim = GTK_IM_CONTEXT_OXIM (context);

  if (context_xim->preedit_length)
    {
      context_xim->preedit_length = 0;
      if (!context_xim->finalizing)
	g_signal_emit_by_name (context_xim, "preedit_changed");
    }

  if (!context_xim->finalizing)
    g_signal_emit_by_name (context, "preedit_end");
}

static gint
xim_text_to_utf8 (GtkIMContextOXIM *context, XIMText *xim_text, gchar **text)
{
  gint text_length = 0;
  GError *error = NULL;
  gchar *result = NULL;

  if (xim_text && xim_text->string.multi_byte)
    {
      if (xim_text->encoding_is_wchar)
	{
	  g_warning ("Wide character return from Xlib not currently supported");
	  *text = NULL;
	  return 0;
	}

      if (strcmp (context->mb_charset, "UTF-8") == 0)
	result = g_strdup (xim_text->string.multi_byte);
      else
	result = g_convert (xim_text->string.multi_byte,
			    -1,
			    "UTF-8",
			    context->mb_charset,
			    NULL, NULL, &error);

      if (result)
	{
	  text_length = g_utf8_strlen (result, -1);

	  if (text_length != xim_text->length)
	    {
	      g_warning ("Size mismatch when converting text from input method: supplied length = %d\n, result length = %d", xim_text->length, text_length);
	    }
	}
      else
	{
	  g_warning ("Error converting text from IM to UCS-4: %s", error->message);
	  g_error_free (error);

	  *text = NULL;
	  return 0;
	}

      *text = result;
      return text_length;
    }
  else
    {
      *text = NULL;
      return 0;
    }
}

static void
preedit_draw_callback (XIC                           xic,
		       XPointer                      client_data,
		       XIMPreeditDrawCallbackStruct *call_data)
{
  GtkIMContextOXIM *context = GTK_IM_CONTEXT_OXIM (client_data);

  XIMText *new_xim_text = call_data->text;
  gint new_text_length;
  gunichar *new_text = NULL;
  gint i;
  gint diff;
  gint new_length;
  gchar *tmp;

  gint chg_first = CLAMP (call_data->chg_first, 0, context->preedit_length);
  gint chg_length = CLAMP (call_data->chg_length, 0, context->preedit_length - chg_first);

  context->preedit_cursor = call_data->caret;

  if (chg_first != call_data->chg_first || chg_length != call_data->chg_length)
    g_warning ("Invalid change to preedit string, first=%d length=%d (orig length == %d)",
	       call_data->chg_first, call_data->chg_length, context->preedit_length);

  new_text_length = xim_text_to_utf8 (context, new_xim_text, &tmp);
  if (tmp)
    {
      new_text = g_utf8_to_ucs4_fast (tmp, -1, NULL);
      g_free (tmp);
    }

  diff = new_text_length - chg_length;
  new_length = context->preedit_length + diff;

  if (new_length > context->preedit_size)
    {
      context->preedit_size = new_length;
      context->preedit_chars = g_renew (gunichar, context->preedit_chars, new_length);
      context->feedbacks = g_renew (XIMFeedback, context->feedbacks, new_length);
    }

  if (diff < 0)
    {
      for (i = chg_first + chg_length ; i < context->preedit_length; i++)
	{
	  context->preedit_chars[i + diff] = context->preedit_chars[i];
	  context->feedbacks[i + diff] = context->feedbacks[i];
	}
    }
  else
    {
      for (i = context->preedit_length - 1; i >= chg_first + chg_length ; i--)
	{
	  context->preedit_chars[i + diff] = context->preedit_chars[i];
	  context->feedbacks[i + diff] = context->feedbacks[i];
	}
    }

  for (i = 0; i < new_text_length; i++)
    {
      context->preedit_chars[chg_first + i] = new_text[i];
      context->feedbacks[chg_first + i] = new_xim_text->feedback[i];
    }

  context->preedit_length += diff;

  g_free (new_text);

  if (!context->finalizing)
    g_signal_emit_by_name (context, "preedit_changed");
}


static void
preedit_caret_callback (XIC                            xic,
			XPointer                       client_data,
			XIMPreeditCaretCallbackStruct *call_data)
{
  GtkIMContextOXIM *context = GTK_IM_CONTEXT_OXIM (client_data);

  if (call_data->direction == XIMAbsolutePosition)
    {
      context->preedit_cursor = call_data->position;
      if (!context->finalizing)
	g_signal_emit_by_name (context, "preedit_changed");
    }
  else
    {
      g_warning ("Caret movement command: %d %d %d not supported",
		 call_data->position, call_data->direction, call_data->style);
    }
}

static void
status_start_callback (XIC      xic,
		       XPointer client_data,
		       XPointer call_data)
{
  return;
}

static void
status_done_callback (XIC      xic,
		      XPointer client_data,
		      XPointer call_data)
{
  return;
}


static void
status_draw_callback (XIC      xic,
		      XPointer client_data,
		      XIMStatusDrawCallbackStruct *call_data)
{
#if 0
  GtkIMContextOXIM *context = GTK_IM_CONTEXT_XIM (client_data);

  if (call_data->type == XIMTextType)
    {
      gchar *text;
      xim_text_to_utf8 (context, call_data->data.text, &text);

      if (context->status_window)
	status_window_set_text (context->status_window, text ? text : "");
    }
  else				/* bitmap */
    {
      g_print ("Status drawn with bitmap - id = %#lx\n", call_data->data.bitmap);
    }
    //may be no need -- wind
#endif
}

static void
string_conversion_callback (XIC xic, XPointer client_data, XPointer call_data)
{
  GtkIMContextOXIM *context_xim;
  XIMStringConversionCallbackStruct *conv_data;
  gchar *surrounding;
  gint  cursor_index;

  context_xim = (GtkIMContextOXIM *)client_data;
  conv_data = (XIMStringConversionCallbackStruct *)call_data;

  if (gtk_im_context_get_surrounding ((GtkIMContext *)context_xim,
                                      &surrounding, &cursor_index))
    {
      gchar *text = NULL;
      gsize text_len = 0;
      gint  subst_offset = 0, subst_nchars = 0;
      gint  i;
      gchar *p = surrounding + cursor_index, *q;
      gshort position = (gshort)conv_data->position;

      if (position > 0)
        {
          for (i = position; i > 0 && *p; --i)
            p = g_utf8_next_char (p);
          if (i > 0)
            return;
        }
      /* According to X11R6.4 Xlib - C Library Reference Manual
       * section 13.5.7.3 String Conversion Callback,
       * XIMStringConversionPosition is starting position _relative_
       * to current client's cursor position. So it should be able
       * to be negative, or referring to a position before the cursor
       * would be impossible. But current X protocol defines this as
       * unsigned short. So, compiler may warn about the value range
       * here. We hope the X protocol is fixed soon.
       */
      else if (position < 0)
        {
          for (i = position; i < 0 && p > surrounding; ++i)
            p = g_utf8_prev_char (p);
          if (i < 0)
            return;
        }

      switch (conv_data->direction)
        {
        case XIMForwardChar:
          for (i = conv_data->factor, q = p; i > 0 && *q; --i)
            q = g_utf8_next_char (q);
          if (i > 0)
            break;
          text = g_locale_from_utf8 (p, q - p, NULL, &text_len, NULL);
          subst_offset = position;
          subst_nchars = conv_data->factor;
          break;

        case XIMBackwardChar:
          for (i = conv_data->factor, q = p; i > 0 && q > surrounding; --i)
            q = g_utf8_prev_char (q);
          if (i > 0)
            break;
          text = g_locale_from_utf8 (q, p - q, NULL, &text_len, NULL);
          subst_offset = position - conv_data->factor;
          subst_nchars = conv_data->factor;
          break;

        case XIMForwardWord:
        case XIMBackwardWord:
        case XIMCaretUp:
        case XIMCaretDown:
        case XIMNextLine:
        case XIMPreviousLine:
        case XIMLineStart:
        case XIMLineEnd:
        case XIMAbsolutePosition:
        case XIMDontChange:
        default:
          break;
        }
      /* block out any failure happenning to "text", including conversion */
      if (text)
        {
          conv_data->text = (XIMStringConversionText *)
                              malloc (sizeof (XIMStringConversionText));
          if (conv_data->text)
            {
              conv_data->text->length = text_len;
              conv_data->text->feedback = NULL;
              conv_data->text->encoding_is_wchar = False;
              conv_data->text->string.mbs = (char *)malloc (text_len);
              if (conv_data->text->string.mbs)
                memcpy (conv_data->text->string.mbs, text, text_len);
              else
                {
                  free (conv_data->text);
                  conv_data->text = NULL;
                }
            }

          g_free (text);
        }
      if (conv_data->operation == XIMStringConversionSubstitution
          && subst_nchars > 0)
        {
          gtk_im_context_delete_surrounding ((GtkIMContext *)context_xim,
                                            subst_offset, subst_nchars);
        }

      g_free (surrounding);
    }
}


static XVaNestedList
set_preedit_callback (GtkIMContextOXIM *context_xim)
{
  context_xim->preedit_start_callback.client_data = (XPointer)context_xim;
  context_xim->preedit_start_callback.callback = (XIMProc)preedit_start_callback;
  context_xim->preedit_done_callback.client_data = (XPointer)context_xim;
  context_xim->preedit_done_callback.callback = (XIMProc)preedit_done_callback;
  context_xim->preedit_draw_callback.client_data = (XPointer)context_xim;
  context_xim->preedit_draw_callback.callback = (XIMProc)preedit_draw_callback;
  context_xim->preedit_caret_callback.client_data = (XPointer)context_xim;
  context_xim->preedit_caret_callback.callback = (XIMProc)preedit_caret_callback;
  return XVaCreateNestedList (0,
			      XNPreeditStartCallback, &context_xim->preedit_start_callback,
			      XNPreeditDoneCallback, &context_xim->preedit_done_callback,
			      XNPreeditDrawCallback, &context_xim->preedit_draw_callback,
			      XNPreeditCaretCallback, &context_xim->preedit_caret_callback,
			      NULL);
}

static XVaNestedList
set_status_callback (GtkIMContextOXIM *context_xim)
{
  context_xim->status_start_callback.client_data = (XPointer)context_xim;
  context_xim->status_start_callback.callback = (XIMProc)status_start_callback;
  context_xim->status_done_callback.client_data = (XPointer)context_xim;
  context_xim->status_done_callback.callback = (XIMProc)status_done_callback;
  context_xim->status_draw_callback.client_data = (XPointer)context_xim;
  context_xim->status_draw_callback.callback = (XIMProc)status_draw_callback;

  return XVaCreateNestedList (0,
			      XNStatusStartCallback, &context_xim->status_start_callback,
			      XNStatusDoneCallback, &context_xim->status_done_callback,
			      XNStatusDrawCallback, &context_xim->status_draw_callback,
			      NULL);
}


static void
set_string_conversion_callback (GtkIMContextOXIM *context_xim, XIC xic)
{
  if (!context_xim->im_info->supports_string_conversion)
    return;

  context_xim->string_conversion_callback.client_data = (XPointer)context_xim;
  context_xim->string_conversion_callback.callback = (XIMProc)string_conversion_callback;

  XSetICValues (xic,
		XNStringConversionCallback,
		(XPointer)&context_xim->string_conversion_callback,
		NULL);
}

static XIC
gtk_im_context_xim_get_ic (GtkIMContextOXIM *context_xim)
{
  if (context_xim->im_info == NULL || context_xim->im_info->im == NULL)
    return NULL;

  if (!context_xim->ic)
    {
      const char *name1 = NULL;
      XVaNestedList list1 = NULL;
      const char *name2 = NULL;
      XVaNestedList list2 = NULL;
      XIMStyle im_style = 0;
      XIC xic = NULL;

      if (context_xim->use_preedit &&
	  (context_xim->im_info->style & PREEDIT_MASK) == XIMPreeditCallbacks)
	{
	  im_style |= XIMPreeditCallbacks;
	  name1 = XNPreeditAttributes;
	  list1 = set_preedit_callback (context_xim);
	}
      else if ((context_xim->im_info->style & PREEDIT_MASK) == XIMPreeditNone)
	im_style |= XIMPreeditNone;
      else
	im_style |= XIMPreeditNothing;

      if ((context_xim->im_info->style & STATUS_MASK) == XIMStatusCallbacks)
	{
	  im_style |= XIMStatusCallbacks;
	  if (name1 == NULL)
	    {
	      name1 = XNStatusAttributes;
	      list1 = set_status_callback (context_xim);
	    }
	  else
	    {
	      name2 = XNStatusAttributes;
	      list2 = set_status_callback (context_xim);
	    }
	}
      else if ((context_xim->im_info->style & STATUS_MASK) == XIMStatusNone)
	im_style |= XIMStatusNone;
      else
	im_style |= XIMStatusNothing;

      xic = XCreateIC (context_xim->im_info->im,
		       XNInputStyle, im_style,
		       XNClientWindow, GDK_DRAWABLE_XID (context_xim->client_window),
		       name1, list1,
		       name2, list2,
		       NULL);

      if (list1)
	XFree (list1);
      if (list2)
	XFree (list2);

      if (xic)
	{
	  /* Don't filter key released events with XFilterEvents unless
	   * input methods ask for. This is a workaround for Solaris input
	   * method bug in C and European locales. It doubles each key
	   * stroke if both key pressed and released events are filtered.
	   * (bugzilla #81759)
	   */
	  gulong mask = 0xaaaaaaaa;
	  XGetICValues (xic,
			XNFilterEvents, &mask,
			NULL);
	  context_xim->filter_key_release = (mask & KeyReleaseMask) != 0;
	  set_string_conversion_callback (context_xim, xic);
	}

      context_xim->ic = xic;

      if (xic && context_xim->has_focus)
	XSetICFocus (xic);
    }
  return context_xim->ic;
}

/*****************************************************************
 * Status Window handling
 *****************************************************************/

/* Finds the GtkWidget that owns the window, or if none, the
 * widget owning the nearest parent that has a widget.
 */
static GtkWidget *
widget_for_window (GdkWindow *window)
{
  while (window)
    {
      gpointer user_data;
      gdk_window_get_user_data (window, &user_data);
      if (user_data)
	return user_data;

      window = gdk_window_get_parent (window);
    }

  return NULL;
}

/**
 * gtk_im_context_xim_shutdown:
 * 
 * Destroys all the status windows that are kept by the XIM contexts.  This
 * function should only be called by the XIM module exit routine.
 **/
void
gtk_im_context_xim_shutdown (void)
{
  while (open_ims)
    {
      GtkOXIMInfo *info = open_ims->data;
      GdkDisplay *display = gdk_screen_get_display (info->screen);

      xim_info_display_closed (display, FALSE, info);
      open_ims = g_slist_remove_link (open_ims, open_ims);
    }
}
