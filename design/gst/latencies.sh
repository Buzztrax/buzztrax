#!/bin/bash

export GST_DEBUG="latency:4"
export GST_DEBUG_NO_COLOR=1

bpm=125
tpb=4

test="latency_alsasink_def"
./latency $bpm $tpb "alsasink" 2>${test}.dbg
./latency.sh $test | gnuplot
echo "[$test] done"

test="latency_pulsesink_push"
./latency $bpm $tpb "pulsesink" 2>${test}.dbg
./latency.sh $test | gnuplot
echo "[$test] done"

test="latency_pulsesink_pull"
./latency $bpm $tpb "pulsesink can-activate-pull=true" 2>${test}.dbg
./latency.sh $test | gnuplot
echo "[$test] done"

# needs pasuspender to work

test="latency_alsasink_hw"
pasuspender -- ./latency $bpm $tpb "alsasink device=hw:0" 2>${test}.dbg
./latency.sh $test | gnuplot
echo "[$test] done"

test="latency_alsasink_plughw"
pasuspender -- ./latency $bpm $tpb "alsasink device=plughw:0" 2>${test}.dbg
./latency.sh $test | gnuplot
echo "[$test] done"

test="latency_jacksink"
pasuspender -- jackd >/dev/null --silent --temporary -d alsa &
sleep 1s
./latency $bpm $tpb "jackaudiosink" 2>${test}.dbg
./latency.sh $test | gnuplot
echo "[$test] done"

# show results

eog latency_*.png

