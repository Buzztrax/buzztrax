## Makefile.am for src/lib/core

#SUBDIRS = . libbtcore

lib_LTLIBRARIES = libbtcore.la
libbtcore_la_LIBADD = -lm $(BASE_DEPS_LIBS)
libbtcore_la_LDFLAGS = -version-info @BT_VERSION_INFO@
libbtcore_la_SOURCES = \
	libbtcore/version.h \
	core.c libbtcore/core.h \
	network.c libbtcore/network.h \
	libbtcore/pattern.h \
	song.c libbtcore/song.h

INCLUDES = \
	-I@srcdir@ -I$(top_builddir)/src/lib/core \
	-DLOCALEDIR=\"$(localedir)\" \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(datadir)/buzztard\" \
	-DG_LOG_DOMAIN=\"buzztard\" \
	$(BASE_DEPS_CFLAGS) \
	$(BT_CFLAGS)

