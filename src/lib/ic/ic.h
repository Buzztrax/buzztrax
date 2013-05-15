/* Buzztrax
 * Copyright (C) 2007 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#ifndef BT_IC_H
#define BT_IC_H

//-- glib/gobject
#include <glib.h>
#include <glib-object.h>
#include <glib/gprintf.h>
//-- gstreamer
#include <gst/gst.h>

//-- libbtic
#include "control.h"
#include "abs-range-control.h"
#include "trigger-control.h"
#include "device.h"
#include "registry.h"
#include "learn.h"

//-- prototypes ----------------------------------------------------------------

GOptionGroup *btic_init_get_option_group(void);
gboolean btic_init_check(gint *argc, gchar **argv[], GError **err);
void btic_init(gint *argc, gchar **argv[]);

#ifndef BTIC_CORE_C
extern const guint btic_major_version;
extern const guint btic_minor_version;
extern const guint btic_micro_version;
#endif

#endif // BT_IC_H
