figdir = figures
docname = bt-edit
lang = C
omffile = bt-edit-C.omf
entities = legal.xml version.entities

all-local:
	cp ../../../version.entities version.entities

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
	if [ -e "/tmp/check_buzzard_pattern properties.png" ]; then \
	  pngtopnm "/tmp/check_buzzard_pattern properties.png" | ../../mk-shadow.sh | pnmtopng >figures/pattern-properties.png; \
	fi; \
	if [ -e "/tmp/check_buzzard_buzztard settings.png" ]; then \
	  pngtopnm "/tmp/check_buzzard_buzztard settings.png" | ../../mk-shadow.sh | pnmtopng >figures/settings.png; \
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
	fi;

## add shadows (now integrated above)
## for PNG in *.png; do pngtopnm "$PNG" | ../../../mk-shadow.sh | pnmtopng > "${PNG//.png}"-with-shadow.png
