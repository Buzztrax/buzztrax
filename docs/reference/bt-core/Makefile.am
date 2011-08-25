## Process this file with automake to produce Makefile.in

# We require automake 1.6 at least.
AUTOMAKE_OPTIONS = 1.6

# The name of the module, e.g. 'glib'.
DOC_MODULE=buzztard-core

# The top-level SGML file. You can change this if you want to.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting the functions and macros.
DOC_SOURCE_DIR=$(top_srcdir)/src/lib/core

# Extra options to pass to gtkdoc-scangobj. Not normally needed.
#SCANGOBJ_OPTIONS=--type-init-func="do { void gst_init(int *argc,char **argv[]);g_type_init();gst_init(&argc,&argv); } while (0)"
#SCANGOBJ_OPTIONS=--type-init-func="g_type_init();gst_init(&argc,&argv)"
SCANGOBJ_OPTIONS=--type-init-func="g_thread_init(NULL);g_type_init();gst_init(&argc,&argv)"

# Extra options to supply to gtkdoc-scan.
# --rebuild-types requires gtk-doc 1.8
SCAN_OPTIONS=--deprecated-guards="BT_DISABLE_DEPRECATED"

# Extra options to supply to gtkdoc-mkdb.
MKDB_OPTIONS=--sgml-mode

# Extra options to supply to gtkdoc-mktmpl
# e.g. MKTMPL_OPTIONS=--only-section-tmpl
MKTMPL_OPTIONS=

# Extra options to supply to gtkdoc-fixref. Not normally needed.
FIXXREF_OPTIONS=--extra-dir=$(GLIB_PREFIX)/share/gtk-doc/html/ \
	--extra-dir=$(GST_PREFIX)/share/gtk-doc/html/ \
	--extra-dir=..

# Used for dependencies. The docs will be rebuilt if any of these change.
HFILE_GLOB=$(top_srcdir)/src/lib/core/libbuzztard-core/*.h
CFILE_GLOB=$(top_srcdir)/src/lib/core/*.c

# Header files to ignore when scanning.
IGNORE_HFILES=core_private.h settings-private.h marshal.h

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
	-I$(top_srcdir)/src/lib/core \
	-I$(top_srcdir)/src/lib/ic \
	-I$(top_builddir) \
	$(BASE_DEPS_CFLAGS) \
	$(BT_CFLAGS)
GTKDOC_LIBS = \
	$(top_builddir)/src/lib/core/libbuzztard-core.la \
	$(top_builddir)/src/lib/ic/libbuzztard-ic.la \
	$(BASE_DEPS_LIBS) \
	$(BT_LIBS)

# include generic part
include $(top_srcdir)/gtk-doc.make

# Other files to distribute
# e.g. EXTRA_DIST += version.xml.in
EXTRA_DIST += $(DOC_MODULE).types

# Comment this out if you want your docs-status tested during 'make check'
if ENABLE_GTK_DOC
TESTS_ENVIRONMENT = cd $(srcdir) && \
	DOC_MODULE=$(DOC_MODULE) DOC_MAIN_SGML_FILE=$(DOC_MAIN_SGML_FILE) \
	SRCDIR=$(abs_srcdir) BUILDDIR=$(abs_builddir)
TESTS = $(GTKDOC_CHECK)
endif

