/* $Id: bt-cmd.h,v 1.5 2004-07-29 15:51:31 ensonic Exp $
 */

#ifndef BT_CMD_H
#define BT_CMD_H

#include <stdio.h>

#include <libbtcore/core.h>

#include "cmd-application-methods.h"

//-- misc
#define GST_CAT_DEFAULT bt_cmd_debug
#ifndef BT_CMD_APPLICATION_C
	GST_DEBUG_CATEGORY_EXTERN(GST_CAT_DEFAULT);
#endif

#endif // BT_CMD_H
