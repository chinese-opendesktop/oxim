if ENABLE_CHEWING_MODULE

module_LTLIBRARIES = chewing.la

INCLUDES = -I$(top_srcdir)/src/include $(ICONV_INC) \
	    $(CHEWING_CFLAGS) -DCHEWING_DATA_DIR="\"$(CHEWING_DATA_DIR)\""

moduledir = $(MODULES_DIR)

if CHEWING_VER032
chewing_la_SOURCES = chewing.0.3.2.c
endif
if CHEWING_VER031
chewing_la_SOURCES = chewing.c
endif
if CHEWING_VER030
chewing_la_SOURCES = chewing2.c
endif

chewing_la_LDFLAGS = \
        -avoid-version \
        -module

chewing_la_LIBADD = $(CHEWING_LIBS) $(ICONV_LIB)

endif
