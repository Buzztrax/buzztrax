#!/bin/sh
# $Id: autogen.sh,v 1.1 2004-04-16 14:23:27 ensonic Exp $
# stupid simple autogen script (to get us started)

cfg=configure.ac

# autopoint --force
# intltoolize --copy --force --automake
libtoolize --force --copy
aclocal
autoheader
automake --add-missing --gnu
autoconf

echo "now do ./configure --enable-maintainer-mode --enable-compile-warnings --enable-debug=yes"

