## Process this file with automake to produce Makefile.in

# We require automake 1.6 at least.
AUTOMAKE_OPTIONS = 1.6

# The name of the module, e.g. 'glib'.
DOC_MODULE=buzztrax-gst

# The top-level SGML file. You can change this if you want to.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.xml

# Some parts are dynamic
if FLUIDSYNTH_SUPPORT
FLUIDSYNTH_LA = $(top_builddir)/libgstfluidsynth.la
FLUIDSYNTH_IGNORE_H =
else
FLUIDSYNTH_LA =
FLUIDSYNTH_IGNORE_H = fluidsynth.h
endif

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting the functions and macros.
DOC_SOURCE_DIR=$(top_srcdir)/src/lib/gst $(top_srcdir)/src/gst

# Extra options to pass to gtkdoc-scangobj. Not normally needed.
SCANGOBJ_OPTIONS=--type-init-func="extern void gst_init(gint *,gchar***);gst_init(&argc,&argv)"

# Extra options to supply to gtkdoc-scan.
# e.g. SCAN_OPTIONS=--deprecated-guards="GTK_DISABLE_DEPRECATED"
SCAN_OPTIONS=--rebuild-types

# Extra options to supply to gtkdoc-mkdb.
# e.g. MKDB_OPTIONS=--sgml-mode --output-format=xml
MKDB_OPTIONS=--sgml-mode --name-space=gstbt --source-suffixes=c,cc,h

# Extra options to supply to gtkdoc-mktmpl
# e.g. MKTMPL_OPTIONS=--only-section-tmpl
MKTMPL_OPTIONS=

# Extra options to supply to gtkdoc-fixref. Not normally needed.
# e.g. FIXXREF_OPTIONS=--extra-dir=../gdk-pixbuf/html --extra-dir=../gdk/html
FIXXREF_OPTIONS=--extra-dir=$(GLIB_PREFIX)/share/gtk-doc/html/ \
	--extra-dir=$(GST_PREFIX)/share/gtk-doc/html/

# Used for dependencies. The docs will be rebuilt if any of these change.
HFILE_GLOB= $(top_srcdir)/src/lib/gst/*.h $(top_srcdir)/src/gst/*/*.h
CFILE_GLOB= $(top_srcdir)/src/lib/gst/*.c $(top_srcdir)/src/gst/*/*.c

# Header files to ignore when scanning.
# e.g. IGNORE_HFILES=gtkdebug.h gtkintl.h
# FIXME: this does not support path and thus is ambigous
IGNORE_HFILES=gstdirectcontrolbinding.h \
	$(BML_IGNORE_H) gstbmlorc.h gstbmlorc-dist.h \
	$(top_srcdir)/src/gst/sidsyn/envelope.h extfilt.h filter.h pot.h siddefs.h sidemu.h spline.h voice.h wave.h \
	$(FLUIDSYNTH_IGNORE_H)

# Images to copy into HTML directory.
# e.g. HTML_IMAGES=$(top_srcdir)/gtk/stock-icons/stock_about_24.png
HTML_IMAGES:=$(wildcard images/*.svg)

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE).
# e.g. content_files=running.sgml building.sgml changes-2.0.sgml
content_files=

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
	$(top_builddir)/libbuzztrax-gst.la \
	$(top_builddir)/libbuzztraxaudio.la \
	$(top_builddir)/libbuzztraxdec.la \
	$(top_builddir)/libgstbml.la \
	$(FLUIDSYNTH_LA) \
	$(top_builddir)/libgstsidsyn.la \
	$(BASE_DEPS_LIBS) \
	$(BT_LIBS)

# This includes the standard gtk-doc make rules, copied by gtkdocize.
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

