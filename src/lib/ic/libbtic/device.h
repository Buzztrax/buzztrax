/* $Id: device.h,v 1.1 2007-03-10 14:49:39 ensonic Exp $
 *
 * Buzztard
 * Copyright (C) 2007 Buzztard team <buzztard-devel@lists.sf.net>
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

#ifndef BTIC_DEVICE_H
#define BTIC_DEVICE_H

#include <glib.h>
#include <glib-object.h>

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
 * buzztards interaction controller registry
 */
struct _BtIcDevice {
  const GObject parent;
  
  /*< private >*/
  BtIcDevicePrivate *priv;
};
/* structure of the registry class */
struct _BtIcDeviceClass {
  const GObjectClass parent;
  
};

/* used by DEVICE_TYPE */
GType btic_device_get_type(void) G_GNUC_CONST;

#endif // BTIC_DEVICE_H
