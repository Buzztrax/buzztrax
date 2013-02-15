/* Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BT_EDIT_H
#define BT_EDIT_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

//-- ansi c
#include <stdlib.h>
#include <string.h>
//-- i18n
#ifndef _
#ifdef ENABLE_NLS
  #include <langinfo.h>
  #include <libintl.h>

  #define _(String) gettext(String)
  #ifdef gettext_noop
    #define N_(String) gettext_noop(String)
  #else
    #define N_(String) (String)
  #endif
#else /* NLS is disabled */
  #define _(String) (String)
  #define __(String) (String)
  #define N_(String) (String)
  #ifdef gettext
    #undef gettext
  #endif
  #define gettext(String) (String)
  #ifdef dgettext
    #undef dgettext
  #endif
  #define dgettext(Domain,String) (String)
  #define textdomain(Domain)
  #define bindtextdomain(Package, Directory)
#endif
#endif
//-- libbtcore & libbtic
#include "core.h"
#include "ic.h"
//-- gstbuzztard
#include <libgstbuzztard/musicenums.h>
#include <libgstbuzztard/toneconversion.h>
//-- glib
#include <glib.h>
#include <glib/gstdio.h>
//-- gtk+
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
//-- libgnomecanvas
/* glib macro wrapper aliases - they have been deprecated
 * this is fixed in gnomecanvas 2.26
 */
#ifndef GTK_CHECK_CAST
#define GTK_CHECK_CAST		G_TYPE_CHECK_INSTANCE_CAST
#define GTK_CHECK_CLASS_CAST	G_TYPE_CHECK_CLASS_CAST
#define GTK_CHECK_GET_CLASS	G_TYPE_INSTANCE_GET_CLASS
#define GTK_CHECK_TYPE		G_TYPE_CHECK_INSTANCE_TYPE
#define GTK_CHECK_CLASS_TYPE	G_TYPE_CHECK_CLASS_TYPE
#endif
#include <libgnomecanvas/libgnomecanvas.h>

//-- librsvg
//#include <librsvg/rsvg.h>

#include "about-dialog.h"
#include "change-log.h"
#include "change-logger.h"
#include "crash-recover-dialog.h"
#include "edit-application.h"
#include "interaction-controller-menu.h"
#include "interaction-controller-learn-dialog.h"
#include "machine-canvas-item.h"
#include "machine-menu.h"
#include "machine-actions.h"
#include "machine-list-model.h"
#include "machine-preset-properties-dialog.h"
#include "machine-properties-dialog.h"
#include "machine-preferences-dialog.h"
#include "machine-rename-dialog.h"
#include "main-menu.h"
#include "main-pages.h"
#include "main-page-machines.h"
#include "main-page-patterns.h"
#include "main-page-sequence.h"
#include "main-page-waves.h"
#include "main-page-info.h"
#include "main-statusbar.h"
#include "main-toolbar.h"
#include "main-window.h"
#include "missing-framework-elements-dialog.h"
#include "missing-song-elements-dialog.h"
#include "object-list-model.h"
#include "panorama-popup.h"
#include "pattern-list-model.h"
#include "pattern-properties-dialog.h"
#include "pattern-editor.h"
#include "playback-controller-socket.h"
#include "render-dialog.h"
#include "sequence-grid-model.h"
#include "sequence-view.h"
#include "settings-dialog.h"
#include "settings-page-audiodevices.h"
#include "settings-page-directories.h"
#include "settings-page-interaction-controller.h"
#include "settings-page-playback-controller.h"
#include "settings-page-shortcuts.h"
#include "signal-analysis-dialog.h"
#include "tip-dialog.h"
#include "tools.h"
#include "ui-resources.h"
#include "volume-popup.h"
#include "wave-list-model.h"
#include "waveform-viewer.h"
#include "wavelevel-list-model.h"
#include "wire-canvas-item.h"

//-- misc
#ifndef GST_CAT_DEFAULT
  #define GST_CAT_DEFAULT bt_edit_debug
#endif
#if defined(BT_EDIT) && !defined(BT_EDIT_APPLICATION_C)
  GST_DEBUG_CATEGORY_EXTERN(GST_CAT_DEFAULT);
#endif

/**
 * GNOME_CANVAS_BROKEN_PROPERTIES:
 *
 * gnome canvas has a broken design,
 * it does not allow derived classes to have G_PARAM_CONSTRUCT_ONLY properties
 */
#define GNOME_CANVAS_BROKEN_PROPERTIES 1

/*
 * lets hope that 64 gives enough space for window-decoration + panels
 * TODO(ensonic): look at http://standards.freedesktop.org/wm-spec/1.3/ar01s05.html#id2523368
 * search for _NET_WM_STRUT_PARTIAL
 */
#define SCREEN_BORDER_HEIGHT 80

/* borders used in hbox/vbox or other containers */
#ifndef USE_COMPACT_UI
#define BOX_BORDER 6
#else
#define BOX_BORDER 3
#endif

#endif // BT_EDIT_H
