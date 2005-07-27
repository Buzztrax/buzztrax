## Process this file with automake to produce Makefile.in

# The name of the module, e.g. 'glib'.
DOC_MODULE=bt-edit

# The top-level SGML file. Change it if you want.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting functions and macros.
DOC_SOURCE_DIR=$(top_srcdir)/src/ui/edit

# Extra options to pass to gtkdoc-scanobj or gtkdoc-scangobj.
SCANOBJ_OPTIONS=--type-init-func="g_type_init();gst_init(&argc,&argv);gtk_init(&argc,&argv)"

# Extra options to supply to gtkdoc-scan.
SCAN_OPTIONS=--deprecated-guards="BT_DISABLE_DEPRECATED"

# Extra options to supply to gtkdoc-mkdb.
MKDB_OPTIONS=--output-format=xml --sgml-mode --ignore-files=parse

# Extra options to supply to gtkdoc-fixref.
FIXXREF_OPTIONS=--extra-dir=$(glib_prefix)/share/gtk-doc/html/gobject \
	--extra-dir=$(glib_prefix)/share/gtk-doc/html/glib \
	--extra-dir=$(gtk_prefix)/share/gtk-doc/html/gtk \
	--extra-dir=$(gcanvas_prefix)/share/gtk-doc/html/libgnomecanvas \
	--extra-dir=$(gst_prefix)/share/gtk-doc/html/gstreamer \
	--extra-dir=$(gst_prefix)/share/gtk-doc/html/gstreamer-libs \
	--extra-dir=../bt-core

# Used for dependencies.
HFILE_GLOB=$(top_srcdir)/src/ui/edit/*.c
CFILE_GLOB=$(top_srcdir)/src/ui/edit/*.h

# Header files to ignore when scanning.
IGNORE_HFILES=config.h

# Images to copy into HTML directory.
HTML_IMAGES =

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE).
content_files =

# Other files to distribute.
extra_files =

# CFLAGS and LDFLAGS for compiling scan program. Only needed if your app/lib
# contains GtkObjects/GObjects and you want to document signals and properties.
GTKDOC_CFLAGS = \
	-I$(top_srcdir)/src/lib/core \
	-I$(top_srcdir)/src/ui/edit \
	-I$(top_builddir) \
	$(BASE_DEPS_CFLAGS) \
	$(GUI_DEPS_CFLAGS) \
	$(BT_CFLAGS)
GTKDOC_LIBS = $(BASE_DEPS_LIBS) \
	$(GUI_DEPS_LIBS) \
	$(top_builddir)/src/lib/core/libbtcore.la \
	$(top_builddir)/src/ui/edit/libbtedit.la

GTKDOC_CC=$(LIBTOOL) --mode=compile $(CC)
GTKDOC_LD=$(LIBTOOL) --mode=link $(CC)

# If you need to override some of the declarations, place them in the
# $(DOC_MODULE)-overrides.txt file and uncomment the second line here.
DOC_OVERRIDES =
#DOC_OVERRIDES = $(DOC_MODULE)-overrides.txt


# include generic part
include $(top_srcdir)/docs/reference/gtk-doc.make
