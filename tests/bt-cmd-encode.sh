#!/bin/bash
# Buzztrax
# Copyright (C) 2007 Buzztrax team <buzztrax-devel@buzztrax.org>
#
# run buzztrax-cmd --command=encode on all example and test for crashes
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
#   E_SONGS="/path/to/songs/*.bzt" T_SONGS=" " TESTRESULTDIR=/tmp FMT=ogg ./bt-cmd-encode.sh

if [ -e tests/bt-cfg.sh ]; then
  . ./tests/bt-cfg.sh
  FMT=ogg
else
  LIBTOOL=
  BUZZTRAX_CMD=buzztrax-cmd
fi

if [ -z "$E_SONGS" ]; then
  E_SONGS="$TESTSONGDIR/buzz*.xml \
      $TESTSONGDIR/combi*.xml \
      $TESTSONGDIR/melo*.xml \
      $TESTSONGDIR/simple*.xml"
fi
      
if [ -z "$T_SONGS" ]; then
  T_SONGS="$TESTSONGDIR/broken*.xml \
      $TESTSONGDIR/test*.xml"
fi

rm -f /tmp/bt_cmd_encode.log
mkdir -p $TESTRESULTDIR
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
  audio=`basename "$song" .xml`
  if [ "$audio" == "$base" ]; then
    audio=`basename "$song" .bzt`
  fi
  audio="$TESTRESULTDIR/$audio.$FMT"
  echo >>/tmp/bt_cmd_encode.log "== $song.$FMT =="
  GST_DEBUG_NO_COLOR=1 GST_DEBUG="*:2,bt-*:4" $LIBTOOL $BUZZTRAX_CMD 2>>/tmp/bt_cmd_encode.log -q --command=encode --input-file="$song" --output-file="$audio"
  if [ $? -ne 0 ]; then res=1; fi
done

# test formats
song="$TESTSONGDIR/simple1.xml"
if [ -e "$song" ]; then
  for format in ogg mp3 wav flac m4a raw; do
    echo "testing $format"
    audio=`basename "$song" .xml`
    if [ "$audio" == "$base" ]; then
      audio=`basename "$song" .bzt`
    fi
    audio="$TESTRESULTDIR/$audio.$format"
    echo >>/tmp/bt_cmd_encode.log "== $song.$format =="
    GST_DEBUG_NO_COLOR=1 GST_DEBUG="*:2,bt-*:4" $LIBTOOL $BUZZTRAX_CMD 2>>/tmp/bt_cmd_encode.log -q --command=encode --input-file="$song" --output-file="$audio"
    if [ $? -ne 0 ]; then res=1; fi
  done
fi

exit $res;

