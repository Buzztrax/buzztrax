#!/bin/sh
# run buzztard-cmd --command=convert on all example and test for crashes

cd tests
. ./bt-cfg.sh

#E_SONGS="$TESTSONGDIR/buzz*.xml $TESTSONGDIR/combi*.xml $TESTSONGDIR/melo*.xml $TESTSONGDIR/simple*.xml"
E_SONGS="$TESTSONGDIR/melo*.xml \
    $TESTSONGDIR/simple*.xml"

T_SONGS="$TESTSONGDIR/broken*.xml \
    $TESTSONGDIR/test*.xml"

rm -f /tmp/bt_cmd_convert.log
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
    ls -1 $BT_CHECKS | grep >/dev/null $song
    if [ $? -eq 1 ]; then
      continue
    fi
  fi

  echo "testing $song"
  output=`basename $song .xml`
  tmpout="$TESTRESULTDIR/$output.out.xml"
  output="$TESTRESULTDIR/$output.out"
  echo >>/tmp/bt_cmd_convert.log "== $song =="
  GST_DEBUG_NO_COLOR=1 GST_DEBUG="*:2,bt-*:4" libtool --mode=execute $BUZZTARD_CMD 2>>/tmp/bt_cmd_convert.log -q --command=convert --input-file=$song --output-file=$tmpout
  mv $tmpout $output
  if [ $? -ne 0 ]; then
    res=1
  else
    xmllint --c14n $song | xmllint --format - >/tmp/bt_src.xml
    xmllint --c14n $output | xmllint --format - >/tmp/bt_dst.xml
    echo "comparing $song"
    /usr/bin/diff -u /tmp/bt_{src,dst}.xml >/dev/null
    #if [ $? -ne 0 ]; then res=1; fi
    if [ $? -ne 0 ]; then
      /usr/bin/diff -u /tmp/bt_{src,dst}.xml | diffstat
    fi
  fi
done

#rm -f /tmp/bt_{src,dst}.xml

exit $res;
