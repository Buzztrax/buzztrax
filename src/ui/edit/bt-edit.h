/* $Id: bt-edit.h,v 1.3 2004-07-29 15:51:31 ensonic Exp $
 */

#ifndef BT_EDIT_H
#define BT_EDIT_H

#include <stdio.h>

#include <libbtcore/core.h>
//-- gtk+
#include <gtk/gtk.h>

#include "edit-application-methods.h"

//-- misc
#define GST_CAT_DEFAULT bt_edit_debug
#ifndef BT_EDIT_APPLICATION_C
	GST_DEBUG_CATEGORY_EXTERN(GST_CAT_DEFAULT);
#endif

#endif // BT_EDIT_H
