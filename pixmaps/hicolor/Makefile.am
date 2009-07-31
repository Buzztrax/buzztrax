## $Id$

iconthemedir=$(datadir)/icons/hicolor
gtk_update_icon_cache = gtk-update-icon-cache -f -t $(iconthemedir)

icon16dir = $(iconthemedir)/16x16/apps
icon16_DATA = 16x16/buzztard.png \
	16x16/buzztard_menu_processor_machine.png \
	16x16/buzztard_menu_sink_machine.png \
	16x16/buzztard_menu_source_machine.png \
	16x16/buzztard_tab_machines.png \
	16x16/buzztard_tab_patterns.png \
	16x16/buzztard_tab_sequence.png \
	16x16/buzztard_tab_waves.png \
	16x16/buzztard_tab_info.png

icon24dir = $(iconthemedir)/24x24/apps
icon24_DATA = \
	24x24/buzztard_menu_grid.png \
	24x24/buzztard_menu_processor_machine.png \
	24x24/buzztard_menu_sink_machine.png \
	24x24/buzztard_menu_source_machine.png

icon48dir = $(iconthemedir)/48x48/apps
icon48_DATA = 48x48/buzztard.png

svgicondir = $(iconthemedir)/scalable/apps
svgicon_DATA = scalable/buzztard.svg

EXTRA_DIST = \
	$(icon16_DATA) $(icon24_DATA) $(icon48_DATA) $(svgicon_DATA)

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

