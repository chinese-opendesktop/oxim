
lib_LTLIBRARIES = liboxim.la

INCLUDES = -I$(top_srcdir) \
	   -I$(top_srcdir)/src/include \
	   -DOXIM_DEFAULT_DIR=\"$(OXIM_DEFAULT_DIR)\"

LIBS = $(DL_LIB) -lz

liboxim_la_SOURCES = \
	oximint.h \
	oxim_config.c \
	oxim_check_file.c \
	oxim_settings.c \
	oxim_qphrase.c \
	oxim_open_file.c \
	oxim_get_line.c \
	oxim_get_word.c \
	oxim_addslashes.c \
	oxim_module.c \
	oxim_malloc.c \
	oxim_perr.c \
	oxim_charcode.c \
	oxim_keymap.c \
	oxim_ascii_wb.c \
	s2t.c \
	oxim_set_locale.c \
	stable_sort.c \
	strcmp_wild.c \
	oxim_ucs4toutf8.c \
	oxim_utf8toucs4.c \
	oxim_utf8len.c \
	wchs_to_mbs.c \
	oxim_filter.c
