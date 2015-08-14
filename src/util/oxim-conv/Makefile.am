
# bin_PROGRAMS = oxim-conv
bin_PROGRAMS = oxim2tab

INCLUDES = -I$(top_srcdir)/src/include

oxim2tab_SOURCES = gencin.c  \
		   oxim-conv.c oxim-conv.h

oxim2tab_LDADD = $(top_builddir)/src/lib/liboxim.la \
		 -lz

# install-exec-local:
# 	$(mkinstalldirs) $(am__installdirs)
# 	ln -fs oxim-conv $(am__installdirs)/oxim2tab

# uninstall-local:
# 	rm -f $(am__installdirs)/oxim2tab
