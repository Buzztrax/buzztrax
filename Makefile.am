## $Id: Makefile.am,v 1.8 2004-07-29 15:51:30 ensonic Exp $

SUBDIRS = src tests docs

if DEBUG
AM_CFLAGS = -pipe -O0 -Werror -g
else
AM_CFLAGS = -pipe -O3
endif

songdatadir = $(datadir)/$(PACKAGE)/songs
songdata_DATA = \
	tests/songs/example.xml \
	tests/songs/simple1.xml \
	tests/songs/simple2.xml

EXTRA_DIST = \
	$(songdata_DATA)
	
install-data-local:
	-$(mkinstalldirs) $(songdatadir)

uninstall-local:
	rm -r $(datadir)/$(PACKAGE)

.PHONY: todo

todo::
	@find . \( -name "*.c" -o -name "*.h" \) -exec grep -Hno "@todo .*" {} \; | sed "s/@todo/error:/"
	@find . \( -name "*.c" -o -name "*.h" \) -exec grep -Hno "@idea .*" {} \; | sed "s/@idea/warning:/"

