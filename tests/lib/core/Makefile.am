## Makefile.am for tests/lib/core

noinst_LTLIBRARIES = libbtcore-check.la
libbtcore_check_la_LIBADD = -lm $(BASE_DEPS_LIBS)
libbtcore_check_la_SOURCES = \
	t-core.c t-core.h t-song.c e-song.c t-song-io.g t-song-io.c t-network.c \
	e-network.c t-setup.c e-setup.c t-wire.c t-sourcemachine.c \
	e-sourcemachine.c t-sinkmachine.c t-sequence.c


INCLUDES = \
	-I@srcdir@ -I$(top_builddir)/src/lib/core \
	-DLOCALEDIR=\"$(localedir)\" \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(datadir)/buzztard\" \
	-DG_LOG_DOMAIN=\"buzztard\" \
	$(BASE_DEPS_CFLAGS) \
	$(BT_CFLAGS)
