## $Id: Makefile.am,v 1.21 2004-10-21 15:22:59 ensonic Exp $

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
	tests/songs/simple3.xml \
	tests/songs/test-simple1.xml \
	tests/songs/test-simple2.xml \
	tests/songs/test-simple3.xml \
	tests/songs/test-simple4.xml

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

schemasdir = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_files = buzztard.schemas.in
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)

EXTRA_DIST = \
	$(songdata_DATA) \
	$(pixmapdata_DATA) \
	$(schemas_DATA) \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	COPYING-DOCS \
	omf.make \
	xmldocs.make

@INTLTOOL_SCHEMAS_RULE@

DISTCLEANFILES = \
	intltool-merge \
	intltool-update

clean-local:
	rm -f po/.intltool-merge-cache
	
##if GCONF_SCHEMAS_INSTALL
install-data-local:
	-$(mkinstalldirs) $(songdatadir)
	-$(mkinstalldirs) $(pixmapdatadir)
	if test -z "$(DESTDIR)" ; then \
		for p in $(schemas_DATA) ; do \
			GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
			$(GCONFTOOL) --makefile-install-rule $(top_srcdir)/$$p ; \
		done ; \
	fi

uninstall-local:
	rm -r $(datadir)/$(PACKAGE)

.PHONY: todo

todo::
	@find . \( -name "*.c" -o -name "*.h" \) -exec grep -Hno "@todo .*" {} \; | sed "s/@todo/error:/"
	@find . \( -name "*.c" -o -name "*.h" \) -exec grep -Hno "@idea .*" {} \; | sed "s/@idea/warning:/"

