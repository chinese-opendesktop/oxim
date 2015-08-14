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

#include <gtk/gtkimcontext.h>
#include <gdk/gdkx.h>

G_BEGIN_DECLS

extern GType gtk_type_im_context_oxim;

#define GTK_TYPE_IM_CONTEXT_OXIM              gtk_type_im_context_oxim
#define GTK_IM_CONTEXT_OXIM(obj)              (GTK_CHECK_CAST ((obj), GTK_TYPE_IM_CONTEXT_OXIM, GtkIMContextOXIM))
#define GTK_IM_CONTEXT_OXIM_CLASS(klass)      (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_IM_CONTEXT_OXIM, GtkIMContextOXIMClass))
#define GTK_IS_IM_CONTEXT_OXIM(obj)           (GTK_CHECK_TYPE ((obj), GTK_TYPE_IM_CONTEXT_OXIM))
#define GTK_IS_IM_CONTEXT_OXIM_CLASS(klass)   (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_IM_CONTEXT_OXIM))
#define GTK_IM_CONTEXT_OXIM_GET_CLASS(obj)    (GTK_CHECK_GET_CLASS ((obj), GTK_TYPE_IM_CONTEXT_OXIM, GtkIMContextOXIMClass))

#define FALLBACK_LOCALE "en_US.UTF-8"

typedef struct _GtkIMContextOXIM       GtkIMContextOXIM;
typedef struct _GtkIMContextOXIMClass  GtkIMContextOXIMClass;

struct _GtkIMContextOXIMClass
{
  GtkIMContextClass parent_class;
};

void gtk_im_context_oxim_register_type (GTypeModule *type_module);
GtkIMContext *gtk_im_context_xim_new (void);

void gtk_im_context_xim_shutdown (void);

G_END_DECLS

#endif /* __GTK_IM_CONTEXT_OXIM_H__ */
