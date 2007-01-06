#!/bin/sh
# $Id: bt-cmd-info.sh,v 1.7 2007-01-06 16:01:33 ensonic Exp $
# run bt-cmd --command=info on all example and test for crashes

source bt-cfg.sh

E_SONGS="$TESTSONGDIR/buzz*.xml $TESTSONGDIR/combi*.xml $TESTSONGDIR/melo*.xml $TESTSONGDIR/simple*.xml $TESTSONGDIR/broken2.xml"
T_SONGS="$TESTSONGDIR/broken1.xml $TESTSONGDIR/test*.xml^"

rm -f /tmp/bt_cmd_info.log

# test examples
for song in $E_SONGS; do
  echo "testing $song"
  info=`basename $song .xml`
  info="$TESTRESULTDIR/$info.txt"
  GST_DEBUG_NO_COLOR=1 GST_DEBUG="*:2,bt-*:4" libtool --mode=execute ../src/ui/cmd/bt-cmd >$info 2>>/tmp/bt_cmd_info.log --command=info --input-file=$song
  if [ $? -ne 0 ]; then exit 1; fi
done
