#!/bin/sh

export GST_DEBUG="event:2"
export GST_DEBUG_NO_COLOR=1

# get data
for type in `seq 0 3`; do
  for flush in `seq 0 1`; do
    ./event 2>event_${flush}_${type}.log $flush $type
  done
done

nl='
'

# plot graphs
for log in event_*.log; do
  base=`basename $log .log`
  # cut interesting section
  #sl=`grep -no "main: started playback" $log | cut -d':' -f1`
  el=`grep -no "main: stopping playback" $log | cut -d':' -f1`
  sl=`grep -no "main: before seek" $log | cut -d':' -f1`
  #el=`grep -no "main: after seek" $log | cut -d':' -f1`
  head -n$el $log | tail -n+$sl | sed 's/\ \ */ /g' >$base.part
  sts=`head -n1 $base.part | cut -c9-17`
  ets=`tail -n1 $base.part | cut -c9-17`
  # get the elements in topological order (from initial state change)
  elements=`egrep "state-changed message from element '[a-z0-9]*': GstMessageState, old-state=\(GstState\)GST_STATE_NULL, new-state=\(GstState\)GST_STATE_READY" $log | sed "s/.*'\([a-z0-9]*\)'.*/\1/"`
  # produce the data file(s)
  IFS=$nl
  # async-done, state-changed
  grep "bus_snoop:<pipeline0>" $base.part | cut -d' ' -f1,7 | cut -c9- | sed "s/\ / 0 bus /" >event.tmp
  grep "async-done" event.tmp >event.bus.async_done
  grep "state-changed" event.tmp >event.bus.state_changed
  y=1
  rm event.elem.*
  for elem in $elements; do
    # flush-start, flush-stop, newsegment, seek
    grep "event_snoop:<$elem:" $base.part | cut -d' ' -f1,7 | cut -c9- | sed "s/\ / $y $elem /" >event.tmp
    grep "flush-start" event.tmp >>event.elem.flush_start
    grep "flush-stop" event.tmp >>event.elem.flush_stop
    grep "newsegment" event.tmp >>event.elem.newsegment
    grep "seek" event.tmp >>event.elem.seek
    y=`expr $y + 1`
  done
  y=`expr $y - 1`
  rm event.tmp
  
  # plot a graph with elements on y axis, time on x-axis and different
  # events/messages. Ideally we'd events as arrows, but its not easy to compute
  # the delta x,y from the script
  cat >event.gp <<EOF
set term png truecolor font "Sans,5" size 1200,400
set output '$base.png'

set pointsize 1.0

set xlabel "Time (sec.msec)"
set xtics nomirror autofreq
set xtics format "%g"
set mxtics 5
set xrange [$sts:$ets]

set ylabel "Elements"
set ytics nomirror autofreq
set yrange [*:*]

set grid xtics ytics mxtics mytics
set key box

plot \\
'event.elem.flush_start' using 1:2:yticlabels(3) axes x1y1 with linespoints title 'flush-start', \\
  'event.elem.flush_stop' using 1:2:yticlabels(3) axes x1y1 with linespoints title 'flush-stop', \\
  'event.elem.newsegment' using 1:2:yticlabels(3) axes x1y1 with linespoints title 'newsegment', \\
  'event.elem.seek' using 1:2:yticlabels(3) axes x1y1 with linespoints title 'seek', \\
  'event.bus.async_done' using 1:2:(0):($y):yticlabels(3) axes x1y1 with vectors title 'async-done', \\
  'event.bus.state_changed' using 1:2:(0):($y):yticlabels(3) axes x1y1 with vectors title 'state-changed'
EOF
  cat event.gp | gnuplot 
done
