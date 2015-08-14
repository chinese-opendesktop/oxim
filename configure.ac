#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# $Id: configure.ac,v 1.89 2011/08/10 07:03:37 wind Exp $

AC_PREREQ(2.59)
AC_INIT([oxim], 1.5.4, [firefly@opendesktop.org.tw])
AM_INIT_AUTOMAKE($PACKAGE_NAME, $PACKAGE_VERSION)

AC_PREFIX_DEFAULT([/usr])  # 預設安裝路徑
AC_CONFIG_SRCDIR([COPYING])
AC_CONFIG_HEADER([config.h])


# i18N
AM_GNU_GETTEXT([external])
PACKAGE_GETTEXT=gettext
VERSION_GETTEXT=1.2.1
AC_SUBST(PACKAGE_GETTEXT)
AC_SUBST(VERSION_GETTEXT)


# Checks for programs.
AC_DISABLE_STATIC
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)

# Checks for header files.
AC_PATH_X
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_MAJOR
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h float.h langinfo.h limits.h locale.h stdlib.h string.h assert.h getopt.h sys/param.h sys/time.h sys/timeb.h sys/mman.h unistd.h utime.h zlib.h libintl.h iconv.h])

# Checks for library
AC_CHECK_LIB(intl, gettext)
AC_CHECK_LIB(iconv, iconv)

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_STRUCT_ST_BLKSIZE
AC_STRUCT_ST_BLOCKS
AC_STRUCT_ST_RDEV

AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_STRUCT_ST_BLOCKS
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_HEADER_TIME
AC_STRUCT_TM

AC_CHECK_SIZEOF(char,      [ 1 ])
AC_CHECK_SIZEOF(short,     [ 2 ])
AC_CHECK_SIZEOF(int,       [ 4 ])
AC_CHECK_SIZEOF(long,      [ 4 ])
AC_CHECK_SIZEOF(long long, [ 8 ])
AC_CHECK_SIZEOF(int *,     [ 4 ])

# Checks for library functions.
#AC_FUNC_CHOWN
#AC_FUNC_CLOSEDIR_VOID
#AC_REPLACE_FNMATCH
#AC_FUNC_FORK
#AC_FUNC_GETPGRP
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
#AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_STAT
#AC_FUNC_STRFTIME
#AC_FUNC_UTIME_NULL
#AC_FUNC_VPRINTF
AC_FUNC_MMAP
AC_CHECK_FUNCS([alarm bzero endpwent fchdir getcwd gethostname getpass gettimeofday lchown memchr memmove memset mkdir nl_langinfo putenv rmdir select setlocale strcasecmp strchr strcspn strdup strerror strncasecmp strrchr strspn strstr strtol strtoul tzset utime gettext iconv])
#-----------------------------------------------------------------------
# 路徑設定
#-----------------------------------------------------------------------
AC_ARG_WITH([conf-dir],
    AS_HELP_STRING(
    	[--with-conf-dir=PATH],
	[Use DIR to store configuration files (default /etc/oxim)]),
	[confdir="$withval"],
	[confdir="yes"])

case "$confdir" in
no|yes)
        confdir="/etc/oxim"
        ;;
*)
        ;;
esac

AC_SUBST(confdir)
OXIM_DEFAULT_RCDIR=${confdir}
AC_DEFINE_UNQUOTED(OXIM_DEFAULT_RCDIR, "$OXIM_DEFAULT_RCDIR",[OXIM configuration directory])
AC_SUBST(OXIM_DEFAULT_RCDIR)

default_rc="oxim.conf"
AC_SUBST(default_rc)
OXIM_DEFAULT_RC=${default_rc}
AC_DEFINE_UNQUOTED(OXIM_DEFAULT_RC, "$OXIM_DEFAULT_RC",[OXIM configuration file])
AC_SUBST(OXIM_DEFAULT_RC)

locale_list="locale.list"
AC_SUBST(locale_list)
OXIM_DEFAULT_LOCALE_LIST=${locale_list}
AC_DEFINE_UNQUOTED(OXIM_DEFAULT_LOCALE_LIST, "$OXIM_DEFAULT_LOCALE_LIST",[OXIM support locale list file])
AC_SUBST(OXIM_DEFAULT_LOCALE_LIST)

