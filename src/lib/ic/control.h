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

#ifndef BTIC_CONTROL_H
#define BTIC_CONTROL_H

#include <glib.h>
#include <glib-object.h>

/* type macros */

#define BTIC_TYPE_CONTROL            (btic_control_get_type ())
#define BTIC_CONTROL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTIC_TYPE_CONTROL, BtIcControl))
#define BTIC_CONTROL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTIC_TYPE_CONTROL, BtIcControlClass))
#define BTIC_IS_CONTROL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTIC_TYPE_CONTROL))
#define BTIC_IS_CONTROL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTIC_TYPE_CONTROL))
#define BTIC_CONTROL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTIC_TYPE_CONTROL, BtIcControlClass))

typedef struct _BtIcControl BtIcControl;
typedef struct _BtIcControlClass BtIcControlClass;
typedef struct _BtIcControlPrivate BtIcControlPrivate;

/**
 * BtIcControl:
 *
 * buzztraxs interaction controller single control
 */
struct _BtIcControl {
  const GObject parent;

  /*< private >*/
  BtIcControlPrivate *priv;
};

struct _BtIcControlClass {
  const GObjectClass parent;

};

GType btic_control_get_type(void) G_GNUC_CONST;

const gchar* btic_control_get_name(const BtIcControl* self);

#endif // BTIC_CONTROL_H
