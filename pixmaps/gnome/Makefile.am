## $Id$

iconthemedir=$(datadir)/icons/gnome
gtk_update_icon_cache = gtk-update-icon-cache -f -t $(iconthemedir)

icon16dir = $(iconthemedir)/16x16/apps
icon16_DATA = 16x16/buzztard.png \
	16x16/menu_processor_machine.png \
	16x16/menu_sink_machine.png \
	16x16/menu_source_machine.png \
	16x16/tab_machines.png \
	16x16/tab_patterns.png \
	16x16/tab_sequence.png \
	16x16/tab_waves.png
##	16x16/tab_info.png

icon22dir = $(iconthemedir)/22x22/apps
icon22_DATA = 22x22/buzztard.png

icon24dir = $(iconthemedir)/24x24/apps
icon24_DATA = 24x24/buzztard.png \
	24x24/menu_grid.png \
	24x24/menu_processor_machine.png \
	24x24/menu_sink_machine.png \
	24x24/menu_source_machine.png

icon32dir = $(iconthemedir)/32x32/apps
icon32_DATA = 32x32/buzztard.png

icon48dir = $(iconthemedir)/48x48/apps
icon48_DATA = 48x48/buzztard.png

svgicondir = $(iconthemedir)/scalable/apps
svgicon_DATA = scalable/buzztard.svg \
	scalable/effect-bypass.svg \
	scalable/effect-mute.svg \
	scalable/effect.svg \
	scalable/generator-mute.svg \
	scalable/generator-solo.svg \
	scalable/generator.svg \
	scalable/master-mute.svg \
	scalable/master.svg


EXTRA_DIST = \
	$(icon16_DATA) $(icon22_DATA) $(icon24_DATA) \
	$(icon32_DATA) $(icon48_DATA) $(svgicon_DATA)

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

