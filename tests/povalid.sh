#!/bin/sh
# $Id: povalid.sh,v 1.2 2005-06-10 10:14:04 ensonic Exp $
# test validity of po i18n files

#res=`cd ../po;intltool-update -m 2>&1 | wc -l;cd ..`

res=`(cd ../po && rm -f ./missing && intltool-update 2>/dev/null -m && \
if [ -f ./missing ]; then exit 1; else exit 0; fi)`

exit $res
