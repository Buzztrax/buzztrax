# $Id: Makefile,v 1.5 2004-04-01 14:49:08 waffel Exp $

.PHONY: clean

TARGETS = gst1 gst2 gst3 browser

# targets

all: $(TARGETS)

clean:
	@rm -rf core *.o *~ $(TARGETS)


# generic rules

%: %.c
	gcc -Wall `pkg-config gstreamer-control-0.7 --cflags --libs` `pkg-config gstreamer-0.7 --cflags --libs` $< -o $@

