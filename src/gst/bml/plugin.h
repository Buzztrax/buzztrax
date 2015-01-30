/* GStreamer
 * Copyright (C) 2004 Stefan Kost <ensonic at users.sf.net>
 *
 * This library is free software; you can redigst_bml_instantiatestribute it and/or
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
/* < private_header > */

#ifndef __GST_BML_PLUGIN_H__
#define __GST_BML_PLUGIN_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
//#ifdef USE_DEBUG
#define _ISOC99_SOURCE /* for isinf() and co. */
//#endif
#include <string.h>
#include <strings.h>
#include <math.h>
//-- glib/gobject
#include <glib.h>
#include <glib-object.h>
#include <glib/gstdio.h>
//-- gstreamer
#include <gst/gst.h>
#include <gst/gstchildproxy.h>
#include <gst/base/gstbasesrc.h>
#include <gst/base/gstbasetransform.h>
#include <gst/audio/audio.h>
//-- gstbuzztrax
#include "gst/childbin.h"
#include "gst/musicenums.h"
#include "gst/toneconversion.h"
#include "gst/propertymeta.h"
#include "gst/tempo.h"
//-- orc
#ifdef HAVE_ORC
#include <orc/orcfunctions.h>
#else
#define orc_memset memset
#endif


//-- libbml
#include "bml/bml.h"

#ifdef BML_WRAPPED
#define bml(a) bmlw_ ## a
#define BML(a) BMLW_ ## a
#else
#ifdef BML_NATIVE
#define bml(a) bmln_ ## a
#define BML(a) BMLN_ ## a
#else
#define bml(a) a
#define BML(a) a
#endif
#endif

#include "gstbml.h"
#include "gstbmlsrc.h"
#include "gstbmltransform.h"
#include "gstbmlv.h"
#include "common.h"          /* gstbml sdk utility functions */

#ifndef BML_VERSION
#define BML_VERSION "1.0"
#endif

#endif /* __GST_BML_PLUGIN_H_ */
