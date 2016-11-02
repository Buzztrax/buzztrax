figdir = figures
docname = buzztrax-edit
lang = C
omffile = buzztrax-edit-C.omf
entities = legal.xml version.entities

all-local:
	@-chmod -R u+w $(srcdir)
	@cp $(top_builddir)/docs/version.entities $(srcdir)/version.entities

clean-local:
	-@rm -f version.entities

# FIXME: only do this if ENABLE_SCROLLKEEPER ?
include $(top_srcdir)/xmldocs.make
dist-hook: app-dist-hook

# make a pdf using dblatex
# - it seems unneccesarily hard to get our logo onto the titlepage
# - we tried all sort of customizations (	 -p dblatex_pdf.xsl )
buzztrax-edit.pdf: buzztrax-edit.xml dblatex_pdf.xsl
	dblatex \
	  -o buzztrax-edit.pdf \
	  -I figures \
	  buzztrax-edit.xml

buzztrax-edit.txt: buzztrax-edit.xml
	xmlto txt buzztrax-edit.xml

buzztrax-edit2.pdf: buzztrax-edit.xml
	xsltproc \
	  --stringparam paper.type A4 \
	  --stringparam fop.extentions 1 \
	  --stringparam fop1.extentions 1 \
	  --stringparam admon.graphics 1 \
		http://docbook.sourceforge.net/release/xsl/current/fo/fo.xsl \
		buzztrax-edit.xml >buzztrax-edit.fo; \
	fop -fo buzztrax-edit.fo -pdf buzztrax-edit.pdf

# statistics for index building:
# sed -e 's/[[:space:]]\+/\n/g' buzztrax-edit.txt | sed 's/[,.\(\)"]//g' | egrep -v '^ *$' | sort -f | uniq -ic | sort -n
# sed -e 's/[[:space:]]\+/\n/g' buzztrax-edit.txt | sed 's/[,.\(\)"]//g' | egrep -v '^ *$' | egrep -v '^..?$' | sort -f | uniq -ic | sort -n


# here we should
# a) invoke xslproc, if available
# stylesheet=/opt/gnome/share/sgml/docbook/yelp/docbook/html/docbook.xsl
# xsltproc --nonet $(stylesheet) $(docname).xml
# b) simple install pregenerated html (pdf)

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
	if [ -e "$$base/song_render_progress.png" ]; then \
	  pngtopnm "$$base/song_render_progress.png" | ../../mk-shadow.sh | pnmtopng >figures/recording-progress.png; \
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

## add shadows (now integrated above)
## for PNG in *.png; do pngtopnm "$PNG" | ../../../mk-shadow.sh | pnmtopng > "${PNG//.png}"-with-shadow.png

.PHONY: update-figures