## Makefile.am for tests

if BUILD_CHECK_TESTS
SUBDIRS = lib ui
TESTS = buzztard
endif

if BUILD_CHECK_TESTS
noinst_PROGRAMS=buzztard
buzztard_LDADD = $(BASE_DEPS_LIBS) -lcheck \
	$(top_builddir)/src/lib/core/libbtcore.la \
	$(top_builddir)/src/ui/cmd/cmd-application.o \
	$(top_builddir)/tests/lib/core/libbtcore-check.la \
	$(top_builddir)/tests/ui/cmd/libbt-cmd-check.la

buzztard_SOURCES= \
	buzztard.c buzztard.h

INCLUDES = \
	-I@srcdir@ -I$(top_builddir)/src/lib/core \
	-DLOCALEDIR=\"$(localedir)\" \
	-DDATADIR=\"$(datadir)/buzztard\" \
	-DG_LOG_DOMAIN=\"buzztard-check\" \
	$(BASE_DEPS_CFLAGS) \
	$(BT_CFLAGS)
endif

