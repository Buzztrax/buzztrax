/** $Id: core.c,v 1.2 2004-04-19 17:19:24 ensonic Exp $
 */

#define BT_CORE
#define BT_CORE_C
#include <libbtcore/core.h>

const unsigned int bt_major_version;
const unsigned int bt_minor_version;
const unsigned int bt_micro_version;

void bt_init(int *argc, char ***argv) {
	// init gst
	gst_init(argc,argv);
  gst_control_init(argc,argv);
	GST_DEBUG_CATEGORY_INIT(bt_core_debug, "buzztard", 0, "music production environment");	
}
