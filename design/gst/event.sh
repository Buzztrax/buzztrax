#!/bin/sh

export GST_DEBUG="event:2"
export GST_DEBUG_NO_COLOR=1

./event 2>event_0_0.log 0 0
./event 2>event_1_0.log 1 0
./event 2>event_0_1.log 0 1
./event 2>event_1_1.log 1 1
./event 2>event_0_2.log 0 2
./event 2>event_1_2.log 1 2
./event 2>event_0_3.log 0 3
./event 2>event_1_3.log 1 3

