## Makefile.am for tests

if BUILD_CHECK_TESTS
SUBDIRS = lib
TESTS = buzztard
endif

if BUILD_CHECK_TESTS
noinst_PROGRAMS=buzztard
buzztard_LDADD = $(BASE_DEPS_LIBS) -lcheck \
	$(top_builddir)/tests/lib/core/libbtcore.la

check_libgitk_SOURCES= \
	buzztard.c

INCLUDES = \
	-I@srcdir@ -I$(top_builddir)/src/lib/core \
	-DLOCALEDIR=\"$(localedir)\" \
	-DDATADIR=\"$(datadir)/buzztard\" \
	-DG_LOG_DOMAIN=\"buzztard-check\" \
	$(BASE_DEPS_CFLAGS) \
	$(BT_CFLAGS)
endif

