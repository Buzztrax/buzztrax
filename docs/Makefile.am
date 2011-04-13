## $Id$

SUBDIRS = help reference

BUILT_SOURCES = version.entities

EXTRA_DIST = version.entities.in \
	buzztard.xsd

# this is a hack to make sure gtkdoc find the file during "out of source build"
all-local:
	@if test "$(top_srcdir)" != "$(top_builddir)"; then \
	  chmod u+w $(srcdir); \
	  cp version.entities $(srcdir)/; \
	fi;

