# $Id: Makefile,v 1.1 2005-03-13 11:33:23 ensonic Exp $

.PHONY: clean

TARGETS = gmem

# targets

all: $(TARGETS)

gmem: gmem.c
	gcc -Wall -g `pkg-config glib-2.0 --cflags --libs` $^ -o $@

clean:
	@rm -rf core *.o *~ $(TARGETS)


# generic rules

#%: %.c
#	gcc -Wall -g `pkg-config glib-2.0 --cflags --libs` `pkg-config gobject-2.0 --cflags --libs` $< -o $@

