/* $Id: device.c,v 1.8 2007-08-16 12:34:42 berzerka Exp $
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
 * Abstract base class for control devices. Subclasses will provide
 * functionality to query capabilities and register #BtIcControl instances.
 * They will also read from the device and trigger the change events on their
 * controls (via value property).
 */
/* @todo: we need a way to export/import controller maps per device
 *        (list of controller id,type,name)
 * @todo: need abstract _start() and _stop() method, whenever we bind/unbind a
 *        control we need to call _start/_stop on the respective device. The
 *        methods inc/dec a counter and if the counter is >0 we run the
 *        g_io_channel
 */
#define BTIC_CORE
#define BTIC_DEVICE_C

#include <libbtic/ic.h>

enum {
  DEVICE_UDI=1,
  DEVICE_NAME,
  DEVICE_CONTROL_LIST
};

struct _BtIcDevicePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* list of BtIcControl objects */
  GList *controls;

  gchar *udi;
  gchar *name;

  /* start/stop counter */
  gulong run_ct;
};

static GObjectClass *parent_class=NULL;

//-- helper

//-- handler

//-- constructor methods

//-- methods

/**
 * btic_device_add_control:
 * @self: the device
 * @control: new control
 *
 * Add the given @control to the list that the device manages.
 */
void btic_device_add_control(const BtIcDevice *self, const BtIcControl *control) {
  g_return_if_fail(BTIC_CONTROL(control));

  // @todo: should we ref?
  self->priv->controls=g_list_append(self->priv->controls,(gpointer)control);
}

static gboolean btic_device_default_start(gconstpointer self) {
  GST_ERROR("virtual method btic_device_start(self=%p) called",self);
  return(FALSE);  // this is a base class that can't do anything
}

static gboolean btic_device_default_stop(gconstpointer self) {
  GST_ERROR("virtual method btic_device_stop(self=%p) called",self);
  return(FALSE);  // this is a base class that can't do anything
}

//-- wrapper

/**
 * btic_device_start:
 * @self: the #Device instance to use
 *
 * Starts the io-loop for the device. This can be called multiple times and must
 * be paired by an equal amount of btic_device_stop() calls.
 *
 * Returns: %TRUE for success
 */
gboolean btic_device_start(const BtIcDevice *self) {
  //const BtIcDevice *self=BTIC_DEVICE(_self);
  gboolean result=TRUE;

  self->priv->run_ct++;
  if(self->priv->run_ct==1) {
    result=BTIC_DEVICE_GET_CLASS(self)->start(self);
  }
  return(result);
}

/**
 * btic_device_stop:
 * @self: the #Device instance to use
 *
 * Stops the io-loop for the device. This must be called as often as the device
 * has been started using  btic_device_start().
 *
 * Returns: %TRUE for success
 */
gboolean btic_device_stop(const BtIcDevice *self) {
  //const BtIcDevice *self=BTIC_DEVICE(_self);
  gboolean result=TRUE;

  g_assert(self->priv->run_ct>0);

  self->priv->run_ct--;
  if(self->priv->run_ct==0) {
    result=BTIC_DEVICE_GET_CLASS(self)->stop(self);
  }
  return(result);
}

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
    case DEVICE_CONTROL_LIST: {
      g_value_set_pointer(value,g_list_copy(self->priv->controls));
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

  if(self->priv->controls) {
    g_list_free(self->priv->controls);
    self->priv->controls=NULL;
  }

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

  klass->start = btic_device_default_start;
  klass->stop  = btic_device_default_stop;

  g_object_class_install_property(gobject_class,DEVICE_UDI,
                                  g_param_spec_string("udi",
                                     "udi prop",
                                     "device udi",
                                     NULL, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,DEVICE_NAME,
                                  g_param_spec_string("name",
                                     "name prop",
                                     "device name",
                                     NULL, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,DEVICE_CONTROL_LIST,
                                  g_param_spec_pointer("controls",
                                     "control list prop",
                                     "A copy of the list of device controls",
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));
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
