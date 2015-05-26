#!/bin/bash
# Buzztrax
# Copyright (C) 2007 Buzztrax team <buzztrax-devel@buzztrax.org>
#
# run buzztrax-edit commandline options
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, see <http://www.gnu.org/licenses/>.

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
  # create log dir
  mkdir -p $log_dir
  # TODO(ensonic): use an existing Xvfb if possible, configure flag?
  if [[ "$XVFB_PATH" != "" && "$BT_CHECK_NO_XVFB" == "" ]]; then
    $XVFB_PATH >$log_dir/xvfb.log 2>&1 :9 -ac -nolisten tcp -fp $XFONT_PATH -noreset -screen 0 1024x786x24 -render &
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
log_dir=/tmp/bt_edit_cli
# load configuration
. ./tests/bt-cfg.sh
# load shunit2
. $TESTSRCDIR/shunit2
