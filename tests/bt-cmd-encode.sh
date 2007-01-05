#!/bin/sh
# $Id: bt-cmd-encode.sh,v 1.8 2007-01-05 20:47:27 ensonic Exp $
# run bt-cmd --command=encode on all example and test for crashes

source bt-cfg.sh

E_SONGS="$TESTSONGDIR/buzz*.xml $TESTSONGDIR/combi*.xml $TESTSONGDIR/melo*.xml $TESTSONGDIR/simple*.xml"
T_SONGS="$TESTSONGDIR/broken*.xml $TESTSONGDIR/test*.xml"

# test examples
for song in $E_SONGS; do
  echo "testing $song"
  audio=`basename $song .xml`
  audio="$TESTRESULTDIR/$audio.ogg"
  GST_DEBUG_NO_COLOR=1 GST_DEBUG="*:2,bt-*:4" libtool --mode=execute ../src/ui/cmd/bt-cmd 2>/tmp/bt_cmd_encode.log -q --command=encode --input-file=$song --output-file=$audio
  if [ $? -ne 0 ]; then exit 1; fi
done
