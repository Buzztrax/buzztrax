# $Id: Makefile,v 1.7 2004-04-02 18:00:31 ensonic Exp $

.PHONY: clean

TARGETS = gst1 gst2 gst3 net1 net2 browser

# targets

all: $(TARGETS) net.h

clean:
	@rm -rf core *.o *~ $(TARGETS)


# generic rules

%: %.c
	gcc -Wall -g `pkg-config gstreamer-control-0.8 --cflags --libs` `pkg-config gstreamer-0.8 --cflags --libs` $< -o $@

