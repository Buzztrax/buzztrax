SUBDIRS = help reference

BUILT_SOURCES = version.entities

EXTRA_DIST = version.entities.in \
	buzztard.xsd

# this is a hack to make sure gtkdoc find the file during "out of source build"
all-local:
	if [ "$(srcdir)" -ne "$(builddir)" ]; then \
	  @-chmod -R u+w $(srcdir); \
	  cp $(builddir)/version.entities $(srcdir)/version.entities; \
	fi;