user_dir=".oxim"
AC_SUBST(user_dir)
OXIM_USER_DIR=${user_dir}
AC_DEFINE_UNQUOTED(OXIM_USER_DIR, "$OXIM_USER_DIR",[OXIM user home dir])
AC_SUBST(OXIM_USER_DIR)

AC_DEFINE_UNQUOTED(OXIM_MIRROR_SITE_URL, "http://opendesktop.org.tw",[OXIM mirror site URL])

#-----------------------------------------------------------------------
# Check for dlopen support
#-----------------------------------------------------------------------
AC_CHECK_FUNCS(dlopen, [ found=y ], [ found=n ])
if test $found = n; then
    AC_CHECK_LIB(dl, dlopen,
        [ found=y; DL_LIB=-ldl; AC_DEFINE(HAVE_DLOPEN) ], [ found=n ])
fi

if test $found = n; then
    AC_CHECK_FUNCS(shl_load, [ found=y ], [ found=n ])
fi
if test $found = n; then
    for f in dl dld; do
        AC_CHECK_LIB($f, shl_load,
            [ found=y; DL_LIB="-l$f"
              AC_DEFINE(HAVE_SHL_LOAD) ], [ found=n ])
        test $found = y && break
    done
fi
AC_SUBST(DL_LIB)

#-----------------------------------------------------------------------
# Check for XPM support
#-----------------------------------------------------------------------
PKG_CHECK_MODULES(XPM, xpm >= 2.0.0, use_xpm="yes", use_xpm="no")
if test "$use_xpm" = "no" ; then
    AC_CHECK_HEADERS([X11/xpm.h], [use_xpm="yes"], [use_xpm="no"])
    if test "$use_xpm" = "yes" ; then
	XPM_CFLAGS=""
	AC_SUBST(XPM_CFLAGS)
	XPM_LIBS="-lX11 -lXpm"
	AC_SUBST(XPM_LIBS)
    else
	AC_MSG_ERROR([required XPM header file missing])
    fi
fi

#-----------------------------------------------------------------------
# Check for XTEST support
#-----------------------------------------------------------------------
PKG_CHECK_MODULES(XTEST, xtst >= 1.0.0, use_xtst="yes", use_xtst="no")
if test "$use_xtst" = "no" ; then
    AC_CHECK_HEADERS([X11/extensions/XTest.h], [use_xtst="yes"], [use_xtst="no"])
    if test "$use_xtst" = "yes" ; then
	XTEST_CFLAGS=""
	AC_SUBST(XTEST_CFLAGS)
	XTEST_LIBS="-lXtst"
	AC_SUBST(XTEST_LIBS)
    else
	AC_MSG_ERROR([required XTEST header file missing])
    fi
fi

#-----------------------------------------------------------------------
# Check for Xft support
#-----------------------------------------------------------------------
PKG_CHECK_MODULES(XFT, xft >= 2.0, use_xft="yes", use_xft="no")
if test "$use_xft" = "no" ; then
    AC_MSG_ERROR([oxim need libxft, but not found or version too old.])
fi

#-----------------------------------------------------------------------
# Check for Xrender support
#-----------------------------------------------------------------------
PKG_CHECK_MODULES(XRENDER, xrender >= 0.9, use_xrender="yes", use_xrender="no")
if test "$use_xrender" = "no" ; then
    AC_MSG_ERROR([oxim need libXrender, but not found or version too old.])
fi

#-----------------------------------------------------------------------
#
#-----------------------------------------------------------------------
AC_ARG_ENABLE([chewing-module],
    AS_HELP_STRING(
        [--enable-chewing-module],
        [Enable build chewing module (default=auto)]),
        [],
        [enable_chewing_module="yes"])

if test "$enable_chewing_module" = "yes" ; then
    if $PKG_CONFIG --exists chewing ; then
	# 檢查版本
        #PKG_CHECK_MODULES(CHEWING030, chewing < 0.3.0, serial030="yes", serial031="yes")
        PKG_CHECK_MODULES(CHEWING032, chewing >= 0.3.2, serial032="yes", 
        [
        PKG_CHECK_MODULES(CHEWING030, chewing < 0.3.0, serial030="yes", serial030="no")
        PKG_CHECK_MODULES(CHEWING031, chewing >= 0.3.0, serial031="yes", serial031="no")
        ]
        )
        
	CHEWING_CFLAGS=`$PKG_CONFIG chewing --cflags`
	AC_SUBST(CHEWING_CFLAGS)

	CHEWING_LIBS=`$PKG_CONFIG chewing --libs`
	AC_SUBST(CHEWING_LIBS)

	CHEWING_DATA_DIR=`$PKG_CONFIG chewing --variable=datadir`
	AC_SUBST(CHEWING_DATA_DIR)
    else
	AC_MSG_RESULT([no]);
	enable_chewing_module="no"
    fi
