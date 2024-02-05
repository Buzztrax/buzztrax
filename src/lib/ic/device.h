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

#ifndef BTIC_DEVICE_H
#define BTIC_DEVICE_H

#include <glib.h>
#include <glib-object.h>

#include "control.h"

/* type macros */

#define BTIC_TYPE_DEVICE            (btic_device_get_type ())
#define BTIC_DEVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTIC_TYPE_DEVICE, BtIcDevice))
#define BTIC_DEVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTIC_TYPE_DEVICE, BtIcDeviceClass))
#define BTIC_IS_DEVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTIC_TYPE_DEVICE))
#define BTIC_IS_DEVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTIC_TYPE_DEVICE))
#define BTIC_DEVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTIC_TYPE_DEVICE, BtIcDeviceClass))

typedef struct _BtIcDevice BtIcDevice;
typedef struct _BtIcDeviceClass BtIcDeviceClass;
typedef struct _BtIcDevicePrivate BtIcDevicePrivate;

/**
 * BtIcDevice:
 *
 * buzztraxs interaction controller device
 */
struct _BtIcDevice {
  const GObject parent;

  /*< private >*/
  BtIcDevicePrivate *priv;
};

/**
 * btic_device_virtual_start:
 * @self: device instance
 *
 * Subclasses will override this methods with a function that counts start calls
 * and runs the device-io for starts>0.
 *
 * Returns: %TRUE for success
 */
typedef gboolean (*btic_device_virtual_start)(gconstpointer self);

/**
 * btic_device_virtual_stop:
 * @self: device instance
 *
 * Subclasses will override this methods with a function that counts stop calls
 * and stops the device-io for starts==0.
 *
 * Returns: %TRUE for success
 */
typedef gboolean (*btic_device_virtual_stop)(gconstpointer self);

/**
 * BtIcDeviceClass:
 * @parent: parent class type
 * @start: virtual method for starting a devie
 * @stop: virtual method for stopping a device
 * @learn: virtual method for a learn function
 *
 * buzztraxs interaction controller device class
 */
struct _BtIcDeviceClass {
  const GObjectClass parent;

  /* class methods */
  btic_device_virtual_start start;
  btic_device_virtual_stop stop;
};

GType btic_device_get_type(void) G_GNUC_CONST;

void btic_device_add_control(const BtIcDevice *self, const BtIcControl *control);
BtIcControl *btic_device_get_control_by_id(const BtIcDevice *self,guint id);
BtIcControl *btic_device_get_control_by_name(const BtIcDevice * self, const gchar *name);
const gchar *btic_device_get_name(const BtIcDevice *self);
gboolean btic_device_has_controls(const BtIcDevice *self);
gboolean btic_device_start(const BtIcDevice *self);
gboolean btic_device_stop(const BtIcDevice *self);

#endif // BTIC_DEVICE_H
