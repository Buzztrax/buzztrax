/** $Id: core.c,v 1.3 2004-04-21 14:38:56 ensonic Exp $
 */

#define BT_CORE
#define BT_CORE_C
#include <libbtcore/core.h>

const unsigned int bt_major_version=BT_MAJOR_VERSION;
const unsigned int bt_minor_version=BT_MINOR_VERSION;
const unsigned int bt_micro_version=BT_MICRO_VERSION;

GST_DEBUG_CATEGORY(GST_CAT_DEFAULT);

void bt_init(int *argc, char ***argv) {
	// init gst
	gst_init(argc,argv);
  gst_control_init(argc,argv);
	GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "bt-core", 0, "music production environment / core library");
}
