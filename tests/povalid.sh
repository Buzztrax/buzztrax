#!/bin/sh
# $Id: povalid.sh,v 1.5 2007-08-06 20:08:32 ensonic Exp $
# test po i18n files

pwd=`pwd`;
cd $srcdir/../po

res=0
fails=0;
checks=0;
report="\n"

echo -n "Running suite(s): po-check"

rm -f ./missing
intltool-update 2>/dev/null -m
checks=$((checks+1))

if [ -e ./missing ]; then
  lines=`wc -l ./missing | cut -f1 -d\ `
  if  [ $lines -gt 0 ]; then
    res=1
    fails=$((fails+1))
    report=$report`echo -n po/missing`":1:E: $lines unassigned files\n"
  fi
fi

#translated=`LANG=C intltool-update -r 2>&1 | grep "translated messages"`
# foreach line in translated | cut -d\  -f5,8

rate=$((($checks-$fails)*100/$checks))
echo
echo -n "$rate%: Checks: $checks, Failures: $fails"
echo -e -n $report

cd $pwd;
exit $res

