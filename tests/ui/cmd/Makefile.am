## $Id: Makefile.am,v 1.5 2005-04-15 17:08:29 ensonic Exp $

noinst_LTLIBRARIES = libbt-cmd-check.la
libbt_cmd_check_la_LIBADD = -lm $(BASE_DEPS_LIBS) \
	$(top_builddir)/src/ui/cmd/libbtcmd.la
libbt_cmd_check_la_SOURCES = \
	t-bt-cmd.c t-bt-cmd.h


INCLUDES = \
	-I@srcdir@ -I$(top_builddir)/src/lib/core \
	-DLOCALEDIR=\"$(localedir)\" \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(datadir)/buzztard\" \
	-DG_LOG_DOMAIN=\"buzztard\" \
	$(BASE_DEPS_CFLAGS) \
	$(BT_CFLAGS)

.PHONY: splint 

splint::
	splint +posixlib -weak -I$(top_builddir) $(INCLUDES) $(libbt_cmd_check_la_SOURCES)
