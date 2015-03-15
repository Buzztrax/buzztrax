#!/bin/bash
# run buzztrax-cmd --command=info on all example and test for crashes
#
# this can also be invoked manually to test ones song-library:
#   E_SONGS="/path/to/songs/*.bzt" T_SONGS=" " TESTRESULTDIR=/tmp ./bt-cmd-info.sh

if [ -e tests/bt-cfg.sh ]; then
  . ./tests/bt-cfg.sh
else
  LIBTOOL=
  BUZZTRAX_CMD=buzztrax-cmd
fi

if [ -z "$E_SONGS" ]; then
  E_SONGS="$TESTSONGDIR/buzz*.xml \
      $TESTSONGDIR/combi*.xml \
      $TESTSONGDIR/melo*.xml \
      $TESTSONGDIR/samples*.bzt \
      $TESTSONGDIR/simple*.xml \
      $TESTSONGDIR/simple*.bzt \
      $TESTSONGDIR/broken2.xml \
      $TESTSONGDIR/test*.xml"
fi
      
if [ -z "$T_SONGS" ]; then
  T_SONGS="$TESTSONGDIR/broken1.xml \
      $TESTSONGDIR/broken1.bzt \
      $TESTSONGDIR/broken3.xml"
fi

rm -f /tmp/bt_cmd_info.log
mkdir -p $TESTRESULTDIR
res=0

trap crashed SIGTERM SIGSEGV
crashed()
{
    echo "!!! crashed"
    res=1
}

# test working examples
for song in $E_SONGS; do
  if [ "x$BT_CHECKS" != "x" ]; then
    ls -1 $BT_CHECKS | grep >/dev/null "$song"
    if [ $? -eq 1 ]; then
      continue
    fi
  fi
  
  echo "testing $song"
  base=`basename "$song"`
  info=`basename "$song" .xml`
  if [ "x$info" == "x$base" ]; then
    info=`basename "$song" .bzt`
  fi
  info="$TESTRESULTDIR/$info.txt"
  echo >>/tmp/bt_cmd_info.log "== $song =="
  GST_DEBUG_NO_COLOR=1 GST_DEBUG="*:2,bt-*:4" $LIBTOOL $BUZZTRAX_CMD >$info 2>>/tmp/bt_cmd_info.log --command=info --input-file="$song"
  if [ $? -ne 0 ]; then
    echo >>/tmp/bt_cmd_info.log "!!! failed"
    res=1
  fi
done

# test failure cases
for song in $T_SONGS; do
  if [ "x$BT_CHECKS" != "x" ]; then
    ls -1 $BT_CHECKS | grep >/dev/null "$song"
    if [ $? -eq 1 ]; then
      continue
    fi
  fi

  echo "testing $song"
  base=`basename '$song'`
  info=`basename '$song' .xml`
  if [ "x$info" == "x$base" ]; then
    info=`basename "$song" .bzt`
  fi
  info="$TESTRESULTDIR/$info.txt"
  echo >>/tmp/bt_cmd_info.log "== $song =="
  GST_DEBUG_NO_COLOR=1 GST_DEBUG="*:2,bt-*:4" $LIBTOOL $BUZZTRAX_CMD >$info 2>>/tmp/bt_cmd_info.log --command=info --input-file="$song"
  if [ $? -eq 0 ]; then
    echo >>/tmp/bt_cmd_info.log "!!! failed"
    res=1
  fi
done

exit $res;

