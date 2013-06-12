#!/bin/sh
set -e

srcdir=`dirname $0`
test -n "$srcdir" && cd "$srcdir"

echo "Updating build configuration files, please wait...."

aclocal -I /usr/local/share/aclocal -I m4
autoheader
automake --foreign --add-missing -c
autoconf
