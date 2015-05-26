#!/bin/bash
# Buzztrax
# Copyright (C) 2007 Buzztrax team <buzztrax-devel@buzztrax.org>
#
# run buzztrax-cmd --command=convert on all example and test for crashes
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

# this can also be invoked manually to test ones song-library:
#   E_SONGS="/path/to/songs/*.bzt" T_SONGS=" " TESTRESULTDIR=/tmp ./bt-cmd-convert.sh

if [ -e tests/bt-cfg.sh ]; then
  . ./tests/bt-cfg.sh
else
  LIBTOOL=
  BUZZTRAX_CMD=buzztrax-cmd
fi

if [ -z "$E_SONGS" ]; then
  #E_SONGS="$TESTSONGDIR/buzz*.xml \
  #   $TESTSONGDIR/combi*.xml \
  #   $TESTSONGDIR/melo*.xml \
  #   $TESTSONGDIR/simple*.xml"
  E_SONGS="$TESTSONGDIR/melo*.xml \
      $TESTSONGDIR/simple*.xml"
fi
      
if [ -z "$T_SONGS" ]; then
  T_SONGS="$TESTSONGDIR/broken*.xml \
      $TESTSONGDIR/test*.xml"
fi

rm -f /tmp/bt_cmd_convert.log
mkdir -p $TESTRESULTDIR
rm -f $TESTRESULTDIR/diff
res=0

trap crashed SIGTERM SIGSEGV
crashed()
{
    echo "!!! crashed"
    res=1
}

# test examples
for song in $E_SONGS; do
  if [ "$BT_CHECKS" != "" ]; then
    ls -1 $BT_CHECKS | grep >/dev/null "$song"
    if [ $? -eq 1 ]; then
      continue
    fi
  fi

  echo "testing $song"
  base=`basename "$song"`
  output=`basename "$song" .xml`
  if [ "$output" == "$base" ]; then
    output=`basename "$song" .bzt`
    unzip 1>>/tmp/bt_cmd_convert.log 2>&1 "$song" song.xml -d$TESTRESULTDIR
    mv $TESTRESULTDIR/song.xml $TESTRESULTDIR/"$base".xml
    input="$TESTRESULTDIR/$base.xml"
  else
    input="$song"
  fi
  tmpout="$TESTRESULTDIR/$output.out.xml"
  output="$TESTRESULTDIR/$output.out"
  echo >>/tmp/bt_cmd_convert.log "== $song =="
  GST_DEBUG_NO_COLOR=1 GST_DEBUG="*:2,bt-*:4" $LIBTOOL $BUZZTRAX_CMD 2>>/tmp/bt_cmd_convert.log -q --command=convert --input-file="$song" --output-file="$tmpout"
  mv "$tmpout" "$output"
  if [ $? -ne 0 ]; then
    res=1
  else
    xmllint --c14n "$input" | xmllint --format - >$TESTRESULTDIR/"$base".src.xml
    xmllint --c14n "$output" | xmllint --format - >$TESTRESULTDIR/"$base".dst.xml
    /usr/bin/diff -u $TESTRESULTDIR/"$base".{src,dst}.xml >>$TESTRESULTDIR/diff
  fi
done
if [ -e $TESTRESULTDIR/diff ]; then
  echo "diff summary"
  diffstat $TESTRESULTDIR/diff
fi

exit $res;
