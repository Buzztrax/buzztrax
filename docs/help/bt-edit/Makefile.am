@YELP_HELP_RULES@
HELP_ID = buzztrax-edit

HELP_FILES = \
	index.docbook \
	legal.xml

HELP_MEDIA = $(patsubst C/%,%,$(wildcard C/figures/*.png))
HELP_MEDIA += \
	version.entities

# We have no translation for the manual
HELP_LINGUAS =

DOC_MODULE = legal

C/version.entities: $(abs_top_builddir)/docs/version.entities
	mkdir -p C/
	@cp $< $@

CLEANFILES = C/version.entities

EXTRA_DIST = C/aspell.pws

if WITH_ASPELL
ASPELL_FILE=$(abs_srcdir)/C/$(DOC_MODULE).xml
ASPELL_ARGS=--lang=en_US -p '$(abs_srcdir)/C/aspell.pws' --mode=sgml

check-local:
	@echo "Spellchecking $(ASPELL_FILE)..."

	@if test 0 -ne `aspell list $(ASPELL_ARGS) < '$(ASPELL_FILE)' | wc -l`; then \
	  echo; \
	  echo "File doesn't pass spellchecking, please fix interactively by running:" >&2; \
	  echo "  aspell check $(ASPELL_ARGS) '$(ASPELL_FILE)'" >&2; \
	  echo; \
	  false; \
	else \
	  echo "  done"; \
	fi
else
check-local:
	@echo "Aspell is needed for 'make check'"
endif

# DB: The following used to live in the file C/Makefile.am. It was moved here to
# support out-of-tree builds.
#
# yelp-tools' "check-help" macro looks for a subdirectory called "C". If it doesn't
# find it then it presumes it's working out-of-tree and so it prepends "srcdir" to
# input file paths.
#
# If a Makefile.am is present in "C/", then this directory is generated in the build
# tree (i.e. build/docs/help/bt-edit/C/Makefile) and yelp-tools thinks it's in-tree,
# and so the checks don't work.

# make a pdf using dblatex
# - it seems unneccesarily hard to get our logo onto the titlepage
# - we tried all sort of customizations (	 -p dblatex_pdf.xsl )
buzztrax-edit.pdf: C/index.docbook C/dblatex_pdf.xsl
	dblatex \
	  -o C/buzztrax-edit.pdf \
	  -I C/figures \
	  C/index.docbook

buzztrax-edit.txt: C/index.docbook
	xmlto txt C/buzztrax-edit.xml

# statistics for index building:
# sed -e 's/[[:space:]]\+/\n/g' buzztrax-edit.txt | sed 's/[,.\(\)"]//g' | egrep -v '^ *$' | sort -f | uniq -ic | sort -n
# sed -e 's/[[:space:]]\+/\n/g' buzztrax-edit.txt | sed 's/[,.\(\)"]//g' | egrep -v '^ *$' | egrep -v '^..?$' | sort -f | uniq -ic | sort -n

# this relies on particular names in the tests
# it would be nicer to fix the names in the tests and apply some rule:
#   $$base/(*).png -> figures/\1.png
# maybe we just have a file with "from:to" entries
update_figures:
	base="/tmp/edit"; \
	cp "$$base/main_window.png" "C/figures/main-window.png"; \
	cp "$$base/song_views_machine_view.png" "C/figures/page-machine-view.png"; \
	cp "$$base/song_views_pattern_view.png" "C/figures/page-pattern-view.png"; \
	cp "$$base/song_views_sequence_view.png" "C/figures/page-sequence-view.png"; \
	cp "$$base/song_views_song_information.png" "C/figures/page-info-view.png"; \
	cp "$$base/song_views_wave_table_view.png" "C/figures/page-wavetable-view.png"; \
	\
	cp "$$base/pattern_properties.png" "C/figures/pattern-properties.png"; \
	cp "$$base/machine_properties_poly-source.png" "C/figures/machine-properties.png"; \
	cp "$$base/machine_preferences.png" "C/figures/machine-preferences.png"; \
	cp "$$base/rename_machine.png" "C/figures/machine-rename.png"; \
	cp "$$base/preset_name_and_comment.png" "C/figures/preset-properties.png"; \
	cp "$$base/wire_analysis.png" "C/figures/signal-analysis.png"; \
	cp "$$base/BtAboutDialog.png" "C/figures/about.png"; \
	cp "$$base/Tip_of_the_day.png" "C/figures/tip.png"; \
	cp "$$base/song_rendering.png" "C/figures/recording-settings.png"; \
	cp "$$base/Missing_elements_in_song.png" "C/figures/missing-song-elements.png"; \
	cp "$$base/Missing_GStreamer_elements.png" "C/figures/missing-gstreamer-elements.png"; \
	cp "$$base/Unsaved_song_recovery.png" "C/figures/crash-recovery.png"; \
	\
	cp "$$base/buzztrax_settings_audio_devices.png" "C/figures/settings-audio.png"; \
	cp "$$base/buzztrax_settings_colors.png" "C/figures/settings-colors.png"; \
	cp "$$base/buzztrax_settings_directories.png" "C/figures/settings-directories.png"; \
	cp "$$base/buzztrax_settings_interaction_controller.png" "C/figures/settings-ic.png"; \
	cp "$$base/buzztrax_settings_playback_controller.png" "C/figures/settings-pc.png"; \
	cp "$$base/buzztrax_settings_shortcuts.png" "C/figures/settings-shortcuts.png"; \
	cp "$$base/buzztrax_settings_ui.png" "C/figures/settings-ui.png";

.PHONY: update-figures
