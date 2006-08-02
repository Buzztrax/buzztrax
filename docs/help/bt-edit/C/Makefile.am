figdir = figures
docname = bt-edit
lang = C
omffile = bt-edit-C.omf
entities = legal.xml ../../../version.entities

include $(top_srcdir)/xmldocs.make
dist-hook: app-dist-hook

# here we should
# a) invoke xslproc, if available
# stylesheet=/opt/gnome/share/sgml/docbook/yelp/docbook/html/docbook.xsl
# xsltproc --nonet $(stylesheet) $(docname).xml
# b) simple install pregenerated html (pdf)

update_figures:
	if [ -e "/tmp/check_buzzard_main window.png" ]; then \
	  cp "/tmp/check_buzzard_main window.png" figures/main-window.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_pattern properties.png" ]; then \
	  cp "/tmp/check_buzzard_pattern properties.png" figures/pattern-properties.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_buzztard settings.png" ]; then \
	  cp "/tmp/check_buzzard_buzztard settings.png" figures/settings.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_song views_machine view.png" ]; then \
	  cp "/tmp/check_buzzard_song views_machine view.png" figures/page-machine-view.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_song views_pattern view.png" ]; then \
	  cp "/tmp/check_buzzard_song views_pattern view.png" figures/page-pattern-view.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_song views_sequence view.png" ]; then \
	  cp "/tmp/check_buzzard_song views_sequence view.png" figures/page-sequence-view.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_song views_song information.png" ]; then \
	  cp "/tmp/check_buzzard_song views_song information.png" figures/page-info-view.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_song views_wave table view.png" ]; then \
	  cp "/tmp/check_buzzard_song views_wave table view.png" figures/page-wavetable-view.png; \
	fi;
