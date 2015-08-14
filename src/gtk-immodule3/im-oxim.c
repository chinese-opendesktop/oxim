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

#include <config.h>

#include <gtk/gtk.h>
#include <gtk/gtkimmodule.h>

#include "gtkintl.h"
//#include "gtk/gtkimmodule.h"
#include "gtkimcontextoxim.h"
#include <string.h>

static const GtkIMContextInfo oxim_zh_info = { 
  "oxim",		           /* ID */
  N_("Open X Input Method"),         /* Human readable name */
  GETTEXT_PACKAGE,		   /* Translation domain */
  GTK_LOCALEDIR,		   /* Dir for bindtextdomain (not strictly needed for "gtk+") */
  ""		           /* Languages for which this module is the default */
};

static const GtkIMContextInfo *info_list[] = {
  &oxim_zh_info
};

G_MODULE_EXPORT const gchar*
g_module_check_init (GModule *module)
{
    return glib_check_version (GLIB_MAJOR_VERSION,
                               GLIB_MINOR_VERSION,
                               0);
}

G_MODULE_EXPORT void
im_module_init (GTypeModule *type_module)
//MODULE_ENTRY (void, init)  (GTypeModule *type_module)
{
    /* make module resident */
    g_type_module_use (type_module);
//    ibus_init ();
g_type_init();
  gtk_im_context_xim_register_type (type_module);
}

G_MODULE_EXPORT void 
im_module_exit (void)
//MODULE_ENTRY (void, exit) (void)
{
  gtk_im_context_xim_shutdown ();
}

G_MODULE_EXPORT void 
im_module_list (const GtkIMContextInfo ***contexts,
//MODULE_ENTRY (void, list) (const GtkIMContextInfo ***contexts,
		int                      *n_contexts)
{
  *contexts = info_list;
  *n_contexts = G_N_ELEMENTS (info_list);
}

G_MODULE_EXPORT GtkIMContext *
im_module_create (const gchar *context_id)
//MODULE_ENTRY (GtkIMContext *, create) (const gchar *context_id)
{
  if (strcmp (context_id, "oxim") == 0)
  {
    GtkIMContextOXIM *context;
//g_debug("[%d]%s\n", __LINE__, __FILE__);
    context = gtk_im_context_xim_new ();
//g_debug("[%d]%s\n", __LINE__, __FILE__);
    return (GtkIMContext *) context;
  }
  else
    return NULL;
}
