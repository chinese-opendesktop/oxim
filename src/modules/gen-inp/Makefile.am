
module_LTLIBRARIES = gen-inp.la

INCLUDES = -I$(top_srcdir)/src/include

LIBS = -lz

moduledir = $(MODULES_DIR)

gen_inp_la_SOURCES = gen-inp.h gen-inp.c

gen_inp_la_LDFLAGS = \
        -avoid-version \
        -module
