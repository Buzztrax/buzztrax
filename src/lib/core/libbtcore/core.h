/** $Id: core.h,v 1.4 2004-04-19 18:30:42 ensonic Exp $*/

#ifndef BT_CORE_H
#define BT_CORE_H

//#define GST_DEBUG_ENABLED 1
#undef GST_DISABLE_GST_DEBUG

#include <glib.h>
#include <gst/gst.h>
#include <gst/control/control.h>

GST_DEBUG_CATEGORY_STATIC(bt_core_debug);
#define GST_CAT_DEFAULT bt_core_debug

#include "song.h"
#include "network.h"
#include "pattern.h"
#include "version.h"

#ifndef BT_CORE_C
	extern void bt_init(int *argc, char ***argv);
#endif

#endif /* BT_CORE_H */
