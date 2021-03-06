#ifndef __GTKINTL_H__
#define __GTKINTL_H__
#define GTK_LOCALEDIR "/usr/share/locale"
#define GETTEXT_PACKAGE "gtk20"
/*#define GTK_LOCALEDIR "@PREFIX@/@PKGLOCALEDIR@/locale"
#define GETTEXT_PACKAGE "oxim"*/


#include <langinfo.h>
#include <glib/gi18n-lib.h>

#ifdef ENABLE_NLS
#define P_(String) dgettext(GETTEXT_PACKAGE "-properties",String)
#else 
#define P_(String) (String)
#endif

/* not really I18N-related, but also a string marker macro */
#define I_(string) g_intern_static_string (string)

#endif