fi
AM_CONDITIONAL(ENABLE_CHEWING_MODULE, [ test "$enable_chewing_module" = "yes" ])
AM_CONDITIONAL(CHEWING_VER030, [ test "$serial030" = "yes" ])
AM_CONDITIONAL(CHEWING_VER031, [ test "$serial031" = "yes" ])
AM_CONDITIONAL(CHEWING_VER032, [ test "$serial032" = "yes" ])

#-----------------------------------------------------------------------
# UniCode Module
#-----------------------------------------------------------------------
AC_ARG_ENABLE([unicode-module],
    AS_HELP_STRING(
        [--enable-unicode-module],
        [Enable build unicode module (default=yes)]),
        [],
        [enable_unicode_module="yes"])
AM_CONDITIONAL(ENABLE_UNICODE_MODULE, [ test "$enable_unicode_module" = "yes" ])

#-----------------------------------------------------------------------
#  Check for Sunpinyin 
#-----------------------------------------------------------------------
AC_ARG_ENABLE([sunpinyin],
    AS_HELP_STRING(
        [--enable-sunpinyin-module],
        [Enable build sunpinyin module (default=auto)]),
        [],
        [enable_sunpinyin_module="yes"])

if test "$enable_sunpinyin_module" = "yes" ; then
    PKG_CHECK_MODULES(SUNPINYIN, sunpinyin-2.0 >= 2.0.0, use_sunpinyin="yes", use_sunpinyin="no")
    if test "$use_sunpinyin" = "no" ; then
	AC_MSG_WARN([SUNPINYIN version must be > 2.0.0!])
	enable_sunpinyin_module="no"
    fi
    #GTK_IM_MODULE3_DIR=`$PKG_CONFIG gtk+-3.0 --variable=libdir`/gtk-3.0/immodules
    #AC_SUBST(GTK_IM_MODULE3_DIR)
fi
AM_CONDITIONAL(ENABLE_SUNPINYIN_MODULE, [ test "$enable_sunpinyin_module" = "yes" ])

#-----------------------------------------------------------------------
# For EeePC
#-----------------------------------------------------------------------
AC_ARG_ENABLE([EeePC],
    AS_HELP_STRING(
        [--enable-EeePC],
        [Enable build for EeePC (default=no)]),
        [],
        [enable_EeePC="no"])
AM_CONDITIONAL(ENABLE_EEEPC, [ test "$enable_EeePC" = "yes" ])
if test "$enable_EeePC" = "yes" ; then
    AC_DEFINE_UNQUOTED(ENABLE_EEEPC, [1], [EEEPC environment])
fi

#-----------------------------------------------------------------------
# For Device
#-----------------------------------------------------------------------
AC_ARG_ENABLE([Device],
    AS_HELP_STRING(
        [--enable-Device],
        [Enable build for Device (default=no)]),
        [],
        [enable_Device="no"])
AM_CONDITIONAL(ENABLE_DEVICE, [ test "$enable_Device" = "yes" ])
if test "$enable_Device" = "yes" ; then
    AC_DEFINE_UNQUOTED(ENABLE_DEVICE, [1], [Device Setup])
fi

#-----------------------------------------------------------------------
#  Check for Gtk2 Library ***
#-----------------------------------------------------------------------
AC_ARG_ENABLE([gtk-immodule],
    AS_HELP_STRING(
        [--enable-gtk-immodule],
        [Enable build gtk2 input method extension (default=auto)]),
        [],
        [enable_gtk_immodule="yes"])

