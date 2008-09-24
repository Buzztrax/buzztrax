## Makefile.am for tests/ui/edit

noinst_LTLIBRARIES = libbt-edit-check.la
libbt_edit_check_la_LIBADD = $(BASE_DEPS_LIBS) $(GUI_DEPS_LIBS) \
	$(top_builddir)/src/ui/edit/libbtedit.la
libbt_edit_check_la_SOURCES = m-bt-edit.h \
	s-bt-about-dialog.c e-bt-about-dialog.c \
	s-bt-edit-application.c e-bt-edit-application.c \
	s-bt-machine-preset-properties-dialog.c e-bt-machine-preset-properties-dialog.c \
	s-bt-machine-preferences-dialog.c e-bt-machine-preferences-dialog.c \
	s-bt-machine-properties-dialog.c e-bt-machine-properties-dialog.c \
	s-bt-machine-rename-dialog.c e-bt-machine-rename-dialog.c \
	s-bt-missing-framework-elements-dialog.c e-bt-missing-framework-elements-dialog.c \
	s-bt-missing-song-elements-dialog.c e-bt-missing-song-elements-dialog.c \
	s-bt-pattern-properties-dialog.c e-bt-pattern-properties-dialog.c \
	s-bt-render-dialog.c e-bt-render-dialog.c \
	s-bt-settings-dialog.c e-bt-settings-dialog.c \
	s-bt-wire-analysis-dialog.c e-bt-wire-analysis-dialog.c

AM_CPPFLAGS = \
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
	splint +posixlib -weak -I$(top_builddir) $(AM_CPPFLAGS) $(libbt_edit_check_la_SOURCES)
