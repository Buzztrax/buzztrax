SUBDIRS = C

DOC_MODULE = buzztard-edit

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
