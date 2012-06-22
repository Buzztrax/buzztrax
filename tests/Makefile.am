if BUILD_CHECK_TESTS
  TESTS_BIN = bt_core bt_ic bt_cmd bt_edit
  ##TESTS_BIN = bt_core
else
  TESTS_BIN =
endif

TESTS = \
  $(TESTS_BIN) \
  bt-cmd-info.sh \
  bt-cmd-cli.sh \
  bt-edit-cli.sh \
  povalid.sh \
  xmlvalid.sh

# prevent endless loops, this requires bash, see idea in bt-check.c
#   ulimit -St20;
#
# adding this can help debug gconfd
#    GCONF_DEBUG_TRACE_CLIENT=1
#
# http://www.cynapses.org/tmp/gcc/malloc_perturb
#    MALLOC_PERTURB_=$(($RANDOM % 255 + 1))
#
# set theme (check which theme is used)
#    GTK2_RC_FILES=$HOME/.themes/Darkilouche/gtk-2.0/gtkrc
#
# hint alsa/pulseaudio to be quiet
#    PULSE_SINK=???
#    ALSA_CARD, ALSA_PCM_CARD, DISPLAY_ALSA_DEVICE, AUDIODEV
#
TESTS_ENVIRONMENT = \
	CK_DEFAULT_TIMEOUT=20 \
	LANG=C \
	XDG_CACHE_HOME=$(abs_builddir) \
	$(LIBTOOL) --mode=execute

if BUILD_CHECK_TESTS
noinst_PROGRAMS=$(TESTS_BIN)

/tmp/test.wav:
	$(AM_V_GEN) gst-launch-@GST_MAJORMINOR@ -q audiotestsrc num-buffers=10 ! wavenc ! filesink location=$@

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/lib/core \
	-I$(top_srcdir)/src/lib/ic \
	-DLOCALEDIR=\"$(localedir)\" \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(datadir)\" \
	-DTESTSONGDIR=\"$(abs_top_srcdir)/tests/songs\" \
	-DG_LOG_DOMAIN=\"buzztard-check\" \
	$(BASE_DEPS_CFLAGS) \
	$(GUI_DEPS_CFLAGS) \
	$(BT_CFLAGS)

AM_LDFLAGS = $(BT_LDFLAGS)

noinst_LTLIBRARIES = \
  libbt-check.la \
  libbtcore-check.la \
  libbtic-check.la \
  libbt-cmd-check.la \
  libbt-edit-check.la

libbt_check_la_LIBADD = \
	$(top_builddir)/src/libbuzztard-core.la \
	$(BASE_DEPS_LIBS) $(GUI_DEPS_LIBS) $(BT_LIBS) -lcheck
libbt_check_la_LDFLAGS = \
	-Wl,--rpath -Wl,$(abs_top_builddir)/src/.libs
libbt_check_la_SOURCES = \
	bt-check.c bt-check.h \
	bt-test-application.c bt-test-application.h \
	bt-test-plugin.c bt-test-plugin.h \
	bt-test-settings.c bt-test-settings.h
libbt_check_la_DEPENDENCIES = /tmp/test.wav

bt_core_LDADD = \
	libbtcore-check.la \
	$(top_builddir)/src/libbuzztard-core.la \
	libbt-check.la $(BASE_DEPS_LIBS) $(BT_LIBS) $(LIBM) -lcheck
bt_core_LDFLAGS =  \
	-Wl,--rpath -Wl,$(abs_top_builddir)/src/.libs
bt_core_SOURCES= \
	m-bt-core.c

bt_ic_LDADD = \
	libbtic-check.la \
	$(top_builddir)/src/libbuzztard-ic.la \
	$(top_builddir)/src/libbuzztard-core.la \
	libbt-check.la $(BASE_DEPS_LIBS) $(BT_LIBS) $(LIBM) -lcheck
bt_ic_LDFLAGS =  \
	-Wl,--rpath -Wl,$(abs_top_builddir)/src/.libs
bt_ic_SOURCES= \
	m-bt-ic.c

bt_cmd_LDADD = \
	libbt-cmd-check.la \
	$(top_builddir)/src/libbuzztard-core.la \
	libbt-check.la $(BASE_DEPS_LIBS) $(BT_LIBS) $(LIBM) -lcheck
bt_cmd_LDFLAGS =  \
	-Wl,--rpath -Wl,$(abs_top_builddir)/src/.libs
bt_cmd_SOURCES= \
	m-bt-cmd.c

