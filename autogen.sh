#!/bin/sh
set -e

srcdir=`dirname $0`
test -n "$srcdir" && cd "$srcdir"

echo "Updating build configuration files, please wait...."

aclocal $ACLOCAL_FLAGS -I m4
autoheader
automake --foreign --add-missing -c
autoconf
