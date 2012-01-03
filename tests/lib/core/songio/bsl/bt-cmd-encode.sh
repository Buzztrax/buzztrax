#!/bin/sh
# $Id$
# run buzztard-cmd --command=encode on all my buzz songs

if [ -z "$BSL_SONG_PATH" ]; then
  echo "please tell me where you have the buzz songs I shall test"
  echo "export BSL_SONG_PATH=/path/to/your/buzz/songs"
  exit 1
fi

. ./bt-cfg.sh

rm -f /tmp/bt_cmd_encode_bsl.log
mkdir -p audio

trap "sig_segv=1" SIGSEGV
trap "sig_int=1" INT

# test examples
(
  IFS=$'\n'
  for song in $(find $BSL_SONG_PATH \( -name "*.bmx" -o -name "*.bmw" \) ); do
    echo "testing $song"
    audio=`basename $song .bmx`
    if [ $audio == `basename $song` ]; then
      audio=`basename $song .bmw`
    fi
	audio="audio/$audio.ogg"
    echo >>/tmp/bt_cmd_encode_bsl.log "== $song =="
    sig_segv=0
    sig_int=0
    res=`env 2>>/tmp/bt_cmd_encode_bsl.log GST_DEBUG_NO_COLOR=1 GST_DEBUG="*:2,bt-*:4" $BINDIR/buzztard-cmd -q --command=encode --input-file=$song --output-file=$audio; echo $?`
    if [ $sig_int -eq "1" ]; then res=1; fi
    if [ $res -eq "1" ]; then
      echo "$song failed"
    fi
  done
)
