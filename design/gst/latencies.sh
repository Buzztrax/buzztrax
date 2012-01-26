#!/bin/sh

export GST_DEBUG="latency:4"
export GST_DEBUG_NO_COLOR=1

test="latency_alsasink"
./latency 125 8 "alsasink" 2>$test.dbg
./latency.sh $test | gnuplot
echo "[$test] done"

test="latency_pulsesink"
./latency 125 8 "pulsesink" 2>$test.dbg
./latency.sh $test | gnuplot
echo "[$test] done"

test="latency_jacksink"
pasuspender -- jackd --temporary -d alsa &
./latency 125 8 "jackaudiosink" 2>$test.dbg
./latency.sh $test | gnuplot
echo "[$test] done"

