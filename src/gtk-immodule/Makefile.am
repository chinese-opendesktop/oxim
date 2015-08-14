if ENABLE_GTK_IMMODULE

module_LTLIBRARIES = gtk-im-oxim.la

moduledir = $(IM_MODULES_DIR)

INCLUDES = -I$(top_srcdir)/src/include $(GTK2_CFLAGS)

gtk_im_oxim_la_SOURCES = gtkimcontextoxim.c \
			 gtkimcontextoxim.h \
			 gtkintl.h \
			 im-oxim.c

gtk_im_oxim_la_LDFLAGS = \
			-avoid-version \
			-module

gtk_im_oxim_la_LIBADD = $(GTK2_LIBS)

install-exec-local:
	$(mkinstalldirs) $(DESTDIR)/$(GTK_IM_MODULE_DIR)
	ln -fs $(moduledir)/gtk-im-oxim.so $(DESTDIR)/$(GTK_IM_MODULE_DIR)/gtk-im-oxim.so

uninstall-local:
	rm -f $(DESTDIR)/$(GTK_IM_MODULE_DIR)/gtk-im-oxim.so
endif
