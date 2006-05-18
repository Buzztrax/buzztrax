## Makefile.am for src/ui/edit

bin_PROGRAMS=bt-edit

bt_edit_LDADD = $(BASE_DEPS_LIBS) $(GUI_DEPS_LIBS) \
	$(top_builddir)/src/lib/core/libbtcore.la \
	libbtedit.la
bt_edit_SOURCES= \
	bt-edit.c bt-edit.h

noinst_LTLIBRARIES      = libbtedit.la
libbtedit_la_SOURCES   = \
	edit-application.c edit-application.h edit-application-methods.h \
	machine-canvas-item.c machine-canvas-item.h machine-canvas-item-methods.h\
	machine-menu.c machine-menu.h machine-menu-methods.h \
	machine-properties-dialog.c machine-properties-dialog.h machine-properties-dialog-methods.h \
	machine-preferences-dialog.c machine-preferences-dialog.h machine-preferences-dialog-methods.h \
	main-window.c main-window.h main-window-methods.h \
	main-menu.c main-menu.h main-menu-methods.h \
	main-toolbar.c main-toolbar.h main-toolbar-methods.h \
	main-pages.c main-pages.h main-pages-methods.h \
	main-page-machines.c main-page-machines.h main-page-machines-methods.h \
	main-page-patterns.c main-page-patterns.h main-page-patterns-methods.h \
	main-page-sequence.c main-page-sequence.h main-page-sequence-methods.h \
	main-page-waves.c main-page-waves.h main-page-waves-methods.h \
	main-page-info.c main-page-info.h main-page-info-methods.h \
	main-statusbar.c main-statusbar.h main-statusbar-methods.h \
	pattern-properties-dialog.c pattern-properties-dialog.h pattern-properties-dialog-methods.h \
	pattern-view.c pattern-view.h pattern-view-methods.h \
	sequence-view.c sequence-view.h sequence-view-methods.h \
	settings-dialog.c settings-dialog.h settings-dialog-methods.h \
	settings-page-audiodevices.c settings-page-audiodevices.h settings-page-audiodevices-methods.h \
	tools.c tools.h \
	ui-ressources.c ui-ressources.h ui-ressources-methods.h \
	volume-popup.c volume-popup.h \
	wire-canvas-item.c wire-canvas-item.h wire-canvas-item-methods.h \
	\
	gtkvumeter.c gtkvumeter.h
	
INCLUDES = \
	-I@srcdir@ -I$(top_srcdir)/src/lib/core \
  -DPREFIX=\"$(prefix)\" \
  -DSYSCONFDIR=\"$(sysconfdir)\" \
	-DLOCALEDIR=\"$(localedir)\" \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(datadir)\" \
	-DG_LOG_DOMAIN=\"buzztard\" \
	$(BASE_DEPS_CFLAGS) \
	$(GUI_DEPS_CFLAGS) \
	$(BT_CFLAGS)

desktopdir = ${with_desktop_dir}/applications
desktop_in_files = bt-edit.desktop.in
desktop_DATA = bt-edit.desktop
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = $(desktop_DATA)

clean-local:
	rm -f bt-edit.desktop

.PHONY: splint

splint::
	splint +posixlib -weak -I$(top_builddir) $(INCLUDES) $(libbtedit_la_SOURCES)
