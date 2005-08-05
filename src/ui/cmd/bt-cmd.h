/* $Id: bt-cmd.h,v 1.7 2005-08-05 09:36:17 ensonic Exp $
 */

#ifndef BT_CMD_H
#define BT_CMD_H

#include <stdio.h>

#include <libbtcore/core.h>

#include "cmd-application-methods.h"

//-- misc
#ifndef GST_CAT_DEFAULT
  #define GST_CAT_DEFAULT bt_cmd_debug
#endif
#if defined(BT_CMD) && !defined(BT_CMD_APPLICATION_C)
  GST_DEBUG_CATEGORY_EXTERN(GST_CAT_DEFAULT);
#endif

#endif // BT_CMD_H
