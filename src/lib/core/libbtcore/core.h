/* $Id: core.h,v 1.41 2004-09-29 16:56:46 ensonic Exp $
 */

#ifndef BT_CORE_H
#define BT_CORE_H

#undef GST_DISABLE_GST_DEBUG

#include "config.h"

//-- ansi c
#include <dirent.h>
//-- locale
#ifdef HAVE_X11_XLOCALE_H
	/* defines a more portable setlocale for X11 (_Xsetlocale) */
	#include <X11/Xlocale.h>
#else
	#include <locale.h>
#endif
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
//-- i18n
#ifdef ENABLE_NLS
	#include <langinfo.h>
	#include <libintl.h>

	#define _(String) dgettext(PACKAGE,String)
	#ifdef GITK_LIB_C
		#define __(String) dgettext(client_package_name,String)
	#endif
	#ifdef gettext_noop
		#define N_(String) gettext_noop(String)
	#else
		#define N_(String) (String)
	#endif
#else /* NLS is disabled */
	#define _(String) (String)
	#define __(String) (String)
	#define N_(String) (String)
	#ifdef gettext
		#undef gettext
	#endif
	#define gettext(String) (String)
	#ifdef dgettext
		#undef dgettext
	#endif
	#define dgettext(Domain,String) (String)
	#define textdomain(Domain)
	#define bindtextdomain(Package, Directory)
#endif
//-- gconf
#ifdef USE_GCONF
#include <gconf/gconf-client.h>
#endif

//-- libbtcore
// method prototype includes do include the data defs themself
#include "song-methods.h"
#include "song-info-methods.h"
#include "machine-methods.h"
#include "processor-machine-methods.h"
#include "sink-machine-methods.h"
#include "source-machine-methods.h"
#include "wire-methods.h"
#include "setup-methods.h"
#include "pattern-methods.h"
#include "timelinetrack-methods.h"
#include "timeline-methods.h"
#include "playline-methods.h"
#include "sequence-methods.h"
#include "song-io-methods.h"
#include "song-io-native-methods.h"
#include "settings-methods.h"
#include "plainfile-settings-methods.h"
#include "application-methods.h"
#include "tools.h"
#include "version.h"

#ifdef USE_GCONF
#include "gconf-settings-methods.h"
#endif

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
 * checks <code>self->priv->dispose_has_run</code> and
 * if true returns with the supplied arg.
 * This macro is handy to use at the start of all class routines
 * such as _get_property(), _set_property(), _dispose().
 */
#define return_if_disposed(a) if(self->priv->dispose_has_run) return a

/**
 * is_string:
 * @a: string pointer
 *
 * checks if the supplied string pointer is not NULL and contains not just '\0'
 */
#define is_string(a) (a && *a)

/**
 * safe_string:
 * @a: string pointer
 *
 * passed the supplied string through or return an empty string when it is NULL
 */
#define safe_string(a) ((a)?a:"")

/**
 * g_object_try_ref:
 * @obj: the object to reference
 *
 * If the supplied object is not NULL then reference it via
 * g_object_ref().
 *
 * Return: the referenced object or NULL
 */
#define g_object_try_ref(obj) (obj)?g_object_ref(obj):NULL

/**
 * g_object_try_unref:
 * @obj: the object to release the reference
 *
 * If the supplied object is not NULL then release the reference via
 * g_object_unref().
 */
#define g_object_try_unref(obj) if(obj) g_object_unref(obj)

/**
 * g_object_try_weak_ref:
 * @obj: the object to reference
 *
 * If the supplied object is not NULL then reference it via
 * g_object_add_weak_pointer().
 */
#define g_object_try_weak_ref(obj) if(obj) g_object_add_weak_pointer(G_OBJECT(obj),(gpointer *)&obj);

/**
 * g_object_try_weak_unref:
 * @obj: the object to release the reference
 *
 * If the supplied object is not NULL then release the reference via
 * g_object_remove_weak_pointer().
 */
#define g_object_try_weak_unref(obj) if(obj) g_object_remove_weak_pointer(G_OBJECT(obj),(gpointer *)&obj);

#ifndef BT_CORE_C
	extern void bt_init(int *argc, char ***argv, struct poptOption *options);
#endif

#endif // BT_CORE_H

