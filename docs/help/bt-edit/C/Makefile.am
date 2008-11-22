figdir = figures
docname = buzztard-edit
lang = C
omffile = buzztard-edit-C.omf
entities = legal.xml version.entities

all-local:
	@-chmod -R u+w $(srcdir)
	cp $(top_builddir)/docs/version.entities $(srcdir)/version.entities

clean-local:
	rm -f version.entities

include $(top_srcdir)/xmldocs.make
dist-hook: app-dist-hook

# here we should
# a) invoke xslproc, if available
# stylesheet=/opt/gnome/share/sgml/docbook/yelp/docbook/html/docbook.xsl
# xsltproc --nonet $(stylesheet) $(docname).xml
# b) simple install pregenerated html (pdf)

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
