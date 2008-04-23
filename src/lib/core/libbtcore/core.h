/* $Id$
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
//-- gstbuzztard
#include <libgstbuzztard/musicenums.h>
#include <libgstbuzztard/childbin.h>
#include <libgstbuzztard/propertymeta.h>
#include <libgstbuzztard/tempo.h>

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

//-- prototypes ----------------------------------------------------------------

#ifndef BT_CORE_C
  extern GOptionGroup *bt_init_get_option_group(void);
  extern void bt_init_add_option_groups(GOptionContext * const ctx);
  extern gboolean bt_init_check(int *argc, char **argv[], GError **err);
  extern void bt_init(int *argc, char **argv[]);
#endif

#endif // BT_CORE_H
