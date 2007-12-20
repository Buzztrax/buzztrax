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

#ifndef BT_WIRE_PATTERN_H
#define BT_WIRE_PATTERN_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_WIRE_PATTERN            (bt_wire_pattern_get_type ())
#define BT_WIRE_PATTERN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_WIRE_PATTERN, BtWirePattern))
#define BT_WIRE_PATTERN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_WIRE_PATTERN, BtWirePatternClass))
#define BT_IS_WIRE_PATTERN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_WIRE_PATTERN))
#define BT_IS_WIRE_PATTERN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_WIRE_PATTERN))
#define BT_WIRE_PATTERN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_WIRE_PATTERN, BtWirePatternClass))

/* type macros */

typedef struct _BtWirePattern BtWirePattern;
typedef struct _BtWirePatternClass BtWirePatternClass;
typedef struct _BtWirePatternPrivate BtWirePatternPrivate;

/**
 * BtWirePattern:
 *
 * Class that holds a sequence of automation events for a #BtWire.
 */
struct _BtWirePattern {
  const GObject parent;
  
  /*< private >*/
  BtWirePatternPrivate *priv;
};

// ugly :/
struct _BtWire;

/* structure of the pattern class */
struct _BtWirePatternClass {
  const GObjectClass parent;

  void (*param_changed_event)(const BtWirePattern * const wire_pattern, const gulong tick, const struct _BtWire * const wire, const gulong param, gconstpointer const user_data);
};

/* used by PATTERN_TYPE */
GType bt_wire_pattern_get_type(void) G_GNUC_CONST;

#endif /* BT_WIRE_PATTERN_H */
