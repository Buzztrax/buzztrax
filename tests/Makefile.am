## Makefile.am for tests

if BUILD_CHECK_TESTS
  SUBDIRS = lib ui
  TESTS_BIN = buzztard bt_cmd bt_edit
else
  TESTS_BIN=
endif

TESTS = \
  $(TESTS_BIN) \
  xmlvalid.sh

# prevent endless loops
TESTS_ENVIRONMENT = ulimit -St20;

if BUILD_CHECK_TESTS
noinst_PROGRAMS=buzztard bt_cmd bt_edit

buzztard_LDADD = $(BASE_DEPS_LIBS) -lcheck \
	$(top_builddir)/src/lib/core/libbtcore.la \
	$(top_builddir)/tests/lib/core/libbtcore-check.la
buzztard_SOURCES= \
	t-buzztard.c t-buzztard.h t-tools.h t-tools.c
	
bt_cmd_LDADD = $(BASE_DEPS_LIBS) -lcheck \
	$(top_builddir)/src/lib/core/libbtcore.la \
	$(top_builddir)/tests/ui/cmd/libbt-cmd-check.la
bt_cmd_SOURCES= \
	t-bt-cmd.c t-tools.h t-tools.c

bt_edit_LDADD = $(BASE_DEPS_LIBS) -lcheck \
	$(top_builddir)/src/lib/core/libbtcore.la \
	$(top_builddir)/tests/ui/edit/libbt-edit-check.la
bt_edit_SOURCES= \
	t-bt-edit.c t-tools.h t-tools.c

INCLUDES = \
	-I@srcdir@ -I$(top_builddir)/src/lib/core \
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
VALOPTIONS = --tool=memcheck -q --show-reachable=yes --leak-check=yes --num-callers=20
VALCMD = $(VALPREFIX) $(VALDEFAULT) $(VALOPTIONS)

valgrind:: $(TESTS_BIN)
	for i in $^; do $(VALCMD) --log-file=/tmp/$$i.valgrind ./$$i; done
endif

splint::
	splint +posixlib -weak -I$(top_builddir) $(INCLUDES) $(buzztard_SOURCES) $(bt_cmd_SOURCES) $(bt_edit_SOURCES)
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir splint ; done
