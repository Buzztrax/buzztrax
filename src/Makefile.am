if USE_GCONF
GCONF_SETTINGS_C_FILES = lib/core/gconf-settings.c
GCONF_SETTINGS_H_FILES = lib/core/gconf-settings.h
else
GCONF_SETTINGS_C_FILES =
GCONF_SETTINGS_H_FILES =
endif

if USE_GUDEV
GUDEV_C_FILES=lib/ic/gudev-discoverer.c
GUDEV_H_FILES=lib/ic/gudev-discoverer.h
else
GUDEV_C_FILES=
GUDEV_H_FILES=
endif

if USE_LINUX_INPUT
LINUX_INPUT_C_FILES=lib/ic/input-device.c
LINUX_INPUT_H_FILES=lib/ic/input-device.h
else
LINUX_INPUT_C_FILES=
LINUX_INPUT_H_FILES=
endif

AM_CPPFLAGS = \
	-I$(srcdir) \
	-I$(top_srcdir)/src/lib/core \
	-I$(top_srcdir)/src/lib/ic \
	-DPREFIX=\"$(prefix)\" \
	-DSYSCONFDIR=\"$(sysconfdir)\" \
	-DLOCALEDIR=\"$(localedir)\" \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(datadir)\" \
	-DG_LOG_DOMAIN=\"buzztard\" \
	$(BASE_DEPS_CFLAGS) \
	$(BT_CFLAGS)

AM_LDFLAGS = $(BT_LDFLAGS)

# -- libs ----------------------------------------------------------------------

lib_LTLIBRARIES = libbuzztard-core.la libbuzztard-ic.la

libbuzztard_core_la_LIBADD = \
	$(top_builddir)/src/lib/ic/libbuzztard-ic.la \
	$(BASE_DEPS_LIBS) $(BT_LIBS) $(LIBM) \
	-lgstaudio-@GST_MAJORMINOR@ -lgstpbutils-@GST_MAJORMINOR@
libbuzztard_core_la_LDFLAGS = \
	-export-symbols-regex ^_?\(bt_\|Bt\|BT_\).* \
	-version-info @BT_VERSION_INFO@
libbuzztard_core_la_CFLAGS = -I$(top_builddir)/src/lib/core
libbuzztard_core_la_SOURCES = \
	lib/core/core.c \
	lib/core/marshal.c \
	lib/core/tools.c \
	lib/core/buzzcallbacks.c \
	lib/core/application.c \
	lib/core/audio-session.c \
	lib/core/childproxy.c \
	lib/core/cmd-pattern.c \
	$(GCONF_SETTINGS_C_FILES) \
	lib/core/machine.c \
	lib/core/parameter-group.c \
	lib/core/pattern.c \
	lib/core/persistence.c \
	lib/core/processor-machine.c \
	lib/core/sequence.c \
	lib/core/settings.c \
	lib/core/setup.c \
	lib/core/sink-bin.c \
	lib/core/sink-machine.c \
	lib/core/song.c \
	lib/core/song-info.c \
	lib/core/song-io.c \
	lib/core/song-io-native.c \
	lib/core/song-io-native-bzt.c \
	lib/core/song-io-native-xml.c \
	lib/core/source-machine.c \
	lib/core/value-group.c \
	lib/core/wavetable.c \
	lib/core/wave.c \
	lib/core/wavelevel.c \
	lib/core/wire.c

# yet unused sources
#	plainfile-settings.c libbuzztard-core/plainfile-settings.h \
#

libbuzztard_coredir = $(includedir)/libbuzztard-core
libbuzztard_core_HEADERS = \
	lib/core/core.h \
	lib/core/marshal.h \
	lib/core/tools.h \
	lib/core/version.h \
	lib/core/application.h \
	lib/core/audio-session.h \
	lib/core/childproxy.h \
	lib/core/cmd-pattern.h \
	$(GCONF_SETTINGS_H_FILES) \
	lib/core/machine.h \
	lib/core/parameter-group.h \
	lib/core/pattern.h \
	lib/core/persistence.h \
	lib/core/plainfile-settings.h \
	lib/core/processor-machine.h \
	lib/core/sequence.h \
	lib/core/settings.h \
	lib/core/setup.h \
	lib/core/sink-bin.h \
	lib/core/sink-machine.h \
	lib/core/song.h \
	lib/core/song-info.h \
	lib/core/song-io.h \
	lib/core/song-io-native.h \
	lib/core/song-io-native-bzt.h \
	lib/core/song-io-native-xml.h \
	lib/core/source-machine.h \
	lib/core/value-group.h \
	lib/core/wavetable.h \
	lib/core/wave.h \
	lib/core/wavelevel.h \
	lib/core/wire.h

