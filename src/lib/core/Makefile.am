## Makefile.am for src/lib/core

#SUBDIRS = . libbtcore

lib_LTLIBRARIES = libbtcore.la
libbtcore_la_LIBADD = -lm $(BASE_DEPS_LIBS)
libbtcore_la_LDFLAGS = -version-info @BT_VERSION_INFO@
libbtcore_la_SOURCES = \
	core.c libbtcore/core.h \
	machine.c libbtcore/machine.h libbtcore/machine-methods.h \
	sequence.c libbtcore/sequence.h libbtcore/sequence-methods.h \
	setup.c libbtcore/setup.h libbtcore/setup-methods.h \
	sink-machine.c libbtcore/sink-machine.h \
	song.c libbtcore/song.h libbtcore/song-methods.h \
	song-info.c libbtcore/song-info.h libbtcore/song-info-methods.h \
	song-io.c libbtcore/song-io.h libbtcore/song-io-methods.h \
	song-io-native.c libbtcore/song-io-native.h \
	wire.c libbtcore/wire.h libbtcore/wire-methods.h \
	libbtcore/version.h

INCLUDES = \
	-I@srcdir@ -I$(top_builddir)/src/lib/core \
	-DLOCALEDIR=\"$(localedir)\" \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(datadir)/buzztard\" \
	-DG_LOG_DOMAIN=\"buzztard\" \
	$(BASE_DEPS_CFLAGS) \
	$(BT_CFLAGS)

