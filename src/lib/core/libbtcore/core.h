/* $Id: core.h,v 1.25 2004-07-28 13:25:21 ensonic Exp $
 */

#ifndef BT_CORE_H
#define BT_CORE_H

#undef GST_DISABLE_GST_DEBUG

#include "config.h"

//-- glib/gobject
#include <glib.h>
#include <glib-object.h>
//-- gstreamer
#include <gst/gst.h>
#include <gst/control/control.h>
//-- libxml2
#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/xmlmemory.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
//-- popt
#include <popt.h>

//-- libbtcore
// method prototype includes do include the data defs themself)
#include "song-methods.h"
#include "song-info-methods.h"
#include "machine-methods.h"
#include "processor-machine.h"
#include "sink-machine.h"
#include "source-machine.h"
#include "wire-methods.h"
#include "setup-methods.h"
#include "pattern-methods.h"
#include "timelinetrack-methods.h"
#include "timeline-methods.h"
#include "playline-methods.h"
#include "sequence-methods.h"
#include "song-io-methods.h"
#include "song-io-native.h"
#include "application-methods.h"
#include "tools.h"
#include "version.h"

//-- global defines ------------------------------------------------------------

// XML related
/**
 * BT_NS_PREFIX:
 *
 * default buzztard xml namespace prefix
 */
#define BT_NS_PREFIX "bt"
/**
 * BT_NS_URL:
 *
 * default buzztard xml namespace url
 */
#define BT_NS_URL    "http://buzztard.sourceforge.net/"

// handy shortcuts that improve readabillity

//#define BT_SONG_GET_BIN GST_BIN(bt_g_object_get_object_property(G_OBJECT(self-private->song),"bin"))

//-- misc
#ifdef BT_CORE
	/**
	 * GST_CAT_DEFAULT:
	 *
	 * default loging category. We use gstreamers logging facillities as we use
	 * gstreamer anyway. All buzztard logging categories are prefixed with "bt_".
	 */
	#define GST_CAT_DEFAULT bt_core_debug
	#ifndef BT_CORE_C
		GST_DEBUG_CATEGORY_EXTERN(GST_CAT_DEFAULT);
	#endif
#endif

/**
 * return_if_disposed:
 * @a: return value or nothing
 *
 * checks <code>self->private->dispose_has_run</code> and
 * if true returns with the supplied arg.
 * This macro is handy to use at the start of all class routines
 * such as _get_property(), _set_property(), _dispose().
 */
#define return_if_disposed(a) if(self->private->dispose_has_run) return a

#ifndef BT_CORE_C
	extern void bt_init(int *argc, char ***argv, struct poptOption *options);
#endif

#endif // BT_CORE_H