# yet unused sources
#	plainfile-settings.h \
#

libbuzztard_ic_la_LIBADD = \
	$(BASE_DEPS_LIBS) $(BT_LIBS) $(LIBM)
libbuzztard_ic_la_LDFLAGS = \
	-export-symbols-regex ^_?\(btic_\|BtIc\|BTIC_\).* \
	-version-info @BT_VERSION_INFO@
libbuzztard_ic_la_CFLAGS = -I$(top_builddir)/src/lib/ic
libbuzztard_ic_la_SOURCES = \
	lib/ic/ic.c \
	lib/ic/registry.c\
	$(GUDEV_C_FILES) \
	lib/ic/device.c \
	lib/ic/learn.c \
	$(LINUX_INPUT_C_FILES) \
	lib/ic/midi-device.c \
	lib/ic/control.c \
	lib/ic/abs-range-control.c \
	lib/ic/trigger-control.c

libbuzztard_icdir = $(includedir)/libbuzztard-ic
libbuzztard_ic_HEADERS = \
	lib/ic/ic.h \
	lib/ic/version.h \
	lib/ic/registry.h \
	$(GUDEV_H_FILES) \
	lib/ic/device.h \
	lib/ic/learn.h \
	$(LINUX_INPUT_H_FILES) \
	lib/ic/midi-device.h \
	lib/ic/control.h \
	lib/ic/abs-range-control.h \
	lib/ic/trigger-control.h

noinst_HEADERS = \
  lib/core/core_private.h \
  lib/core/marshal.h \
  lib/core/settings-private.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = \
  lib/core/libbuzztard-core.pc \
  lib/ic/libbuzztard-ic.pc

lib/core/marshal.h: lib/core/marshal.list
	glib-genmarshal --header --prefix=bt_marshal $(srcdir)< > $@

lib/core/marshal.c: lib/core/marshal.list
	glib-genmarshal --body --prefix=bt_marshal $(srcdir)/$< > $@

# -- songio plugins
songiodir = ${exec_prefix}/lib/buzztard-songio
songio_LTLIBRARIES = libbtbsl.la
libbtbsl_la_LIBADD = \
	$(top_builddir)/src/lib/core/libbuzztard-core.la \
  $(BASE_DEPS_LIBS) $(BT_LIBS) $(LIBINTL)
libbtbsl_la_LDFLAGS = \
	$(BT_LDFLAGS) \
	-export-symbols-regex ^_?\(bt_\|Bt\|BT_\).* \
	-module -avoid-version
libbtbsl_la_LIBTOOLFLAGS = --tag=disable-static
libbtbsl_la_SOURCES = \
	lib/core/songio/bsl/bsl.c \
	lib/core/songio/bsl/song-io-buzz.c \
	lib/core/songio/bsl/bsl.h \
	lib/core/songio/bsl/song-io-buzz.h \
	lib/core/songio/bsl/song-io-buzz-private.h

# -- apps ----------------------------------------------------------------------

bin_PROGRAMS = buzztard-cmd buzztard-edit
noinst_LTLIBRARIES = libbtcmd.la libbtedit.la
plugin_LTLIBRARIES = libbuzztarddec.la

buzztard_cmd_LDADD = \
	$(top_builddir)/src/lib/core/libbuzztard-core.la \
	$(top_builddir)/src/lib/ic/libbuzztard-ic.la \
	libbtcmd.la \
	$(BASE_DEPS_LIBS) $(BT_LIBS)
buzztard_cmd_SOURCES= \
	ui/cmd/bt-cmd.c ui/cmd/bt-cmd.h

libbtcmd_la_LIBDADD = \
	$(top_builddir)/src/lib/core/libbuzztard-core.la \
	$(top_builddir)/src/lib/ic/libbuzztard-ic.la \
	$(BASE_DEPS_LIBS) $(BT_LIBS) $(LIBM)
