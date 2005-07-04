## $Id: Makefile.am,v 1.27 2005-07-04 14:05:11 ensonic Exp $

noinst_LTLIBRARIES = libbtcore-check.la
libbtcore_check_la_LIBADD = -lm $(BASE_DEPS_LIBS)
libbtcore_check_la_SOURCES = m-bt-core.h \
	s-core.c t-core.c \
	s-machine.c e-machine.c t-machine.c \
	s-network.c e-network.c t-network.c \
	s-pattern.c e-pattern.c t-pattern.c \
	s-processor-machine.c e-processor-machine.c \
	s-sequence.c e-sequence.c t-sequence.c \
	s-setup.c e-setup.c t-setup.c \
	s-sink-machine.c t-sink-machine.c \
	s-song.c t-song.c e-song.c \
	s-song-io.c t-song-io.c \
	s-song-info.c e-song-info.c \
	s-source-machine.c e-source-machine.c t-source-machine.c \
	s-timeline.c e-timeline.c \
	s-wire.c t-wire.c \
	s-settings.c t-settings.c

INCLUDES = \
	-I@srcdir@ -I$(top_srcdir)/src/lib/core \
	-DLOCALEDIR=\"$(localedir)\" \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(datadir)/buzztard\" \
	-DG_LOG_DOMAIN=\"buzztard\" \
	$(BASE_DEPS_CFLAGS) \
	$(BT_CFLAGS)

.PHONY: splint 

splint::
	splint +posixlib -weak -I$(top_builddir) $(INCLUDES) $(libbtcore_check_la_SOURCES)