if test "$enable_gtk_immodule" = "yes" ; then
    PKG_CHECK_MODULES(GTK2, gtk+-2.0 >= 2.2.0, use_gtk2="yes", use_gtk2="no")
    if test "$use_gtk2" = "no" ; then
	AC_MSG_WARN([Gtk2 version must be > 2.2.0!])
	enable_gtk_immodule="no"
    fi
    GTK_IM_MODULE_DIR=`$PKG_CONFIG gtk+-2.0 --variable=libdir`/gtk-2.0/immodules
    AC_SUBST(GTK_IM_MODULE_DIR)
fi
AM_CONDITIONAL(ENABLE_GTK_IMMODULE, [ test "$enable_gtk_immodule" = "yes" ])

#-----------------------------------------------------------------------
#  Check for Gtk3 Library ***
#-----------------------------------------------------------------------
AC_ARG_ENABLE([gtk-immodule3],
    AS_HELP_STRING(
        [--enable-gtk-immodule3],
        [Enable build gtk3 input method extension (default=auto)]),
        [],
        [enable_gtk_immodule3="yes"])

if test "$enable_gtk_immodule3" = "yes" ; then
    PKG_CHECK_MODULES(GTK3, gtk+-3.0 >= 3.0.0, use_gtk3="yes", use_gtk3="no")
    if test "$use_gtk3" = "no" ; then
	AC_MSG_WARN([Gtk3 version must be > 3.0.0!])
	enable_gtk_immodule3="no"
    fi
    GTK_IM_MODULE3_DIR=`$PKG_CONFIG gtk+-3.0 --variable=libdir`/gtk-3.0/immodules
    AC_SUBST(GTK_IM_MODULE3_DIR)
fi
AM_CONDITIONAL(ENABLE_GTK_IMMODULE3, [ test "$enable_gtk_immodule3" = "yes" ])

#-----------------------------------------------------------------------
# Check for QT4 support
#-----------------------------------------------------------------------
#PKG_CHECK_MODULES(QT4, Qt >= 4.3.3, use_qt4="yes", use_qt4="no")
#if test "$use_qt4" = "yes" ; then
#	PKG_CHECK_MODULES(QTCORE, QtCore >= 4.3.3, [], [])
#	PKG_CHECK_MODULES(QTGUI, QtGui >= 4.3.3, [], [])
#	PKG_CHECK_MODULES(QT3SUPPORT, Qt3Support >= 4.3.3, [], [])
#	QT4_IM_MODULE_DIR=`pkg-config --variable=plugindir Qt`/inputmethods
#	AC_SUBST(QT4_IM_MODULE_DIR)
#fi

#-----------------------------------------------------------------------
# GUI Setup tool (USE QT Library)
#-----------------------------------------------------------------------
AC_ARG_ENABLE([qt-immodule],
    AS_HELP_STRING(
        [--enable-qt-immodule],
        [Enable build QT input method extension (default=auto)]),
        [],
        [enable_qt_immodule="yes"])

AC_ARG_WITH([qt-dir],
    AS_HELP_STRING(
    	[--with-qt-dir=PATH],
	[set the install root of QT libraries (default=$QTDIR)]),
	[test x$withval != x && qt_dir="$withval"],
	[qt_dir="$QTDIR"])

AC_ARG_WITH([qt-imdir],
    AS_HELP_STRING(
    	[--with-qt-imdir=PATH],
	[set the QT3 input methods directory]),
	[test x$withval != x && qt_imdir="$withval"],
	[qt_imdir="$QTDIR/plugins/inputmethods"])

