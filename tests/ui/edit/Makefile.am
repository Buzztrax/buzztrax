## Makefile.am for tests/ui/edit

noinst_LTLIBRARIES = libbt-edit-check.la
libbt_edit_check_la_LIBADD = -lm $(BASE_DEPS_LIBS) \
	$(top_builddir)/src/ui/edit/libbtedit.la
libbt_edit_check_la_SOURCES = \
	t-bt-edit.c t-bt-edit.h


INCLUDES = \
	-I@srcdir@ -I$(top_builddir)/src/lib/core \
	-DLOCALEDIR=\"$(localedir)\" \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(datadir)/buzztard\" \
	-DG_LOG_DOMAIN=\"buzztard\" \
	$(BASE_DEPS_CFLAGS) \
	$(BT_CFLAGS)

.PHONY: splint 

splint::
	splint +posixlib -weak -I$(top_builddir) $(INCLUDES) $(libbt_edit_check_la_SOURCES)
