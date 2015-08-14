if ENABLE_SUNPINYIN_MODULE

#SRC_DIR = $(top_srcdir)/src

module_LTLIBRARIES = sunpinyin.la

INCLUDES = -I$(top_srcdir)/src/include $(SUNPINYIN_CFLAGS) $(ICONV_INC)

moduledir = $(MODULES_DIR)

#sunpinyin_la_SOURCES = sunpinyin.cpp sunpinyinh.h imi_win.h imi_win.cpp
sunpinyin_la_SOURCES =  imi_win.cpp sunpinyin.cpp

sunpinyin_la_CXXFLAGS = -I$(top_builddir) -I$(top_builddir)/src 
sunpinyin_la_LDFLAGS = -lm $(SUNPINYIN_LIBS) $(ICONV_LIB) \
        -avoid-version \
        -module
#sunpinyinoxim_la_LIBADD = $(SUNPINYIN_LIBS) $(ICONV_LIB)

endif
