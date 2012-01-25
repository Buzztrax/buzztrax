SUBDIRS = ic core

# generate and install .xml mime info file
sharedmimedir = $(datadir)/mime
sharedmimepackagedir = $(sharedmimedir)/packages
sharedmimepackage_in_files = buzztard.xml.in
sharedmimepackage_DATA = $(sharedmimepackage_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@
buzztard.xml: buzztard.xml.in

CLEANFILES = $(sharedmimepackage_DATA)
EXTRA_DIST = $(sharedmimepackage_in_files)


# update mime database
install-data-hook:
	test -z "$(UPDATE_MIME_DATABASE)" || $(UPDATE_MIME_DATABASE) "$(DESTDIR)$(sharedmimedir)";
uninstall-local: uninstall-sharedmimepackageDATA
	test -z "$(UPDATE_MIME_DATABASE)" || $(UPDATE_MIME_DATABASE) "$(DESTDIR)$(sharedmimedir)";


include $(top_srcdir)/common.mak