if test "$enable_qt_immodule" = "yes" ; then
    have_qmake="no"
    have_uic="no"
    have_moc="no"
    have_qinputcontextplugin="no"

    AC_MSG_CHECKING([for qmake])
    if test -f ${qt_dir}/bin/qmake ; then
	AC_MSG_RESULT(${qt_dir}/bin/qmake)
	have_qmake="yes"
    else
	AC_MSG_RESULT([not found!])
    fi

    AC_MSG_CHECKING([for uic])
    if test -f ${qt_dir}/bin/uic ; then
	AC_MSG_RESULT(${qt_dir}/bin/uic)
	have_uic="yes"
    else
	AC_MSG_RESULT([not found!])
    fi

    AC_MSG_CHECKING([for moc])
    if test -f ${qt_dir}/bin/moc ; then
	AC_MSG_RESULT(${qt_dir}/bin/moc)
	have_moc="yes"
    else
	AC_MSG_RESULT([not found!])
    fi

    if test "$enable_qt_immodule" = "yes"
    then
	AC_MSG_CHECKING([for qinputcontextplugin.h])
	if test -f ${qt_dir}/include/qinputcontextplugin.h ; then
	    AC_MSG_RESULT(${qt_dir}/include/qinputcontextplugin.h)
	    have_qinputcontextplugin="yes"
	    QT_IM_MODULE_DIR="${qt_imdir}"
	    AC_SUBST(QT_IM_MODULE_DIR)
	else
	    AC_MSG_RESULT([not found!])
	    enable_qt_immodule="no"
	fi
    fi

    if test $have_qmake = "no" || test $have_uic = "no" || test $have_moc = "no"
    then
	AC_MSG_ERROR([Can't find QT build tools! Plese set with-qt-dir="Your QT install root PATH"])
    fi

    QTDIR="$qt_dir"
    AC_SUBST(QTDIR)
fi
AM_CONDITIONAL(ENABLE_QT4_IMMODULE, [ test "$use_qt4" = "yes" ])
#AM_CONDITIONAL(ENABLE_QT_IMMODULE, test "$enable_qt_immodule" = "yes" -a "$use_qt4" = "no")
AM_CONDITIONAL(ENABLE_QT_IMMODULE, test "$enable_qt_immodule" = "yes")

#-----------------------------------------------------------------------
# 指定下載 CIN 檔案列表的 URL
#-----------------------------------------------------------------------
#AC_ARG_WITH([external-download-url],
#    AS_HELP_STRING(
#    	[--with-external-download-url=URL],
#	[specify external CIN tables URL]),
#	[test x$withval != x && ext_url="$withval"],
#	[ext_url="no"])

#if test "$ext_url" != "no" ; then
    #AC_DEFINE_UNQUOTED(EXTERNAL_DOWNLOAD_URL, "$ext_url", [OXIM external CIN tables URL])
#    EXTERNAL_DOWNLOAD_URL=$ext_url
#    AC_SUBST(EXTERNAL_DOWNLOAD_URL)
#fi

#-----------------------------------------------------------------------
# 
#-----------------------------------------------------------------------
OXIM_DEFAULT_DIR=${libdir}/oxim
AC_SUBST(OXIM_DEFAULT_DIR)

IM_MODULES_DIR=${OXIM_DEFAULT_DIR}/immodules
AC_SUBST(IM_MODULES_DIR)

MODULES_DIR=${OXIM_DEFAULT_DIR}/modules
AC_SUBST(MODULES_DIR)

TABLES_DIR=${OXIM_DEFAULT_DIR}/tables
AC_SUBST(TABLES_DIR)

PANELS_DIR=${OXIM_DEFAULT_DIR}/panels
AC_SUBST(PANELS_DIR)


#-----------------------------------------------------------------------
# 
#-----------------------------------------------------------------------
AC_OUTPUT(Makefile \
	  src/lib/Makefile \
	  src/gtk-immodule/Makefile \
	  src/gtk-immodule3/Makefile \
	  src/qt-immodule/Makefile \
	  src/qt4-immodule-test/Makefile \
	  src/modules/gen-inp/Makefile \
	  src/modules/gen-inp-v1/Makefile \
	  src/modules/unicode/Makefile \
	  src/modules/chewing/Makefile \
	  src/modules/sunpinyin/Makefile \
	  src/util/oxim-conv/Makefile \
	  src/xim/Makefile \
	  src/xim/IMdkit/Makefile \
	  tables/Makefile \
	  panels/Makefile \
	  etc/Makefile \
	  po/Makefile.in \
	  oxim.pc
	 )
#	  src/qt4-immodule/Makefile \
AC_MSG_RESULT([
Build options:
  Version		$VERSION
  Install prefix	$prefix
  IM Module directory	$IM_MODULES_DIR
  Module directory	$MODULES_DIR
  Table directory	$TABLES_DIR

Input Method modules:
  Chewing		$enable_chewing_module
  Sunpinyin		$enable_sunpinyin_module
  Unicode		$enable_unicode_module

Optional:
  GTK2 IM module	$enable_gtk_immodule
  GTK3 IM module	$enable_gtk_immodule3
  QT3  IM module	$enable_qt_immodule
  QT4  IM module		$use_qt4
  For EeePC environment	$enable_EeePC
  For Device		$enable_Device
])
