## Makefile.am for tests/ui/edit

noinst_LTLIBRARIES = libbt-edit-check.la
libbt_edit_check_la_LIBADD = $(BASE_DEPS_LIBS) $(GUI_DEPS_LIBS) \
	$(top_builddir)/src/ui/edit/libbtedit.la
libbt_edit_check_la_SOURCES = m-bt-edit.h \
	s-bt-edit-application.c e-bt-edit-application.c \
	s-bt-machine-preset-properties-dialog.c e-bt-machine-preset-properties-dialog.c \
	s-bt-pattern-properties-dialog.c e-bt-pattern-properties-dialog.c \
	s-bt-settings-dialog.c e-bt-settings-dialog.c

INCLUDES = \
	-I$(top_srcdir)/src/lib/core \
	-I$(top_srcdir)/src/lib/ic \
	-DLOCALEDIR=\"$(localedir)\" \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(datadir)\" \
	-DG_LOG_DOMAIN=\"buzztard\" \
	$(BASE_DEPS_CFLAGS) \
	$(GUI_DEPS_CFLAGS) \
	$(BT_CFLAGS)

.PHONY: splint 

splint::
	splint +posixlib -weak -I$(top_builddir) $(INCLUDES) $(libbt_edit_check_la_SOURCES)
