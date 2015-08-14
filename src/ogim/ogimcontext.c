
#include <gtk/gtk.h>
#include <gtk/gtkimcontext.h>
#include <gdk/gdkx.h>
#include "gtkintl.h"

#define GTK_TYPE_IM_CONTEXT_OGIM type_ogim_context
#define GTK_IM_CONTEXT_OGIM(obj) \
	(GTK_CHECK_CAST ((obj), GTK_TYPE_IM_CONTEXT_OGIM, GtkIMContextOGIM))
typedef struct _GtkIMContextOGIM       GtkIMContextOGIM;
typedef struct _GtkIMContextOGIMClass  GtkIMContextOGIMClass;

struct _GtkIMContextOGIMClass
{
    GtkIMContextClass parent_class;
};

struct _GtkIMContextOGIM
{
    GtkIMContext object;

    GdkWindow *client_window;
    GtkWidget *client_widget;

    guint focus : 1;
    GdkRectangle location; /* 游標位置 */
    guint is_active;	/* 輸入狀態 */
    guint inp_num;	/* 輸入法索引 */
    guint preedit_length;
    guint preedit_cursor;
    gunichar *preedit_chars;
};

static void ogim_class_init(GtkIMContextOGIMClass *class);
static void ogim_init(GtkIMContextOGIM *im_context);
static GObjectClass *parent_class;

static GtkWidget *key_window = NULL;
static GtkWidget *toplevel = NULL;

GType type_ogim_context = 0;

void
ogim_register_type(GTypeModule *module)
{
    static const GTypeInfo object_info =
    {
	sizeof (GtkIMContextOGIMClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc)ogim_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data */
	sizeof (GtkIMContextOGIM),
	0,
	(GInstanceInitFunc)ogim_init,
    };

    type_ogim_context = g_type_module_register_type (module,
			GTK_TYPE_IM_CONTEXT,
			"GtkIMContextOGIM",
			 &object_info, 0);
}

GtkIMContext *
ogim_create(void)
{
    return g_object_new(type_ogim_context, NULL);
}
static void
on_key_window_style_set (GtkWidget *toplevel,
			    GtkStyle  *previous_style,
			    GtkWidget *label)
{
  gint i;

  for (i = 0; i < 5; i++)
    gtk_widget_modify_fg (label, i, &toplevel->style->text[i]);
}

/* Draw the background (normally white) and border for the status window
 */
static gboolean
on_key_window_expose_event (GtkWidget      *widget,
			       GdkEventExpose *event)
{
  gdk_draw_rectangle (widget->window,
		      widget->style->base_gc [GTK_STATE_NORMAL],
		      TRUE,
		      0, 0,
		      widget->allocation.width, widget->allocation.height);
  gdk_draw_rectangle (widget->window,
		      widget->style->text_gc [GTK_STATE_NORMAL],
		      FALSE,
		      0, 0,
		      widget->allocation.width - 1, widget->allocation.height - 1);

  return FALSE;
}

static void
ogim_set_client_window(GtkIMContext *context, GdkWindow *client_window)
{
    printf("ogim_set_client_window\n");
    GtkIMContextOGIM *context_ogim = GTK_IM_CONTEXT_OGIM(context);

    GtkWidget *window;
    GtkWidget *label;

    context_ogim->client_window = client_window;

    gpointer user_data;
    gdk_window_get_user_data(context_ogim->client_window, &user_data);
    context_ogim->client_widget  = user_data;
    toplevel       = gtk_widget_get_toplevel(context_ogim->client_widget);

    if (!key_window)
    {
	key_window = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_window_set_resizable(GTK_WINDOW(key_window), FALSE);
	gtk_widget_set_app_paintable(key_window, TRUE);

	label = gtk_label_new(_("assemble\nword"));	//組\n字
	gtk_misc_set_padding(GTK_MISC(label), 1, 1);
	gtk_widget_show(label);
	g_signal_connect (key_window, "style_set",
		    G_CALLBACK (on_key_window_style_set), label);
	gtk_container_add(GTK_CONTAINER(key_window), label);

	g_signal_connect (key_window, "expose_event",
		    G_CALLBACK (on_key_window_expose_event), NULL);

	gtk_window_set_screen(GTK_WINDOW(key_window),
			 gtk_widget_get_screen(toplevel));

	gint height = gdk_screen_get_height(gtk_widget_get_screen(toplevel));
	gtk_widget_show(key_window);
	printf("Height = %d\n", height);
    }

}

static gboolean
ogim_filter_key(GtkIMContext *context, GdkEventKey *event)
{
    GtkIMContextOGIM *context_ogim = GTK_IM_CONTEXT_OGIM(context);
    gboolean retval = FALSE;
    if (event->state & GDK_CONTROL_MASK)
    {
	switch (event->keyval)
	{
	    case ' ': // Ctrl+Space
	    {
		printf("Ctrl+Space\n");
		context_ogim->is_active = !context_ogim->is_active;
		return FALSE;
		break;
	    }
	}
    }
    return TRUE;
}

