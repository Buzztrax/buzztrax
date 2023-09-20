/* Buzztrax
 * Copyright (C) 2012 Buzztrax team <buzztrax-devel@buzztrax.org>
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
/**
 * SECTION:bticinputdevice
 * @short_description: buzztraxs interaction controller test device
 *
 * Dummy device for tests.
 */
#define BTIC_CORE
#define BTIC_TEST_DEVICE_C

#include "ic/ic_private.h"
#include "btic-test-device.h"

enum
{
  DEVICE_CONTROLCHANGE = 1,
};


struct _BtIcTestDevicePrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  gboolean running;
  gboolean learn_mode;
  guint learn_key;
};

//-- the class

static void btic_test_device_interface_init (gpointer const g_iface,
    gpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtIcTestDevice, btic_test_device, BTIC_TYPE_DEVICE,
    G_IMPLEMENT_INTERFACE (BTIC_TYPE_LEARN, btic_test_device_interface_init)
    G_ADD_PRIVATE (BtIcTestDevice));

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
  return BTIC_TEST_DEVICE (g_object_new (BTIC_TYPE_TEST_DEVICE, "name", name,
          NULL));
}

//-- methods

static gboolean
btic_test_device_start (gconstpointer _self)
{
  BtIcTestDevice *self = BTIC_TEST_DEVICE (_self);

  self->priv->running = TRUE;
  return TRUE;
}

static gboolean
btic_test_device_stop (gconstpointer _self)
{
  BtIcTestDevice *self = BTIC_TEST_DEVICE (_self);

  self->priv->running = FALSE;
  return TRUE;
}

//-- learn interface

static gboolean
btic_test_device_learn_start (gconstpointer _self)
{
  BtIcTestDevice *self = BTIC_TEST_DEVICE (_self);

  self->priv->learn_mode = TRUE;
  btic_device_start (BTIC_DEVICE (self));

  return TRUE;
}

static gboolean
btic_test_device_learn_stop (gconstpointer _self)
{
  BtIcTestDevice *self = BTIC_TEST_DEVICE (_self);

  self->priv->learn_mode = FALSE;
  btic_device_stop (BTIC_DEVICE (self));

  return TRUE;
}

static BtIcControl *
btic_test_device_register_learned_control (gconstpointer _self,
    const gchar * name)
{
  BtIcControl *control = NULL;
  BtIcTestDevice *self = BTIC_TEST_DEVICE (_self);

  GST_INFO ("registering test control as %s", name);

  /* this avoids that we learn a key again */
  if (!(control =
          btic_device_get_control_by_id (BTIC_DEVICE (self),
              self->priv->learn_key))) {
    control =
        BTIC_CONTROL (btic_abs_range_control_new (BTIC_DEVICE (self), name,
            self->priv->learn_key, 0, 255, 0));
    self->priv->learn_key++;
  }
  return control;
}

static void
btic_test_device_interface_init (gpointer const g_iface,
    gpointer const iface_data)
{
  BtIcLearnInterface *iface = (BtIcLearnInterface *) g_iface;

  iface->learn_start = btic_test_device_learn_start;
  iface->learn_stop = btic_test_device_learn_stop;
  iface->register_learned_control = btic_test_device_register_learned_control;
}


//-- wrapper

//-- class internals

static void
btic_test_device_get_property (GObject * const object, const guint property_id,
    GValue * const value, GParamSpec * const pspec)
{
  const BtIcTestDevice *const self = BTIC_TEST_DEVICE (object);
  return_if_disposed ();
  switch (property_id) {
    case DEVICE_CONTROLCHANGE:
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
btic_test_device_set_property (GObject * const object, const guint property_id,
    const GValue * const value, GParamSpec * const pspec)
{
  const BtIcTestDevice *const self = BTIC_TEST_DEVICE (object);
  return_if_disposed ();
  switch (property_id) {
    case DEVICE_CONTROLCHANGE:
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
btic_test_device_dispose (GObject * const object)
{
  const BtIcTestDevice *const self = BTIC_TEST_DEVICE (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (self));
  btic_test_device_stop (self);

  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (btic_test_device_parent_class)->dispose (object);
  GST_DEBUG ("  done");
}

static void
btic_test_device_init (BtIcTestDevice * self)
{
  guint ix = 0;

  self->priv = btic_test_device_get_instance_private (self);

  // register some static controls
  btic_abs_range_control_new (BTIC_DEVICE (self), "abs1", ix++, 0, 255, 0);
  btic_trigger_control_new (BTIC_DEVICE (self), "trig1", ix++);
  self->priv->learn_key = ix;
}

static void
btic_test_device_class_init (BtIcTestDeviceClass * const klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);
  BtIcDeviceClass *const bticdevice_class = BTIC_DEVICE_CLASS (klass);

  gobject_class->set_property = btic_test_device_set_property;
  gobject_class->get_property = btic_test_device_get_property;
  gobject_class->dispose = btic_test_device_dispose;

  bticdevice_class->start = btic_test_device_start;
  bticdevice_class->stop = btic_test_device_stop;

  // override learn interface
  g_object_class_override_property (gobject_class, DEVICE_CONTROLCHANGE,
      "device-controlchange");
}
