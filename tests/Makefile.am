if BUILD_CHECK_TESTS
  SUBDIRS = lib ui
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
TESTS_ENVIRONMENT = \
	CK_DEFAULT_TIMEOUT=20 \
	LANG=C \
	XDG_CACHE_HOME=$(abs_builddir) \
	$(LIBTOOL) --mode=execute

if BUILD_CHECK_TESTS
noinst_PROGRAMS=$(TESTS_BIN)

/tmp/test.wav:
	gst-launch-0.10 -q audiotestsrc num-buffers=10 ! wavenc ! filesink location=$@

noinst_LTLIBRARIES = libbt-check.la
libbt_check_la_LIBADD = \
	$(top_builddir)/src/lib/core/libbuzztard-core.la \
	$(BASE_DEPS_LIBS) $(GUI_DEPS_LIBS) $(BT_LIBS) -lcheck
libbt_check_la_LDFLAGS = \
	$(BT_LDFLAGS) \
	-Wl,--rpath -Wl,$(abs_top_builddir)/src/lib/core/.libs
libbt_check_la_SOURCES = \
	bt-check.c bt-check.h \
	bt-test-application.c bt-test-application.h \
	bt-test-plugin.c bt-test-plugin.h \
	bt-test-settings.c bt-test-settings.h
libbt_check_la_DEPENDENCIES = /tmp/test.wav

bt_core_LDADD = \
	$(top_builddir)/tests/lib/core/libbtcore-check.la \
	$(top_builddir)/src/lib/core/libbuzztard-core.la \
	libbt-check.la $(BASE_DEPS_LIBS) $(BT_LIBS) $(LIBM) -lcheck
bt_core_LDFLAGS =  \
	$(BT_LDFLAGS) \
	-Wl,--rpath -Wl,$(abs_top_builddir)/src/lib/core/.libs
bt_core_SOURCES= \
	m-bt-core.c

bt_ic_LDADD = \
	$(top_builddir)/tests/lib/ic/libbtic-check.la \
	$(top_builddir)/src/lib/ic/libbuzztard-ic.la \
	$(top_builddir)/src/lib/core/libbuzztard-core.la \
	libbt-check.la $(BASE_DEPS_LIBS) $(BT_LIBS) $(LIBM) -lcheck
bt_ic_LDFLAGS =  \
	$(BT_LDFLAGS) \
	-Wl,--rpath -Wl,$(abs_top_builddir)/src/lib/ic/.libs
bt_ic_SOURCES= \
	m-bt-ic.c

bt_cmd_LDADD = \
	$(top_builddir)/tests/ui/cmd/libbt-cmd-check.la \
	$(top_builddir)/src/lib/core/libbuzztard-core.la \
	libbt-check.la $(BASE_DEPS_LIBS) $(BT_LIBS) $(LIBM) -lcheck
bt_cmd_LDFLAGS =  \
	$(BT_LDFLAGS) \
	-Wl,--rpath -Wl,$(abs_top_builddir)/src/lib/core/.libs
bt_cmd_SOURCES= \
	m-bt-cmd.c

bt_edit_LDADD = \
	$(top_builddir)/tests/ui/edit/libbt-edit-check.la \
	$(top_builddir)/src/lib/core/libbuzztard-core.la \
	$(top_builddir)/src/lib/ic/libbuzztard-ic.la \
	libbt-check.la $(BASE_DEPS_LIBS) $(GUI_DEPS_LIBS) $(BT_LIBS) $(LIBM) -lcheck
bt_edit_LDFLAGS =  \
	$(BT_LDFLAGS) \
	-Wl,--rpath -Wl,$(abs_top_builddir)/src/lib/core/.libs \
	-Wl,--rpath -Wl,$(abs_top_builddir)/src/lib/ic/.libs
bt_edit_SOURCES= \
	m-bt-edit.c

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/lib/core \
	-I$(top_srcdir)/src/lib/ic \
	-DLOCALEDIR=\"$(localedir)\" \
	-DDATADIR=\"$(datadir)\" \
	-DTESTSONGDIR=\"$(abs_top_srcdir)/tests/songs\" \
	-DG_LOG_DOMAIN=\"buzztard-check\" \
	$(BASE_DEPS_CFLAGS) \
	$(GUI_DEPS_CFLAGS) \
	$(BT_CFLAGS)

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
## LD_PRELOAD=../src/lib/core/.libs/libbuzztard-core.so.0.6.0 CK_FORK=no bcov -l../src/lib/core/.libs/libbuzztard-core.so.0.6.0 ./.libs/bt_cmd
## LD_PRELOAD=../src/lib/core/.libs/libbuzztard-core.so CK_FORK=no bcov -l../src/lib/core/.libs/libbuzztard-core.so ./.libs/bt_cmd
## CK_FORK=no bcov -l../src/lib/core/.libs/libbuzztard-core.so ./.libs/bt_cmd
# make coverage        -- generate coverage report from make check run
coverage:: $(TESTS_BIN)
	echo "command make check" >.bcovdump.all; \
	echo "date "`date` >>.bcovdump.all; \
	for i in $^; do \
	  CK_FORK="no" $(TESTS_ENVIRONMENT) bcov \
	    -l$(top_builddir)/src/lib/core/.libs/libbuzztard-core.so \
	    -l$(top_builddir)/src/lib/ic/.libs/libbuzztard-ic.so \
	    ./$$i; \
	  if test -f .bcovdump; then \
	    tail -n +5 ~/temp/.bcovdump >>.bcovdump.all; \
	    rm -f .bcovdump; \
	  fi \
	done
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
