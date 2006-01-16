#!/bin/sh
# $Id: povalid.sh,v 1.3 2006-01-16 21:39:26 ensonic Exp $
# test validity of po i18n files

#res=`cd ../po;intltool-update -m 2>&1 | wc -l;cd ..`

res=`(cd $srcdir/../po && rm -f ./missing && intltool-update 2>/dev/null -m && \
if [ -f ./missing ]; then exit \`grep -v "buzztard-" po/missing | wc -l\`; else exit 0; fi)`

exit $res

