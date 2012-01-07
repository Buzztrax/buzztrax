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

