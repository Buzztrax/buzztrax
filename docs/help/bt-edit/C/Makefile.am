figdir = figures
docname = buzztard-edit
lang = C
omffile = buzztard-edit-C.omf
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
buzztard-edit.pdf: buzztard-edit.xml dblatex_pdf.xsl
	dblatex \
	  -o buzztard-edit.pdf \
	  -I figures \
	  buzztard-edit.xml

buzztard-edit.txt: buzztard-edit.xml
	xmlto txt buzztard-edit.xml

buzztard-edit2.pdf: buzztard-edit.xml
	xsltproc \
	  --stringparam paper.type A4 \
	  --stringparam fop.extentions 1 \
	  --stringparam fop1.extentions 1 \
	  --stringparam admon.graphics 1 \
		http://docbook.sourceforge.net/release/xsl/current/fo/fo.xsl \
		buzztard-edit.xml >buzztard-edit.fo; \
	fop -fo buzztard-edit.fo -pdf buzztard-edit.pdf

# statistics for index building:
# sed -e 's/[[:space:]]\+/\n/g' buzztard-edit.txt | sed 's/[,.\(\)"]//g' | egrep -v '^ *$' | sort -f | uniq -ic | sort -n
# sed -e 's/[[:space:]]\+/\n/g' buzztard-edit.txt | sed 's/[,.\(\)"]//g' | egrep -v '^ *$' | egrep -v '^..?$' | sort -f | uniq -ic | sort -n


# here we should
# a) invoke xslproc, if available
# stylesheet=/opt/gnome/share/sgml/docbook/yelp/docbook/html/docbook.xsl
# xsltproc --nonet $(stylesheet) $(docname).xml
# b) simple install pregenerated html (pdf)

# this relies on particular names in the tests
# it would be nicer to fix the names in the tests and apply some rule:
#   /tmp/check_buzzard_help_(*).png -> figures/\1.png
#   - here we would mark the images by having an additional "_help" in the name
#   - right now this is prgname + widget name + optinal extra name
update_figures:
	if [ -e "/tmp/check_buzzard_main window.png" ]; then \
	  pngtopnm "/tmp/check_buzzard_main window.png" | ../../mk-shadow.sh | pnmtopng >figures/main-window.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_song views_machine view.png" ]; then \
	  pngtopnm "/tmp/check_buzzard_song views_machine view.png" | ../../mk-shadow.sh | pnmtopng >figures/page-machine-view.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_song views_pattern view.png" ]; then \
	  pngtopnm "/tmp/check_buzzard_song views_pattern view.png" | ../../mk-shadow.sh | pnmtopng >figures/page-pattern-view.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_song views_sequence view.png" ]; then \
	  pngtopnm "/tmp/check_buzzard_song views_sequence view.png" | ../../mk-shadow.sh | pnmtopng >figures/page-sequence-view.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_song views_song information.png" ]; then \
	  pngtopnm "/tmp/check_buzzard_song views_song information.png" | ../../mk-shadow.sh | pnmtopng >figures/page-info-view.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_song views_wave table view.png" ]; then \
	  pngtopnm "/tmp/check_buzzard_song views_wave table view.png" | ../../mk-shadow.sh | pnmtopng >figures/page-wavetable-view.png; \
	fi; \
	\
	if [ -e "/tmp/check_buzzard_pattern properties.png" ]; then \
	  pngtopnm "/tmp/check_buzzard_pattern properties.png" | ../../mk-shadow.sh | pnmtopng >figures/pattern-properties.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_machine properties.png" ]; then \
	  pngtopnm "/tmp/check_buzzard_machine properties.png" | ../../mk-shadow.sh | pnmtopng >figures/machine-properties.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_machine preferences.png" ]; then \
	  pngtopnm "/tmp/check_buzzard_machine preferences.png" | ../../mk-shadow.sh | pnmtopng >figures/machine-preferences.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_rename machine.png" ]; then \
	  pngtopnm "/tmp/check_buzzard_rename machine.png" | ../../mk-shadow.sh | pnmtopng >figures/machine-rename.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_preset name and comment.png" ]; then \
	  pngtopnm "/tmp/check_buzzard_preset name and comment.png" | ../../mk-shadow.sh | pnmtopng >figures/preset-properties.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_wire analysis.png" ]; then \
	  pngtopnm "/tmp/check_buzzard_wire analysis.png" | ../../mk-shadow.sh | pnmtopng >figures/signal-analysis.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_BtAboutDialog.png" ]; then \
	  pngtopnm "/tmp/check_buzzard_BtAboutDialog.png" | ../../mk-shadow.sh | pnmtopng >figures/about.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_Tip of the day.png" ]; then \
	  pngtopnm "/tmp/check_buzzard_Tip of the day.png" | ../../mk-shadow.sh | pnmtopng >figures/tip.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_song rendering.png" ]; then \
	  pngtopnm "/tmp/check_buzzard_song rendering.png" | ../../mk-shadow.sh | pnmtopng >figures/recording-settings.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_song render progress.png" ]; then \
	  pngtopnm "/tmp/check_buzzard_song render progress.png" | ../../mk-shadow.sh | pnmtopng >figures/recording-progress.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_Missing elements in song.png" ]; then \
	  pngtopnm "/tmp/check_buzzard_Missing elements in song.png" | ../../mk-shadow.sh | pnmtopng >figures/missing-song-elements.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_Missing GStreamer elements.png" ]; then \
	  pngtopnm "/tmp/check_buzzard_Missing GStreamer elements.png" | ../../mk-shadow.sh | pnmtopng >figures/missing-gstreamer-elements.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_Unsaved song recovery.png" ]; then \
	  pngtopnm "/tmp/check_buzzard_Unsaved song recovery.png" | ../../mk-shadow.sh | pnmtopng >figures/crash-recovery.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_interaction controller learn.png" ]; then \
	  pngtopnm "/tmp/check_buzzard_interaction controller learn.png" | ../../mk-shadow.sh | pnmtopng >figures/interaction-controller-learn.png; \
	fi; \
	\
	if [ -e "/tmp/check_buzzard_buzztard settings_audio devices.png" ]; then \
	  pngtopnm "/tmp/check_buzzard_buzztard settings_audio devices.png" | ../../mk-shadow.sh | pnmtopng >figures/settings-audio.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_buzztard settings_colors.png" ]; then \
	  pngtopnm "/tmp/check_buzzard_buzztard settings_colors.png" | ../../mk-shadow.sh | pnmtopng >figures/settings-colors.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_buzztard settings_directories.png" ]; then \
	  pngtopnm "/tmp/check_buzzard_buzztard settings_directories.png" | ../../mk-shadow.sh | pnmtopng >figures/settings-directories.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_buzztard settings_interaction controller.png" ]; then \
	  pngtopnm "/tmp/check_buzzard_buzztard settings_interaction controller.png" | ../../mk-shadow.sh | pnmtopng >figures/settings-ic.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_buzztard settings_playback controller.png" ]; then \
	  pngtopnm "/tmp/check_buzzard_buzztard settings_playback controller.png" | ../../mk-shadow.sh | pnmtopng >figures/settings-pc.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_buzztard settings_shortcuts.png" ]; then \
	  pngtopnm "/tmp/check_buzzard_buzztard settings_shortcuts.png" | ../../mk-shadow.sh | pnmtopng >figures/settings-shortcuts.png; \
	fi;

## add shadows (now integrated above)
## for PNG in *.png; do pngtopnm "$PNG" | ../../../mk-shadow.sh | pnmtopng > "${PNG//.png}"-with-shadow.png
