/* $Id: bt-edit.h,v 1.1 2004-07-28 14:59:10 ensonic Exp $
 */

#ifndef BT_EDIT_H
#define BT_EDIT_H

#include <stdio.h>

#include <libbtcore/core.h>
#include <glib.h>
#include <glib-object.h>

//#include "edit-application-methods.h"

//-- misc
#define GST_CAT_DEFAULT bt_edit_debug
#ifndef BT_EDIT_APPLICATION_C
	GST_DEBUG_CATEGORY_EXTERN(GST_CAT_DEFAULT);
#endif

#endif // BT_EDIT_H
