## Makefile.am for tests/ui/edit

noinst_LTLIBRARIES = libbt-edit-check.la
libbt_edit_check_la_LIBADD = \
	$(top_builddir)/src/ui/edit/libbtedit.la \
	$(BASE_DEPS_LIBS) $(GUI_DEPS_LIBS)
libbt_edit_check_la_SOURCES = m-bt-edit.h \
	s-bt-about-dialog.c e-bt-about-dialog.c \
	s-bt-change-log.c e-bt-change-log.c \
	s-bt-controller-learn-dialog.c e-bt-controller-learn-dialog.c \
	s-bt-crash-recover-dialog.c e-bt-crash-recover-dialog.c \
	s-bt-edit-application.c e-bt-edit-application.c \
	s-bt-interaction-controller-menu.c e-bt-interaction-controller-menu.c \
	s-bt-machine-actions.c e-bt-machine-actions.c \
	s-bt-machine-page.c e-bt-machine-page.c \
	s-bt-machine-preset-properties-dialog.c e-bt-machine-preset-properties-dialog.c \
	s-bt-machine-preferences-dialog.c e-bt-machine-preferences-dialog.c \
	s-bt-machine-properties-dialog.c e-bt-machine-properties-dialog.c \
	s-bt-machine-rename-dialog.c e-bt-machine-rename-dialog.c \
	s-bt-main-window.c e-bt-main-window.c \
	s-bt-missing-framework-elements-dialog.c e-bt-missing-framework-elements-dialog.c \
	s-bt-missing-song-elements-dialog.c e-bt-missing-song-elements-dialog.c \
	s-bt-pattern-page.c e-bt-pattern-page.c t-bt-pattern-page.c \
	s-bt-pattern-properties-dialog.c e-bt-pattern-properties-dialog.c \
	s-bt-render-dialog.c e-bt-render-dialog.c \
	s-bt-render-progress-dialog.c e-bt-render-progress-dialog.c \
	s-bt-sequence-page.c e-bt-sequence-page.c \
	s-bt-settings-dialog.c e-bt-settings-dialog.c \
	s-bt-signal-analysis-dialog.c e-bt-signal-analysis-dialog.c \
	s-bt-tip-dialog.c e-bt-tip-dialog.c

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

include $(top_srcdir)/common.mak
