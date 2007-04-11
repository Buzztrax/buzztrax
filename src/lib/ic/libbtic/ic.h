/* $Id: ic.h,v 1.7 2007-04-11 18:31:07 ensonic Exp $
 *
 * Buzztard
 * Copyright (C) 2007 Buzztard team <buzztard-devel@lists.sf.net>
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

#ifndef BTIC_CORE_H
#define BTIC_CORE_H

#include "config.h"

//-- ansi c
#define __USE_ISOC99 /* for isinf() and co. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#define _XOPEN_SOURCE /* glibc2 needs this */
#define __USE_XOPEN
#include <time.h>
#include <unistd.h>
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

//-- hal/dbus
#ifdef USE_HAL
#include <glib.h>
#include <hal/libhal.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#endif

//-- libbtcore
#include "libbtcore/core.h"

//-- libbtic
// method prototype includes do include the data defs themself

#include "control-methods.h"
#include "abs-range-control-methods.h"
#include "trigger-control-methods.h"
#include "device-methods.h"
#include "input-device-methods.h"
#include "midi-device-methods.h"
#include "registry-methods.h"

#include "version.h"

//-- global defines ------------------------------------------------------------

//-- misc
#ifdef BTIC_CORE
  /**
   * GST_CAT_DEFAULT:
   *
   * default loging category. We use gstreamers logging facillities as we use
   * gstreamer anyway. All buzztard logging categories are prefixed with "bt-".
   */
  #define GST_CAT_DEFAULT bt_ic_debug
  #ifndef BTIC_CORE_C
    GST_DEBUG_CATEGORY_EXTERN(GST_CAT_DEFAULT);
  #endif
#endif


#ifndef BTIC_CORE_C
  extern GOptionGroup *btic_init_get_option_group(void);
  extern gboolean btic_init_check(int *argc, char **argv[], GError **err);
  extern void btic_init(int *argc, char **argv[]);
#endif

#endif // BTIC_CORE_H