libbtcmd_la_SOURCES =	\
	ui/cmd/cmd-application.c ui/cmd/cmd-application.h

buzztard_edit_LDADD = \
	$(top_builddir)/src/lib/core/libbuzztard-core.la \
	$(top_builddir)/src/lib/ic/libbuzztard-ic.la \
	libbtedit.la \
	$(BASE_DEPS_LIBS) $(GUI_DEPS_LIBS) $(BT_LIBS) \
	-lgstinterfaces-@GST_MAJORMINOR@
buzztard_edit_CFLAGS = -I$(top_builddir)/src/ui/edit $(GUI_DEPS_CFLAGS)
buzztard_edit_SOURCES= \
	ui/edit/bt-edit.c ui/edit/bt-edit.h

libbtedit_la_LIBADD = \
	$(top_builddir)/src/lib/core/libbuzztard-core.la \
	$(top_builddir)/src/lib/ic/libbuzztard-ic.la \
	$(BASE_DEPS_LIBS) $(GUI_DEPS_LIBS) $(BT_LIBS) $(LIBM) \
	-lgstinterfaces-@GST_MAJORMINOR@
libbtedit_la_CFLAGS = -I$(top_builddir)/src/ui/edit $(GUI_DEPS_CFLAGS)
libbtedit_la_SOURCES	= \
	ui/edit/about-dialog.c ui/edit/about-dialog.h \
	ui/edit/change-log.c ui/edit/change-log.h \
	ui/edit/change-logger.c ui/edit/change-logger.h \
	ui/edit/crash-recover-dialog.c ui/edit/crash-recover-dialog.h \
	ui/edit/edit-application.c ui/edit/edit-application.h \
	ui/edit/interaction-controller-menu.c ui/edit/interaction-controller-menu.h \
	ui/edit/interaction-controller-learn-dialog.c ui/edit/interaction-controller-learn-dialog.h \
	ui/edit/machine-canvas-item.c ui/edit/machine-canvas-item.h \
	ui/edit/machine-menu.c ui/edit/machine-menu.h \
	ui/edit/machine-actions.c ui/edit/machine-actions.h \
	ui/edit/machine-list-model.c ui/edit/machine-list-model.h \
	ui/edit/machine-preset-properties-dialog.c ui/edit/machine-preset-properties-dialog.h \
	ui/edit/machine-properties-dialog.c ui/edit/machine-properties-dialog.h \
	ui/edit/machine-preferences-dialog.c ui/edit/machine-preferences-dialog.h \
	ui/edit/machine-rename-dialog.c ui/edit/machine-rename-dialog.h \
	ui/edit/main-window.c ui/edit/main-window.h \
	ui/edit/main-menu.c ui/edit/main-menu.h \
	ui/edit/main-toolbar.c ui/edit/main-toolbar.h \
	ui/edit/main-pages.c ui/edit/main-pages.h \
	ui/edit/main-page-machines.c ui/edit/main-page-machines.h \
	ui/edit/main-page-patterns.c ui/edit/main-page-patterns.h \
	ui/edit/main-page-sequence.c ui/edit/main-page-sequence.h \
	ui/edit/main-page-waves.c ui/edit/main-page-waves.h \
	ui/edit/main-page-info.c ui/edit/main-page-info.h \
	ui/edit/main-statusbar.c ui/edit/main-statusbar.h \
	ui/edit/missing-framework-elements-dialog.c ui/edit/missing-framework-elements-dialog.h \
	ui/edit/missing-song-elements-dialog.c ui/edit/missing-song-elements-dialog.h \
	ui/edit/object-list-model.c ui/edit/object-list-model.h \
	ui/edit/panorama-popup.c ui/edit/panorama-popup.h \
	ui/edit/pattern-list-model.c ui/edit/pattern-list-model.h \
	ui/edit/pattern-properties-dialog.c ui/edit/pattern-properties-dialog.h \
	ui/edit/playback-controller-socket.c ui/edit/playback-controller-socket.h \
	ui/edit/render-dialog.c ui/edit/render-dialog.h \
	ui/edit/render-progress.c ui/edit/render-progress.h \
	ui/edit/sequence-grid-model.c ui/edit/sequence-grid-model.h \
	ui/edit/sequence-view.c ui/edit/sequence-view.h \
	ui/edit/settings-dialog.c ui/edit/settings-dialog.h \
	ui/edit/settings-page-audiodevices.c ui/edit/settings-page-audiodevices.h \
	ui/edit/settings-page-directories.c ui/edit/settings-page-directories.h \
	ui/edit/settings-page-interaction-controller.c ui/edit/settings-page-interaction-controller.h \
	ui/edit/settings-page-playback-controller.c ui/edit/settings-page-playback-controller.h \
	ui/edit/settings-page-shortcuts.c ui/edit/settings-page-shortcuts.h \
	ui/edit/signal-analysis-dialog.c ui/edit/signal-analysis-dialog.h \
	ui/edit/tip-dialog.c ui/edit/tip-dialog.h \
	ui/edit/tools.c ui/edit/tools.h \
	ui/edit/ui-resources.c ui/edit/ui-resources.h \
	ui/edit/volume-popup.c ui/edit/volume-popup.h \
	ui/edit/wave-viewer.c ui/edit/wave-viewer.h \
	ui/edit/wire-canvas-item.c ui/edit/wire-canvas-item.h \
	\
	ui/edit/pattern-editor.c ui/edit/pattern-editor.h \
	ui/edit/gtkvumeter.c ui/edit/gtkvumeter.h \
	\
	ui/edit/btmemoryaudiosrc.c ui/edit/btmemoryaudiosrc.h \
	\
	ui/edit/marshal.c ui/edit/marshal.h

