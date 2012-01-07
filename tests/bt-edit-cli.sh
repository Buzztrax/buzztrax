#!/bin/sh
# run buzztard-edit commandline options

. ./bt-cfg.sh

res=0

trap crashed TERM
crashed()
{
    echo "!!! crashed"
    res=1
}

# test the output a little
echo "testing output"
libtool --mode=execute $BUZZTARD_EDIT --help | grep >/dev/null -- "--help-bt-core"
if [ $? -ne 0 ]; then exit 1; fi

libtool --mode=execute $BUZZTARD_EDIT --version | grep >/dev/null -- "buzztard-edit from buzztard"
if [ $? -ne 0 ]; then exit 1; fi

# here we test that these don't crash
echo "testing options"
libtool --mode=execute $BUZZTARD_EDIT  >/dev/null --nonsense-option
if [ $? -ne 1 ]; then exit 1; fi

# other tests would launch the UI - and this is how we could terminate them
# http://www.rekk.de/bloggy/2007/finding-child-pids-in-bash-shell-scripts/
if [ ! -z `which 2>/dev/null Xvfb` ]; then
  Xvfb :9 -ac -nolisten tcp -fp $XFONT_PATH -noreset -screen 0 1024x786x24 -render &
  xvfb_pid=$!

  echo "testing startup"
  DISPLAY=:9 libtool --mode=execute $BUZZTARD_EDIT &
  btedit_pid=$!
  sleep 1s && kill $btedit_pid

  echo "testing startup with options"
  DISPLAY=:9 libtool --mode=execute $BUZZTARD_EDIT >/dev/null --command=test5 &
  btedit_pid=$!
  sleep 1s && kill $btedit_pid

  kill $xvfb_pid
fi

exit $res;

