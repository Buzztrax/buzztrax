/* $Id$
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
/**
 * SECTION:btictriggercontrol
 * @short_description: buzztards interaction controller single trigger control
 *
 * Trigger control. The state of the hardware control can be read from
 * BtIcTriggerControl:value.
 */
#define BTIC_CORE
#define BTIC_TRIGGER_CONTROL_C

#include "ic_private.h"

enum {
  TRIGGER_CONTROL_VALUE=1
};

struct _BtIcTriggerControlPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  gboolean value;
};

static GObjectClass *parent_class=NULL;

//-- helper

//-- handler

//-- constructor methods

/**
 * btic_trigger_control_new:
 * @device: the device it belongs to
 * @name: human readable name
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtIcTriggerControl *btic_trigger_control_new(const BtIcDevice *device,const gchar *name) {
  BtIcTriggerControl *self=BTIC_TRIGGER_CONTROL(g_object_new(BTIC_TYPE_TRIGGER_CONTROL,"device",device,"name",name,NULL));
  if(!self) {
    goto Error;
  }
  // register myself with the device
  btic_device_add_control(device,BTIC_CONTROL(self));
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}


//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void btic_trigger_control_get_property(GObject * const object, const guint property_id, GValue * const value, GParamSpec * const pspec) {
  const BtIcTriggerControl * const self = BTIC_TRIGGER_CONTROL(object);
  return_if_disposed();
  switch (property_id) {
    case TRIGGER_CONTROL_VALUE: {
      g_value_set_boolean(value, self->priv->value);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void btic_trigger_control_set_property(GObject * const object, const guint property_id, const GValue * const value, GParamSpec * const pspec) {
  const BtIcTriggerControl * const self = BTIC_TRIGGER_CONTROL(object);
  return_if_disposed();
  switch (property_id) {
    case TRIGGER_CONTROL_VALUE: {
      self->priv->value = g_value_get_boolean(value);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void btic_trigger_control_dispose(GObject * const object) {
  const BtIcTriggerControl * const self = BTIC_TRIGGER_CONTROL(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p, self->ref_ct=%d",self,G_OBJECT(self)->ref_count);

  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(parent_class)->dispose(object);
  GST_DEBUG("  done");
}

static void btic_trigger_control_finalize(GObject * const object) {
  const BtIcTriggerControl * const self = BTIC_TRIGGER_CONTROL(object);

  GST_DEBUG("!!!! self=%p",self);

  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(parent_class)->finalize(object);
  GST_DEBUG("  done");
}

static void btic_trigger_control_init(const GTypeInstance * const instance, gconstpointer const g_class) {
  BtIcTriggerControl * const self = BTIC_TRIGGER_CONTROL(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BTIC_TYPE_TRIGGER_CONTROL, BtIcTriggerControlPrivate);
}

static void btic_trigger_control_class_init(BtIcTriggerControlClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtIcTriggerControlPrivate));

  gobject_class->set_property = btic_trigger_control_set_property;
  gobject_class->get_property = btic_trigger_control_get_property;
  gobject_class->dispose      = btic_trigger_control_dispose;
  gobject_class->finalize     = btic_trigger_control_finalize;

  g_object_class_install_property(gobject_class,TRIGGER_CONTROL_VALUE,
                                  g_param_spec_boolean("value",
                                     "value prop",
                                     "control value",
                                     FALSE, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));
}

GType btic_trigger_control_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      (guint16)(sizeof(BtIcTriggerControlClass)),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)btic_trigger_control_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      (guint16)(sizeof(BtIcTriggerControl)),
      0,   // n_preallocs
      (GInstanceInitFunc)btic_trigger_control_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(BTIC_TYPE_CONTROL,"BtIcTriggerControl",&info,0);
  }
  return type;
}
