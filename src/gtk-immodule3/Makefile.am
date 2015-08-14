if ENABLE_GTK_IMMODULE3

module_LTLIBRARIES = gtk3-im-oxim.la

moduledir = $(IM_MODULES_DIR)

INCLUDES = -I$(top_srcdir)/src/include $(GTK3_CFLAGS)

gtk3_im_oxim_la_SOURCES = gtkimcontextoxim.c \
			 gtkimcontextoxim.h \
			 gtkintl.h \
			 im-oxim.c

gtk3_im_oxim_la_LDFLAGS = \
			-avoid-version \
			-module

gtk3_im_oxim_la_LIBADD = $(GTK3_LIBS)

install-exec-local:
	$(mkinstalldirs) $(DESTDIR)/$(GTK_IM_MODULE3_DIR)
	ln -fs $(moduledir)/gtk3-im-oxim.so $(DESTDIR)/$(GTK_IM_MODULE3_DIR)/gtk3-im-oxim.so

uninstall-local:
	rm -f $(DESTDIR)/$(GTK_IM_MODULE3_DIR)/gtk3-im-oxim.so
endif
