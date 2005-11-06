SUBDIRS = core

.PHONY: splint 

# install .applications files
dotapplicationsdir = $(datadir)/application-registry
dotapplications_DATA = buzztard.applications
EXTRA_DIST = $(dotapplications_DATA)

# generate and install .xml mime info file
sharedmimedir = $(datadir)/mime
sharedmimepackagedir = $(sharedmimedir)/packages
sharedmimepackage_in_files = buzztard.xml.in
sharedmimepackage_DATA = $(sharedmimepackage_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@
CLEANFILES = buzztard.xml
buzztard.xml: buzztard.xml.in
EXTRA_DIST += $(sharedmimepackage_in_files)

# generate and install .mime and .keys files
mimedir = $(datadir)/mime-info
mime_in_file = buzztard.mime buzztard.keys.in
mime_DATA = $(mime_in_file:.keys.in=.keys)
@INTLTOOL_KEYS_RULE@
CLEANFILES += buzztard.keys
buzztard.keys: buzztard.keys.in
EXTRA_DIST += $(mime_in_file)

# update mime database
install-data-hook:
	test -z "$(UPDATE_MIME_DATABASE)" || $(UPDATE_MIME_DATABASE) "$(DESTDIR)$(sharedmimedir)";
uninstal-local: uninstall-sharedmimepackageDATA
	test -z "$(UPDATE_MIME_DATABASE)" || $(UPDATE_MIME_DATABASE) "$(DESTDIR)$(sharedmimedir)";

splint::
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir splint ; done