static gboolean
ogim_filter_keypress(GtkIMContext *context, GdkEventKey *event)
{
    GtkIMContextOGIM *context_ogim = GTK_IM_CONTEXT_OGIM(context);
    gboolean retval = FALSE;
    guint32 ucs4;

    if (event->type == GDK_KEY_RELEASE)
    {
	return FALSE;
    }

    if (!GDK_IS_WINDOW(context_ogim->client_window))
    {
	return FALSE;
    }

    printf("ogim_filter_keypress(%d)\n", context_ogim->is_active);
    if (!ogim_filter_key(context, event))
	return TRUE;

    ucs4 = gdk_keyval_to_unicode (event->keyval);
    if (ucs4)
    {
	guchar utf8[10];
	int len = g_unichar_to_utf8 (ucs4, utf8);
	utf8[len] = 0;
	g_signal_emit_by_name (context_ogim, "commit", utf8);
	retval = TRUE;
    }

    return retval;
}

static void
ogim_focus_in(GtkIMContext *context)
{
    printf("ogim_focus_in\n");
    GtkIMContextOGIM *context_ogim = GTK_IM_CONTEXT_OGIM(context);

    if (!GDK_IS_WINDOW(context_ogim->client_window))
    {
	return;
    }
    context_ogim->focus = TRUE;
}

static void
ogim_focus_out(GtkIMContext *context)
{
    printf("ogim_focus_out\n");
    GtkIMContextOGIM *context_ogim = GTK_IM_CONTEXT_OGIM(context);

    if (!GDK_IS_WINDOW(context_ogim->client_window))
    {
	return;
    }
    context_ogim->focus = FALSE;
}

static void
ogim_set_cursor_location(GtkIMContext *context, GdkRectangle *area)
{
    GtkIMContextOGIM *context_ogim = GTK_IM_CONTEXT_OGIM(context);

    if (context_ogim->location.x == area->x &&
	context_ogim->location.y == area->y &&
	context_ogim->location.width == area->width &&
	context_ogim->location.height == area->height)
    {
	return;
    }

    printf("ogim_set_cursor_location(x=%d,y=%d)\n", area->x + area->width, area->y + area->height);
    context_ogim->location = *area;

    gint x, y;

    gdk_window_get_origin(context_ogim->client_window, &x, &y);
    printf("rect.x=%d, rect.y=%d\n", x, y);
    gtk_window_move(GTK_WINDOW(key_window), x+area->x, y+area->y+area->height);
}

static void
ogim_set_use_preedit(GtkIMContext *context, gboolean use_preedit)
{
    printf("ogim_set_use_preedit = %d\n", use_preedit);
    GtkIMContextOGIM *context_ogim = GTK_IM_CONTEXT_OGIM(context);
}

static void
ogim_reset(GtkIMContext *context)
{
    printf("ogim_reset\n");
    GtkIMContextOGIM *context_ogim = GTK_IM_CONTEXT_OGIM(context);
}

static void
ogim_get_preedit_string(GtkIMContext *context, gchar **str,
			PangoAttrList **attrs, gint *cursor_pos)
{
    printf("ogim_get_preedit_string\n");
    GtkIMContextOGIM *context_ogim = GTK_IM_CONTEXT_OGIM(context);

    gchar *utf8 = g_ucs4_to_utf8 (context_ogim->preedit_chars, context_ogim->preedit_length, NULL, NULL, NULL);

    if (attrs)
    {
	*attrs = pango_attr_list_new();
    }

    if (str)
    {
	*str = utf8;
    }
    else
    {
	g_free(utf8);
    }

    if (cursor_pos)
    {
	*cursor_pos = context_ogim->preedit_cursor;
    }
}

static void
ogim_class_init(GtkIMContextOGIMClass *class)
{
    printf("ogim_class_init\n");
    GtkIMContextClass *im_context_class = GTK_IM_CONTEXT_CLASS(class);
    GObjectClass *gobject_class = G_OBJECT_CLASS(class);
    parent_class = g_type_class_peek_parent(class);

    im_context_class->set_client_window = ogim_set_client_window;
    im_context_class->filter_keypress = ogim_filter_keypress;
    im_context_class->reset = ogim_reset;
    im_context_class->get_preedit_string = ogim_get_preedit_string;
    im_context_class->focus_in = ogim_focus_in;
    im_context_class->focus_out = ogim_focus_out;
    im_context_class->set_cursor_location = ogim_set_cursor_location;
    im_context_class->set_use_preedit = ogim_set_use_preedit;
//    gobject_class->finalize = NULL;
}

static void
ogim_init(GtkIMContextOGIM *im_context)
{
    GtkWidget *window;
    GtkWidget *label;

    printf("ogim_init default_inp = %d\n",oxim_get_Default_IM());
    im_context->client_window   = NULL;
    im_context->focus           = FALSE;
    im_context->location.x      = 0;
    im_context->location.y      = 0;
    im_context->location.width  = 0;
    im_context->location.height = 0;
    im_context->is_active	= FALSE;
    im_context->inp_num         = oxim_get_Default_IM();
    im_context->preedit_cursor  = 0;
    im_context->preedit_length  = 0;
    im_context->preedit_chars   = NULL;



}
