SRCDIRS = src tests
SUBDIRS = $(SRCDIRS) po docs

ACLOCAL_AMFLAGS = -I m4
DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc --enable-debug --disable-scrollkeeper

# -- gconf

schemasdir = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_files = buzztard.schemas.in
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)

# -- icons

## FIXME: move those to theme icons
pixmapdatadir = $(datadir)/$(PACKAGE)/pixmaps
pixmapdata_DATA = \
	pixmaps/popup-menu.png \
	pixmaps/prefs-audio-card.png pixmaps/prefs-input-gaming.png pixmaps/prefs-input-keyboard.png \
	pixmaps/stock_repeat.png pixmaps/stock_volume.png

hicolor_iconthemedir=$(datadir)/icons/hicolor
hicolor_icon16dir = $(hicolor_iconthemedir)/16x16/apps
hicolor_icon16_DATA = pixmaps/hicolor/16x16/buzztard.png \
	pixmaps/hicolor/16x16/buzztard_menu_processor_machine.png \
	pixmaps/hicolor/16x16/buzztard_menu_sink_machine.png \
	pixmaps/hicolor/16x16/buzztard_menu_source_machine.png \
	pixmaps/hicolor/16x16/buzztard_tab_machines.png \
	pixmaps/hicolor/16x16/buzztard_tab_patterns.png \
	pixmaps/hicolor/16x16/buzztard_tab_sequence.png \
	pixmaps/hicolor/16x16/buzztard_tab_waves.png \
	pixmaps/hicolor/16x16/buzztard_tab_info.png

hicolor_icon24dir = $(hicolor_iconthemedir)/24x24/apps
hicolor_icon24_DATA = \
	pixmaps/hicolor/24x24/buzztard_menu_grid.png \
	pixmaps/hicolor/24x24/buzztard_menu_processor_machine.png \
	pixmaps/hicolor/24x24/buzztard_menu_sink_machine.png \
	pixmaps/hicolor/24x24/buzztard_menu_source_machine.png

hicolor_icon48dir = $(hicolor_iconthemedir)/48x48/apps
hicolor_icon48_DATA = pixmaps/hicolor/48x48/buzztard.png

hicolor_svgicondir = $(hicolor_iconthemedir)/scalable/apps
hicolor_svgicon_DATA = pixmaps/hicolor/scalable/buzztard.svg

gnome_iconthemedir=$(datadir)/icons/gnome

gnome_icon16dir = $(gnome_iconthemedir)/16x16/apps
gnome_icon16_DATA = pixmaps/gnome/16x16/buzztard.png \
	pixmaps/gnome/16x16/buzztard_menu_processor_machine.png \
	pixmaps/gnome/16x16/buzztard_menu_sink_machine.png \
	pixmaps/gnome/16x16/buzztard_menu_source_machine.png \
	pixmaps/gnome/16x16/buzztard_tab_machines.png \
	pixmaps/gnome/16x16/buzztard_tab_patterns.png \
	pixmaps/gnome/16x16/buzztard_tab_sequence.png \
	pixmaps/gnome/16x16/buzztard_tab_waves.png
##	pixmaps/gnome/16x16/buzztard_tab_info.png

gnome_icon22dir = $(gnome_iconthemedir)/22x22/apps
gnome_icon22_DATA = pixmaps/gnome/22x22/buzztard.png

gnome_icon24dir = $(gnome_iconthemedir)/24x24/apps
gnome_icon24_DATA = pixmaps/gnome/24x24/buzztard.png \
	pixmaps/gnome/24x24/buzztard_menu_grid.png \
	pixmaps/gnome/24x24/buzztard_menu_processor_machine.png \
	pixmaps/gnome/24x24/buzztard_menu_sink_machine.png \
	pixmaps/gnome/24x24/buzztard_menu_source_machine.png

gnome_icon32dir = $(gnome_iconthemedir)/32x32/apps
gnome_icon32_DATA = pixmaps/gnome/32x32/buzztard.png \
  pixmaps/gnome/32x32/buzztard_effect_bypass.png \
  pixmaps/gnome/32x32/buzztard_effect_mute.png \
  pixmaps/gnome/32x32/buzztard_effect.png \
  pixmaps/gnome/32x32/buzztard_generator_mute.png \
  pixmaps/gnome/32x32/buzztard_generator_solo.png \
  pixmaps/gnome/32x32/buzztard_generator.png \
  pixmaps/gnome/32x32/buzztard_master_mute.png \
  pixmaps/gnome/32x32/buzztard_master.png

gnome_icon48dir = $(gnome_iconthemedir)/48x48/apps
gnome_icon48_DATA = pixmaps/gnome/48x48/buzztard.png \
  pixmaps/gnome/48x48/buzztard_effect_bypass.png \
  pixmaps/gnome/48x48/buzztard_effect_mute.png \
  pixmaps/gnome/48x48/buzztard_effect.png \
  pixmaps/gnome/48x48/buzztard_generator_mute.png \
  pixmaps/gnome/48x48/buzztard_generator_solo.png \
  pixmaps/gnome/48x48/buzztard_generator.png \
  pixmaps/gnome/48x48/buzztard_master_mute.png \
  pixmaps/gnome/48x48/buzztard_master.png

gnome_icon64dir = $(gnome_iconthemedir)/64x64/apps
gnome_icon64_DATA = pixmaps/gnome/64x64/buzztard.png \
  pixmaps/gnome/64x64/buzztard_effect_bypass.png \
  pixmaps/gnome/64x64/buzztard_effect_mute.png \
  pixmaps/gnome/64x64/buzztard_effect.png \
  pixmaps/gnome/64x64/buzztard_generator_mute.png \
  pixmaps/gnome/64x64/buzztard_generator_solo.png \
  pixmaps/gnome/64x64/buzztard_generator.png \
  pixmaps/gnome/64x64/buzztard_master_mute.png \
  pixmaps/gnome/64x64/buzztard_master.png

