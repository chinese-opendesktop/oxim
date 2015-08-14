if ENABLE_UNICODE_MODULE

module_LTLIBRARIES = unicode.la

moduledir = $(MODULES_DIR)

INCLUDES = -I$(top_srcdir)/src/include

unicode_la_SOURCES = unicode.c unicode.h

unicode_la_LDFLAGS = \
        -avoid-version \
        -module

endif
