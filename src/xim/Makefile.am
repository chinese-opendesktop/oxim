bin_PROGRAMS = oxim oxim-agent

INCLUDES = -I$(top_srcdir)/src/include \
	   -I$(top_srcdir)/src/xim/IMdkit \
	   $(X11_INCLUDES) $(XFT_CFLAGS) $(XPM_CFLAGS) $(XTEST_CFLAGS) $(XRENDER_CFLAGS)

LIBS = $(DL_LIB) $(XFT_LIBS) $(X11_LIBS) $(XPM_LIBS) $(XTEST_LIBS) $(XRENDER_LIBS) -lz -lm -lXext -lfontconfig

oxim_SOURCES = oxim_main.c \
	       fkey.c \
	       xim.c \
	       xim_IC.c \
	       gui.c \
	       gui_preedit.c \
	       gui_xcin.c \
	       gui_fixed.c \
	       gui_status.c \
	       gui_tray.c \
	       gui_menu.c \
	       gui_symbol.c \
	       gui_keyboard.c \
	       gui_msgbox.c \
	       gui_selectmenu.c

oxim_LDADD = $(top_builddir)/src/xim/IMdkit/libIMdkit.la \
	     $(top_builddir)/src/lib/liboxim.la

oxim_agent_SOURCES = oxim-agent.c \
	       fkey.c \
	       xim.c \
	       xim_IC.c \
	       gui.c \
	       gui_preedit.c \
	       gui_xcin.c \
	       gui_fixed.c \
	       gui_status.c \
	       gui_tray.c \
	       gui_menu.c \
	       gui_symbol.c \
	       gui_keyboard.c \
	       gui_msgbox.c \
	       gui_selectmenu.c


oxim_agent_LDADD = $(top_builddir)/src/xim/IMdkit/libIMdkit.la \
	     $(top_builddir)/src/lib/liboxim.la
