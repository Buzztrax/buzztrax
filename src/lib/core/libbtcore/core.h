/** $Id: core.h,v 1.10 2004-05-04 15:24:59 ensonic Exp $
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

//#include "song.h"
// contains only method prototypes and should be included AFTER song.h,
// the same for all other classes

#include "song-methods.h"
#include "machine-methods.h"
#include "wire-methods.h"
#include "version.h"

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

