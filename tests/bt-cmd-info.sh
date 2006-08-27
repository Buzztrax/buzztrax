#!/bin/sh
# $Id: bt-cmd-info.sh,v 1.5 2006-08-27 20:02:55 ensonic Exp $
# run bt-cmd --command=info on all example and test for crashes

source bt-cfg.sh

E_SONGS="$TESTSONGDIR/buzz*.xml $TESTSONGDIR/combi*.xml $TESTSONGDIR/melo*.xml $TESTSONGDIR/simple*.xml $TESTSONGDIR/broken2.xml"
T_SONGS="$TESTSONGDIR/broken1.xml $TESTSONGDIR/test*.xml^"

# test examples
for song in $E_SONGS; do
  echo "testing $song"
  info=`basename $song .xml`
  info="$TESTRESULTDIR/$info.txt"
  libtool --mode=execute ../src/ui/cmd/bt-cmd >$info --command=info --input-file=$song
  if [ $? -ne 0 ]; then exit 1; fi
done
