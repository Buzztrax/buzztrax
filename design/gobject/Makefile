# $Id: Makefile,v 1.1 2004-04-27 15:22:36 waffel Exp $

.PHONY: clean

TARGETS = main

# targets

all: $(TARGETS)

main: main.c song.c
	gcc -Wall -g `pkg-config glib-2.0 --cflags --libs` `pkg-config gobject-2.0 --cflags --libs` $^ -o $@

clean:
	@rm -rf core *.o *~ $(TARGETS)


# generic rules

#%: %.c
#	gcc -Wall -g `pkg-config glib-2.0 --cflags --libs` `pkg-config gobject-2.0 --cflags --libs` $< -o $@

