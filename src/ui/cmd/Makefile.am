## Makefile.am for src/ui/cmd

bin_PROGRAMS=bt-cmd

bt_cmd_LDADD = $(BASE_DEPS_LIBS) \
	$(top_builddir)/src/lib/core/libbtcore.la \
	libbtcmd.la
bt_cmd_SOURCES= \
	bt-cmd.c bt-cmd.h

noinst_LTLIBRARIES      = libbtcmd.la
libbtcmd_la_SOURCES   =	\
	cmd-application.c cmd-application.h cmd-application-methods.h

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
	splint +posixlib -weak -I$(top_builddir) $(INCLUDES) $(libbtcmd_la_SOURCES)
