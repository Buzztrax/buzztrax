#!/bin/sh
# $Id$
# run bt-cmd --command=info on all example and test for crashes

. ./bt-cfg.sh

E_SONGS="$TESTSONGDIR/buzz*.xml \
    $TESTSONGDIR/combi*.xml \
    $TESTSONGDIR/melo*.xml \
    $TESTSONGDIR/simple*.xml \
    $TESTSONGDIR/simple*.bzt \
    $TESTSONGDIR/broken2.xml \
    $TESTSONGDIR/test*.xml"

T_SONGS="$TESTSONGDIR/broken1.xml \
    $TESTSONGDIR/broken1.bzt \
    $TESTSONGDIR/broken3.xml"

rm -f /tmp/bt_cmd_info.log
mkdir -p $TESTRESULTDIR
res=0

trap crashed TERM
crashed()
{
    echo "!!! crashed"
    res=1
}

# test working examples
for song in $E_SONGS; do
  echo "testing $song"
  info=`basename $song .xml`
  info="$TESTRESULTDIR/$info.txt"
  echo >>/tmp/bt_cmd_info.log "== $song =="
  GST_DEBUG_NO_COLOR=1 GST_DEBUG="*:2,bt-*:4" libtool --mode=execute ../src/ui/cmd/bt-cmd >$info 2>>/tmp/bt_cmd_info.log --command=info --input-file=$song
  if [ $? -ne 0 ]; then
    echo "!!! failed"
    res=1;
  fi
done

# test failure cases
for song in $T_SONGS; do
  echo "testing $song"
  info=`basename $song .xml`
  info="$TESTRESULTDIR/$info.txt"
  echo >>/tmp/bt_cmd_info.log "== $song =="
  GST_DEBUG_NO_COLOR=1 GST_DEBUG="*:2,bt-*:4" libtool --mode=execute ../src/ui/cmd/bt-cmd >$info 2>>/tmp/bt_cmd_info.log --command=info --input-file=$song
  if [ $? -eq 0 ]; then
    echo "!!! failed"
    res=1;
  fi
done

exit $res;

