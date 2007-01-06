## $Id: Makefile.am,v 1.41 2007-01-06 16:01:33 ensonic Exp $

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
  gtkdoccomplete.sh \
  povalid.sh \
  xmlvalid.sh

# prevent endless loops
TESTS_ENVIRONMENT = \
	ulimit -St20;

if BUILD_CHECK_TESTS
noinst_PROGRAMS=$(TESTS_BIN)

noinst_LTLIBRARIES = libbt-check.la
libbt_check_la_LIBADD = -lm -lcheck $(BASE_DEPS_LIBS) $(GUI_DEPS_LIBS)
libbt_check_la_SOURCES = \
	bt-check.c bt-check.h \
	bt-test-plugin.c bt-test-plugin.h \
  bt-test-settings.c bt-test-settings.h

bt_core_LDADD = \
	$(top_builddir)/src/lib/core/libbtcore.la \
	$(top_builddir)/tests/lib/core/libbtcore-check.la \
  libbt-check.la $(BASE_DEPS_LIBS) -lcheck
bt_core_SOURCES= \
	m-bt-core.c

bt_cmd_LDADD = \
	$(top_builddir)/src/lib/core/libbtcore.la \
	$(top_builddir)/tests/ui/cmd/libbt-cmd-check.la \
  libbt-check.la $(BASE_DEPS_LIBS) -lcheck
bt_cmd_SOURCES= \
	m-bt-cmd.c

bt_edit_LDADD = \
	$(top_builddir)/src/lib/core/libbtcore.la \
	$(top_builddir)/tests/ui/edit/libbt-edit-check.la \
  libbt-check.la $(BASE_DEPS_LIBS) -lcheck
bt_edit_SOURCES= \
	m-bt-edit.c

INCLUDES = \
	-I@srcdir@ -I$(top_srcdir)/src/lib/core \
	-DLOCALEDIR=\"$(localedir)\" \
	-DDATADIR=\"$(datadir)\" \
	-DTESTSONGDIR=\"$(top_srcdir)/tests/songs\" \
	-DG_LOG_DOMAIN=\"buzztard-check\" \
	$(BASE_DEPS_CFLAGS) \
	$(GUI_DEPS_CFLAGS) \
	$(BT_CFLAGS)\
	@VALGRIND_CFLAGS@
endif

EXTRA_DIST = gtkdoccomplete.sh povalid.sh xmlvalid.sh \
  bt-cfg.sh.in bt-cmd-info.sh bt-cmd-encode.sh

clean-local:
	rm -f $(top_builddir)/tests/songs/*.txt $(top_builddir)/tests/songs/*.ogg

# run any given test by running make <test>.check
%.check: %
	@$(TESTS_ENVIRONMENT) ./$*

# gdb any given test by running make <test>.gdb
%.gdb: %
	$(TESTS_ENVIRONMENT)		\
	CK_FORK=no						  \
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
VALPREFIX = $(TESTS_ENVIRONMENT) G_SLICE=always-malloc GLIBCPP_FORCE_NEW=
VALDEFAULT = @VALGRIND@/bin/valgrind
VALSUPP = @VALGRIND@/lib/valgrind
VALSUPPRESSIONDEF = --suppressions=$(VALSUPP)/default.supp
VALSUPPRESSIONOWN = --suppressions=$(top_builddir)/buzztard.supp
VALSUPPRESSION = $(VALSUPPRESSIONDEF) $(VALSUPPRESSIONOWN)
VALOPTIONS = -q --trace-children=yes --num-callers=20 \
  --tool=memcheck  --show-reachable=yes --leak-check=full --leak-resolution=high
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
	splint +posixlib -weak -I$(top_builddir) $(INCLUDES) $(buzztard_SOURCES) $(bt_cmd_SOURCES) $(bt_edit_SOURCES)
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
