## $Id: Makefile.am,v 1.39 2005-07-04 11:37:10 ensonic Exp $

SRCDIRS = src tests
SUBDIRS = $(SRCDIRS) po docs

songdatadir = $(datadir)/$(PACKAGE)/songs
songdata_DATA = \
	tests/songs/buzz1.xml \
	tests/songs/buzz2.xml \
	tests/songs/buzz3.xml \
	tests/songs/buzz4.xml \
	tests/songs/buzz5.xml \
	tests/songs/buzz6.xml \
	tests/songs/example.xml \
	tests/songs/simple1.xml \
	tests/songs/simple2.xml \
	tests/songs/simple3.xml \
	tests/songs/simple4.xml \
	tests/songs/simple5.xml \
	tests/songs/test-simple1.xml \
	tests/songs/test-simple2.xml \
	tests/songs/test-simple3.xml \
	tests/songs/test-simple4.xml

pixmapdatadir = $(datadir)/$(PACKAGE)/pixmaps
pixmapdata_DATA = \
	pixmaps/buzztard.png \
	pixmaps/grid.png \
	pixmaps/menu_processor_machine.png \
	pixmaps/menu_sink_machine.png \
	pixmaps/menu_source_machine.png \
	pixmaps/stock_about.png \
	pixmaps/stock_media-fwd.png \
	pixmaps/stock_media-pause.png \
	pixmaps/stock_media-play.png \
	pixmaps/stock_media-prev.png \
	pixmaps/stock_media-rec.png \
	pixmaps/stock_media-rew.png \
	pixmaps/stock_media-stop.png \
	pixmaps/stock_repeat.png \
	pixmaps/tab_machines.png \
	pixmaps/tab_patterns.png \
	pixmaps/tab_sequence.png \
	pixmaps/tab_waves.png \
	pixmaps/tab_info.png

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
	
##if GCONF_SCHEMAS_INSTALL (this should be conditional)
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

.PHONY: coverage splint todo valgrind

if USE_COVERAGE
coverage::
	mkdir -p ./coverage
	lcov --directory . --zerocounters
	-$(MAKE) check
	## workaround for lcov not liking libtool
	for file in `find . -name "*.da" | grep ".libs" ` ;do \
		mv $$file `echo $$file | sed -e 's/\/\.libs//'`; \
	done
	lcov --directory ./src --capture --output-file ./coverage/$(PACKAGE).info
	genhtml -o ./coverage --num-spaces 2 ./coverage/$(PACKAGE).info
endif

splint::
	for dir in $(SRCDIRS); do $(MAKE) -C $$dir splint ; done

todo::
	@find . \( -name "*.c" -o -name "*.h" \) -exec grep -Hno "@todo .*" {} \; | sed "s/@todo/error:/"
	@find . \( -name "*.c" -o -name "*.h" \) -exec grep -Hno "@idea .*" {} \; | sed "s/@idea/warning:/"

if USE_VALGRIND
valgrind::
	$(MAKE) -C tests valgrind
endif
