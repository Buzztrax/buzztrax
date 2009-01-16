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

#ifndef BT_MACHINE_H
#define BT_MACHINE_H

#include <glib.h>
#include <glib-object.h>
#include "pattern.h"

#define BT_TYPE_MACHINE            (bt_machine_get_type ())
#define BT_MACHINE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_MACHINE, BtMachine))
#define BT_MACHINE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_MACHINE, BtMachineClass))
#define BT_IS_MACHINE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_MACHINE))
#define BT_IS_MACHINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_MACHINE))
#define BT_MACHINE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_MACHINE, BtMachineClass))

/* type macros */

typedef struct _BtMachine BtMachine;
typedef struct _BtMachineClass BtMachineClass;
typedef struct _BtMachinePrivate BtMachinePrivate;

/**
 * BtMachine:
 *
 * base object for a virtual piece of hardware
 */
struct _BtMachine {
  const GObject parent;

  /* convinience pointers to first and last GstElement in local chain.
   * (accessed a lot by the wire object) */
  GstElement *dst_elem;
  GstElement *src_elem;

  /*< private >*/
  BtMachinePrivate *priv;
};

/* structure of the machine class */
struct _BtMachineClass {
  const GObjectClass parent;

  /*< private >*/
  /* signal callbacks */
  void (*pattern_added_event)(const BtMachine * const machine, const BtPattern * const pattern, gconstpointer const user_data);
  void (*pattern_removed_event)(const BtMachine * const machine, const BtPattern * const pattern, gconstpointer const user_data);

  /*< public >*/
  /* virtual methods for subclasses */
  gboolean (*check_type)(const BtMachine * const machine, const gulong pad_src_ct, const gulong pad_sink_ct);
};

#define BT_TYPE_MACHINE_STATE       (bt_machine_state_get_type())

/**
 * BtMachineState:
 * @BT_MACHINE_STATE_NORMAL: just working
 * @BT_MACHINE_STATE_MUTE: be quiet
 * @BT_MACHINE_STATE_SOLO: be the only one playing
 * @BT_MACHINE_STATE_BYPASS: be uneffective (pass through)
 *
 * A machine is always in one of the 4 states.
 * Use the "state" property of the #BtMachine to change or query the current state.
 */
typedef enum {
  BT_MACHINE_STATE_NORMAL=0,
  BT_MACHINE_STATE_MUTE,
  BT_MACHINE_STATE_SOLO,
  BT_MACHINE_STATE_BYPASS,
  BT_MACHINE_STATE_COUNT  
} BtMachineState;

/* used by MACHINE_TYPE */
GType bt_machine_get_type(void) G_GNUC_CONST;
/* used by MACHINE_STATE_TYPE */
GType bt_machine_state_get_type(void) G_GNUC_CONST;

#endif // BT_MACHINE_H
