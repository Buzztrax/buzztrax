# $Id: Makefile,v 1.6 2004-04-01 15:29:49 ensonic Exp $

.PHONY: clean

TARGETS = gst1 gst2 gst3 net1 browser

# targets

all: $(TARGETS)

clean:
	@rm -rf core *.o *~ $(TARGETS)


# generic rules

%: %.c
	gcc -Wall `pkg-config gstreamer-control-0.8 --cflags --libs` `pkg-config gstreamer-0.8 --cflags --libs` $< -o $@

