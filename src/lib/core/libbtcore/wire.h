/* $Id: wire.h,v 1.16 2006-09-03 13:21:44 ensonic Exp $
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

#ifndef BT_WIRE_H
#define BT_WIRE_H

#include <glib.h>
#include <glib-object.h>
#include "wire-pattern.h"

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
 * connects two #BtMachine instances
 */
struct _BtWire {
  const GObject parent;
  
  /*< private >*/
  BtWirePrivate *priv;
};

/* structure of the wire class */
struct _BtWireClass {
  const GObjectClass parent;

  void (*pattern_created_event)(const BtWire * const wire, const BtWirePattern * const wire_pattern, gconstpointer const user_data);
};

/**
 * BT_WIRE_MAX_NUM_PARAMS:
 * 
 * Maximum number of parameters per wire.
 */
#define BT_WIRE_MAX_NUM_PARAMS 2

/* used by WIRE_TYPE */
GType bt_wire_get_type(void) G_GNUC_CONST;

#endif // BT_WIRE_H
