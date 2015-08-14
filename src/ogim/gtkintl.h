#ifndef __GTKINTL_H__
#define __GTKINTL_H__
#define ENABLE_NLS 1
#define GTK_LOCALEDIR "/usr/share/locale"
#define GETTEXT_PACKAGE "gtk20"

#ifdef ENABLE_NLS
#include <libintl.h>
#define P_(String) dgettext(GETTEXT_PACKAGE "-properties",String)
#ifdef gettext_noop
#define N_(String) gettext_noop(String)
#else
#define N_(String) (String)
#endif
#else 
#define P_(String) (String)
#endif

#endif
