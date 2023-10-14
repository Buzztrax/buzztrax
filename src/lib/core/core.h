/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
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
//-- gstreamer
#include <gst/gst.h>

//-- libbtcore
#include "core/childproxy.h"

#include "core/application.h"
#include "core/audio-session.h"
#include "core/cmd-pattern.h"
#include "core/cmd-pattern-control-source.h"
#include "core/experiments.h"
#include "core/machine.h"
#include "core/parameter-group.h"
#include "core/pattern.h"
#include "core/pattern-control-source.h"
#include "core/processor-machine.h"
#include "core/sequence.h"
#include "core/settings.h"
#include "core/setup.h"
#include "core/sink-bin.h"
#include "core/sink-machine.h"
#include "core/song-info.h"
#include "core/song-io.h"
#include "core/song.h"
#include "core/source-machine.h"
#include "core/value-group.h"
#include "core/wave.h"
#include "core/wavelevel.h"
#include "core/wavetable.h"
#include "core/wire.h"

#include "core/tools.h"

//-- prototypes ----------------------------------------------------------------

GOptionGroup *bt_init_get_option_group(void);
void bt_init_add_option_groups(GOptionContext * const ctx);
gboolean bt_init_check(GOptionContext *ctx,  gint *argc, gchar **argv[], GError **err);
void bt_init(GOptionContext * const ctx, gint *argc, gchar **argv[]);
void bt_deinit(void);

void bt_setup_for_local_install();

#ifndef BT_CORE_C
extern const guint bt_major_version;
extern const guint bt_minor_version;
extern const guint bt_micro_version;
#endif

#endif // BT_CORE_H
