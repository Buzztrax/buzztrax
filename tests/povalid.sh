#!/bin/sh
# $Id: povalid.sh,v 1.4 2006-01-17 13:47:15 ensonic Exp $
# test validity of po i18n files

#res=`cd ../po;intltool-update -m 2>&1 | wc -l;cd ..`

pwd=`pwd`;
cd $srcdir/../po

rm -f ./missing
intltool-update 2>/dev/null -m
if [ -f ./missing ]; then 
  res=`grep -v "buzztard-" ./missing | wc -l`;
else
  res="0";
fi

cd $pwd;
exit $res

