/** $Id: core.h,v 1.2 2004-04-19 13:43:37 ensonic Exp $*/

//#define GST_DEBUG_ENABLED 1
#undef GST_DISABLE_GST_DEBUG

#include <gst/gst.h>
#include <gst/control/control.h>

GST_DEBUG_CATEGORY_STATIC(bt_core_debug);
#define GST_CAT_DEFAULT bt_core_debug

#include "network.h"
#include "pattern.h"
#include "version.h"

