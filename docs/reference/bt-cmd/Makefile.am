## Process this file with automake to produce Makefile.in

# The name of the module, e.g. 'glib'.
DOC_MODULE=bt-cmd

# The top-level SGML file. Change it if you want.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting functions and macros.
DOC_SOURCE_DIR=$(top_builddir)/src/ui/cmd

# Extra options to pass to gtkdoc-scanobj or gtkdoc-scangobj.
SCANOBJ_OPTIONS=--type-init-func="g_type_init();gst_init(&argc,&argv)"

# Extra options to supply to gtkdoc-scan.
SCAN_OPTIONS=

# Extra options to supply to gtkdoc-mkdb.
MKDB_OPTIONS=--output-format=xml --sgml-mode --ignore-files=parse

# Extra options to supply to gtkdoc-fixref.
FIXXREF_OPTIONS=--extra-dir=$(glib_prefix)/share/gtk-doc/html/gobject \
  --extra-dir=$(glib_prefix)/share/gtk-doc/html/glib \
	--extra-dir=$(gst_prefix)/share/gtk-doc/html/gstreamer \
	--extra-dir=$(gst_prefix)/share/gtk-doc/html/gstreamer-libs \
	--extra-dir=../bt-core

# Used for dependencies.
HFILE_GLOB=
CFILE_GLOB=

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
	-I$(top_builddir)/src/lib/core \
	-I$(top_builddir)/src/ui/cmd \
	-I$(top_builddir) \
	$(BASE_DEPS_CFLAGS) \
	$(BT_CFLAGS)
GTKDOC_LIBS = $(BASE_DEPS_LIBS) \
							$(top_builddir)/src/lib/core/libbtcore.la \
							$(top_builddir)/src/ui/cmd/libbtcmd.la

GTKDOC_CC=$(LIBTOOL) --mode=compile $(CC)
GTKDOC_LD=$(LIBTOOL) --mode=link $(CC)

# If you need to override some of the declarations, place them in the
# $(DOC_MODULE)-overrides.txt file and uncomment the second line here.
DOC_OVERRIDES =
#DOC_OVERRIDES = $(DOC_MODULE)-overrides.txt



###########################################################################
# Everything below here is generic and you shouldn't need to change it.
###########################################################################

TARGET_DIR=$(HTML_DIR)/$(DOC_MODULE)

EXTRA_DIST = 				\
	$(content_files)		\
	$(extra_files)			\
	$(HTML_IMAGES)			\
	$(DOC_MAIN_SGML_FILE)		\
	$(DOC_MODULE).types		\
	$(DOC_MODULE)-sections.txt	\
	$(DOC_OVERRIDES)

DOC_STAMPS=scan-build.stamp tmpl-build.stamp sgml-build.stamp html-build.stamp \
	   $(srcdir)/tmpl.stamp $(srcdir)/sgml.stamp $(srcdir)/html.stamp

SCANOBJ_FILES = 		 \
	$(DOC_MODULE).args 	 \
	$(DOC_MODULE).hierarchy  \
	$(DOC_MODULE).interfaces \
	$(DOC_MODULE).prerequisites \
	$(DOC_MODULE).signals

if ENABLE_GTK_DOC
all-local: html-build.stamp

#### scan ####

scan-build.stamp: $(HFILE_GLOB)
	@echo '*** Scanning header files ***'
	if grep -l '^..*$$' $(srcdir)/$(DOC_MODULE).types > /dev/null ; then \
	    CC="$(GTKDOC_CC)" LD="$(GTKDOC_LD)" CFLAGS="$(GTKDOC_CFLAGS)" LDFLAGS="$(GTKDOC_LIBS)" gtkdoc-scangobj $(SCANOBJ_OPTIONS) --module=$(DOC_MODULE) --output-dir=$(srcdir) ; \
	else \
	    cd $(srcdir) ; \
	    for i in $(SCANOBJ_FILES) ; do \
               test -f $$i || touch $$i ; \
	    done \
	fi
	cd $(srcdir) && \
	  gtkdoc-scan --module=$(DOC_MODULE) --source-dir=$(DOC_SOURCE_DIR) --ignore-headers="$(IGNORE_HFILES)" $(SCAN_OPTIONS) $(EXTRA_HFILES)
	touch scan-build.stamp

$(DOC_MODULE)-decl.txt $(SCANOBJ_FILES): scan-build.stamp
	@true

#### templates ####

tmpl-build.stamp: $(DOC_MODULE)-decl.txt $(SCANOBJ_FILES) $(DOC_MODULE)-sections.txt $(DOC_OVERRIDES)
	@echo '*** Rebuilding template files ***'
	cd $(srcdir) && gtkdoc-mktmpl --module=$(DOC_MODULE)
	touch tmpl-build.stamp

tmpl.stamp: tmpl-build.stamp
	@true

#### sgml ####

