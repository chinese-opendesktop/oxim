/*
 *
 */

#include <config.h>
#include <string.h>
#include <gtk/gtkimmodule.h>
#include "gtkintl.h"
#include "oximtool.h"

static int is_init = FALSE;

static const GtkIMContextInfo ogim_info = {
  "ogim",		   /* ID */
  N_("OXIM Input Method System"), /* Human readable name */
  GETTEXT_PACKAGE,	   /* Translation domain */
   GTK_LOCALEDIR,	   /* Dir for bindtextdomain (not strictly needed for "gtk+") */
  ""			   /* Languages for which this module is the default */
};

static const GtkIMContextInfo *info_list[] =
{
    &ogim_info
};

void
im_module_init(GTypeModule *module)
{
    if (!is_init)
    {
	printf("im_module_init\n");
	oxim_init();
	is_init = TRUE;
    }
    else
    {
	printf("Init finish.");
    }
    ogim_register_type(module);
}

void
im_module_exit(void)
{
    printf("im_module_exit\n");
}

void
im_module_list(const GtkIMContextInfo ***contexts, int *n_contexts)
{
    *contexts = info_list;
    *n_contexts = G_N_ELEMENTS(info_list);
}

GtkIMContext *
im_module_create(const gchar *context_id)
{
    if (strcmp(context_id, "ogim") == 0)
    {
	printf("im_module_create\n");
	return ogim_create();
    }
    return NULL;
}
