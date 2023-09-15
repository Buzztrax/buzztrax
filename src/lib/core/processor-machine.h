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

#ifndef BT_PROCESSOR_MACHINE_H
#define BT_PROCESSOR_MACHINE_H

#include <glib.h>
#include <glib-object.h>

#include "machine.h"

#define BT_TYPE_PROCESSOR_MACHINE             (bt_processor_machine_get_type ())
#define BT_PROCESSOR_MACHINE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PROCESSOR_MACHINE, BtProcessorMachine))
#define BT_PROCESSOR_MACHINE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_PROCESSOR_MACHINE, BtProcessorMachineClass))
#define BT_IS_PROCESSOR_MACHINE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_PROCESSOR_MACHINE))
#define BT_IS_PROCESSOR_MACHINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_PROCESSOR_MACHINE))
#define BT_PROCESSOR_MACHINE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_PROCESSOR_MACHINE, BtProcessorMachineClass))

/* type macros */

typedef struct _BtProcessorMachine BtProcessorMachine;
typedef struct _BtProcessorMachineClass BtProcessorMachineClass;

/**
 * BtProcessorMachine:
 *
 * Sub-class of a #BtMachine that implements an effect-processor
 * (a machine with in and outputs).
 */
struct _BtProcessorMachine {
  const BtMachine parent;
};

struct _BtProcessorMachineClass {
  const BtMachineClass parent;
};

/**
 * BtProcessorMachinePatternIndex:
 * @BT_PROCESSOR_MACHINE_PATTERN_INDEX_BREAK: stop the pattern
 * @BT_PROCESSOR_MACHINE_PATTERN_INDEX_MUTE: mute the machine
 * @BT_PROCESSOR_MACHINE_PATTERN_INDEX_BYPASS: bypass the machine
 * @BT_PROCESSOR_MACHINE_PATTERN_INDEX_OFFSET: offset for real pattern ids
 *
 * Use this with bt_machine_get_pattern_by_index() to get the command patterns.
 */
typedef enum {
  BT_PROCESSOR_MACHINE_PATTERN_INDEX_BREAK=0,
  BT_PROCESSOR_MACHINE_PATTERN_INDEX_MUTE,
  BT_PROCESSOR_MACHINE_PATTERN_INDEX_BYPASS,
  BT_PROCESSOR_MACHINE_PATTERN_INDEX_OFFSET
} BtProcessorMachinePatternIndex;

GType bt_processor_machine_get_type(void) G_GNUC_CONST;

BtProcessorMachine *bt_processor_machine_new(BtMachineConstructorParams* const params, const gchar * const plugin_name, const glong voices, GError **err);

#endif // BT_PROCESSOR_MACHINE_H
