#!/bin/sh
# $Id: bt-cmd-encode.sh,v 1.2 2006-05-25 16:29:21 ensonic Exp $
# run bt-cmd --command=encode on all example and test for crashes

# add these later: songs/buzz*.xml 
E_SONGS="songs/melo*.xml songs/simple*.xml"
T_SONGS="songs/broken*.xml songs/test*.xml"

# test examples
for song in $E_SONGS; do
  echo "testing $song"
  audio=`basename $song .xml`
  audio="songs/$audio.ogg"
  libtool --mode=execute ../src/ui/cmd/bt-cmd >$audio.log 2>&1 --command=encode --input-file=$song --output-file=$audio
  if [ $? -ne 0 ]; then exit 1; fi
done

