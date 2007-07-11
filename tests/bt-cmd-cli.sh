#!/bin/sh
# $Id: bt-cmd-cli.sh,v 1.1 2007-07-11 20:41:30 ensonic Exp $
# run bt-cmd commandline options

. ./bt-cfg.sh

BINARY=../src/ui/cmd/bt-cmd

# test the output a little
libtool --mode=execute $BINARY --help | grep >/dev/null -- "--help-bt-core"
if [ $? -ne 0 ]; then exit 1; fi

libtool --mode=execute $BINARY --version | grep >/dev/null -- "bt-cmd from buzztard"
if [ $? -ne 0 ]; then exit 1; fi

# here we test that these don't crash
libtool --mode=execute $BINARY >/dev/null 2>&1 --quiet
libtool --mode=execute $BINARY >/dev/null 2>&1 --command
libtool --mode=execute $BINARY >/dev/null 2>&1 --command=info
libtool --mode=execute $BINARY >/dev/null 2>&1 --command=play
libtool --mode=execute $BINARY >/dev/null 2>&1 --command=convert
libtool --mode=execute $BINARY >/dev/null 2>&1 --command=encode
libtool --mode=execute $BINARY >/dev/null 2>&1 --command=does_not_exist
libtool --mode=execute $BINARY >/dev/null 2>&1 --input-file
libtool --mode=execute $BINARY >/dev/null 2>&1 --input-file=$TESTSONGDIR
libtool --mode=execute $BINARY >/dev/null 2>&1 --output-file
libtool --mode=execute $BINARY >/dev/null 2>&1 --output-file=$TESTRESULTDIR

# do something real
libtool --mode=execute $BINARY >/dev/null 2>&1 --command=play -q --input-file=$TESTSONGDIR/test-simple1.xml

libtool --mode=execute $BINARY >/dev/null 2>&1 --command=convert -q --input-file=$TESTSONGDIR/test-simple1.xml --output-file=$TESTRESULTDIR/test-simple1.out.xml
if [ ! -r $TESTRESULTDIR/test-simple1.out.xml ]; then exit 1; fi
rm -f $TESTRESULTDIR/test-simple1.out.xml

libtool --mode=execute $BINARY >/dev/null 2>&1 --command=encode -q --input-file=$TESTSONGDIR/test-simple1.xml --output-file=$TESTRESULTDIR/test-simple1.ogg
if [ ! -r $TESTRESULTDIR/test-simple1.ogg ]; then exit 1; fi
rm -f $TESTRESULTDIR/test-simple1.ogg

