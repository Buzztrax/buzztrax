## Makefile.am for tests/lib/core

lib_LTLIBRARIES = libbtcore-check.la
libbtcore_check_la_LIBADD = -lm $(BASE_DEPS_LIBS)
libbtcore_check_la_SOURCES = \
	core.c core.h \
	network.c

INCLUDES = \
	-I@srcdir@ -I$(top_builddir)/src/lib/core \
	-DLOCALEDIR=\"$(localedir)\" \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(datadir)/buzztard\" \
	-DG_LOG_DOMAIN=\"buzztard\" \
	$(BASE_DEPS_CFLAGS) \
	$(BT_CFLAGS)

