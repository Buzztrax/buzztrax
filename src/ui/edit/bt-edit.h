/* $Id: bt-edit.h,v 1.4 2004-08-06 19:42:45 ensonic Exp $
 */

#ifndef BT_EDIT_H
#define BT_EDIT_H

#include <stdio.h>

#include <libbtcore/core.h>
//-- gtk+
#include <gtk/gtk.h>

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
