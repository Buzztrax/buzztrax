## Makefile.am for src/ui/cmd

bin_PROGRAMS=buzztard-cmd

buzztard_cmd_LDADD = \
	$(top_builddir)/src/lib/core/libbuzztard-core.la \
    $(top_builddir)/src/lib/ic/libbuzztard-ic.la \
	libbtcmd.la \
	$(BASE_DEPS_LIBS) $(BT_LIBS)
buzztard_cmd_SOURCES= \
	bt-cmd.c bt-cmd.h

noinst_LTLIBRARIES      = libbtcmd.la
libbtcmd_la_LIBDADD = \
	$(top_builddir)/src/lib/core/libbuzztard-core.la \
    $(top_builddir)/src/lib/ic/libbuzztard-ic.la \
	$(BASE_DEPS_LIBS) $(BT_LIBS) $(LIBM)
libbtcmd_la_SOURCES   =	\
	cmd-application.c cmd-application.h

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/lib/core \
	-I$(top_srcdir)/src/lib/ic \
	-DLOCALEDIR=\"$(localedir)\" \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(datadir)\" \
	-DG_LOG_DOMAIN=\"buzztard\" \
	$(BASE_DEPS_CFLAGS) \
	$(BT_CFLAGS)

AM_LDFLAGS = $(BT_LDFLAGS)

include $(top_srcdir)/common.mak
