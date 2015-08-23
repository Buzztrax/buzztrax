#!/bin/bash
# Buzztrax
# Copyright (C) 2015 Buzztrax team <buzztrax-devel@buzztrax.org>
#
# run bt-dec gst-launch lines
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

testPull() {
  gst-launch-1.0 -q filesrc location=$TESTSONGDIR/simple2.xml ! buzztrax-dec ! fakesink sync=false
}

testPush() {
  gst-launch-1.0 -q filesrc location=$TESTSONGDIR/simple2.xml ! queue ! buzztrax-dec ! fakesink sync=false
}

testPlaybin() {
  gst-launch-1.0 -q playbin uri=file://$TESTSONGDIR/simple2.xml audio-sink="fakesink sync=false"
}


# load configuration
. ./tests/bt-cfg.sh
# load shunit2
. $TESTSRCDIR/shunit2
