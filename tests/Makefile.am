## $Id: Makefile.am,v 1.22 2005-06-24 16:54:42 ensonic Exp $

if BUILD_CHECK_TESTS
  SUBDIRS = lib ui
  TESTS_BIN = bt_core bt_cmd bt_edit
else
  TESTS_BIN=
endif

TESTS = \
  $(TESTS_BIN) \
  gtkdoccomplete.sh \
  povalid.sh \
  xmlvalid.sh

# prevent endless loops
TESTS_ENVIRONMENT = ulimit -St20;

if BUILD_CHECK_TESTS
noinst_PROGRAMS=$(TESTS_BIN)

COMMON_SOURCES = bt-check.c bt-check.h

bt_core_LDADD = $(BASE_DEPS_LIBS) -lcheck \
	$(top_builddir)/src/lib/core/libbtcore.la \
	$(top_builddir)/tests/lib/core/libbtcore-check.la
bt_core_SOURCES= \
	m-bt-core.c $(COMMON_SOURCES)
	
bt_cmd_LDADD = $(BASE_DEPS_LIBS) -lcheck \
	$(top_builddir)/src/lib/core/libbtcore.la \
	$(top_builddir)/tests/ui/cmd/libbt-cmd-check.la
bt_cmd_SOURCES= \
	m-bt-cmd.c $(COMMON_SOURCES)

bt_edit_LDADD = $(BASE_DEPS_LIBS) -lcheck \
	$(top_builddir)/src/lib/core/libbtcore.la \
	$(top_builddir)/tests/ui/edit/libbt-edit-check.la
bt_edit_SOURCES= \
	m-bt-edit.c $(COMMON_SOURCES)

INCLUDES = \
	-I@srcdir@ -I$(top_srcdir)/src/lib/core \
	-DLOCALEDIR=\"$(localedir)\" \
	-DDATADIR=\"$(datadir)/buzztard\" \
	-DG_LOG_DOMAIN=\"buzztard-check\" \
	$(BASE_DEPS_CFLAGS) \
	$(BT_CFLAGS)\
 @VALGRIND_CFLAGS@
endif

.PHONY: splint valgrind

if USE_VALGRIND
VALPREFIX = $(TESTS_ENVIRONMENT) GLIBCPP_FORCE_NEW=
VALDEFAULT = @VALGRIND@/bin/valgrind
VALSUPP = @VALGRIND@/lib/valgrind
VALSUPPRESSIONDEF = --suppressions=$(VALSUPP)/default.supp
VALSUPPRESSIONOWN = --suppressions=$(top_builddir)/buzztard.supp
VALSUPPRESSION = $(VALSUPPRESSIONDEF) $(VALSUPPRESSIONOWN)
VALOPTIONS = --tool=memcheck -q --trace-children=yes --show-reachable=yes --leak-check=yes --num-callers=20
VALCMD = $(VALPREFIX) $(VALDEFAULT) $(VALOPTIONS) $(VALSUPPRESSION)

valgrind:: $(TESTS_BIN)
	for i in $^; do \
	  rm -rf /tmp/$$i.valgrind.pid*; \
		$(VALCMD) --log-file=/tmp/$$i.valgrind ./$$i; \
	done
endif

splint::
	splint +posixlib -weak -I$(top_builddir) $(INCLUDES) $(buzztard_SOURCES) $(bt_cmd_SOURCES) $(bt_edit_SOURCES)
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir splint ; done
