/* Buzztard
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
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
#include <gst/audio/gstbaseaudiosrc.h>
#include <gst/base/gstbasesrc.h>
#include <gst/base/gstbasesink.h>
#include <gst/base/gstbasetransform.h>
#include <gst/controller/gstcontroller.h>
#include <gst/pbutils/pbutils.h>
#include <gst/pbutils/missing-plugins.h>
// needs 0.10.22
//#include <gst/app/gstappsrc.h>
//#include <gst/app/gstappbuffer.h>
//-- gstbuzztard
#include <libgstbuzztard/musicenums.h>
#include <libgstbuzztard/childbin.h>
#include <libgstbuzztard/propertymeta.h>
#include <libgstbuzztard/tempo.h>

//-- libbtcore
#include "childproxy.h"
#include "persistence.h"

#include "application.h"
#include "audio-session.h"
#include "cmd-pattern.h"
#include "gconf-settings.h"
#include "machine.h"
#include "parameter-group.h"
#include "pattern.h"
#include "pattern-control-source.h"
#include "processor-machine.h"
#include "sequence.h"
#include "settings.h"
#include "setup.h"
#include "sink-bin.h"
#include "sink-machine.h"
#include "song-info.h"
#include "song-io-native-bzt.h"
#include "song-io-native-xml.h"
#include "song-io-native.h"
#include "song-io.h"
#include "song.h"
#include "source-machine.h"
#include "value-group.h"
#include "wave.h"
#include "wavelevel.h"
#include "wavetable.h"
#include "wire.h"

#include "tools.h"

//-- prototypes ----------------------------------------------------------------

GOptionGroup *bt_init_get_option_group(void);
void bt_init_add_option_groups(GOptionContext * const ctx);
gboolean bt_init_check(gint *argc, gchar **argv[], GError **err);
void bt_init(gint *argc, gchar **argv[]);
void bt_deinit(void);

#ifndef BT_CORE_C
extern const guint bt_major_version;
extern const guint bt_minor_version;
extern const guint bt_micro_version;
#endif

#endif // BT_CORE_H
