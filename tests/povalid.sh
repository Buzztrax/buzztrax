#!/bin/bash       
# Buzztrax
# Copyright (C) 2005 Buzztrax team <buzztrax-devel@buzztrax.org>
#
# test po i18n files
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, see <http://www.gnu.org/licenses/>.

if [ -z $srcdir ]; then
  srcdir=.
fi

# check for needed tools
which intltool-update || exit 77

pwd=`pwd`;
cd $srcdir/po

res=0
fails=0;
checks=0;
report="\n"

echo -n "Running suite(s): po-missing"

rm -f ./missing
intltool-update 2>/dev/null -m
checks=$(($checks+1))

if [ -e ./missing ]; then
  lines=`wc -l ./missing | cut -f1 -d\ `
  if  [ $lines -gt 0 ]; then
    res=1
    fails=$(($fails+1))
    report=$report"po/missing:1:E: $lines unassigned files\n"
  fi
fi

echo -n " po-translated"
checks=$(($checks+1))
fuzzy=0
missing=0

# we need to redirect to a file-descriptor to avoid a subshell
exec 3< <(LANG=C intltool-update -r 2>&1 | grep "translated messages")
while read line; do
  #echo "$line"
  n=`echo $line| cut -d\  -f5`
  if [ "$n" != "" ]; then
    fuzzy=$(($fuzzy+$n))
  fi
  n=`echo $line| cut -d\  -f8`
  if [ "$n" != "" ]; then
    missing=$(($missing+$n))
  fi
done <&3
if [ \( $fuzzy -gt 0 \) -o \( $missing -gt 0 \) ]; then
  #echo "fail"
  # we don't fail here
  #res=1
  fails=$(($fails+1))
  report=$report"po/*.po:1:E: $fuzzy fuzzy translations, $missing untranslated messages\n"
else
  echo "ok"
fi

rate=$((($checks-$fails)*100/$checks))
echo
echo -n "$rate%: Checks: $checks, Failures: $fails"
echo -e -n $report

cd $pwd;
exit $res

