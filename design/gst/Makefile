# $Id: Makefile,v 1.8 2005-09-13 21:18:33 ensonic Exp $

.PHONY: clean

#TARGETS = gst1 gst2 gst3 net1 net2 browser
TARGETS = link states

# targets

all: $(TARGETS) net.h

clean:
	@rm -rf core *.o *~ $(TARGETS)


# generic rules

%: %.c
	gcc -Wall -g `pkg-config gstreamer-0.9 --cflags --libs` $< -o $@

