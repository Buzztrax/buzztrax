@YELP_HELP_RULES@
HELP_ID = buzztrax-edit

HELP_FILES = \
	index.docbook \
	legal.xml

HELP_MEDIA = $(patsubst C/%,%,$(wildcard C/figures/*.png))
HELP_MEDIA += \
	version.entities

# We have no translation for the manual
HELP_LINUAGS =

C/version.entities: $(abs_top_builddir)/docs/version.entities
	@cp $< $@

CLEANFILES = C/version.entities

SUBDIRS = C

EXTRA_DIST = C/aspell.pws

if WITH_ASPELL
check-local:
	@echo "Spellchecking C/$(DOC_MODULE).xml..."
	@if test 0 -ne `cat $(srcdir)/C/$(DOC_MODULE).xml | aspell list --lang=en_US -p $(PWD)/$(srcdir)/C/aspell.pws --mode=sgml | wc -l`; then \
	  echo; \
	  echo "C/$(DOC_MODULE).xml doesn't pass spellchecking, please fix with" >&2; \
	  echo "  aspell check --lang=en_US -p ./C/aspell.pws --mode=sgml C/$(DOC_MODULE).xml" >&2; \
	  echo; \
	  false; \
	else \
	  echo "  done"; \
	fi
else
check-local:
	@echo "Aspell is needed for 'make check'"
endif