ui/edit/marshal.h: ui/edit/marshal.list
	glib-genmarshal --header --prefix=bt_marshal $(srcdir)/$< > $@

ui/edit/marshal.c: ui/edit/marshal.list
	glib-genmarshal --body --prefix=bt_marshal $(srcdir)/$< > $@

# FIXME: don't install bt-edit.compact.gtkrc if not selected
gtkrcdir = $(datadir)/$(PACKAGE)
gtkrc_DATA = ui/edit/bt-edit.gtkrc ui/edit/bt-edit.compact.gtkrc


libbuzztarddec_la_CFLAGS = $(GST_PLUGIN_CFLAGS)
libbuzztarddec_la_LIBADD = \
	$(top_builddir)/src/lib/core/libbuzztard-core.la \
	$(top_builddir)/src/lib/ic/libbuzztard-ic.la \
	$(BASE_DEPS_LIBS) $(LIBM)
libbuzztarddec_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libbuzztarddec_la_SOURCES = ui/dec/bt-dec.c ui/dec/bt-dec.h
libbuzztarddec_la_LIBTOOLFLAGS = --tag=disable-static

# -- common --------------------------------------------------------------------

desktopdir = ${with_desktop_dir}/applications
desktop_in_files = \
  lib/core/songio/bsl/buzztard-songio-buzz.desktop.in \
  ui/edit/buzztard-edit.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@
lib/core/songio/bsl/buzztard-songio-buzz.desktop: lib/core/songio/bsl/buzztard-songio-buzz.desktop.in
ui/edit/buzztard-edit.desktop: ui/edit/buzztard-edit.desktop.in

# install .xml mime info file
sharedmimedir = $(datadir)/mime
sharedmimepackagedir = $(sharedmimedir)/packages
sharedmimepackage_in_files = \
  lib/buzztard.xml.in \
  lib/core/songio/bsl/buzztard-songio-buzz.xml.in
