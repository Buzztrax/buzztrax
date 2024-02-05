/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
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
#include "core/core.h"
//-- glib
#include <glib.h>
//-- gtk+
#include <gtk/gtk.h>
#include <adwaita.h>

#include "about-dialog.h"
#include "change-log.h"
#include "change-logger.h"
#include "crash-recover-dialog.h"
#include "edit-application.h"
#include "interaction-controller-menu.h"
#include "machine-canvas-item.h"
#include "machine-menu.h"
#include "machine-actions.h"
#include "machine-list-model.h"
#include "machine-preferences-dialog.h"
#include "machine-rename-dialog.h"
#include "main-menu.h"
#include "main-pages.h"
#include "main-page-machines.h"
#include "main-page-patterns.h"
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
#include "piano-keys.h"
#include "playback-controller-ic.h"
#include "playback-controller-socket.h"
#include "preset-list-model.h"
#include "render-dialog.h"
#include "sequence-grid-model.h"
#include "settings-dialog.h"
#include "settings-page-audiodevices.h"
#include "settings-page-directories.h"
#include "settings-page-interaction-controller.h"
#include "settings-page-playback-controller.h"
#include "settings-page-shortcuts.h"
#include "settings-page-ui.h"
#include "signal-analysis-dialog.h"
#include "tip-dialog.h"
#include "tools.h"
#include "ui-resources.h"
#include "volume-popup.h"
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

/* borders used in hbox/vbox or other containers */
#define BOX_BORDER 6

/* space between a label and a widget in a grid layout */
#define LABEL_PADDING 3

#endif // BT_EDIT_H
