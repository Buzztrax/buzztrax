#!/bin/sh
# run buzztard-cmd --command=convert on all example and test for crashes
#
# this can also be invoked manually to test ones song-library:
#   E_SONGS="/path/to/songs/*.bzt" T_SONGS=" " TESTRESULTDIR=/tmp ./bt-cmd-convert.sh

if [ -e tests/bt-cfg.sh ]; then
  cd tests
  . ./bt-cfg.sh
else
  LIBTOOL=
  BUZZTARD_CMD=buzztard-cmd
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
  GST_DEBUG_NO_COLOR=1 GST_DEBUG="*:2,bt-*:4" $LIBTOOL $BUZZTARD_CMD 2>>/tmp/bt_cmd_convert.log -q --command=convert --input-file="$song" --output-file="$tmpout"
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
