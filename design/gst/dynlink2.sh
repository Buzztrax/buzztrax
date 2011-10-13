#!/bin/bash

gcc -Wall -g `pkg-config gstreamer-0.10 --cflags --libs` dynlink2.c -o dynlink2
rm *.{dot,png}
GST_DEBUG="dynlink2:2" GST_DEBUG_DUMP_DOT_DIR=$PWD ./dynlink2
for file in dyn*.dot; do echo $file; dot -Tpng $file -o${file/dot/png}; done
eog *.png

