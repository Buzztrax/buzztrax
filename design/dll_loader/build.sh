#!/bin/bash

CC=g++
CFLAGS="-g -O0"
LD_LIBRARY_PATH="/home/hoehmi/wine//lib:$LD_LIBRARY_PATH"

echo "0. make clean"
make clean

echo "1. compile test.cc"
$CC $CFLAGS -c  -I. -Iinclude -I/home/hoehmi/wine//include/wine/windows/.. -I/home/hoehmi/wine//include/wine/windows/../.. -I/home/hoehmi/wine//include/wine/windows -fPIC -I .. -I./vst  -DDLLPATH=\"/home/hoehmi/wine//lib/wine\" -DLIBPATH=\"/usr/local/lib\"  -D_REENTRANT -o test.o test.cc

#echo "2. generate libtest.spec.cc"
#/home/hoehmi/wine//bin/winebuild -fPIC -o libtest.spec.cc --exe libtest -mgui test.o -L/home/hoehmi/wine//lib/wine -L/home/hoehmi/wine//lib/wine  -ladvapi32 -lcomdlg32 -lgdi32 -lkernel32 -lodbc32 -lole32 -loleaut32 -lshell32 -luser32 -lwinspool -lwindebug

echo "3. compile libtest.spec.cc"
$CC $CFLAGS -c  -I. -Iinclude -I/home/hoehmi/wine//include/wine/windows/.. -I/home/hoehmi/wine//include/wine/windows/../.. -I/home/hoehmi/wine//include/wine/windows -fPIC -DDLLPATH=\"/home/hoehmi/wine//lib/wine\" -DLIBPATH=\"/usr/local/lib\"  -D_REENTRANT -o libtest.spec.o libtest.spec.cc

echo "4. build libtest.so"
$CC -shared -Wl,-Bsymbolic -o libtest.so test.o libtest.spec.o -DDLLPATH=\"/home/hoehmi/wine//lib/wine\" -L/home/hoehmi/wine/lib -L/home/hoehmi/wine/lib/wine -ldl -lpthread -lwine -lwine_unicode -lm /home/hoehmi/wine/lib/wine/kernel32.dll.so

echo "5. compile main.c"
$CC $CFLAGS -shared -Wl,-Bsymbolic -L. -ltest -o main main.c
