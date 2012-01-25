SRCDIRS = src tests
SUBDIRS = $(SRCDIRS) pixmaps po docs

ACLOCAL_AMFLAGS = -I m4
DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc --enable-debug --disable-scrollkeeper

schemasdir = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_files = buzztard.schemas.in
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)

EXTRA_DIST = \
	$(schemas_DATA) \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	COPYING-DOCS \
	omf.make \
	xmldocs.make \
	TODO \
	po/$(GETTEXT_PACKAGE).pot

@INTLTOOL_SCHEMAS_RULE@

DISTCLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update

clean-local:
	rm -f po/.intltool-merge-cache

if GCONF_SCHEMAS_INSTALL
install-data-local:
	@if test -z "$(DESTDIR)" ; then \
		for p in $(schemas_DATA) ; do \
			GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
			$(GCONFTOOL) --makefile-install-rule $(top_srcdir)/$$p ; \
		done \
	fi
endif

if GCONF_SCHEMAS_INSTALL
uninstall-local:
	@if test -z "$(DESTDIR)" ; then \
		for p in $(schemas_DATA) ; do \
			GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
			$(GCONFTOOL) --makefile-uninstall-rule $(top_srcdir)/$$p ; \
		done \
	fi
endif

## this seems to fail during make distcheck
##uninstall-local:
##	rm -r $(datadir)/$(PACKAGE)


## We get warnings for left-over files when doing 'make distuninstallcheck'
distuninstallcheck_listfiles = find . -type f -print

## Workaround for scrollkeeper bug
distuninstallcheck_listfiles += | grep -v '/var/scrollkeeper'
## Workaround for installing files in own prefix
distuninstallcheck_listfiles += | grep -v '/share/mime'
## Workaround for installing icon theme in own prefix
distuninstallcheck_listfiles += | grep -v 'icon-theme.cache'


.PHONY: coverage valgrind dict cc10 cc20

# make coverage        -- run all checks and generate html coverage report (only availble when built using --enable-coverage)
if USE_LCOV
coverage::
	mkdir -p ./coverage
	lcov --compat-libtool --directory . --zerocounters
	-$(MAKE) check
	lcov --compat-libtool --directory ./src --capture --output-file ./coverage/$(PACKAGE)_tmp.info
	lcov --compat-libtool --extract ./coverage/$(PACKAGE)_tmp.info "$(PWD)/src*" -o ./coverage/$(PACKAGE).info
	$(RM) ./coverage/$(PACKAGE)_tmp.info
	genhtml -o ./coverage --num-spaces 2 ./coverage/$(PACKAGE).info
endif
if USE_BCOV
coverage::
	mkdir -p ./coverage
	$(MAKE) -C tests coverage
	bcov-report -i $(abs_top_builddir)/src tests/.bcovdump.all ./coverage
endif

# make valgrind        -- run all checks under valgrind
if USE_VALGRIND
valgrind::
	$(MAKE) -C tests valgrind
endif

# make dict            -- create an aspell dictionary from tags
dict:: tags
	echo "personal_ws" >$(PACKAGE).aspell_dict
	@find . -name tags -exec grep -v "$\!" {} \; | cut -f1 | sort | uniq >>$(PACKAGE).aspell_dict

# -k2 in sort for Tranditional McCabe Cyclomatic Complexity
# make cc10            -- report McCabe Cyclomatic Complexity
# make cc20            -- report McCabe Cyclomatic Complexity
cc10::
	@pmccabe `find src -name '*.c'` | sort -nr | awk '($$1 > 10)'
cc20::
	@pmccabe `find src -name '*.c'` | sort -nr | awk '($$1 > 20)'

include $(top_srcdir)/common.mak
