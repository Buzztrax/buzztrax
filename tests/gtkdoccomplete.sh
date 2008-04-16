#!/bin/sh
# $Id$

res=0
fails=0;
checks=0;
report="\n"

echo -n "Running suite(s): "
for dir in ../docs/reference/bt-*; do
  file=$dir/*-undocumented.txt;
  if [ -e $file ]; then
    echo -n `basename $dir` ""
    checks=$(($checks+3))

    if [ `head -n1 $file | cut -d% -f1` -ne "100" ]; then
      res=1
      fails=$(($fails+1))
      report=$report`echo -n $file`":1:E: undocumented symbols\n"
    fi
    file=$dir/*-unused.txt;
    if [ -e $file ]; then
      lines=`wc -l $file | cut -f1 -d\ `
      if [ $lines -gt 0 ]; then
        res=1
        fails=$(($fails+1))
        report=$report`echo -n $file`":1:E: $lines unused documentation entries\n"
      fi
    fi
    file=$dir/*-undeclared.txt;
    if [ -e $file ]; then
      lines=`wc -l $file | cut -f1 -d\ `
      if [ $lines -gt 0 ]; then
        res=1
        fails=$(($fails+1))
        report=$report`echo -n $file`":1:E: $lines undeclared symbols\n"
      fi
    fi
  fi
done
echo
if [ $checks -gt 0 ];then
  rate=$((($checks-$fails)*100/$checks))
  echo -n "$rate%: Checks: $checks, Failures: $fails"
  echo -e -n $report
fi

exit $res
