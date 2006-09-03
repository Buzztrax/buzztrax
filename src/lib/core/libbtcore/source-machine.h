/* $Id: source-machine.h,v 1.17 2006-09-03 13:21:44 ensonic Exp $
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

#ifndef BT_SOURCE_MACHINE_H
#define BT_SOURCE_MACHINE_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_SOURCE_MACHINE            (bt_source_machine_get_type ())
#define BT_SOURCE_MACHINE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SOURCE_MACHINE, BtSourceMachine))
#define BT_SOURCE_MACHINE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SOURCE_MACHINE, BtSourceMachineClass))
#define BT_IS_SOURCE_MACHINE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SOURCE_MACHINE))
#define BT_IS_SOURCE_MACHINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SOURCE_MACHINE))
#define BT_SOURCE_MACHINE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SOURCE_MACHINE, BtSourceMachineClass))

/* type macros */

typedef struct _BtSourceMachine BtSourceMachine;
typedef struct _BtSourceMachineClass BtSourceMachineClass;
typedef struct _BtSourceMachinePrivate BtSourceMachinePrivate;

/**
 * BtSourceMachine:
 *
 * Sub-class of a #BtMachine that implements a signal generator
 * (a machine with outputs only).
 */
struct _BtSourceMachine {
  const BtMachine parent;
  
  /*< private >*/
  BtSourceMachinePrivate *priv;
};
/* structure of the source_machine class */
struct _BtSourceMachineClass {
  const BtMachineClass parent;
};

/* used by SOURCE_MACHINE_TYPE */
GType bt_source_machine_get_type(void) G_GNUC_CONST;

/**
 * BtSourceMachinePatternIndex:
 * @BT_SOURCE_MACHINE_PATTERN_INDEX_BREAK: stop the pattern
 * @BT_SOURCE_MACHINE_PATTERN_INDEX_MUTE: mute the machine
 * @BT_SOURCE_MACHINE_PATTERN_INDEX_SOLO: play only this machine
 * @BT_SOURCE_MACHINE_PATTERN_INDEX_OFFSET: offset for real pattern ids
 *
 * Use this with bt_machine_get_pattern_by_index() to get the command patterns.
 */
typedef enum {
  BT_SOURCE_MACHINE_PATTERN_INDEX_BREAK=0,
  BT_SOURCE_MACHINE_PATTERN_INDEX_MUTE,
  BT_SOURCE_MACHINE_PATTERN_INDEX_SOLO,
  BT_SOURCE_MACHINE_PATTERN_INDEX_OFFSET
} BtSourceMachinePatternIndex;

#endif // BT_SOURCE_MACHINE_H