sgml-build.stamp: tmpl.stamp $(CFILE_GLOB) $(srcdir)/tmpl/*.sgml
	@echo '*** Building SGML ***'
	cd $(srcdir) && \
	gtkdoc-mkdb --module=$(DOC_MODULE) --source-dir=$(DOC_SOURCE_DIR) --main-sgml-file=$(DOC_MAIN_SGML_FILE) $(MKDB_OPTIONS)
	touch sgml-build.stamp

sgml.stamp: sgml-build.stamp
	@true

#### html ####

html-build.stamp: sgml.stamp $(DOC_MAIN_SGML_FILE) $(content_files)
	@echo '*** Building HTML ***'
	test -d $(srcdir)/html || mkdir $(srcdir)/html
	cd $(srcdir)/html && gtkdoc-mkhtml $(DOC_MODULE) ../$(DOC_MAIN_SGML_FILE)
	test "x$(HTML_IMAGES)" = "x" || ( cd $(srcdir) && cp $(HTML_IMAGES) html )
	@echo '-- Fixing Crossreferences' 
	cd $(srcdir) && gtkdoc-fixxref --module-dir=html --html-dir=$(HTML_DIR) $(FIXXREF_OPTIONS)
	touch html-build.stamp
endif

##############

clean-local:
	rm -f *~ *.bak $(SCANOBJ_FILES) *-unused.txt $(DOC_STAMPS)

maintainer-clean-local: clean
	cd $(srcdir) && rm -rf sgml html $(DOC_MODULE)-decl-list.txt $(DOC_MODULE)-decl.txt

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(TARGET_DIR)
	(installfiles=`echo $(srcdir)/html/*.html`; \
	if test "$$installfiles" = '$(srcdir)/html/*.html'; \
	then echo '-- Nothing to install' ; \
	else \
	  for i in $$installfiles; do \
	    echo '-- Installing '$$i ; \
	    $(INSTALL_DATA) $$i $(DESTDIR)$(TARGET_DIR); \
	  done; \
	  pngfiles=`echo ./html/*.png`; \
	  if test "$$pngfiles" != './html/*.png'; then \
	    for i in $$pngfiles; do \
	      echo '-- Installing '$$i ; \
	      $(INSTALL_DATA) $$i $(DESTDIR)$(TARGET_DIR); \
	    done; \
	  fi; \
	  echo '-- Installing $(srcdir)/html/$(DOC_MODULE).devhelp' ; \
	  $(INSTALL_DATA) $(srcdir)/html/$(DOC_MODULE).devhelp \
	    $(DESTDIR)$(TARGET_DIR)/$(DOC_MODULE).devhelp; \
	  echo '-- Installing $(srcdir)/html/index.sgml' ; \
	  $(INSTALL_DATA) $(srcdir)/html/index.sgml $(DESTDIR)$(TARGET_DIR); \
	fi)

uninstall-local:
	(installfiles=`echo ./html/*.html`; \
	if test "$$installfiles" = './html/*.html'; \
	then echo '-- Nothing to uninstall' ; \
	else \
	  for i in $$installfiles; do \
	    rmfile=`basename $$i` ; \
	    echo '-- Uninstalling $(DESTDIR)$(TARGET_DIR)/'$$rmfile ; \
	    rm -f $(DESTDIR)$(TARGET_DIR)/$$rmfile; \
	  done; \
	  pngfiles=`echo ./html/*.png`; \
	  if test "$$pngfiles" != './html/*.png'; then \
	    for i in $$pngfiles; do \
	      rmfile=`basename $$i` ; \
	      echo '-- Uninstalling $(DESTDIR)$(TARGET_DIR)/'$$rmfile ; \
	      rm -f $(DESTDIR)$(TARGET_DIR)/$$rmfile; \
	    done; \
	  fi; \
	  echo '-- Uninstalling $(DESTDIR)$(TARGET_DIR)/$(DOC_MODULE).devhelp' ; \
	  rm -f $(DESTDIR)$(TARGET_DIR)/$(DOC_MODULE)-@BT_MAJORMINOR@.devhelp; \
	  echo '-- Uninstalling $(DESTDIR)$(TARGET_DIR)/index.sgml' ; \
	  rm -f $(DESTDIR)$(TARGET_DIR)/index.sgml; \
	fi) 
	if test -d $(DESTDIR)$(TARGET_DIR); then rmdir -p --ignore-fail-on-non-empty $(DESTDIR)$(TARGET_DIR); fi

#
# Require gtk-doc when making dist
#
if ENABLE_GTK_DOC
dist-check-gtkdoc:
else
dist-check-gtkdoc:
	@echo "*** gtk-doc must be installed and enabled in order to make dist"
	@false
endif

dist-hook: dist-check-gtkdoc dist-hook-local
	mkdir $(distdir)/tmpl
	mkdir $(distdir)/sgml
	mkdir $(distdir)/html
	-cp $(srcdir)/tmpl/*.sgml $(distdir)/tmpl
	-cp $(srcdir)/sgml/*.sgml $(distdir)/sgml
	-cp $(srcdir)/html/index.sgml $(distdir)/html
	-cp $(srcdir)/html/*.html $(srcdir)/html/*.css $(distdir)/html

	images=$(HTML_IMAGES) ;    	      \
	for i in $$images ; do		      \
	  cp $(srcdir)/$$i $(distdir)/html ;  \
	done

.PHONY : dist-hook-local
