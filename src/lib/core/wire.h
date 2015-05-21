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

#ifndef BT_WIRE_H
#define BT_WIRE_H

#include <gst/gst.h>

#define BT_TYPE_WIRE            (bt_wire_get_type ())
#define BT_WIRE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_WIRE, BtWire))
#define BT_WIRE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_WIRE, BtWireClass))
#define BT_IS_WIRE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_WIRE))
#define BT_IS_WIRE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_WIRE))
#define BT_WIRE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_WIRE, BtWireClass))

/* type macros */

typedef struct _BtWire BtWire;
typedef struct _BtWireClass BtWireClass;
typedef struct _BtWirePrivate BtWirePrivate;

/**
 * BtWire:
 *
 * A link between two #BtMachine instances.
 */
struct _BtWire {
  const GstBin parent;
  
  /*< private >*/
  BtWirePrivate *priv;
};

struct _BtWireClass {
  const GstBinClass parent;
};

/**
 * BT_WIRE_MAX_NUM_PARAMS:
 *
 * Maximum number of parameters per wire.
 */
#define BT_WIRE_MAX_NUM_PARAMS 2

GType bt_wire_get_type(void) G_GNUC_CONST;

#include "song.h"

BtWire *bt_wire_new(const BtSong *song, const BtMachine *src_machine, const BtMachine *dst_machine, GError **err);

gboolean bt_wire_reconnect(BtWire *self);
gboolean bt_wire_can_link(const BtMachine * const src, const BtMachine * const dst);

BtParameterGroup *bt_wire_get_param_group(const BtWire * const self);

#endif // BT_WIRE_H