bt_edit_LDADD = \
	libbt-edit-check.la \
	$(top_builddir)/src/libbuzztard-core.la \
	$(top_builddir)/src/libbuzztard-ic.la \
	libbt-check.la $(BASE_DEPS_LIBS) $(GUI_DEPS_LIBS) $(BT_LIBS) $(LIBM) -lcheck
bt_edit_LDFLAGS =  \
	-Wl,--rpath -Wl,$(abs_top_builddir)/src/.libs
bt_edit_SOURCES= \
	m-bt-edit.c

libbtcore_check_la_LIBADD = $(BASE_DEPS_LIBS)
libbtcore_check_la_SOURCES = lib/core/m-bt-core.h \
	lib/core/s-application.c lib/core/e-application.c \
	lib/core/s-audio-session.c lib/core/e-audio-session.c \
	lib/core/s-cmd-pattern.c lib/core/e-cmd-pattern.c lib/core/t-cmd-pattern.c \
	lib/core/s-core.c lib/core/e-core.c lib/core/t-core.c \
	lib/core/s-gconf-settings.c lib/core/e-gconf-settings.c \
	lib/core/s-machine.c lib/core/e-machine.c lib/core/t-machine.c \
	lib/core/s-parameter-group.c lib/core/e-parameter-group.c lib/core/t-parameter-group.c \
	lib/core/s-pattern.c lib/core/e-pattern.c lib/core/t-pattern.c \
	lib/core/s-persistence.c lib/core/e-persistence.c lib/core/t-persistence.c \
	lib/core/s-processor-machine.c lib/core/e-processor-machine.c lib/core/t-processor-machine.c \
	lib/core/s-sequence.c lib/core/e-sequence.c lib/core/t-sequence.c \
	lib/core/s-settings.c lib/core/e-settings.c \
	lib/core/s-setup.c lib/core/e-setup.c lib/core/t-setup.c \
	lib/core/s-sink-machine.c lib/core/e-sink-machine.c lib/core/t-sink-machine.c \
	lib/core/s-song.c lib/core/e-song.c lib/core/t-song.c \
	lib/core/s-song-io.c lib/core/e-song-io.c lib/core/t-song-io.c \
	lib/core/s-song-io-native.c lib/core/e-song-io-native.c lib/core/t-song-io-native.c \
	lib/core/s-song-info.c lib/core/e-song-info.c lib/core/t-song-info.c \
	lib/core/s-source-machine.c lib/core/e-source-machine.c lib/core/t-source-machine.c \
	lib/core/s-tools.c lib/core/e-tools.c lib/core/t-tools.c \
	lib/core/s-value-group.c lib/core/e-value-group.c lib/core/t-value-group.c \
	lib/core/s-wire.c lib/core/e-wire.c lib/core/t-wire.c

libbtic_check_la_LIBADD = $(BASE_DEPS_LIBS)
libbtic_check_la_SOURCES = lib/ic/m-bt-ic.h \
	lib/ic/s-ic.c lib/ic/e-ic.c lib/ic/t-ic.c \
	lib/ic/s-registry.c lib/ic/e-registry.c

libbt_cmd_check_la_LIBADD = \
	$(top_builddir)/src/libbtcmd.la \
	$(BASE_DEPS_LIBS)
libbt_cmd_check_la_SOURCES = ui/cmd/m-bt-cmd.h \
	ui/cmd/s-bt-cmd-application.c ui/cmd/e-bt-cmd-application.c ui/cmd/t-bt-cmd-application.c

libbt_edit_check_la_CFLAGS = $(GUI_DEPS_CFLAGS)
libbt_edit_check_la_LIBADD = \
	$(top_builddir)/src/libbtedit.la \
	$(BASE_DEPS_LIBS) $(GUI_DEPS_LIBS)
