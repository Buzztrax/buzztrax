## $Id: Makefile.am,v 1.12 2007-08-03 21:08:16 ensonic Exp $

noinst_LTLIBRARIES = libbt-cmd-check.la
libbt_cmd_check_la_LIBADD = $(BASE_DEPS_LIBS) \
	$(top_builddir)/src/ui/cmd/libbtcmd.la
libbt_cmd_check_la_SOURCES = m-bt-cmd.h \
	s-bt-cmd-application.c e-bt-cmd-application.c t-bt-cmd-application.c

INCLUDES = \
	-I$(top_srcdir)/src/lib/core \
	-I$(top_srcdir)/src/lib/ic \
	-DLOCALEDIR=\"$(localedir)\" \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(datadir)\" \
	-DG_LOG_DOMAIN=\"buzztard\" \
	$(BASE_DEPS_CFLAGS)  \
	$(GUI_DEPS_CFLAGS) \
	$(BT_CFLAGS)

.PHONY: splint

splint::
	splint +posixlib -weak -I$(top_builddir) $(INCLUDES) $(libbt_cmd_check_la_SOURCES)
