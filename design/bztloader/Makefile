# $Id: Makefile,v 1.1 2005-10-07 13:30:54 waffel Exp $

.PHONY: clean

#TARGETS = gst1 gst2 gst3 net1 net2 browser
TARGETS = bztloader

# targets

all: $(TARGETS) bztloader.h

clean:
	@rm -rf core *.o *~ $(TARGETS)


# generic rules

%: %.c
	gcc -Wall -g `pkg-config libgsf-1 --cflags --libs` $< -o $@
