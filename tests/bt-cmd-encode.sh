#!/bin/sh
# $Id: bt-cmd-encode.sh,v 1.6 2006-08-27 20:02:55 ensonic Exp $
# run bt-cmd --command=encode on all example and test for crashes

source bt-cfg.sh

E_SONGS="$TESTSONGDIR/buzz*.xml $TESTSONGDIR/combi*.xml $TESTSONGDIR/melo*.xml $TESTSONGDIR/simple*.xml"
T_SONGS="$TESTSONGDIR/broken*.xml $TESTSONGDIR/test*.xml"

# test examples
for song in $E_SONGS; do
  echo "testing $song"
  audio=`basename $song .xml`
  audio="$TESTRESULTDIR/$audio.ogg"
  libtool --mode=execute ../src/ui/cmd/bt-cmd >$audio.log 2>&1 --command=encode --input-file=$song --output-file=$audio
  if [ $? -ne 0 ]; then exit 1; fi
done
