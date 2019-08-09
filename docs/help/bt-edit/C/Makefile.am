# make a pdf using dblatex
# - it seems unneccesarily hard to get our logo onto the titlepage
# - we tried all sort of customizations (	 -p dblatex_pdf.xsl )
buzztrax-edit.pdf: index.docbook dblatex_pdf.xsl
	dblatex \
	  -o buzztrax-edit.pdf \
	  -I figures \
	  index.docbook

buzztrax-edit.txt: index.docbook
	xmlto txt buzztrax-edit.xml

# statistics for index building:
# sed -e 's/[[:space:]]\+/\n/g' buzztrax-edit.txt | sed 's/[,.\(\)"]//g' | egrep -v '^ *$' | sort -f | uniq -ic | sort -n
# sed -e 's/[[:space:]]\+/\n/g' buzztrax-edit.txt | sed 's/[,.\(\)"]//g' | egrep -v '^ *$' | egrep -v '^..?$' | sort -f | uniq -ic | sort -n

# this relies on particular names in the tests
# it would be nicer to fix the names in the tests and apply some rule:
#   $$base/(*).png -> figures/\1.png
# maybe we just have a file with "from:to" entries
update_figures:
	base="/tmp/bt_edit"; \
  ../../mk-shadow-png.sh "$$base/main_window.png" "figures/main-window.png"; \
	../../mk-shadow-png.sh "$$base/song_views_machine_view.png" "figures/page-machine-view.png"; \
	../../mk-shadow-png.sh "$$base/song_views_pattern_view.png" "figures/page-pattern-view.png"; \
	../../mk-shadow-png.sh "$$base/song_views_sequence_view.png" "figures/page-sequence-view.png"; \
  ../../mk-shadow-png.sh "$$base/song_views_song_information.png" "figures/page-info-view.png"; \
	../../mk-shadow-png.sh "$$base/song_views_wave_table_view.png" "figures/page-wavetable-view.png"; \
	\
	../../mk-shadow-png.sh "$$base/pattern_properties.png" "figures/pattern-properties.png"; \
	../../mk-shadow-png.sh "$$base/machine_properties_poly-source.png" "figures/machine-properties.png"; \
	../../mk-shadow-png.sh "$$base/machine_preferences.png" "figures/machine-preferences.png"; \
	../../mk-shadow-png.sh "$$base/rename_machine.png" "figures/machine-rename.png"; \
	../../mk-shadow-png.sh "$$base/preset_name_and_comment.png" "figures/preset-properties.png"; \
	../../mk-shadow-pmg.sh "$$base/wire_analysis.png" "figures/signal-analysis.png"; \
	../../mk-shadow-png.sh "$$base/BtAboutDialog.png" "figures/about.png"; \
	../../mk-shadow-png.sh "$$base/Tip_of_the_day.png" "figures/tip.png"; \
	../../mk-shadow-png.sh "$$base/song_rendering.png" "figures/recording-settings.png"; \
	../../mk-shadow-png.sh "$$base/Missing_elements_in_song.png" "figures/missing-song-elements.png"; \
	../../mk-shadow-png.sh "$$base/Missing_GStreamer_elements.png" "figures/missing-gstreamer-elements.png"; \
	../../mk-shadow-png.sh "$$base/Unsaved_song_recovery.png" "figures/crash-recovery.png"; \
	\
	../../mk-shadow-png.sh "$$base/buzztrax_settings_audio_devices.png" "figures/settings-audio.png"; \
	../../mk-shadow-png.sh "$$base/buzztrax_settings_colors.png" "figures/settings-colors.png"; \
	../../mk-shadow-png.sh "$$base/buzztrax_settings_directories.png" "figures/settings-directories.png"; \
	../../mk-shadow-png.sh "$$base/buzztrax_settings_interaction_controller.png" "figures/settings-ic.png"; \
	../../mk-shadow-png.sh "$$base/buzztrax_settings_playback_controller.png" "figures/settings-pc.png"; \
	../../mk-shadow-png.sh "$$base/buzztrax_settings_shortcuts.png" "figures/settings-shortcuts.png"; \
	../../mk-shadow-png.sh "$$base/buzztrax_settings_ui.png" "figures/settings-ui.png";

.PHONY: update-figures