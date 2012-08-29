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
/**
 * SECTION:bticinputdevice
 * @short_description: buzztards interaction controller test device
 *
 * Dummy device for tests.
 */
#define BTIC_CORE
#define BTIC_TEST_DEVICE_C

#include "ic_private.h"
#include "btic-test-device.h"

struct _BtIcTestDevicePrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  gboolean running;
};

//-- the class

G_DEFINE_TYPE (BtIcTestDevice, btic_test_device, BTIC_TYPE_DEVICE);

//-- constructor methods

/**
 * btic_test_device_new:
 * @name: human readable name
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtIcTestDevice *
btic_test_device_new (const gchar * name)
{
  return (BTIC_TEST_DEVICE (g_object_new (BTIC_TYPE_TEST_DEVICE, "name", name,
              NULL)));
}

//-- methods

static gboolean
btic_test_device_start (gconstpointer _self)
{
  BtIcTestDevice *self = BTIC_TEST_DEVICE (_self);

  self->priv->running = TRUE;
  return (TRUE);
}

static gboolean
btic_test_device_stop (gconstpointer _self)
{
  BtIcTestDevice *self = BTIC_TEST_DEVICE (_self);

  self->priv->running = FALSE;
  return (TRUE);
}

//-- wrapper

//-- class internals

static void
btic_test_device_dispose (GObject * const object)
{
  const BtIcTestDevice *const self = BTIC_TEST_DEVICE (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p, self->ref_ct=%d", self, G_OBJECT_REF_COUNT (self));
  btic_test_device_stop (self);

  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (btic_test_device_parent_class)->dispose (object);
  GST_DEBUG ("  done");
}

static void
btic_test_device_init (BtIcTestDevice * self)
{
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, BTIC_TYPE_TEST_DEVICE,
      BtIcTestDevicePrivate);
}

static void
btic_test_device_class_init (BtIcTestDeviceClass * const klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);
  BtIcDeviceClass *const bticdevice_class = BTIC_DEVICE_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BtIcTestDevicePrivate));

  gobject_class->dispose = btic_test_device_dispose;

  bticdevice_class->start = btic_test_device_start;
  bticdevice_class->stop = btic_test_device_stop;
}
