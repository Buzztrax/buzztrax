## Makefile.am for tests

if BUILD_CHECK_TESTS
SUBDIRS = lib ui
TESTS = buzztard bt_cmd
endif

if BUILD_CHECK_TESTS
noinst_PROGRAMS=buzztard bt_cmd

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

INCLUDES = \
	-I@srcdir@ -I$(top_builddir)/src/lib/core \
	-DLOCALEDIR=\"$(localedir)\" \
	-DDATADIR=\"$(datadir)/buzztard\" \
	-DG_LOG_DOMAIN=\"buzztard-check\" \
	$(BASE_DEPS_CFLAGS) \
	$(BT_CFLAGS)
endif

.PHONY: splint 

splint::
	splint +posixlib -weak -I$(top_builddir) $(INCLUDES) $(buzztard_SOURCES)
	splint +posixlib -weak -I$(top_builddir) $(INCLUDES) $(bt_cmd_SOURCES)
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir splint ; done
