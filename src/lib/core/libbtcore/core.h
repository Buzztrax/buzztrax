/** $Id: core.h,v 1.13 2004-05-06 18:26:58 ensonic Exp $
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
// contain only method prototypes (will include the data defs themself)
#include "song-methods.h"
#include "song-info-methods.h"
#include "machine-methods.h"
#include "wire-methods.h"
#include "setup-methods.h"
#include "sequence-methods.h"
#include "song-io-methods.h"
#include "version.h"

//-- global defines ------------------------------------------------------------

/** @brief default buzztard xml namespace prefix */
#define BUZZTARD_NS_PREFIX "bt"
/** @brief default buzztard xml namespace url */
#define BUZZTARD_NS_URL    "http://buzztard.sourceforge.net/"

//-- misc
#ifdef BT_CORE
	#define GST_CAT_DEFAULT bt_core_debug
	#ifndef BT_CORE_C
		GST_DEBUG_CATEGORY_EXTERN(GST_CAT_DEFAULT);
		#define return_if_disposed(a) if(self->private->dispose_has_run) return a
	#endif
#endif

#ifndef BT_CORE_C
	extern void bt_init(int *argc, char ***argv);
#endif

#endif // BT_CORE_H