libbt_edit_check_la_SOURCES = ui/edit/m-bt-edit.h \
	ui/edit/s-bt-about-dialog.c ui/edit/e-bt-about-dialog.c \
	ui/edit/s-bt-change-log.c ui/edit/e-bt-change-log.c \
	ui/edit/s-bt-controller-learn-dialog.c ui/edit/e-bt-controller-learn-dialog.c \
	ui/edit/s-bt-crash-recover-dialog.c ui/edit/e-bt-crash-recover-dialog.c \
	ui/edit/s-bt-edit-application.c ui/edit/e-bt-edit-application.c \
	ui/edit/s-bt-interaction-controller-menu.c ui/edit/e-bt-interaction-controller-menu.c \
	ui/edit/s-bt-machine-actions.c ui/edit/e-bt-machine-actions.c \
	ui/edit/s-bt-machine-page.c ui/edit/e-bt-machine-page.c \
	ui/edit/s-bt-machine-preset-properties-dialog.c ui/edit/e-bt-machine-preset-properties-dialog.c \
	ui/edit/s-bt-machine-preferences-dialog.c ui/edit/e-bt-machine-preferences-dialog.c \
	ui/edit/s-bt-machine-properties-dialog.c ui/edit/e-bt-machine-properties-dialog.c \
	ui/edit/s-bt-machine-rename-dialog.c ui/edit/e-bt-machine-rename-dialog.c \
	ui/edit/s-bt-main-window.c ui/edit/e-bt-main-window.c \
	ui/edit/s-bt-missing-framework-elements-dialog.c ui/edit/e-bt-missing-framework-elements-dialog.c \
	ui/edit/s-bt-missing-song-elements-dialog.c ui/edit/e-bt-missing-song-elements-dialog.c \
	ui/edit/s-bt-pattern-page.c ui/edit/e-bt-pattern-page.c ui/edit/t-bt-pattern-page.c \
	ui/edit/s-bt-pattern-properties-dialog.c ui/edit/e-bt-pattern-properties-dialog.c \
	ui/edit/s-bt-render-dialog.c ui/edit/e-bt-render-dialog.c \
	ui/edit/s-bt-render-progress-dialog.c ui/edit/e-bt-render-progress-dialog.c \
	ui/edit/s-bt-sequence-page.c ui/edit/e-bt-sequence-page.c \
	ui/edit/s-bt-settings-dialog.c ui/edit/e-bt-settings-dialog.c \
	ui/edit/s-bt-signal-analysis-dialog.c ui/edit/e-bt-signal-analysis-dialog.c \
	ui/edit/s-bt-tip-dialog.c ui/edit/e-bt-tip-dialog.c

endif

songdatadir = $(datadir)/$(PACKAGE)/songs
songdata_DATA = \
	songs/buzz1.xml \
	songs/buzz2.xml \
	songs/buzz3.xml \
	songs/buzz4.xml \
	songs/buzz5.xml \
	songs/buzz6.xml \
	songs/buzz7.xml \
	songs/buzz8.xml \
	songs/buzz9.xml \
	songs/combi1.xml \
	songs/combi2.xml \
	songs/combi3.xml \
	songs/melo1.xml \
	songs/melo2.xml \
	songs/melo3.xml \
	songs/melo4.xml \
	songs/melo5.xml \
	songs/melo6.xml

noinst_songdata = \
	songs/broken1.xml \
	songs/broken1.bzt \
	songs/broken2.xml \
	songs/broken3.xml \
	songs/broken4.xml \
	songs/empty.xml \
	songs/example.xml \
	songs/samples1.bzt \
	songs/samples2.bzt \
	songs/simple1.xml \
	songs/simple2.xml \
	songs/simple3.xml \
	songs/simple4.xml \
	songs/simple5.xml \
	songs/simple6.xml \
	songs/simple1.bzt \
	songs/simple2.bzt \
	songs/simple3.bzt \
	songs/simple4.bzt \
	songs/simple5.bzt \
	songs/test-simple0.xml \
	songs/test-simple1.xml \
	songs/test-simple2.xml \
	songs/test-simple3.xml \
	songs/test-simple4.xml \
	songs/test-simple5.xml

EXTRA_DIST = \
	$(songdata_DATA) $(noinst_songdata) \
	gtkdoccomplete.sh povalid.sh xmlvalid.sh \
	bt-cfg.sh.in bt-cmd-info.sh bt-cmd-encode.sh \
	bt-cmd-cli.sh bt-edit-cli.sh

