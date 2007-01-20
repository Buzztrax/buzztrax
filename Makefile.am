## $Id: Makefile.am,v 1.63 2007-01-20 19:43:34 ensonic Exp $

SRCDIRS = src tests
SUBDIRS = $(SRCDIRS) po docs

DISTCHECK_CONFIGURE_FLAGS=--enable-gtk-doc --enable-debug

songdatadir = $(datadir)/$(PACKAGE)/songs
songdata_DATA = \
	tests/songs/broken1.xml \
	tests/songs/broken2.xml \
	tests/songs/buzz1.xml \
	tests/songs/buzz2.xml \
	tests/songs/buzz3.xml \
	tests/songs/buzz4.xml \
	tests/songs/buzz5.xml \
	tests/songs/buzz6.xml \
	tests/songs/buzz7.xml \
	tests/songs/buzz8.xml \
	tests/songs/buzz9.xml \
	tests/songs/combi1.xml \
	tests/songs/example.xml \
	tests/songs/melo1.xml \
	tests/songs/melo2.xml \
	tests/songs/melo3.xml \
	tests/songs/melo4.xml \
	tests/songs/melo5.xml \
	tests/songs/simple1.xml \
	tests/songs/simple2.xml \
	tests/songs/simple3.xml \
	tests/songs/simple4.xml \
	tests/songs/simple5.xml \
	tests/songs/test-simple1.xml \
	tests/songs/test-simple2.xml

pixmapdatadir = $(datadir)/$(PACKAGE)/pixmaps
pixmapdata_DATA = \
	pixmaps/grid.png \
	pixmaps/menu_processor_machine.png \
	pixmaps/menu_sink_machine.png \
	pixmaps/menu_source_machine.png \
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

icon16dir = $(datadir)/icons/hicolor/16x16/apps
icon16_DATA = pixmaps/buzztard.png
icon48dir = $(datadir)/icons/hicolor/48x48/apps
icon48_DATA = pixmaps/48x48/buzztard.png
svgicondir = $(datadir)/icons/hicolor/scalable/apps
svgicon_DATA = pixmaps/buzztard.svg

schemasdir = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_files = buzztard.schemas.in
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)

EXTRA_DIST = \
	$(songdata_DATA) \
	$(pixmapdata_DATA) $(icon16_DATA) $(icon48_DATA) $(svgicon_DATA) \
	$(schemas_DATA) \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	COPYING-DOCS \
	omf.make \
	xmldocs.make \
	TODO \
	m4/README

@INTLTOOL_SCHEMAS_RULE@

DISTCLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update

clean-local:
	rm -f po/.intltool-merge-cache
  
gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor
install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

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

## this seems to fail during make distcheck
##uninstall-local:
##	rm -r $(datadir)/$(PACKAGE)


## Workaround for scrollkeeper bug
## otherwise we get warnings for left-over files when doing 'make distuninstallcheck'
distuninstallcheck_listfiles = find . -type f -print | grep -v '^\./var/scrollkeeper' | grep -v '^\./share/mime'


.PHONY: help coverage valgrind dict todo stats splint

if USE_COVERAGE
coverage::
	mkdir -p ./coverage
	lcov --directory . --zerocounters
	-$(MAKE) check
	lcov --directory ./src --capture --output-file ./coverage/$(PACKAGE).info
	genhtml -o ./coverage --num-spaces 2 ./coverage/$(PACKAGE).info
endif

if USE_VALGRIND
valgrind::
	$(MAKE) -C tests valgrind
endif

dict:: tags
	echo "personal_ws" >$(PACKAGE).aspell_dict
	@find . -name tags -exec grep -v "$\!" {} \; | cut -f1 | sort | uniq >>$(PACKAGE).aspell_dict

todo::
	@find . \( -name "*.c" -o -name "*.h" \) -exec grep -Hno "@todo .*" {} \; | sed "s/@todo/error:/"
	@find . \( -name "*.c" -o -name "*.h" \) -exec grep -Hno "@idea .*" {} \; | sed "s/@idea/warning:/"

stats::
	@find . \( -name "*.c" -o -name "*.h" \) -printf "%s %p\n" | sort -n

splint::
	for dir in $(SRCDIRS); do $(MAKE) -C $$dir splint ; done

help::
	@echo "make coverage                -- run all checks and generate html coverage report"
	@echo "                                only availble when built using --enable-coverage"
	@echo "make valgrind                -- run all checks under valgrind"
	@echo
	@echo "make dict                    -- create an aspell dictionary from tags"
	@echo
	@echo "make todo                    -- generate a list of TODO items in the form of gcc error/warnings"
	@echo "make stats                   -- run source code stats"
	@echo "make splint                  -- run splint source code checks"
