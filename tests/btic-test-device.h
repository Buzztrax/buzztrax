/* Buzztard
 * Copyright (C) 2012 Buzztard team <buzztard-devel@lists.sf.net>
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

#ifndef BTIC_TEST_DEVICE_H
#define BTIC_TEST_DEVICE_H

#include <glib.h>
#include <glib-object.h>

/* type macros */

#define BTIC_TYPE_TEST_DEVICE            (btic_test_device_get_type ())
#define BTIC_TEST_DEVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTIC_TYPE_TEST_DEVICE, BtIcTestDevice))
#define BTIC_TEST_DEVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTIC_TYPE_TEST_DEVICE, BtIcTestDeviceClass))
#define BTIC_IS_TEST_DEVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTIC_TYPE_TEST_DEVICE))
#define BTIC_IS_TEST_DEVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTIC_TYPE_TEST_DEVICE))
#define BTIC_TEST_DEVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTIC_TYPE_TEST_DEVICE, BtIcTestDeviceClass))

typedef struct _BtIcTestDevice BtIcTestDevice;
typedef struct _BtIcTestDeviceClass BtIcTestDeviceClass;
typedef struct _BtIcTestDevicePrivate BtIcTestDevicePrivate;

/**
 * BtIcTestDevice:
 *
 * buzztards interaction controller registry
 */
struct _BtIcTestDevice {
  const BtIcDevice parent;
  
  /*< private >*/
  BtIcTestDevicePrivate *priv;
};

struct _BtIcTestDeviceClass {
  const BtIcDeviceClass parent;
  
};

GType btic_test_device_get_type(void) G_GNUC_CONST;

BtIcTestDevice *btic_test_device_new(const gchar *name);

#endif // BTIC_TEST_DEVICE_H
