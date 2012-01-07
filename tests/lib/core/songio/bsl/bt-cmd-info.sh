#!/bin/sh
# run buzztard-cmd --command=info on all my buzz songs

if [ -z "$BSL_SONG_PATH" ]; then
  echo "please tell me where you have the buzz songs I shall test"
  echo "export BSL_SONG_PATH=/path/to/your/buzz/songs"
  exit 1
fi

. ./bt-cfg.sh

rm -f /tmp/bt_cmd_info_bsl.log
mkdir -p info

trap "sig_segv=1" SIGSEGV
trap "sig_int=1" INT

# test examples
(
  IFS=$'\n'
  for song in $(find $BSL_SONG_PATH \( -name "*.bmx" -o -name "*.bmw" \) ); do
    echo "testing $song"
    info=`basename $song .bmx`
    if [ $info == `basename $song` ]; then
      info=`basename $song .bmw`
    fi
    info="info/$info.txt"
    echo >>/tmp/bt_cmd_info_bsl.log "== $song =="
    sig_segv=0
    sig_int=0
    res=`env >$info 2>>/tmp/bt_cmd_info_bsl.log GST_DEBUG_NO_COLOR=1 GST_DEBUG="*:2,bt-*:4" $BINDIR/buzztard-cmd --command=info --input-file=$song; echo $?`
    if [ $sig_int -eq "1" ]; then res=1; fi
    if [ $res -eq "1" ]; then
      echo "$song failed"
    fi
  done
)

cd info
#grep >.missing.info song.setup.number_of_missing_machines *.txt | sort -n -k2
grep -ho "  bml-.*" *.txt | sort | uniq -c | sort -nr >.missing.info
cd ..
