## Makefile.am for src/ui/bin

plugin_LTLIBRARIES = libbuzztarddec.la

# sources used to compile this plug-in
libbuzztarddec_la_SOURCES = bt-dec.c

# flags used to compile this plugin
libbuzztarddec_la_CFLAGS = \
	-I$(top_srcdir)/src/lib/core \
	-I$(top_srcdir)/src/lib/ic \
	-DLOCALEDIR=\"$(localedir)\" \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(datadir)\" \
	-DG_LOG_DOMAIN=\"buzztard\" \
	$(GST_PLUGIN_CFLAGS) \
	$(BASE_DEPS_CFLAGS)
libbuzztarddec_la_LIBADD = \
	$(top_builddir)/src/lib/core/libbuzztard-core.la \
    $(top_builddir)/src/lib/ic/libbuzztard-ic.la \
    $(BASE_DEPS_LIBS) $(LIBM)
libbuzztarddec_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libbuzztarddec_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = bt-dec.h

include $(top_srcdir)/common.mak
