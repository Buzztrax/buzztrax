#!/bin/sh

if [ -n "$1" ]; then
  base="latencies/$1"
else
  base="latencies/unknown"
fi

dbg="${base}.dbg"
data="${base}.dat"
out="${base}.png"
mkdir -p $base

title=`grep "µs=" $dbg | cut -c89-`
target=`echo $title | cut -d'|' -f2 | cut -d' ' -f5 | cut -d= -f2`
title=`echo $title | sed 's/|/:/'`
maxtarget=`expr 6 \* $target`
grep ":data_probe:" $dbg | cut -c94- >$data

cat <<EOF
set term png truecolor size 1024,768
set output '$out'

set title "$title"
set xlabel "Time in ms"
set xtics nomirror autofreq
set ylabel "Latency in ms"
set ytics nomirror autofreq
set yrange [0:$maxtarget]
set grid
set key box
plot \\
EOF

cut -d' ' -f1 $data | sort | uniq | while read pad; do
  pname=`echo $pad | tr ':' '_' | sed 's/[<>]//g'`
  pdata="${base}/${pname}.dat"
  grep "$pad" "$data" | cut -d' ' -f2- >$pdata
  echo "\"$pdata\" using 1:2 with lines title \"$pname\", \\"
done
# plot target latency
echo "$target with lines title \"target\", \\"
echo "2*$target with lines title \"2*target\""
