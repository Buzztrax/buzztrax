#!/bin/bash
# run buzztrax-edit commandline options


# test the output a little
testHelpOutput()
{
  $LIBTOOL $BUZZTRAX_EDIT --help | grep >/dev/null -- "--help-bt-core"
  assertEquals $? 0
}

testVersionOutput()
{
  $LIBTOOL $BUZZTRAX_EDIT --version | grep >/dev/null -- "buzztrax-edit from buzztrax"
  assertEquals $? 0
}

# here we test that these don't crash
testOptions()
{
  $LIBTOOL $BUZZTRAX_EDIT  >/dev/null --nonsense-option
  assertNotEquals $? 0
}

# tests that launch the UI
testStartup()
{
  $LIBTOOL $BUZZTRAX_EDIT &
  btedit_pid=$!
  sleep 1s && kill $btedit_pid
}

testStartupWithOptions()
{
  $LIBTOOL $BUZZTRAX_EDIT >/dev/null --command=test5 &
  btedit_pid=$!
  sleep 1s && kill $btedit_pid
}

oneTimeSetUp()
{
  # TODO(ensonic): use an existing Xvfb if possible, configure flag?
  if [ "$XVFB_PATH" != "" ]; then
    $XVFB_PATH :9 -ac -nolisten tcp -fp $XFONT_PATH -noreset -screen 0 1024x786x24 -render &
    xvfb_pid=$!
    export DISPLAY=:9
  fi
}

oneTimeTearDown()
{
  if [ "$xvfb_pid" != "" ]; then
    kill $xvfb_pid
  fi
}

xvfb_pid=
# load configuration
. ./tests/bt-cfg.sh
# load shunit2
. ./tests/shunit2
