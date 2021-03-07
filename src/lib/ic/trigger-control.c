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
/**
 * SECTION:btictriggercontrol
 * @short_description: buzztraxss interaction controller single trigger control
 *
 * Boolean value control. The state of the hardware control can be read from
 * BtIcTriggerControl:value.
 */
#define BTIC_CORE
#define BTIC_TRIGGER_CONTROL_C

#include "ic_private.h"

enum
{
  TRIGGER_CONTROL_VALUE = 1
};

struct _BtIcTriggerControlPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  gboolean value;
};


//-- the class

G_DEFINE_TYPE_WITH_CODE (BtIcTriggerControl, btic_trigger_control, BTIC_TYPE_CONTROL,
						 G_ADD_PRIVATE(BtIcTriggerControl));

//-- helper

//-- handler

//-- constructor methods

/**
 * btic_trigger_control_new:
 * @device: the device it belongs to
 * @name: human readable name
 * @id: unique identifier per device
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtIcTriggerControl *
btic_trigger_control_new (const BtIcDevice * device, const gchar * name,
    guint id)
{
  BtIcTriggerControl *self =
      BTIC_TRIGGER_CONTROL (g_object_new (BTIC_TYPE_TRIGGER_CONTROL, "device",
          device, "name", name, "id", id, NULL));
  if (!self) {
    goto Error;
  }
  // register myself with the device
  btic_device_add_control (device, BTIC_CONTROL (self));
  return self;
Error:
  g_object_try_unref (self);
  return NULL;
}


//-- methods

//-- wrapper

//-- class internals

static void
btic_trigger_control_get_property (GObject * const object,
    const guint property_id, GValue * const value, GParamSpec * const pspec)
{
  const BtIcTriggerControl *const self = BTIC_TRIGGER_CONTROL (object);
  return_if_disposed ();
  switch (property_id) {
    case TRIGGER_CONTROL_VALUE:
      g_value_set_boolean (value, self->priv->value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
btic_trigger_control_set_property (GObject * const object,
    const guint property_id, const GValue * const value,
    GParamSpec * const pspec)
{
  const BtIcTriggerControl *const self = BTIC_TRIGGER_CONTROL (object);
  return_if_disposed ();
  switch (property_id) {
    case TRIGGER_CONTROL_VALUE:
      self->priv->value = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
btic_trigger_control_dispose (GObject * const object)
{
  const BtIcTriggerControl *const self = BTIC_TRIGGER_CONTROL (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (btic_trigger_control_parent_class)->dispose (object);
  GST_DEBUG ("  done");
}

static void
btic_trigger_control_init (BtIcTriggerControl * self)
{
  self->priv = btic_trigger_control_get_instance_private(self);
}

static void
btic_trigger_control_class_init (BtIcTriggerControlClass * const klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = btic_trigger_control_set_property;
  gobject_class->get_property = btic_trigger_control_get_property;
  gobject_class->dispose = btic_trigger_control_dispose;

  g_object_class_install_property (gobject_class, TRIGGER_CONTROL_VALUE,
      g_param_spec_boolean ("value", "value prop", "control value", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}
