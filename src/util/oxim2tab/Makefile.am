
bin_PROGRAMS = oxim2tab

INCLUDES = -I$(top_srcdir)/src/include

oxim2tab_SOURCES = gencin.c  \
		   oxim2tab.c oxim2tab.h

oxim2tab_LDADD = $(top_builddir)/src/lib/liboxim.la \
		 -lz

install-exec-local:
	$(mkinstalldirs) $(am__installdirs)
	ln -fs oxim2tab $(am__installdirs)/oxim2cin

uninstall-local:
	rm -f $(am__installdirs)/oxim2cin
