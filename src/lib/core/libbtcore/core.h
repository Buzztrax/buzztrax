/* $Id: core.h,v 1.93 2007-11-02 15:29:54 ensonic Exp $
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

// @todo: whats that?!
//#undef GST_DISABLE_GST_DEBUG

#include "config.h"

//-- ansi c
#include <ctype.h>
#include <dirent.h>
#define __USE_ISOC99 /* for isinf() and co. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
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
#include <gst/audio/audio.h>
#include <gst/audio/gstbaseaudiosink.h>
#include <gst/base/gstbasesrc.h>
#include <gst/base/gstbasesink.h>
#include <gst/base/gstbasetransform.h>
#include <gst/controller/gstcontroller.h>

#include <gst/childbin/childbin.h>
#include <gst/propertymeta/propertymeta.h>
#include <gst/tempo/tempo.h>

//-- libxml2
#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/xmlmemory.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
//-- gnome-vfs
#include <libgnomevfs/gnome-vfs.h>
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
#include "wire-pattern-methods.h"
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

//-- misc
#ifdef BT_CORE
  /**
   * GST_CAT_DEFAULT:
   *
   * default loging category. We use gstreamers logging facillities as we use
   * gstreamer anyway. All buzztard logging categories are prefixed with "bt-".
   */
  #define GST_CAT_DEFAULT bt_core_debug
  #ifndef BT_CORE_C
    GST_DEBUG_CATEGORY_EXTERN(GST_CAT_DEFAULT);
  #endif
#endif

#ifndef BT_CORE_C
  extern GOptionGroup *bt_init_get_option_group(void);
  extern void bt_init_add_option_groups(GOptionContext * const ctx);
  extern gboolean bt_init_check(int *argc, char **argv[], GError **err);
  extern void bt_init(int *argc, char **argv[]);
#endif

#endif // BT_CORE_H
