## $Id: Makefile.am,v 1.10 2004-08-09 16:42:45 ensonic Exp $

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

