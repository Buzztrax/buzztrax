/* $Id: core.h,v 1.19 2004-05-13 09:35:29 ensonic Exp $
 */

#ifndef BT_CORE_H
#define BT_CORE_H

#undef GST_DISABLE_GST_DEBUG

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
#include "sequence-methods.h"
#include "song-io-methods.h"
#include "song-io-native.h"
#include "application-methods.h"
#include "version.h"

//-- global defines ------------------------------------------------------------

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
	extern void bt_init(int *argc, char ***argv);
#endif

#endif // BT_CORE_H

