/** $Id: m-bt-core.h,v 1.3 2005-09-13 18:51:00 ensonic Exp $
 */

#include <check.h>
#include <signal.h>
#include <libbtcore/core.h>
#include <libbtcore/application-private.h>
#include "../../bt-check.h"

//-- globals

GST_DEBUG_CATEGORY_EXTERN(bt_core_debug);

//-- prototypes

extern void bt_core_setup(void);
extern void bt_core_teardown(void);
