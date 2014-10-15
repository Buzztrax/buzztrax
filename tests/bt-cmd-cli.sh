#!/bin/bash
# run buzztrax-cmd commandline options

# test the output a little
testHelpOutput()
{
  $LIBTOOL $BUZZTRAX_CMD --help | grep >/dev/null -- "--help-bt-core"
  assertEquals $? 0
}

testVersionOutput()
{
  $LIBTOOL $BUZZTRAX_CMD --version | grep >/dev/null -- "buzztrax-cmd from buzztrax"
  assertEquals $? 0
}

# here we test that these don't crash
testOptions()
{
  $LIBTOOL $BUZZTRAX_CMD >/dev/null 2>&1 --quiet
  $LIBTOOL $BUZZTRAX_CMD >/dev/null 2>&1 --command
  $LIBTOOL $BUZZTRAX_CMD >/dev/null 2>&1 --command=info
  $LIBTOOL $BUZZTRAX_CMD >/dev/null 2>&1 --command=play
  $LIBTOOL $BUZZTRAX_CMD >/dev/null 2>&1 --command=convert
  $LIBTOOL $BUZZTRAX_CMD >/dev/null 2>&1 --command=encode
  $LIBTOOL $BUZZTRAX_CMD >/dev/null 2>&1 --command=does_not_exist
  $LIBTOOL $BUZZTRAX_CMD >/dev/null 2>&1 --input-file
  $LIBTOOL $BUZZTRAX_CMD >/dev/null 2>&1 --input-file=$TESTSONGDIR
  $LIBTOOL $BUZZTRAX_CMD >/dev/null 2>&1 --output-file
  $LIBTOOL $BUZZTRAX_CMD >/dev/null 2>&1 --output-file=$TESTRESULTDIR
}

# do something real
testPlayCommand()
{
  $LIBTOOL $BUZZTRAX_CMD >/dev/null 2>&1 --command=play --input-file=$TESTSONGDIR/test-simple1.xml
}

testPlayCommandWithQuietOption()
{
  $LIBTOOL $BUZZTRAX_CMD >/dev/null 2>&1 --command=play -q --input-file=$TESTSONGDIR/simple2.xml
}

testConvertCommand() {
  tmpfile=$SHUNIT_TMPDIR/simple2.out.xml
  $LIBTOOL $BUZZTRAX_CMD >/dev/null 2>&1 --command=convert -q --input-file=$TESTSONGDIR/simple2.xml --output-file=$tmpfile
  if [ ! -r $tmpfile ]; then
    fail "output $tmpfile file missing, have: $(ls -al $SHUNIT_TMPDIR/simple2*)"
  fi
  rm -f $tmpfile
}

testEncodeCommand() {
  tmpfile=$SHUNIT_TMPDIR/simple2.wav
  $LIBTOOL $BUZZTRAX_CMD >/dev/null 2>&1 --command=encode -q --input-file=$TESTSONGDIR/simple2.xml --output-file=$tmpfile
  if [ ! -r $tmpfile ]; then
    fail "output $tmpfile file missing, have: $(ls -al $SHUNIT_TMPDIR/simple2*)"
  fi
  rm -f $tmpfile
}
testEncodeCommandGuessFormat() {
  tmpfile=$SHUNIT_TMPDIR/simple2.vorbis.ogg
  $LIBTOOL $BUZZTRAX_CMD >/dev/null 2>&1 --command=encode -q --input-file=$TESTSONGDIR/simple2.xml --output-file=$SHUNIT_TMPDIR/simple2
  if [ ! -r $tmpfile ]; then
    fail "output $tmpfile file missing, have: $(ls -al $SHUNIT_TMPDIR/simple2*)"
  fi
  rm -f $tmpfile
}

# check what happens when we face a broken setup
#GST_PLUGIN_SYSTEM_PATH=/tmp GST_PLUGIN_PATH=/tmp $BUZZTRAX_CMD --command=play --input-file=$TESTSONGDIR/melo3.xml


# load configuration
. ./tests/bt-cfg.sh
# load shunit2
. $TESTSRCDIR/shunit2
