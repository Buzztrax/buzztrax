# $Id: Makefile,v 1.3 2004-02-12 18:01:30 ensonic Exp $

all: gst1 gst2

gst1: gst1.c
	gcc -Wall `pkg-config gstreamer-control-0.7 --cflags --libs` `pkg-config gstreamer-0.7 --cflags --libs` gst1.c -o gst1
	
gst2: gst2.c
	gcc -Wall `pkg-config gstreamer-control-0.7 --cflags --libs` `pkg-config gstreamer-0.7 --cflags --libs` gst2.c -o gst2
