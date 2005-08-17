/** $Id: m-bt-edit.h,v 1.2 2005-08-17 16:58:56 ensonic Exp $
 */

#include <check.h>
#include "../../bt-check.h"
#include "../../../src/ui/edit/bt-edit.h"

//-- globals

GST_DEBUG_CATEGORY_EXTERN(bt_core_debug);
GST_DEBUG_CATEGORY_EXTERN(bt_edit_debug);

//-- prototypes

extern void bt_edit_setup(void);
extern void bt_edit_teardown(void);
