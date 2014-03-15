#!/bin/bash

gcc -Wall -g dynlink5.c -o dynlink5 `pkg-config gstreamer-1.0 --cflags --libs`
if test $? -ne 0; then
  exit 1
fi
rm dyn*.{dot,png}

trap "echo ' aborted ...'" SIGINT
GST_DEBUG="dynlink:2" GST_DEBUG_DUMP_DOT_DIR=$PWD ./dynlink5
#GST_DEBUG="*:7" GST_DEBUG_NO_COLOR=1 GST_DEBUG_DUMP_DOT_DIR=$PWD ./dynlink5 2>debug.log
# grep "tee0:src_0" debug.log
# grep -A20 "0:00:01.879991378" debug.log | grep -v REFCOU

for file in dyn*.dot; do echo $file; dot -Tpng $file -o${file/dot/png}; done
eog *.png