MACHINE_ICONS = \
  pixmaps/gnome/scalable/buzztard_effect_bypass.svg \
  pixmaps/gnome/scalable/buzztard_effect_mute.svg \
  pixmaps/gnome/scalable/buzztard_effect.svg \
  pixmaps/gnome/scalable/buzztard_generator_mute.svg \
  pixmaps/gnome/scalable/buzztard_generator_solo.svg \
  pixmaps/gnome/scalable/buzztard_generator.svg \
  pixmaps/gnome/scalable/buzztard_master_mute.svg \
  pixmaps/gnome/scalable/buzztard_master.svg

gnome_svgicondir = $(gnome_iconthemedir)/scalable/apps
gnome_svgicon_DATA = pixmaps/gnome/scalable/buzztard.svg $(MACHINE_ICONS)

prescale-svg:
	for size in 32 48 64; do \
		for icon in $(MACHINE_ICONS); do \
			rsvg-convert "$$icon" -w $$size -h $$size -f png -o "pixmaps/gnome/$${size}x$${size}/`basename $$icon .svg`.png"; \
		done; \
	done

EXTRA_DIST = \
	$(gnome_icon16_DATA) $(gnome_icon22_DATA) $(gnome_icon24_DATA) \
	$(gnome_icon32_DATA) $(gnome_icon48_DATA) $(gnome_icon64_DATA) $(gnome_svgicon_DATA) \
  $(hicolor_icon16_DATA) $(hicolor_icon24_DATA) $(hicolor_icon48_DATA) $(hicolor_svgicon_DATA) \
  $(pixmapdata_DATA) \
	$(schemas_DATA) \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	COPYING-DOCS \
	omf.make \
	xmldocs.make \
	TODO \
	po/$(GETTEXT_PACKAGE).pot

@INTLTOOL_SCHEMAS_RULE@

DISTCLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update

clean-local:
	rm -f po/.intltool-merge-cache

if GCONF_SCHEMAS_INSTALL
install-data-local:
	@if test -z "$(DESTDIR)" ; then \
		for p in $(schemas_DATA) ; do \
			GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
			$(GCONFTOOL) --makefile-install-rule $(top_srcdir)/$$p ; \
		done \
	fi
endif

if GCONF_SCHEMAS_INSTALL
uninstall-local:
	@if test -z "$(DESTDIR)" ; then \
		for p in $(schemas_DATA) ; do \
			GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
			$(GCONFTOOL) --makefile-uninstall-rule $(top_srcdir)/$$p ; \
		done \
	fi
endif

## this seems to fail during make distcheck
##uninstall-local:
##	rm -r $(datadir)/$(PACKAGE)

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		gtk-update-icon-cache -f -t $(hicolor_iconthemedir); \
		gtk-update-icon-cache -f -t $(gnome_iconthemedir); \
	else \
		echo "*** Icon cache not updated.  After install, run this:"; \
		echo "***   gtk-update-icon-cache -f -t $(hicolor_iconthemedir)"; \
		echo "***   gtk-update-icon-cache -f -t $(gnome_iconthemedir)"; \
	fi


## We get warnings for left-over files when doing 'make distuninstallcheck'
distuninstallcheck_listfiles = find . -type f -print

## Workaround for scrollkeeper bug
distuninstallcheck_listfiles += | grep -v '/var/scrollkeeper'
## Workaround for installing files in own prefix
distuninstallcheck_listfiles += | grep -v '/share/mime'
## Workaround for installing icon theme in own prefix
distuninstallcheck_listfiles += | grep -v 'icon-theme.cache'


.PHONY: coverage valgrind dict cc10 cc20

# make coverage        -- run all checks and generate html coverage report (only availble when built using --enable-coverage)
if USE_LCOV
coverage::
	mkdir -p ./coverage
	lcov --compat-libtool --directory . --zerocounters
	-$(MAKE) check
	lcov --compat-libtool --directory ./src --capture --output-file ./coverage/$(PACKAGE)_tmp.info
	lcov --compat-libtool --extract ./coverage/$(PACKAGE)_tmp.info "$(PWD)/src*" -o ./coverage/$(PACKAGE).info
	$(RM) ./coverage/$(PACKAGE)_tmp.info
	genhtml -o ./coverage --num-spaces 2 ./coverage/$(PACKAGE).info
endif
if USE_BCOV
coverage::
	mkdir -p ./coverage
	$(MAKE) -C tests coverage
	bcov-report -i $(abs_top_builddir)/src tests/.bcovdump.all ./coverage
endif

# make valgrind        -- run all checks under valgrind
if USE_VALGRIND
valgrind::
	$(MAKE) -C tests valgrind
endif

# make dict            -- create an aspell dictionary from tags
dict:: tags
	echo "personal_ws" >$(PACKAGE).aspell_dict
	@find . -name tags -exec grep -v "$\!" {} \; | cut -f1 | sort | uniq >>$(PACKAGE).aspell_dict

# -k2 in sort for Tranditional McCabe Cyclomatic Complexity
# make cc10            -- report McCabe Cyclomatic Complexity
# make cc20            -- report McCabe Cyclomatic Complexity
cc10::
	@pmccabe `find src -name '*.c'` | sort -nr | awk '($$1 > 10)'
cc20::
	@pmccabe `find src -name '*.c'` | sort -nr | awk '($$1 > 20)'

include $(top_srcdir)/common.mak