sharedmimepackage_DATA = $(sharedmimepackage_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@
lib/buzztard.xml: lib/buzztard.xml.in
lib/core/songio/bsl/buzztard-songio-buzz.xml: lib/core/songio/bsl/buzztard-songio-buzz.xml.in

# update mime database
install-data-hook:
	test -z "$(UPDATE_DESKTOP_DATABASE)" || $(UPDATE_DESKTOP_DATABASE) "$(DESTDIR)$(desktopdir)";
	test -z "$(UPDATE_MIME_DATABASE)" || $(UPDATE_MIME_DATABASE) "$(DESTDIR)$(sharedmimedir)";
uninstall-local:
	test -z "$(UPDATE_DESKTOP_DATABASE)" || $(UPDATE_DESKTOP_DATABASE) "$(DESTDIR)$(desktopdir)";
	test -z "$(UPDATE_MIME_DATABASE)" || $(UPDATE_MIME_DATABASE) "$(DESTDIR)$(sharedmimedir)";

if USE_GIR
libbtcore_gir_sources=$(patsubst %,$(srcdir)/%, $(libbuzztard_core_la_SOURCES))

BuzztardCore-0.0.gir: $(G_IR_SCANNER) libbuzztard-core.la
	-$(AM_V_GEN)$(G_IR_SCANNER) -v \
	     --namespace=BuzztardCore \
	     --nsversion 0.0 \
	     -I$(builddir) \
	     -I$(srcdir) \
	     -I$(top_srcdir)/src/lib/ic/ \
	     --add-include-path=$(top_srcdir)/src/lib/ic/ \
	     --identifier-prefix=Bt \
	     --symbol-prefix=bt \
	     --c-include='lib/core/core.h' \
	     --include=GLib-2.0 \
	     --include=GObject-2.0 \
	     --include=Gst-@GST_MAJORMINOR@ \
	     --include=libxml2-2.0 \
	     --include=BuzztardIc-0.0 \
	     --library=libbuzztard-core.la \
	     --libtool="$(top_builddir)/libtool" \
	     --pkg glib-2.0 \
	     --pkg gobject-2.0 \
	     --pkg libgstbuzztard \
	     --pkg libxml-2.0 \
	     --pkg gstreamer-@GST_MAJORMINOR@ \
	     --pkg-export libbuzztard-core \
	     --add-init-section="bt_init(NULL,NULL);" \
	     --output $@ \
	     $(libbtcore_gir_sources) || touch $@

libbtic_gir_sources=$(patsubst %,$(srcdir)/%, $(libbuzztard_ic_la_SOURCES))

BuzztardIc-0.0.gir: $(G_IR_SCANNER) libbuzztard-ic.la
	-$(AM_V_GEN)$(G_IR_SCANNER) -v \
	     --namespace=BuzztardIc \
	     --nsversion 0.0 \
	     -I$(builddir) \
	     -I$(srcdir) \
	     --identifier-prefix=BtIc \
	     --symbol-prefix=btic \
	     --c-include='lib/ic/ic.h' \
	     --include=GLib-2.0 \
	     --include=GObject-2.0 \
	     --include=Gst-@GST_MAJORMINOR@ \
	     --library=lib/ic/libbuzztard-ic.la \
	     --libtool="$(top_builddir)/libtool" \
	     --pkg glib-2.0 \
	     --pkg gobject-2.0 \
	     --pkg gstreamer-@GST_MAJORMINOR@ \
	     --pkg-export libbuzztard-ic \
	     --add-init-section="btic_init(NULL,NULL);" \
	     --output $@ \
	     $(libbtic_gir_sources) || touch $@

BUILT_GIRSOURCES = BuzztardCore-0.0.gir BuzztardIc-0.0.gir

girdir = $(datadir)/gir
dist_gir_DATA = $(BUILT_GIRSOURCES)

typelibsdir = $(libdir)/girepository
typelibs_DATA = $(dist_gir_DATA:.gir=.typelib)

TXML = $(dist_gir_DATA:.gir=.gir.txml)

.gir.typelib:
	-$(AM_V_GEN)$(G_IR_COMPILER) --includedir=. $(G_IR_COMPILER_OPTS) $< -o $@  || touch $@
else
BUILT_GIRSOURCES=
endif

EXTRA_DIST = \
  $(desktop_in_files) $(sharedmimepackage_in_files) \
  $(gtkrc_DATA) \
	lib/core/libbuzztard-core.pc.in \
	lib/core/marshal.list \
	lib/core/version.h.in \
	lib/ic/libbuzztard-ic.pc.in \
	lib/ic/version.h.in \
	ui/edit/marshal.list

# CLEANFILES is for files generated by make
CLEANFILES = \
  $(desktop_DATA) $(sharedmimepackage_DATA) \
  $(typelibs_DATA) $(TXML) \
  $(BUILT_GIRSOURCES)

# DISTCLEANFILES is for files generated by configure
DISTCLEANFILES = \
  $(pkgconfig_DATA) \
  lib/core/version.h \
  lib/ic/version.h

include $(top_srcdir)/common.mak
