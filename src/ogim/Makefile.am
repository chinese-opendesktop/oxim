if ENABLE_GTK_IMMODULE

module_LTLIBRARIES = ogim.la

moduledir = $(IM_MODULES_DIR)

INCLUDES = -I$(top_srcdir)/src/include $(GTK2_CFLAGS)

ogim_la_SOURCES = ogim.c \
		  ogimcontext.c \
		  gtkintl.h

ogim_la_LDFLAGS = \
		-avoid-version \
		-module

ogim_la_LIBADD = $(GTK2_LIBS) $(top_builddir)/src/lib/liboxim.la

install-exec-local:
	$(mkinstalldirs) $(DESTDIR)/$(GTK_IM_MODULE_DIR)
	ln -fs $(moduledir)/ogim.so $(DESTDIR)/$(GTK_IM_MODULE_DIR)/ogim.so

uninstall-local:
	rm -f $(DESTDIR)/$(GTK_IM_MODULE_DIR)/ogim.so
endif
