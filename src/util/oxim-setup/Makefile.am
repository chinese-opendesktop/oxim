if ENABLE_SETUP_TOOL

bin_PROGRAMS = oxim-setup

INCLUDES = -I$(top_srcdir)/src/include

oxim_setup_SOURCES = oxim-setup.pro \
		    oxim-setup.ui \
		    oxim-setup.ui.h \
		    globolsetting.ui \
		    globolsetting.ui.h
		    gencin.ui \
		    gencin.ui.h \
		    chewing.h \
		    installim.h \
		    installim.ui.h \
		    main.cpp 

oxim-setup: oxim-setup.pro
	$(QTDIR)/bin/qmake oxim-setup.pro -o Makefile.qt
	QTDIR=$(QTDIR) make -f Makefile.qt

CLEANFILES = Makefile.qt .ui/* .moc/* .obj/*

endif
