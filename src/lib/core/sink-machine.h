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

#ifndef BT_SINK_MACHINE_H
#define BT_SINK_MACHINE_H

#include <glib.h>
#include <glib-object.h>

#include "machine.h"

#define BT_TYPE_SINK_MACHINE            (bt_sink_machine_get_type ())
#define BT_SINK_MACHINE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SINK_MACHINE, BtSinkMachine))
#define BT_SINK_MACHINE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SINK_MACHINE, BtSinkMachineClass))
#define BT_IS_SINK_MACHINE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SINK_MACHINE))
#define BT_IS_SINK_MACHINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SINK_MACHINE))
#define BT_SINK_MACHINE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SINK_MACHINE, BtSinkMachineClass))

/* type macros */

typedef struct _BtSinkMachine BtSinkMachine;
typedef struct _BtSinkMachineClass BtSinkMachineClass;

/**
 * BtSinkMachine:
 *
 * Sub-class of a #BtMachine that implements a signal output
 * (a machine with inputs only).
 */
struct _BtSinkMachine {
  const BtMachine parent;
};

struct _BtSinkMachineClass {
  const BtMachineClass parent;
};

/**
 * BtSinkMachinePatternIndex:
 * @BT_SINK_MACHINE_PATTERN_INDEX_BREAK: stop the pattern
 * @BT_SINK_MACHINE_PATTERN_INDEX_MUTE: mute the machine
 * @BT_SINK_MACHINE_PATTERN_INDEX_OFFSET: offset for real pattern ids
 *
 * Use this with bt_machine_get_pattern_by_index() to get the command patterns.
 */
typedef enum {
  BT_SINK_MACHINE_PATTERN_INDEX_BREAK=0,
  BT_SINK_MACHINE_PATTERN_INDEX_MUTE,
  BT_SINK_MACHINE_PATTERN_INDEX_OFFSET
} BtSinkMachinePatternIndex;

GType bt_sink_machine_get_type(void) G_GNUC_CONST;

BtSinkMachine *bt_sink_machine_new(BtMachineConstructorParams* const params, GError **err);

#endif // BT_SINK_MACHINE_H
