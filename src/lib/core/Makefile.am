## Makefile.am for src/lib/core

SUBDIRS = . libbtcore

if USE_GCONF
GCONF_SETTINGS_SRC = gconf-settings.c libbtcore/gconf-settings.h libbtcore/gconf-settings-methods.h
else
GCONF_SETTINGS_SRC = 
endif


lib_LTLIBRARIES = libbtcore.la
libbtcore_la_LIBADD = -lm $(BASE_DEPS_LIBS)
libbtcore_la_LDFLAGS = -version-info @BT_VERSION_INFO@
libbtcore_la_SOURCES = \
	core.c libbtcore/core.h \
	tools.c libbtcore/tools.h \
	libbtcore/version.h \
	\
	application.c libbtcore/application.h libbtcore/application-methods.h libbtcore/application-private.h \
	$(GCONF_SETTINGS_SRC) \
	machine.c libbtcore/machine.h libbtcore/machine-methods.h libbtcore/machine-private.h \
	pattern.c libbtcore/pattern.h libbtcore/pattern-methods.h \
	plainfile-settings.c libbtcore/plainfile-settings.h libbtcore/plainfile-settings-methods.h \
	playline.c libbtcore/playline.h libbtcore/playline-methods.h \
	processor-machine.c libbtcore/processor-machine.h libbtcore/processor-machine-methods.h \
	sequence.c libbtcore/sequence.h libbtcore/sequence-methods.h \
	settings.c libbtcore/settings.h libbtcore/settings-methods.h libbtcore/settings-private.h \
	setup.c libbtcore/setup.h libbtcore/setup-methods.h \
	sink-machine.c libbtcore/sink-machine.h libbtcore/sink-machine-methods.h \
	song.c libbtcore/song.h libbtcore/song-methods.h \
	song-info.c libbtcore/song-info.h libbtcore/song-info-methods.h \
	song-io.c libbtcore/song-io.h libbtcore/song-io-methods.h \
	song-io-native.c libbtcore/song-io-native.h libbtcore/song-io-native-methods.h \
	source-machine.c libbtcore/source-machine.h libbtcore/source-machine-methods.h \
	timeline.c libbtcore/timeline.h libbtcore/timeline-methods.h \
	timelinetrack.c libbtcore/timelinetrack.h libbtcore/timelinetrack-methods.h \
	wire.c libbtcore/wire.h libbtcore/wire-methods.h

INCLUDES = \
	-I@srcdir@ -I$(top_builddir)/src/lib/core \
	-DLOCALEDIR=\"$(localedir)\" \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(datadir)/buzztard\" \
	-DG_LOG_DOMAIN=\"buzztard\" \
	$(BASE_DEPS_CFLAGS) \
	$(BT_CFLAGS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libbtcore.pc

EXTRA_DIST = \
	libbtcore.pc.in

.PHONY: splint

splint::
	splint +posixlib -weak -I$(top_builddir) $(INCLUDES) $(libbtcore_la_SOURCES)
