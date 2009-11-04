/* $Id$
 *
 * Buzztard
 * Copyright (C) 2008 Buzztard team <buzztard-devel@lists.sf.net>
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

#ifndef BT_CORE_PRIVATE_H
#define BT_CORE_PRIVATE_H

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

//-- ansi c
#define __USE_ISOC99 /* for isinf() and co. */
#include <ctype.h>
#include <dirent.h>
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
//-- libxml2
#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/xmlmemory.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
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

#include <libbuzztard-core/core.h>

/*
 * default caps to avoid plugging converters
 */
extern GstCaps *bt_default_caps;

//-- defines for workarounds ---------------------------------------------------

/**
 * XML_CHAR_PTR:
 * @str: the string to cast
 *
 * Cast to xmlChar*
 */
#define XML_CHAR_PTR(str) ((xmlChar *)(str))

#define GST_CAT_DEFAULT bt_core_debug
#ifndef BT_CORE_C
  GST_DEBUG_CATEGORY_EXTERN(GST_CAT_DEFAULT);
#endif

#endif // BT_CORE_PRIVATE_H
