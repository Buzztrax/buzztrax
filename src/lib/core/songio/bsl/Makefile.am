## Makefile.am for src

libdir = ${exec_prefix}/lib/buzztard-songio

lib_LTLIBRARIES = libbtbsl.la
libbtbsl_la_LIBADD = \
	$(top_builddir)/src/lib/core/libbuzztard-core.la \
  $(BASE_DEPS_LIBS) $(LIBINTL)
libbtbsl_la_LDFLAGS = -module -avoid-version
libbtbsl_la_SOURCES = \
	bsl.c bsl.h \
	song-io-buzz.c song-io-buzz.h song-io-buzz-private.h \
	version.h
libbtbsl_la_LIBTOOLFLAGS = --tag=disable-static
libbtbsl_la_CFLAGS = \
	$(BASE_DEPS_CFLAGS) \
	$(BT_CFLAGS)

AM_CPPFLAGS = \
	-I$(srcdir) \
	-I$(top_srcdir)/src/lib/core \
	-I$(top_srcdir)/src/lib/ic \
	-DLOCALEDIR=\"$(localedir)\" \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(datadir)/buzztard\" \
	-DG_LOG_DOMAIN=\"buzztard\"

desktopdir = ${with_desktop_dir}/applications
desktop_in_files = buzztard-songio-buzz.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@
buzztard-songio-buzz.desktop: buzztard-songio-buzz.desktop.in

# install .xml mime info file
sharedmimedir = $(datadir)/mime
sharedmimepackagedir = $(sharedmimedir)/packages
sharedmimepackage_in_files = buzztard-songio-buzz.xml.in
sharedmimepackage_DATA = $(sharedmimepackage_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@
buzztard-songio-buzz.xml: buzztard-songio-buzz.xml.in

CLEANFILES = $(desktop_DATA) $(sharedmimepackage_DATA)
EXTRA_DIST = $(desktop_in_files) $(sharedmimepackage_in_files)

# update mime database
install-data-hook:
	test -z "$(UPDATE_DESKTOP_DATABASE)" || $(UPDATE_DESKTOP_DATABASE) "$(DESTDIR)$(desktopdir)";
	test -z "$(UPDATE_MIME_DATABASE)" || $(UPDATE_MIME_DATABASE) "$(DESTDIR)$(sharedmimedir)";
uninstall-local:
	test -z "$(UPDATE_DESKTOP_DATABASE)" || $(UPDATE_DESKTOP_DATABASE) "$(DESTDIR)$(desktopdir)";
	test -z "$(UPDATE_MIME_DATABASE)" || $(UPDATE_MIME_DATABASE) "$(DESTDIR)$(sharedmimedir)";


include $(top_srcdir)/common.mak
