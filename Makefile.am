## $Id: Makefile.am,v 1.11 2004-08-13 20:44:06 ensonic Exp $

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
	tests/songs/test-simple1.xml

pixmapdatadir = $(datadir)/$(PACKAGE)/pixmaps
pixmapdata_DATA = \
	pixmaps/stock_media-fwd.xpm \
	pixmaps/stock_media-pause.xpm \
	pixmaps/stock_media-play.xpm \
	pixmaps/stock_media-rec.xpm \
	pixmaps/stock_media-stop.xpm \
	pixmaps/stock_about.png \
	pixmaps/stock_media-next.xpm \
	pixmaps/stock_media-play.png \
  pixmaps/stock_media-prev.xpm \
	pixmaps/stock_media-rew.xpm \
	pixmaps/buzztard.png

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

