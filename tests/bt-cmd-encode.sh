#!/bin/sh
# run buzztrax-cmd --command=encode on all example and test for crashes
#
# this can also be invoked manually to test ones song-library:
#   E_SONGS="/path/to/songs/*.bzt" T_SONGS=" " TESTRESULTDIR=/tmp FMT=ogg ./bt-cmd-encode.sh

if [ -e tests/bt-cfg.sh ]; then
  cd tests
  . ./bt-cfg.sh
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

