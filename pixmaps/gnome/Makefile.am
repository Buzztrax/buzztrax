## $Id$

iconthemedir=$(datadir)/icons/gnome
gtk_update_icon_cache = gtk-update-icon-cache -f -t $(iconthemedir)

icon16dir = $(iconthemedir)/16x16/apps
icon16_DATA = 16x16/buzztard.png \
	16x16/buzztard_menu_processor_machine.png \
	16x16/buzztard_menu_sink_machine.png \
	16x16/buzztard_menu_source_machine.png \
	16x16/buzztard_tab_machines.png \
	16x16/buzztard_tab_patterns.png \
	16x16/buzztard_tab_sequence.png \
	16x16/buzztard_tab_waves.png
##	16x16/buzztard_tab_info.png

icon22dir = $(iconthemedir)/22x22/apps
icon22_DATA = 22x22/buzztard.png

icon24dir = $(iconthemedir)/24x24/apps
icon24_DATA = 24x24/buzztard.png \
	24x24/buzztard_menu_grid.png \
	24x24/buzztard_menu_processor_machine.png \
	24x24/buzztard_menu_sink_machine.png \
	24x24/buzztard_menu_source_machine.png

icon32dir = $(iconthemedir)/32x32/apps
icon32_DATA = 32x32/buzztard.png \
  32x32/buzztard_effect_bypass.png \
  32x32/buzztard_effect_mute.png \
  32x32/buzztard_effect.png \
  32x32/buzztard_generator_mute.png \
  32x32/buzztard_generator_solo.png \
  32x32/buzztard_generator.png \
  32x32/buzztard_master_mute.png \
  32x32/buzztard_master.png

icon48dir = $(iconthemedir)/48x48/apps
icon48_DATA = 48x48/buzztard.png \
  48x48/buzztard_effect_bypass.png \
  48x48/buzztard_effect_mute.png \
  48x48/buzztard_effect.png \
  48x48/buzztard_generator_mute.png \
  48x48/buzztard_generator_solo.png \
  48x48/buzztard_generator.png \
  48x48/buzztard_master_mute.png \
  48x48/buzztard_master.png

icon64dir = $(iconthemedir)/64x64/apps
icon64_DATA = 64x64/buzztard.png \
  64x64/buzztard_effect_bypass.png \
  64x64/buzztard_effect_mute.png \
  64x64/buzztard_effect.png \
  64x64/buzztard_generator_mute.png \
  64x64/buzztard_generator_solo.png \
  64x64/buzztard_generator.png \
  64x64/buzztard_master_mute.png \
  64x64/buzztard_master.png

MACHINE_ICONS = scalable/buzztard_effect_bypass.svg \
  scalable/buzztard_effect_mute.svg \
  scalable/buzztard_effect.svg \
  scalable/buzztard_generator_mute.svg \
  scalable/buzztard_generator_solo.svg \
  scalable/buzztard_generator.svg \
  scalable/buzztard_master_mute.svg \
  scalable/buzztard_master.svg

svgicondir = $(iconthemedir)/scalable/apps
svgicon_DATA = scalable/buzztard.svg $(MACHINE_ICONS)

EXTRA_DIST = \
	$(icon16_DATA) $(icon22_DATA) $(icon24_DATA) \
	$(icon32_DATA) $(icon48_DATA) $(icon64_DATA) $(svgicon_DATA)

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

prescale-svg:
	for size in 32 48 64; do \
		for icon in $(MACHINE_ICONS); do \
			rsvg-convert "$$icon" -w $$size -h $$size -f png -o "$${size}x$${size}/`basename $$icon .svg`.png"; \
		done; \
	done
