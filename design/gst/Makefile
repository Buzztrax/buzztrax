# $Id: Makefile,v 1.4 2004-02-13 15:36:13 ensonic Exp $

.PHONY: clean

TARGETS = gst1 gst2 gst3

# targets

all: $(TARGETS)

clean:
	@rm -rf core *.o *~ $(TARGETS)


# generic rules

%: %.c
	gcc -Wall `pkg-config gstreamer-control-0.7 --cflags --libs` `pkg-config gstreamer-0.7 --cflags --libs` $< -o $@

