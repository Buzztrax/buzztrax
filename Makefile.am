## $Id: Makefile.am,v 1.13 2004-09-22 16:05:05 ensonic Exp $

SUBDIRS = src po tests docs

if DEBUG
AM_CFLAGS = -pipe -O0 -Werror -g
else
AM_CFLAGS = -pipe -O3
endif

songdatadir = $(datadir)/$(PACKAGE)/songs
songdata_DATA = \
	tests/songs/example.xml \
	tests/songs/simple1.xml \
	tests/songs/simple2.xml \
	tests/songs/test-simple1.xml \
	tests/songs/test-simple2.xml

pixmapdatadir = $(datadir)/$(PACKAGE)/pixmaps
pixmapdata_DATA = \
	pixmaps/buzztard.png \
	pixmaps/stock_about.png \
	pixmaps/stock_media-fwd.png \
	pixmaps/stock_media-pause.png \
	pixmaps/stock_media-play.png \
	pixmaps/stock_media-prev.png \
	pixmaps/stock_media-rec.png \
	pixmaps/stock_media-rew.png \
	pixmaps/stock_media-stop.png \
	pixmaps/stock_repeat.png

EXTRA_DIST = \
	$(songdata_DATA) \
	$(pixmapdata_DATA)
	
install-data-local:
	-$(mkinstalldirs) $(songdatadir)
	-$(mkinstalldirs) $(pixmapdatadir)

uninstall-local:
	rm -r $(datadir)/$(PACKAGE)

.PHONY: todo

todo::
	@find . \( -name "*.c" -o -name "*.h" \) -exec grep -Hno "@todo .*" {} \; | sed "s/@todo/error:/"
	@find . \( -name "*.c" -o -name "*.h" \) -exec grep -Hno "@idea .*" {} \; | sed "s/@idea/warning:/"

