/** $Id: m-bt-cmd.h,v 1.2 2005-09-13 18:51:00 ensonic Exp $
 */

#include <check.h>
#include "../../bt-check.h"
#include "../../../src/ui/cmd/bt-cmd.h"

//-- globals

GST_DEBUG_CATEGORY_EXTERN(bt_core_debug);
GST_DEBUG_CATEGORY_EXTERN(bt_cmd_debug);

//-- prototypes

extern void bt_cmd_setup(void);
extern void bt_cmd_teardown(void);
