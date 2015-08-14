#!/bin/sh

set -x

rm -f stamp-h1
rm -f oxim.conf
rm -f  config.*
rm -f *.log
rm -f  aclocal.m4
rm -f install-sh
rm -f libtool
rm -f ltmain.sh
rm -f missing
rm -f configure
rm -f depcomp
rm -fr autom4te.cache
touch config.rpath

aclocal
autoconf
autoheader
libtoolize --force --automake --copy
automake --copy --add-missing --gnu
rm -fr autom4te.cache
