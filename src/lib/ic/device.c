/* $Id: device.c,v 1.2 2007-03-11 20:19:20 ensonic Exp $
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
 * SECTION:bticdevice
 * @short_description: buzztards interaction controller device
 *
 * Abstract base class for controll devices.
 */
/* @todo: we need a way to export/import controller maps per device
 *        (list of controller id,type,name)
 */
#define BTIC_CORE
#define BTIC_DEVICE_C

#include <libbtic/ic.h>

enum {
  DEVICE_UDI=1,
  DEVICE_NAME
};

struct _BtIcDevicePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* list of BtIcController objects */
  GList *controllers;

  gchar *udi;
  gchar *name;  
};

static GObjectClass *parent_class=NULL;

/* we need device type specific subtypes inheriting from this:
 *   BtIcMidiDevice, BtIcJoystick
 * they provice the device specific GSource and list of controllers
 */

//-- helper

//-- handler

//-- constructor methods

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void btic_device_get_property(GObject      * const object,
                               const guint         property_id,
                               GValue       * const value,
                               GParamSpec   * const pspec)
{
  const BtIcDevice * const self = BTIC_DEVICE(object);
  return_if_disposed();
  switch (property_id) {
    case DEVICE_UDI: {
      g_value_set_string(value, self->priv->udi);
    } break;
    case DEVICE_NAME: {
      g_value_set_string(value, self->priv->name);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void btic_device_set_property(GObject      * const object,
                              const guint         property_id,
                              const GValue * const value,
                              GParamSpec   * const pspec)
{
  const BtIcDevice * const self = BTIC_DEVICE(object);
  return_if_disposed();
  switch (property_id) {
    case DEVICE_UDI: {
      g_free(self->priv->udi);
      self->priv->udi = g_value_dup_string(value);
    } break;
    case DEVICE_NAME: {
      g_free(self->priv->name);
      self->priv->name = g_value_dup_string(value);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void btic_device_dispose(GObject * const object) {
  const BtIcDevice * const self = BTIC_DEVICE(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p, self->ref_ct=%d",self,G_OBJECT(self)->ref_count);
  
  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(parent_class)->dispose(object);
  GST_DEBUG("  done");
}

static void btic_device_finalize(GObject * const object) {
  const BtIcDevice * const self = BTIC_DEVICE(object);

  GST_DEBUG("!!!! self=%p",self);

  g_free(self->priv->udi);
  g_free(self->priv->name);

  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(parent_class)->finalize(object);
  GST_DEBUG("  done");
}

static void btic_device_init(const GTypeInstance * const instance, gconstpointer const g_class) {
  BtIcDevice * const self = BTIC_DEVICE(instance);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BTIC_TYPE_DEVICE, BtIcDevicePrivate);
}

static void btic_device_class_init(BtIcDeviceClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtIcDevicePrivate));

  gobject_class->set_property = btic_device_set_property;
  gobject_class->get_property = btic_device_get_property;
  gobject_class->dispose      = btic_device_dispose;
  gobject_class->finalize     = btic_device_finalize;

  g_object_class_install_property(gobject_class,DEVICE_UDI,
                                  g_param_spec_string("udi",
                                     "udi prop",
                                     "device udi",
                                     NULL, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property(gobject_class,DEVICE_NAME,
                                  g_param_spec_string("name",
                                     "name prop",
                                     "device name",
                                     NULL, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));
}

GType btic_device_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      (guint16)(sizeof(BtIcDeviceClass)),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)btic_device_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      (guint16)(sizeof(BtIcDevice)),
      0,   // n_preallocs
      (GInstanceInitFunc)btic_device_init, // instance_init
      NULL // value_table
    };
    // @bug: http://bugzilla.gnome.org/show_bug.cgi?id=417047
    //type = g_type_register_static(G_TYPE_OBJECT,"BtIcDevice",&info,G_TYPE_FLAG_VALUE_ABSTRACT);
    type = g_type_register_static(G_TYPE_OBJECT,"BtIcDevice",&info,0);
  }
  return type;
}
