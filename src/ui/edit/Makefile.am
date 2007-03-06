## Makefile.am for src/ui/edit

bin_PROGRAMS=bt-edit

bt_edit_LDADD = $(BASE_DEPS_LIBS) $(GUI_DEPS_LIBS) \
	$(top_builddir)/src/lib/core/libbtcore.la \
  $(top_builddir)/src/lib/ic/libbtic.la \
	libbtedit.la
bt_edit_SOURCES= \
	bt-edit.c bt-edit.h

noinst_LTLIBRARIES      = libbtedit.la
libbtedit_la_SOURCES   = \
	edit-application.c edit-application.h edit-application-methods.h \
	machine-canvas-item.c machine-canvas-item.h machine-canvas-item-methods.h\
	machine-menu.c machine-menu.h machine-menu-methods.h \
	machine-actions.c machine-actions.h \
  machine-preset-properties-dialog.c machine-preset-properties-dialog.h machine-preset-properties-dialog-methods.h \
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
  playback-controller-socket.c playback-controller-socket.h playback-controller-socket-methods.h \
	sequence-view.c sequence-view.h sequence-view-methods.h \
	settings-dialog.c settings-dialog.h settings-dialog-methods.h \
	settings-page-audiodevices.c settings-page-audiodevices.h settings-page-audiodevices-methods.h \
	settings-page-controldevices.c settings-page-controldevices.h settings-page-controldevices-methods.h \
	tools.c tools.h \
	ui-ressources.c ui-ressources.h ui-ressources-methods.h \
	volume-popup.c volume-popup.h \
	wire-analysis-dialog.c wire-analysis-dialog.h wire-analysis-dialog-methods.h \
	wire-canvas-item.c wire-canvas-item.h wire-canvas-item-methods.h \
	\
	gtkvumeter.c gtkvumeter.h
	
INCLUDES = \
	-I$(top_srcdir)/src/lib/core \
	-I$(top_srcdir)/src/lib/ic \
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

.PHONY: help todo stats splint

todo::
	@find . \( -name "*.c" -o -name "*.h" \) -exec grep -Hno "@todo .*" {} \; | sed "s/@todo/error:/"
	@find . \( -name "*.c" -o -name "*.h" \) -exec grep -Hno "@idea .*" {} \; | sed "s/@idea/warning:/"

stats:: tags
	@echo "files by size:"
	@find . \( -name "*.c" -o -name "*.h" \) -printf "%s %p\n" | sort -rn
	@echo
	@echo "files by tags:"
	@for file in *.c; do size=`grep $${file} tags | wc -l`;echo $${size} $${file}; done | sort -rn

splint::
	splint +posixlib -weak -I$(top_builddir) $(INCLUDES) $(libbtedit_la_SOURCES)

help::
	@echo "make todo                    -- generate a list of TODO items in the form of gcc error/warnings"
	@echo "make stats                   -- run source code stats"
	@echo "make splint                  -- run splint source code checks"
