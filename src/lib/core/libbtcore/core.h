/* $Id: core.h,v 1.81 2006-08-24 20:00:52 ensonic Exp $
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef BT_CORE_H
#define BT_CORE_H

#undef GST_DISABLE_GST_DEBUG

#include "config.h"

//-- ansi c
#include <dirent.h>
#define __USE_ISOC99 /* for isinf() and co. */
#include <math.h>
#include <stdio.h>
#include <string.h>
//#define _XOPEN_SOURCE /* glibc2 needs this */
#define __USE_XOPEN
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/resource.h>
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
#include <glib/gprintf.h>
//-- gstreamer
#include <gst/gst.h>
#include <gst/controller/gstcontroller.h>

#include <gst/childbin/childbin.h>
#include <gst/propertymeta/propertymeta.h>
#include <gst/tempo/tempo.h>

#include <gst/base/gstbasesink.h>
#include <gst/base/gstbasetransform.h>
//-- libxml2
#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/xmlmemory.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
//-- gnome-vfs
#include <libgnomevfs/gnome-vfs.h>
//-- popt
#include <popt.h>
//-- i18n
#ifndef _
#ifdef ENABLE_NLS
  #include <langinfo.h>
  #include <libintl.h>

  #define _(String) gettext(String)
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
#endif
//-- gconf
#ifdef USE_GCONF
#include <gconf/gconf-client.h>
#endif

//-- libbtcore
// method prototype includes do include the data defs themself
#include "persistence-methods.h"

#include "song-methods.h"
#include "song-info-methods.h"
#include "machine-methods.h"
#include "processor-machine-methods.h"
#include "sink-machine-methods.h"
#include "source-machine-methods.h"
#include "sink-bin-methods.h"
#include "wire-methods.h"
#include "setup-methods.h"
#include "pattern-methods.h"
#include "sequence-methods.h"
#include "song-io-methods.h"
#include "song-io-native-methods.h"
#include "settings-methods.h"
#include "plainfile-settings-methods.h"
#include "application-methods.h"
#include "wave-methods.h"
#include "wavelevel-methods.h"
#include "wavetable-methods.h"

#include "marshal.h"
#include "tools.h"
#include "version.h"

#ifdef USE_GCONF
#include "gconf-settings-methods.h"
#endif

//-- defines for workarounds ---------------------------------------------------

/**
 * G_ABS_STRUCT_OFFSET:
 * @struct_type: a structure type, e.g. GtkWidget.
 * @member: a field in the structure, e.g. window.
 * 
 * Returns the offset, in bytes, of a member of a struct.
 *
 * Returns: the offset of member from the start of struct_type.
 */  
#define G_ABS_STRUCT_OFFSET(struct_type, member)    \
    ((guint) ((guint8*) &((struct_type*) 0)->member))
  
/**
 * XML_CHAR_PTR:
 * @str: the string to cast
 *
 * Cast to xmlChar*
 */
#define XML_CHAR_PTR(str) ((xmlChar *)(str))

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
#define BT_NS_URL "http://www.buzztard.org/"

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
#define return_if_disposed() if(self->priv->dispose_has_run) return 

/**
 * G_STRUCT_SIZE:
 * @s: struct name
 *
 * Applies the sizeof operator to a struct and casts the result for use in
 * GTypeInfo definitions.
 */
#define G_STRUCT_SIZE(s) (guint16)sizeof(s)

/**
 * BT_IS_STRING:
 * @a: string pointer
 *
 * checks if the supplied string pointer is not %NULL and contains not just '\0'
 */
#define BT_IS_STRING(a) (a && *a)

/**
 * safe_string:
 * @a: string pointer
 *
 * passed the supplied string through or return an empty string when it is NULL
 *
 * Returns: the given string or an empty string in the case of a NULL argument
 */
#define safe_string(a) ((gchar *)(a)?(gchar *)(a):"")

/**
 * g_object_try_ref:
 * @obj: the object to reference
 *
 * If the supplied object is not %NULL then reference it via
 * g_object_ref().
 *
 * Returns: the referenced object or %NULL
 */
#define g_object_try_ref(obj) (obj)?g_object_ref(obj):NULL

/**
 * g_object_try_unref:
 * @obj: the object to release the reference
 *
 * If the supplied object is not %NULL then release the reference via
 * g_object_unref().
 */
#define g_object_try_unref(obj) if(obj) g_object_unref(obj)


/*
GCC 4.1 introduced this crazy warning that complains about casting between
different pointer types. The question is why this includes void* ?
Sadly they don't gave tips how they belive to get rid of the warning.

#define bt_type_pun_to_gpointer(val) \
  (((union { gpointer __a; __typeof__((val)) __b; }){__b:(val)}).__a)
*/

/**
 * g_object_try_weak_ref:
 * @obj: the object to reference
 *
 * If the supplied object is not %NULL then reference it via
 * g_object_add_weak_pointer().
 */
#define g_object_try_weak_ref(obj) if(obj) g_object_add_weak_pointer(G_OBJECT(obj),(gpointer *)&obj##_ptr);
/*
#define g_object_try_weak_ref(obj) \
  if(obj) { \
    gpointer *ptr=&bt_type_pun_to_gpointer(obj); \
    GST_DEBUG("  reffing : %p",ptr); \
    g_object_add_weak_pointer(G_OBJECT(obj),ptr); \
  }
*/

/**
 * g_object_try_weak_unref:
 * @obj: the object to release the reference
 *
 * If the supplied object is not %NULL then release the reference via
 * g_object_remove_weak_pointer().
 */
#define g_object_try_weak_unref(obj) if(obj) g_object_remove_weak_pointer(G_OBJECT(obj),(gpointer *)&obj##_ptr);
/*
#define g_object_try_weak_unref(obj) \
  if(obj) { \
    gpointer *ptr=&bt_type_pun_to_gpointer(obj); \
    GST_DEBUG("  unreffing : %p",ptr); \
    g_object_remove_weak_pointer(G_OBJECT(obj),(gpointer *)&bt_type_pun_to_gpointer(obj)); \
  }
*/

/*
@idea g_alloca_printf
	
#define g_alloca_printf(str,format,...) \
sprintf((str=alloca(g_printf_string_upper_bound(format, args)),format, args)
*/

// workaround for glib<2.8
#ifndef GST_HAVE_GLIB_2_8
#define G_OPTION_FLAG_NO_ARG 0
#endif

#ifndef BT_CORE_C
  extern GOptionGroup *bt_init_get_option_group(void);
  extern void bt_init_add_option_groups(GOptionContext *ctx);
  extern gboolean bt_init_check(int *argc, char **argv[], GError **err);
  extern void bt_init(int *argc, char **argv[]);
#endif

#endif // BT_CORE_H
