# $Id$

.PHONY: clean

PROGRAMS = bztloader

all: $(PROGRAMS)

clean:
	@rm -rf core *.o *~ $(PROGRAMS)

%: %.c
	gcc -Wall -g `pkg-config libgsf-1 --cflags --libs` $< -o $@
