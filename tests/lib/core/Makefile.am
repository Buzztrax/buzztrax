noinst_LTLIBRARIES = libbtcore-check.la
libbtcore_check_la_LIBADD = $(BASE_DEPS_LIBS)
libbtcore_check_la_SOURCES = m-bt-core.h \
	s-application.c e-application.c \
	s-core.c e-core.c t-core.c \
	s-machine.c e-machine.c t-machine.c \
	s-param-group.c t-param-group.c \
	s-pattern.c e-pattern.c t-pattern.c \
	s-processor-machine.c e-processor-machine.c t-processor-machine.c \
	s-sequence.c e-sequence.c t-sequence.c \
	s-setup.c e-setup.c t-setup.c \
	s-sink-machine.c e-sink-machine.c t-sink-machine.c \
	s-song.c e-song.c t-song.c \
	s-song-io.c e-song-io.c t-song-io.c \
	s-song-io-native.c e-song-io-native.c t-song-io-native.c \
	s-song-info.c e-song-info.c t-song-info.c \
	s-source-machine.c e-source-machine.c t-source-machine.c \
	s-tools.c e-tools.c t-tools.c \
	s-wire.c e-wire.c t-wire.c \
	s-settings.c t-settings.c

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/lib/ic \
	-I$(top_srcdir)/src/lib/core \
	-DLOCALEDIR=\"$(localedir)\" \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(datadir)\" \
	-DG_LOG_DOMAIN=\"buzztard\" \
	$(BASE_DEPS_CFLAGS)  \
	$(GUI_DEPS_CFLAGS) \
	$(BT_CFLAGS)

include $(top_srcdir)/common.mak
