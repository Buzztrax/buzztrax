## $Id$

pixmapdatadir = $(datadir)/$(PACKAGE)/pixmaps
pixmapdata_DATA = \
	grid.png \
	menu_processor_machine.png \
	menu_sink_machine.png \
	menu_source_machine.png \
	\
	prefs-audio-card.png \
	prefs-input-gaming.png \
	prefs-input-keyboard.png \
	\
	stock_repeat.png \
	\
	tab_machines.png \
	tab_patterns.png \
	tab_sequence.png \
	tab_waves.png \
	tab_info.png

# these are in gtk since 2.6
#	stock_media-fwd.png \
#	stock_media-pause.png \
#	stock_media-play.png \
#	stock_media-prev.png \
#	stock_media-rec.png \
#	stock_media-rew.png \
#	stock_media-stop.png

# the current tab icons shold go to hicolor
# delouns tab icons should go to tango

icon16dir = $(datadir)/icons/hicolor/16x16/apps
icon16_DATA = hicolor/16x16/buzztard.png

icon48dir = $(datadir)/icons/hicolor/48x48/apps
icon48_DATA = hicolor/48x48/buzztard.png

svgicondir = $(datadir)/icons/hicolor/scalable/apps
svgicon_DATA = hicolor/scalable/buzztard.svg

EXTRA_DIST = \
	$(pixmapdata_DATA) \
	$(icon16_DATA) $(icon48_DATA) $(svgicon_DATA)

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

