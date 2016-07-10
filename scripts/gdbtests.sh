#!/bin/sh
# rerun failed tests under gdb and get backtraces

log=$1
IFS="
"
for l in `grep "Received signal" "$log"`; do
  t=`echo $l | cut -d':' -f5`;
  s=`echo $l | cut -d'/' -f3`;
  BT_CHECKS="$t" make bt_$s.gdbrun;
done
