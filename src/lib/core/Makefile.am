## Makefile.am for libbtcore

#SUBDIRS = . libbtcore

lib_LTLIBRARIES = libbtcore.la
libbtcore_la_LIBADD = -lm $(BASE_DEPS_LIBS)
libbtcore_la_LDFLAGS = -version-info @BT_VERSION_INFO@
libbtcore_la_SOURCES = \
	core.c \
	network.c

INCLUDES = \
	-I@srcdir@ -I$(top_builddir)/src/lib/core \
	-DLOCALEDIR=\"$(localedir)\" \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(datadir)/buzztard\" \
	-DG_LOG_DOMAIN=\"buzztard\" \
	$(BASE_DEPS_CFLAGS)

