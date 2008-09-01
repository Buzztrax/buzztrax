/* $Id$
 *
 * Buzztard
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef BT_EDIT_H
#define BT_EDIT_H

//-- ansi c
//#define __USE_ISOC99 /* for round() and co. */
#include <math.h>
#include <stdio.h>
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
#include <libbtcore/core.h>
#include <libbtic/ic.h>
//-- gstreamer
//-- gstbuzztard
#include <libgstbuzztard/help.h>
#include <libgstbuzztard/musicenums.h>
#include <libgstbuzztard/note2frequency.h>
#if !GST_CHECK_VERSION(0,10,20)
#include <libgstbuzztard/preset.h>
#endif
//-- gtk+
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
//-- libgnomecanvas
/* glib macro wrappers (compatibility) - they have been deprecated */
#ifndef GTK_CHECK_CAST
#define GTK_CHECK_CAST		G_TYPE_CHECK_INSTANCE_CAST
#define GTK_CHECK_CLASS_CAST	G_TYPE_CHECK_CLASS_CAST
#define GTK_CHECK_GET_CLASS	G_TYPE_INSTANCE_GET_CLASS
#define GTK_CHECK_TYPE		G_TYPE_CHECK_INSTANCE_TYPE
#define GTK_CHECK_CLASS_TYPE	G_TYPE_CHECK_CLASS_TYPE
#include <libgnomecanvas/libgnomecanvas.h>
#undef GTK_CHECK_CAST
#undef GTK_CHECK_CLASS_CAST
#undef GTK_CHECK_GET_CLASS
#undef GTK_CHECK_TYPE
#undef GTK_CHECK_CLASS_TYPE
#else
#include <libgnomecanvas/libgnomecanvas.h>
#endif

//-- librsvg
#include <librsvg/rsvg.h>
//-- libgnome
#ifdef USE_GNOME
#include <libgnome/gnome-init.h>
#include <libgnome/gnome-help.h>
#include <libgnome/gnome-url.h>
#endif
//-- gnome-vfs
#include <libgnomevfs/gnome-vfs.h>
//-- hildon
#ifdef USE_HILDON
#include <hildon-widgets/hildon-program.h>
#endif

#include "about-dialog-methods.h"
#include "edit-application-methods.h"
#include "interaction-controller-menu-methods.h"
#include "interaction-controller-learn-dialog-methods.h"
#include "machine-canvas-item-methods.h"
#include "machine-menu-methods.h"
#include "machine-actions.h"
#include "machine-preset-properties-dialog-methods.h"
#include "machine-properties-dialog-methods.h"
#include "machine-preferences-dialog-methods.h"
#include "machine-rename-dialog-methods.h"
#include "main-menu-methods.h"
#include "main-pages-methods.h"
#include "main-page-machines-methods.h"
#include "main-page-patterns-methods.h"
#include "main-page-sequence-methods.h"
#include "main-page-waves-methods.h"
#include "main-page-info-methods.h"
#include "main-statusbar-methods.h"
#include "main-toolbar-methods.h"
#include "main-window-methods.h"
#include "missing-framework-elements-dialog-methods.h"
#include "missing-song-elements-dialog-methods.h"
#include "panorama-popup.h"
#include "pattern-properties-dialog-methods.h"
#include "pattern-editor.h"
#include "playback-controller-socket-methods.h"
#include "render-dialog-methods.h"
#include "render-progress-methods.h"
#include "sequence-view-methods.h"
#include "settings-dialog-methods.h"
#include "settings-page-audiodevices-methods.h"
#include "settings-page-directories-methods.h"
#include "settings-page-interaction-controller-methods.h"
#include "settings-page-playback-controller-methods.h"
#include "tools.h"
#include "ui-resources-methods.h"
#include "volume-popup.h"
#include "wave-viewer.h"
#include "wire-analysis-dialog-methods.h"
#include "wire-canvas-item-methods.h"

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

#endif // BT_EDIT_H
