if ENABLE_QT4_IMMODULE

module_LTLIBRARIES = qt-im-oxim.la

moduledir = $(IM_MODULES_DIR)

#INCLUDES = -I$(top_srcdir)/src/include -I$(QTDIR)/include -I$(QTDIR)/mkspecs/default $(XFT_CFLAGS) -DQT_NO_DEBUG -DQT_PLUGIN -DQT_SHARED -DQT_THREAD_SUPPORT `pkg-config --cflags --libs QtGui`
#INCLUDES = -I$(top_srcdir)/src/include $(XFT_CFLAGS) -DQT_NO_DEBUG -DQT_PLUGIN -DQT_SHARED -DQT_THREAD_SUPPORT `pkg-config --cflags --libs QtGui Qt3Support`
INCLUDES = -I$(top_srcdir)/src/include $(XFT_CFLAGS) -DQT_NO_DEBUG -DQT_PLUGIN -DQT_SHARED -DQT_THREAD_SUPPORT $(QTGUI_CFLAGS) $(QT3SUPPORT_CFLAGS)

LIBS+=$(QTGUI_LIBS) $(QT3SUPPORT_LIBS)

qt_im_oxim_la_SOURCES = moc_qoximinputcontext.cpp \
			moc_qoximinputcontextplugin.cpp \
			qoximinputcontext.h \
			qoximinputcontextplugin.cpp \
			qoximinputcontextplugin.h \
			qoximinputcontext_x11.cpp

qt_im_oxim_la_LDFLAGS = \
			-avoid-version \
			-module

qt_im_oxim_la_LIBADD = -L$(QTDIR)/lib \
			$(top_builddir)/src/lib/liboxim.la
install-exec-local:
	$(mkinstalldirs) $(DESTDIR)/$(QT4_IM_MODULE_DIR)
	ln -fs $(moduledir)/qt-im-oxim.so $(DESTDIR)/$(QT4_IM_MODULE_DIR)/qt-im-oxim.so

uninstall-local:
	rm -f $(DESTDIR)/$(QT4_IM_MODULE_DIR)/qt-im-oxim.so
endif
