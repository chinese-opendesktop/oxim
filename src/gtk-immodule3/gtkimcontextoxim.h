/* GTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat Software
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

#ifndef __GTK_IM_CONTEXT_OXIM_H__
#define __GTK_IM_CONTEXT_OXIM_H__

#include <gtk/gtk.h>
//#include <gtk/gtkimcontext.h>
#include <gdk/gdkx.h>


#define GTK_TYPE_IM_CONTEXT_OXIM              (gtk_oxim_context_get_type())
#define GTK_IM_CONTEXT_OXIM(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_IM_CONTEXT_OXIM, GtkIMContextOXIM))
#define GTK_IM_CONTEXT_OXIM_CLASS(klass)      (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_IM_CONTEXT_OXIM, GtkIMContextOXIMClass))
#define GTK_IS_IM_CONTEXT_OXIM(obj)           (GTK_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_IM_CONTEXT_OXIM))
#define GTK_IS_IM_CONTEXT_OXIM_CLASS(klass)   (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_IM_CONTEXT_OXIM))
#define GTK_IM_CONTEXT_OXIM_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_IM_CONTEXT_OXIM, GtkIMContextOXIMClass))

#define FALLBACK_LOCALE "en_US.UTF-8"

G_BEGIN_DECLS

GType    gtk_oxim_context_get_type (void);
typedef struct _GtkIMContextOXIM       GtkIMContextOXIM;
typedef struct _GtkIMContextOXIMClass  GtkIMContextOXIMClass;

struct _GtkIMContextOXIMClass
{
  GtkIMContextClass parent_class;
#if 0
// test/
  /*< public >*/
  /* Signals */
  void     (*preedit_start)        (GtkIMContext *context);
  void     (*preedit_end)          (GtkIMContext *context);
  void     (*preedit_changed)      (GtkIMContext *context);
  void     (*commit)               (GtkIMContext *context, const gchar *str);
  gboolean (*retrieve_surrounding) (GtkIMContext *context);
  gboolean (*delete_surrounding)   (GtkIMContext *context,
                                    gint          offset,
                                    gint          n_chars);

  /* Virtual functions */
  void     (*set_client_window)   (GtkIMContext   *context,
                                   GdkWindow      *window);
  void     (*get_preedit_string)  (GtkIMContext   *context,
                                   gchar         **str,
                                   PangoAttrList **attrs,
                                   gint           *cursor_pos);
  gboolean (*filter_keypress)     (GtkIMContext   *context,
                                   GdkEventKey    *event);
  void     (*focus_in)            (GtkIMContext   *context);
  void     (*focus_out)           (GtkIMContext   *context);
  void     (*reset)               (GtkIMContext   *context);
  void     (*set_cursor_location) (GtkIMContext   *context,
                                   GdkRectangle   *area);
  void     (*set_use_preedit)     (GtkIMContext   *context,
                                   gboolean        use_preedit);
  void     (*set_surrounding)     (GtkIMContext   *context,
                                   const gchar    *text,
                                   gint            len,
                                   gint            cursor_index);
  gboolean (*get_surrounding)     (GtkIMContext   *context,
                                   gchar         **text,
                                   gint           *cursor_index);
  /*< private >*/
  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
  void (*_gtk_reserved5) (void);
  void (*_gtk_reserved6) (void);
#endif
};

void gtk_im_context_oxim_register_type (GTypeModule *type_module);
//GtkIMContext *gtk_im_context_xim_new (void);
GtkIMContextOXIM *gtk_im_context_xim_new (void);

void gtk_im_context_xim_shutdown (void);

G_END_DECLS

#endif /* __GTK_IM_CONTEXT_OXIM_H__ */
