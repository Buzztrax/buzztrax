## Makefile.am for src/ui/cmd

#bin_PROGRAMS=bt-cmd network1 network2 
bin_PROGRAMS=bt-cmd

bt_cmd_LDADD = $(BASE_DEPS_LIBS) $(top_builddir)/src/lib/core/libbtcore.la
bt_cmd_SOURCES= bt-cmd.c

network1_LDADD = $(BASE_DEPS_LIBS) $(top_builddir)/src/lib/core/libbtcore.la
network1_SOURCES= network1.c

network2_LDADD = $(BASE_DEPS_LIBS) $(top_builddir)/src/lib/core/libbtcore.la
network2_SOURCES= network2.c

INCLUDES = \
	-I@srcdir@ -I$(top_builddir)/src/lib/core \
	-DLOCALEDIR=\"$(localedir)\" \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(datadir)/buzztard\" \
	-DG_LOG_DOMAIN=\"buzztard\" \
	$(BASE_DEPS_CFLAGS) \
	$(BT_CFLAGS)

