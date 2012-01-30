#!/bin/sh

export GST_DEBUG="latency:4"
export GST_DEBUG_NO_COLOR=1

test="latency_alsasink_def"
./latency 125 8 "alsasink" 2>${test}.dbg
./latency.sh $test | gnuplot
echo "[$test] done"

test="latency_pulsesink_push"
./latency 125 8 "pulsesink" 2>${test}.dbg
./latency.sh $test | gnuplot
echo "[$test] done"

test="latency_pulsesink_pull"
./latency 125 8 "pulsesink can-activate-pull=true" 2>${test}.dbg
./latency.sh $test | gnuplot
echo "[$test] done"

# needs pasuspend to work

test="latency_alsasink_plughw"
pasuspender -- ./latency 125 8 "alsasink device=plughw:0" 2>${test}.dbg
./latency.sh $test | gnuplot
echo "[$test] done"

#test="latency_jacksink"
#pasuspender -- jackd --temporary -d alsa &
#./latency 125 8 "jackaudiosink" 2>${test}.dbg
#./latency.sh $test | gnuplot
#echo "[$test] done"

