#!/bin/bash
# Buzztrax
# Copyright (C) 2005 Buzztrax team <buzztrax-devel@buzztrax.org>
#
# test validity of xml files
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
which >/dev/null xmllint || exit 77

XML_OPTS="--noout --nonet"

E_SONGS="$srcdir/tests/songs/buzz*.xml $srcdir/tests/songs/combi*.xml $srcdir/tests/songs/melo*.xml $srcdir/tests/songs/simple*.xml"

res=0
fails=0;
checks=0;
report="\n"

echo -n "Running suite(s): wellformedness"
checks=$(($checks+1))
# do wellformed checking
xmllint 2>/dev/null $XML_OPTS $E_SONGS $srcdir/docs/buzztrax.xsd
if [ $? -ne 0 ]; then
  res=1
  fails=$(($fails+1))
  report=$report"wellformedness:1:E: xmllint failed: $?\n"
fi


echo -n " schema-valid"
checks=$(($checks+1))
# do schema validation
xmllint 2>/dev/null $XML_OPTS --schema $srcdir/docs/buzztrax.xsd $E_SONGS
if [ $? -ne 0 ]; then
  res=1
  fails=$(($fails+1))
  report=$report"schema-valid:1:E: xmllint failed: $?\n"
fi


echo -n " docs-valid"
checks=$(($checks+1))
# test the docs
xmllint $XML_OPTS --xinclude --postvalid $srcdir/docs/help/bt-edit/C/index.docbook
if [ $? -ne 0 ]; then
  # this can fail it the dtd is not install, fallback to simple validity check
  # I/O error : Attempt to load network entity http://www.oasis-open.org/docbook/xml/4.7/docbookx.dtd
  xmllint $XML_OPTS --xinclude $srcdir/docs/help/bt-edit/C/index.docbook
  if [ $? -ne 0 ]; then
    res=1
    fails=$(($fails+1))
    report=$report"docs-valid:1:E: xmllint failed: $?\n"
  fi
fi

rate=$((($checks-$fails)*100/$checks))
echo
echo -n "$rate%: Checks: $checks, Failures: $fails"
echo -e -n $report

exit $res
