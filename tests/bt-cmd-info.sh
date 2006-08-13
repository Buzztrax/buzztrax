#!/bin/sh
# $Id: bt-cmd-info.sh,v 1.3 2006-08-13 20:24:33 ensonic Exp $
# run bt-cmd --command=info on all example and test for crashes

E_SONGS="songs/buzz*.xml songs/melo*.xml songs/simple*.xml songs/broken2.xml"
T_SONGS="songs/broken1.xml songs/test*.xml^"

# test examples
for song in $E_SONGS; do
  echo "testing $song"
  libtool --mode=execute ../src/ui/cmd/bt-cmd >$song.txt --command=info --input-file=$song
  if [ $? -ne 0 ]; then exit 1; fi
done
