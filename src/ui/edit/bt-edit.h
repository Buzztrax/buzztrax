/* $Id: bt-edit.h,v 1.20 2004-12-18 14:44:27 ensonic Exp $
 */

#ifndef BT_EDIT_H
#define BT_EDIT_H

#include <stdio.h>

#include <libbtcore/core.h>
//-- gtk+
#include <gtk/gtk.h>
//-- libgnomecanvas
#include <libgnomecanvas/libgnomecanvas.h>

#include "edit-application-methods.h"
#include "machine-canvas-item-methods.h"
#include "machine-properties-dialog-methods.h"
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
#include "settings-dialog-methods.h"
#include "settings-page-audiodevices-methods.h"
#include "tools.h"
#include "wire-canvas-item-methods.h"

//-- misc
#ifndef GST_CAT_DEFAULT
  #define GST_CAT_DEFAULT bt_edit_debug
#endif
#if defined(BT_EDIT) && !defined(BT_EDIT_APPLICATION_C)
	GST_DEBUG_CATEGORY_EXTERN(GST_CAT_DEFAULT);
#endif

//-- gnome canvas has a broken design,
// that does not allow derived classes to have G_PARAM_CONSTRUCT_ONLY properties
#define GNOME_CANVAS_BROKEN_PROPERTIES 1

#endif // BT_EDIT_H