clean-local:
	rm -f \
	  $(top_builddir)/tests/songs/*.txt \
	  $(top_builddir)/tests/songs/*.ogg \
	  $(abs_builddir)/buzztard/*.log \
	  $(top_builddir)/tests/event-sound-cache.tdb.* \
	  /tmp/test.wav
	-rmdir $(abs_builddir)/buzztard

# make check           -- run all checks
# make (test).check    -- run the given check once
#	make (test).check BT_CHECKS="test5*" -- run the given testcase(s) only
%.check: %
	@$(TESTS_ENVIRONMENT)	\
	./$*

# make (test).gdb      -- start up gdb for the given test
%.gdb: %
	@CK_FORK=no		\
	$(TESTS_ENVIRONMENT)	\
	gdb ./$*

# make (test).refdbg   -- start up refdbg for the given test
%.refdbg: %
	@CK_FORK=no		\
	$(TESTS_ENVIRONMENT)	\
	refdbg -c "btnum=20 ; logobjs=0 ; $(REFDBG_RULE)" \
	./$*

## todo: check if code has been compiled using --enable-debug
##
##check-local:
##	echo "========================================"
##	echo "No check based tests will run"
##	echo "build with --enable-debug=yes"
##	echo "========================================"

.PHONY: coverage valgrind test-status

if USE_BCOV
## CK_FORK=no bcov -l../src/lib/core/.libs/libbuzztard-core.so ./.libs/bt_cmd
## CK_FORK=no bcov -l../src/lib/core/.libs/libbuzztard-core.so -l../src/lib/ic/.libs/libbuzztard-ic.so ./.libs/bt_core
## CK_FORK=no libtool --mode=execute bcov -l../src/lib/core/.libs/libbuzztard-core.so -l../src/lib/ic/.libs/libbuzztard-ic.so ./bt_core
## - we're missing the libs coverage, but it is not due to BSymbolic :/
# make coverage        -- generate coverage report from make check run
coverage:: $(TESTS_BIN)
	echo "command make check" >.bcovdump.all; \
	echo "date "`date` >>.bcovdump.all; \
	for i in $^; do \
	  CK_FORK="no" $(TESTS_ENVIRONMENT) bcov \
	    -l$(top_builddir)/src/.libs/libbuzztard-core.so \
	    -l$(top_builddir)/src/.libs/libbuzztard-ic.so \
	    ./$$i; \
	  if test -f .bcovdump; then \
	    tail -n +5 .bcovdump >>.bcovdump.all; \
	    rm -f .bcovdump; \
	  fi \
	done; \
	mkdir -p coverage; \
	bcov-report -i $(top_abs_srcdir) .bcovdump.all coverage
endif

if USE_VALGRIND
VALPREFIX = \
	CK_FORK=no CK_DEFAULT_TIMEOUT=500 \
	LANG=C XDG_CACHE_HOME=$(abs_builddir) \
	G_SLICE=always-malloc G_DEBUG=gc-friendly \
	GLIBCPP_FORCE_NEW=1 GLIBCXX_FORCE_NEW=1
VALDEFAULT = @VALGRIND@/bin/valgrind
VALSUPP = @VALGRIND@/lib/valgrind
VALSUPPRESSIONDEF = --suppressions=$(VALSUPP)/default.supp
VALSUPPRESSIONOWN = --suppressions=$(top_builddir)/buzztard.supp \
	--suppressions=$(top_builddir)/gst.supp \
	--suppressions=$(top_builddir)/gtk.supp
VALSUPPRESSION = $(VALSUPPRESSIONDEF) $(VALSUPPRESSIONOWN)
VALOPTIONS = --trace-children=yes --num-callers=30 --read-var-info=yes \
  --tool=memcheck --leak-check=full --leak-resolution=high --track-origins=yes
VALCMD = $(VALPREFIX) $(VALDEFAULT) $(VALOPTIONS) $(VALSUPPRESSION)

## for file in /tmp/bt_core.valgrind.pid*; do grep -q "ERROR SUMMARY: 0 errors from 0 contexts" $file ; if test $? == 0; then rm $file; fi; done
## grep -H "ERROR SUMMARY: " /tmp/bt_core.valgrind.pid* | uniq

# make valgrind        -- valgrind all tests
valgrind:: $(TESTS_BIN)
	for i in $^; do \
	  rm -rf /tmp/$$i.valgrind.pid*; \
	  $(VALCMD) --log-file=/tmp/$$i_valgrind.pid%p ./$$i; \
	done

# make (test).valgrind -- valgrind the given test"
%.valgrind: %
	rm -rf /tmp/$*.valgrind.pid*; \
	$(VALCMD) --log-file=/tmp/$*.valgrind.pid%p ./$*;
endif

# make test-status     -- show info about disabled tests
test-status::
	@grep -r -A2 -B6 -Hn --include="*.c" "#ifdef __CHECK_DISABLED__" . | grep -e '\.c-[0-9][0-9]*-\(\/\/\|\/\*\|\ \*\|ST\)' 
	@echo "number of disabled tests"
	@grep -r -Hn --include="*.c" --color=auto "#ifdef __CHECK_DISABLED__" . | wc -l

include $(top_srcdir)/common.mak
