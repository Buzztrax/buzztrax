/* $Id: bt-edit.h,v 1.12 2004-10-11 16:19:15 ensonic Exp $
 */

#ifndef BT_EDIT_H
#define BT_EDIT_H

#include <stdio.h>

#include <libbtcore/core.h>
//-- gtk+
#include <gtk/gtk.h>
//-- libgnomecanvas
#include <libgnomecanvas/libgnomecanvas.h>

#include "tools.h"
#include "settings-dialog.h"
#include "main-menu-methods.h"
#include "main-toolbar-methods.h"
#include "main-pages-methods.h"
#include "main-page-machines-methods.h"
#include "main-page-patterns-methods.h"
#include "main-page-sequence-methods.h"
#include "main-page-info-methods.h"
#include "main-statusbar-methods.h"
#include "main-window-methods.h"
#include "edit-application-methods.h"

//-- misc
#ifndef GST_CAT_DEFAULT
  #define GST_CAT_DEFAULT bt_edit_debug
#endif
#if defined(BT_EDIT) && !defined(BT_EDIT_APPLICATION_C)
	GST_DEBUG_CATEGORY_EXTERN(GST_CAT_DEFAULT);
#endif

#endif // BT_EDIT_H
