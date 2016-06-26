## Process this file with automake to produce Makefile.in

# We require automake 1.6 at least.
AUTOMAKE_OPTIONS = 1.6

# The name of the module, e.g. 'glib'.
DOC_MODULE=buzztrax-ic

# The top-level SGML file. You can change this if you want to.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.xml

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting the functions and macros.
DOC_SOURCE_DIR=$(top_srcdir)/src/lib/ic

# Extra options to pass to gtkdoc-scangobj. Not normally needed.
#SCANGOBJ_OPTIONS=""

# Extra options to supply to gtkdoc-scan.
SCAN_OPTIONS=--deprecated-guards="BT_DISABLE_DEPRECATED"

# Extra options to supply to gtkdoc-mkdb.
MKDB_OPTIONS=--sgml-mode

# Extra options to supply to gtkdoc-fixref. Not normally needed.
FIXXREF_OPTIONS=--extra-dir=$(GLIB_PREFIX)/share/gtk-doc/html/ \
	--extra-dir=$(GST_PREFIX)/share/gtk-doc/html/ \
	--extra-dir=..

# Used for dependencies. The docs will be rebuilt if any of these change.
HFILE_GLOB=$(top_srcdir)/src/lib/ic/*.h
CFILE_GLOB=$(top_srcdir)/src/lib/ic/*.c

if USE_LINUX_INPUT
LINUX_INPUT_FILES=
else
LINUX_INPUT_FILES=input-device.h
endif

if USE_ALSA
ALSA_FILES=
else
ALSA_FILES=aseq-discoverer.h aseq-device.h
endif

if USE_GUDEV
GUDEV_FILES=
else
GUDEV_FILES=gudev-discoverer.h
endif

# Header files to ignore when scanning.
IGNORE_HFILES=ic_private.h \
  $(ALSA_FILES) \
  $(GUDEV_FILES) \
  $(LINUX_INPUT_FILES)

# Images to copy into HTML directory.
HTML_IMAGES =

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE).
# e.g. content_files=running.sgml building.sgml changes-2.0.sgml
content_files =

# SGML files where gtk-doc abbrevations (#GtkWidget) are expanded
# These files must be listed here *and* in content_files
# e.g. expand_content_files=running.sgml
expand_content_files=

# CFLAGS and LDFLAGS for compiling gtkdoc-scangobj with your library.
# Only needed if you are using gtkdoc-scangobj to dynamically query widget
# signals and properties.
GTKDOC_CFLAGS = \
	-I$(top_srcdir)/src/lib \
	-I$(top_builddir) \
	$(BASE_DEPS_CFLAGS) \
	$(BT_CFLAGS)
GTKDOC_LIBS = \
	$(top_builddir)/libbuzztrax-ic.la \
	$(BASE_DEPS_LIBS) \
	$(BT_LIBS)

# include generic part
include $(top_srcdir)/gtk-doc.make

# Other files to distribute
# e.g. EXTRA_DIST += version.xml.in
EXTRA_DIST += $(DOC_MODULE).types

# Comment this out if you want your docs-status tested during 'make check'
if ENABLE_GTK_DOC
AM_TESTS_ENVIRONMENT = \
	DOC_MODULE=$(DOC_MODULE) DOC_MAIN_SGML_FILE=$(DOC_MAIN_SGML_FILE) \
	SRCDIR=$(abs_srcdir) BUILDDIR=$(abs_builddir)
TESTS = $(GTKDOC_CHECK)
endif

