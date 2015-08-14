if ENABLE_QT_IMMODULE

module_LTLIBRARIES = qt-im-oxim.la

moduledir = $(IM_MODULES_DIR)

#INCLUDES = -I$(top_srcdir)/src/include -I$(QTDIR)/include -I$(QTDIR)/mkspecs/default $(XFT_CFLAGS) -DQT_NO_DEBUG -DQT_PLUGIN -DQT_SHARED -DQT_THREAD_SUPPORT -DQT_XIM_DEBUG
INCLUDES = -I$(top_srcdir)/src/include -I$(QTDIR)/include -I$(QTDIR)/mkspecs/default $(XFT_CFLAGS) -DQT_PLUGIN -DQT_SHARED -DQT_THREAD_SUPPORT -DQT_XIM_DEBUG

qt_im_oxim_la_SOURCES = moc_qoximinputcontext.cpp \
			moc_qoximinputcontextplugin.cpp \
			qoximinputcontext.h \
			qoximinputcontextplugin.cpp \
			qoximinputcontextplugin.h \
			qoximinputcontext_x11.cpp

qt_im_oxim_la_LDFLAGS = \
			-avoid-version \
			-module

qt_im_oxim_la_LIBADD = -L$(QTDIR)/lib -lqt-mt \
			$(top_builddir)/src/lib/liboxim.la

install-exec-local:
	$(mkinstalldirs) $(DESTDIR)/$(QT_IM_MODULE_DIR)
	ln -fs $(moduledir)/qt-im-oxim.so $(DESTDIR)/$(QT_IM_MODULE_DIR)/qt-im-oxim.so

uninstall-local:
	rm -f $(DESTDIR)/$(QT_IM_MODULE_DIR)/qt-im-oxim.so
endif
