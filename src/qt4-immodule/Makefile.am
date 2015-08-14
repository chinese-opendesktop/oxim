if ENABLE_QT4_IMMODULE

module_LTLIBRARIES = lib-qt-im-oxim.la

moduledir = $(IM_MODULES_DIR)

INCLUDES = -I$(top_srcdir)/src/include $(XFT_CFLAGS) -DQT_NO_DEBUG -DQT_PLUGIN -DQT_SHARED -DQT_THREAD_SUPPORT $(QTCORE_CFLAGS) $(QTGUI_CFLAGS) $(QT3SUPPORT_CFLAGS) 

LIBS+= $(QTCORE_LIBS) $(QTGUI_LIBS) $(QT3SUPPORT_LIBS)

#qt_im_oxim_la_SOURCES = moc_qoximinputcontext.cpp \
#			moc_qoximinputcontextplugin.cpp \
#			qoximinputcontext.h \
#			qoximinputcontextplugin.cpp \
#			qoximinputcontextplugin.h \
#			qoximinputcontext_x11.cpp
lib_qt_im_oxim_la_SOURCES = moc_gcin-qt.cpp \
			gcin-qt.cpp \
			qoximinputcontextplugin.cpp \
			gcin-imcontext-qt.cpp 
#			qoximinputcontext_x11.cpp
#			moc_qoximinputcontext.cpp
lib_qt_im_oxim_la_LDFLAGS = \
			-avoid-vesion
			-module

lib_qt_im_oxim_la_LIBADD = $(QTGUI_LIBS) $(QTCORE_LIBS) \
			$(top_builddir)/src/lib/liboxim.la
install-exec-local:
	$(mkinstalldirs) $(DESTDIR)/$(QT4_IM_MODULE_DIR)
	ln -fs $(moduledir)/lib-qt-im-oxim.so $(DESTDIR)/$(QT4_IM_MODULE_DIR)/qt-im-oxim.so

uninstall-local:
	rm -f $(DESTDIR)/$(QT4_IM_MODULE_DIR)/qt-im-oxim.so
endif
