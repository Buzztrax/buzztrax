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
	if [ -e "$$base/main_window.png" ]; then \
	  pngtopnm "$$base/main_window.png" | ../../mk-shadow.sh | pnmtopng >figures/main-window.png; \
	fi; \
	if [ -e "$$base/song_views_machine_view.png" ]; then \
	  pngtopnm "$$base/song_views_machine_view.png" | ../../mk-shadow.sh | pnmtopng >figures/page-machine-view.png; \
	fi; \
	if [ -e "$$base/song_views_pattern_view.png" ]; then \
	  pngtopnm "$$base/song_views_pattern_view.png" | ../../mk-shadow.sh | pnmtopng >figures/page-pattern-view.png; \
	fi; \
	if [ -e "$$base/song_views_sequence_view.png" ]; then \
	  pngtopnm "$$base/song_views_sequence_view.png" | ../../mk-shadow.sh | pnmtopng >figures/page-sequence-view.png; \
	fi; \
	if [ -e "$$base/song_views_song_information.png" ]; then \
	  pngtopnm "$$base/song_views_song_information.png" | ../../mk-shadow.sh | pnmtopng >figures/page-info-view.png; \
	fi; \
	if [ -e "$$base/song_views_wave_table_view.png" ]; then \
	  pngtopnm "$$base/song_views_wave_table_view.png" | ../../mk-shadow.sh | pnmtopng >figures/page-wavetable-view.png; \
	fi; \
	\
	if [ -e "$$base/pattern_properties.png" ]; then \
	  pngtopnm "$$base/pattern_properties.png" | ../../mk-shadow.sh | pnmtopng >figures/pattern-properties.png; \
	fi; \
	if [ -e "$$base/machine_properties_poly-source.png" ]; then \
	  pngtopnm "$$base/machine_properties_poly-source.png" | ../../mk-shadow.sh | pnmtopng >figures/machine-properties.png; \
	fi; \
	if [ -e "$$base/machine_preferences.png" ]; then \
	  pngtopnm "$$base/machine_preferences.png" | ../../mk-shadow.sh | pnmtopng >figures/machine-preferences.png; \
	fi; \
	if [ -e "$$base/rename_machine.png" ]; then \
	  pngtopnm "$$base/rename_machine.png" | ../../mk-shadow.sh | pnmtopng >figures/machine-rename.png; \
	fi; \
	if [ -e "$$base/preset_name_and_comment.png" ]; then \
	  pngtopnm "$$base/preset_name_and_comment.png" | ../../mk-shadow.sh | pnmtopng >figures/preset-properties.png; \
	fi; \
	if [ -e "$$base/wire_analysis.png" ]; then \
	  pngtopnm "$$base/wire_analysis.png" | ../../mk-shadow.sh | pnmtopng >figures/signal-analysis.png; \
	fi; \
	if [ -e "$$base/BtAboutDialog.png" ]; then \
	  pngtopnm "$$base/BtAboutDialog.png" | ../../mk-shadow.sh | pnmtopng >figures/about.png; \
	fi; \
	if [ -e "$$base/Tip_of_the_day.png" ]; then \
	  pngtopnm "$$base/Tip_of_the_day.png" | ../../mk-shadow.sh | pnmtopng >figures/tip.png; \
	fi; \
	if [ -e "$$base/song_rendering.png" ]; then \
	  pngtopnm "$$base/song_rendering.png" | ../../mk-shadow.sh | pnmtopng >figures/recording-settings.png; \
	fi; \
	if [ -e "$$base/Missing_elements_in_song.png" ]; then \
	  pngtopnm "$$base/Missing_elements_in_song.png" | ../../mk-shadow.sh | pnmtopng >figures/missing-song-elements.png; \
	fi; \
	if [ -e "$$base/Missing_GStreamer_elements.png" ]; then \
	  pngtopnm "$$base/Missing_GStreamer_elements.png" | ../../mk-shadow.sh | pnmtopng >figures/missing-gstreamer-elements.png; \
	fi; \
	if [ -e "$$base/Unsaved_song_recovery.png" ]; then \
	  pngtopnm "$$base/Unsaved_song_recovery.png" | ../../mk-shadow.sh | pnmtopng >figures/crash-recovery.png; \
	fi; \
	\
	if [ -e "$$base/buzztrax_settings_audio_devices.png" ]; then \
	  pngtopnm "$$base/buzztrax_settings_audio_devices.png" | ../../mk-shadow.sh | pnmtopng >figures/settings-audio.png; \
	fi; \
	if [ -e "$$base/buzztrax_settings_colors.png" ]; then \
	  pngtopnm "$$base/buzztrax_settings_colors.png" | ../../mk-shadow.sh | pnmtopng >figures/settings-colors.png; \
	fi; \
	if [ -e "$$base/buzztrax_settings_directories.png" ]; then \
	  pngtopnm "$$base/buzztrax_settings_directories.png" | ../../mk-shadow.sh | pnmtopng >figures/settings-directories.png; \
	fi; \
	if [ -e "$$base/buzztrax_settings_interaction_controller.png" ]; then \
	  pngtopnm "$$base/buzztrax_settings_interaction_controller.png" | ../../mk-shadow.sh | pnmtopng >figures/settings-ic.png; \
	fi; \
	if [ -e "$$base/buzztrax_settings_playback_controller.png" ]; then \
	  pngtopnm "$$base/buzztrax_settings_playback_controller.png" | ../../mk-shadow.sh | pnmtopng >figures/settings-pc.png; \
	fi; \
	if [ -e "$$base/buzztrax_settings_shortcuts.png" ]; then \
	  pngtopnm "$$base/buzztrax_settings_shortcuts.png" | ../../mk-shadow.sh | pnmtopng >figures/settings-shortcuts.png; \
	fi; \
	if [ -e "$$base/buzztrax_settings_ui.png" ]; then \
	  pngtopnm "$$base/buzztrax_settings_ui.png" | ../../mk-shadow.sh | pnmtopng >figures/settings-ui.png; \
	fi;

.PHONY: update-figures