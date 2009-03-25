## $Id$

if BUILD_CHECK_TESTS
  SUBDIRS = lib ui
  TESTS_BIN = bt_core bt_cmd bt_edit
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
	LANG=en_GB \
	$(LIBTOOL) --mode=execute

if BUILD_CHECK_TESTS
noinst_PROGRAMS=$(TESTS_BIN)

noinst_LTLIBRARIES = libbt-check.la
libbt_check_la_LIBADD = $(BASE_DEPS_LIBS) $(GUI_DEPS_LIBS) $(BT_LIBS) -lcheck
libbt_check_la_SOURCES = \
	bt-check.c bt-check.h \
	bt-test-plugin.c bt-test-plugin.h \
	bt-test-settings.c bt-test-settings.h

bt_core_LDADD = \
	$(top_builddir)/src/lib/core/libbuzztard-core.la \
	$(top_builddir)/tests/lib/core/libbtcore-check.la \
	libbt-check.la $(BASE_DEPS_LIBS) $(BT_LIBS) -lcheck
bt_core_LDFLAGS = -Wl,--rpath -Wl,$(top_builddir)/src/lib/core 
bt_core_SOURCES= \
	m-bt-core.c

bt_cmd_LDADD = \
	$(top_builddir)/src/lib/core/libbuzztard-core.la \
	$(top_builddir)/tests/ui/cmd/libbt-cmd-check.la \
	libbt-check.la $(BASE_DEPS_LIBS) $(BT_LIBS) -lcheck
bt_cmd_LDFLAGS = -Wl,--rpath -Wl,$(top_builddir)/src/lib/core 
bt_cmd_SOURCES= \
	m-bt-cmd.c

bt_edit_LDADD = \
	$(top_builddir)/src/lib/core/libbuzztard-core.la \
	$(top_builddir)/src/lib/ic/libbuzztard-ic.la \
	$(top_builddir)/tests/ui/edit/libbt-edit-check.la \
	libbt-check.la $(BASE_DEPS_LIBS) $(GUI_DEPS_LIBS) $(BT_LIBS) -lcheck
bt_edit_LDFLAGS = \
	-Wl,--rpath -Wl,$(top_builddir)/src/lib/core \
	-Wl,--rpath -Wl,$(top_builddir)/src/lib/ic 
bt_edit_SOURCES= \
	m-bt-edit.c

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/lib/core \
	-I$(top_srcdir)/src/lib/ic \
	-DLOCALEDIR=\"$(localedir)\" \
	-DDATADIR=\"$(datadir)\" \
	-DTESTSONGDIR=\"$(top_srcdir)/tests/songs\" \
	-DG_LOG_DOMAIN=\"buzztard-check\" \
	$(BASE_DEPS_CFLAGS) \
	$(GUI_DEPS_CFLAGS) \
	$(BT_CFLAGS)\
	@VALGRIND_CFLAGS@
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
	songs/melo5.xml
noinst_songdata = \
	songs/broken1.xml \
	songs/broken1.bzt \
	songs/broken2.xml \
	songs/broken3.xml \
	songs/broken4.xml \
	songs/example.xml \
	songs/samples1.bzt \
	songs/samples2.bzt \
	songs/simple1.xml \
	songs/simple2.xml \
	songs/simple3.xml \
	songs/simple4.xml \
	songs/simple5.xml \
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
	rm -f $(top_builddir)/tests/songs/*.txt $(top_builddir)/tests/songs/*.ogg

# run any given test by running make <test>.check
%.check: %
	@$(TESTS_ENVIRONMENT)	\
	./$*

# gdb any given test by running make <test>.gdb
%.gdb: %
	CK_FORK=no				\
	CK_DEFAULT_TIMEOUT=20	\
	libtool --mode=execute	\
	gdb ./$*

## todo: check if code has been compiled using --enable-debug
##
##check-local:
##	echo "========================================"
##	echo "No check based tests will run"
##	echo "build with --enable-debug=yes"
##	echo "========================================"

.PHONY: splint valgrind todo

if USE_VALGRIND
VALPREFIX = \
	CK_FORK=no CK_DEFAULT_TIMEOUT=360 \
	G_SLICE=always-malloc GLIBCPP_FORCE_NEW=
VALDEFAULT = @VALGRIND@/bin/valgrind
VALSUPP = @VALGRIND@/lib/valgrind
VALSUPPRESSIONDEF = --suppressions=$(VALSUPP)/default.supp
VALSUPPRESSIONOWN = --suppressions=$(top_builddir)/buzztard.supp \
	--suppressions=$(top_builddir)/gst.supp
VALSUPPRESSION = $(VALSUPPRESSIONDEF) $(VALSUPPRESSIONOWN)
VALOPTIONS = -q --trace-children=yes --num-callers=20 \
  --tool=memcheck --leak-check=full --leak-resolution=high
VALCMD = $(VALPREFIX) $(VALDEFAULT) $(VALOPTIONS) $(VALSUPPRESSION)

valgrind:: $(TESTS_BIN)
	for i in $^; do \
	  rm -rf /tmp/$$i.valgrind.pid*; \
	  $(VALCMD) --log-file=/tmp/$$i.valgrind ./$$i; \
	done

# valgrind any given test by running make <test>.valgrind
%.valgrind: %
	rm -rf /tmp/$$i.valgrind.pid*; \
	$(VALCMD) --log-file=/tmp/$*.valgrind ./$*;
endif

splint::
	splint +posixlib -weak -I$(top_builddir) $(AM_CPPFLAGS) $(buzztard_SOURCES) $(bt_cmd_SOURCES) $(bt_edit_SOURCES)
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir splint ; done

##find . -name "*.c" -exec grep -A7 -Hn "#ifdef __CHECK_DISABLED__" {}; echo "--" \;
##grep -r -A7 -Hn --include="*.c" --color=auto "#ifdef __CHECK_DISABLED__" .
todo::
	grep -r -A1 -B6 -Hn --include="*.c" --color=auto "#ifdef __CHECK_DISABLED__" .

help::
	@echo "make check                   -- run all checks"
	@echo "make (test).check            -- run the given check once"
	@echo
	@echo "make (test).gdb              -- start up gdb for the given test"
	@echo
	@echo "make valgrind                -- valgrind all tests"
	@echo "make (test).valgrind         -- valgrind the given test"
	@echo
	@echo "make todo                    -- check for todos in tests"
	@echo
	@echo "BT_CHECKS=\"(testname)\" make (test).check -- run the given testcase only"
