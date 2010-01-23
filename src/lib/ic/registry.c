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
 * SECTION:bticregistry
 * @short_description: buzztards interaction controller registry
 *
 * Manages a dynamic list of controller devices. It uses HAL and dbus.
 */
/*
 * http://webcvs.freedesktop.org/hal/hal/doc/spec/hal-spec.html?view=co
 */
#define BTIC_CORE
#define BTIC_REGISTRY_C

#include "ic_private.h"

enum {
  REGISTRY_DEVICE_LIST=1
};

struct _BtIcRegistryPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* list of BtIcDevice objects */
  GList *devices;

#ifdef USE_HAL
  LibHalContext *ctx;
  DBusError dbus_error;
  DBusConnection *dbus_conn;
#endif
};

static GObjectClass *parent_class=NULL;
static BtIcRegistry *singleton=NULL;

//-- helper

//-- handler

#ifdef USE_HAL
static void on_device_added(LibHalContext *ctx, const gchar *udi) {
  BtIcRegistry *self=BTIC_REGISTRY(singleton);
  gchar **cap;
  gchar *hal_category;
  gchar *temp,*parent_udi;
  gchar *name,*devnode,*type;
  size_t n;
  BtIcDevice *device=NULL;

  if(!(cap=libhal_device_get_property_strlist(ctx,udi,"info.capabilities",NULL))) {
    return;
  }
  if(!(hal_category=libhal_device_get_property_string(ctx,udi,"info.category",NULL))) {
    libhal_free_string_array(cap);
    return;
  }
  name=libhal_device_get_property_string(ctx,udi,"info.product",NULL);

  for(n=0;cap[n];n++) {
    // midi devices seem to appear only as oss under hal?
    // @todo: try alsa.sequencer
    if(!strcmp(cap[n],"alsa.sequencer")) {
      temp=libhal_device_get_property_string(ctx,udi,"info.parent",NULL);
      parent_udi=libhal_device_get_property_string(ctx,temp,"info.parent",NULL);
      libhal_free_string(temp);

      devnode=libhal_device_get_property_string(ctx,udi,"alsa.device_file",NULL);

      GST_INFO("alsa device added: type=%s, device_file=%s, vendor=%s",
        libhal_device_get_property_string(ctx,udi,"alsa.type",NULL),
        devnode,
        libhal_device_get_property_string(ctx,parent_udi,"info.vendor",NULL)
      );
      // create device
      device=BTIC_DEVICE(btic_midi_device_new(udi,name,devnode));
      libhal_free_string(devnode);
      libhal_free_string(parent_udi);
    }
    if(!strcmp(cap[n],"oss")) {
      type=libhal_device_get_property_string(ctx,udi,"oss.type",NULL);
      if(!strcmp(type,"midi")) {
        devnode=libhal_device_get_property_string(ctx,udi,"oss.device_file",NULL);

        GST_INFO("midi device added: product=%s, devnode=%s",
          name,devnode);
        // create device
        device=BTIC_DEVICE(btic_midi_device_new(udi,name,devnode));
        libhal_free_string(devnode);
      }
      libhal_free_string(type);
    }
#if 0
    else if(!strcmp(cap[n],"input.joystick")) {
      devnode=libhal_device_get_property_string(ctx,udi,"input.device",NULL);

      GST_INFO("input device added: product=%s, devnode=%s",
        name,devnode);
      // create device
      device=BTIC_DEVICE(btic_input_device_new(udi,name,devnode));
      libhal_free_string(devnode);
    }
#endif
#ifdef HAVE_LINUX_INPUT_H
    else if(!strcmp(cap[n],"input")) {
      devnode=libhal_device_get_property_string(ctx,udi,"input.device",NULL);

      GST_INFO("input device added: product=%s, devnode=%s", name,devnode);
      // create device
      device=BTIC_DEVICE(btic_input_device_new(udi,name,devnode));
      libhal_free_string(devnode);
    }
#endif
  }
  libhal_free_string_array(cap);

  // finished checking devices regarding capabilities, now checking category
  if(!strcmp(hal_category,"alsa")) {
    gchar *alsatype = libhal_device_get_property_string(ctx,udi,"alsa.type",NULL);
    if(!strcmp(alsatype,"midi")) {
      devnode=libhal_device_get_property_string(ctx,udi,"linux.device_file",NULL);

      GST_INFO("midi device added: product=%s, devnode=%s", name,devnode);
      // create device
      device=BTIC_DEVICE(btic_midi_device_new(udi,name,devnode));
      libhal_free_string(devnode);
    }
    libhal_free_string(alsatype);
  }

  if(device) {
    // add devices to our list and trigger notify
    self->priv->devices=g_list_append(self->priv->devices,(gpointer)device);
    g_object_notify(G_OBJECT(self),"devices");
    device=NULL;
  }
  else
    GST_INFO("unknown device found, not added: name=%s",name);

  libhal_free_string(hal_category);
  libhal_free_string(name);
}

static void on_device_removed(LibHalContext *ctx, const gchar *udi) {
  BtIcRegistry *self=BTIC_REGISTRY(singleton);
  GList *node;
  BtIcDevice *device;
  gchar *device_udi;

  // search for device by udi
  for(node=self->priv->devices;node;node=g_list_next(node)) {
    device=BTIC_DEVICE(node->data);
    g_object_get(device,"udi",&device_udi,NULL);
    if(!strcmp(udi,device_udi)) {
      // remove devices from our list and trigger notify
      self->priv->devices=g_list_delete_link(self->priv->devices,node);
      g_object_unref(device);
      g_object_notify(G_OBJECT(self),"devices");
      break;
    }
    g_free(device_udi);
  }
}
#endif

//-- constructor methods

/**
 * btic_registry_new:
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtIcRegistry *btic_registry_new(void) {
  return(g_object_new(BTIC_TYPE_REGISTRY,NULL));
}

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void btic_registry_get_property(GObject * const object, const guint property_id, GValue * const value, GParamSpec * const pspec) {
  const BtIcRegistry * const self = BTIC_REGISTRY(object);
  return_if_disposed();
  switch (property_id) {
    case REGISTRY_DEVICE_LIST: {
      g_value_set_pointer(value,g_list_copy(self->priv->devices));
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void btic_registry_set_property(GObject * const object, const guint property_id, const GValue * const value, GParamSpec * const pspec) {
  const BtIcRegistry * const self = BTIC_REGISTRY(object);
  return_if_disposed();
  switch (property_id) {
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void btic_registry_dispose(GObject * const object) {
  const BtIcRegistry * const self = BTIC_REGISTRY(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p, self->ref_ct=%d",self,G_OBJECT(self)->ref_count);

#ifdef USE_HAL
  libhal_ctx_free(self->priv->ctx);
  dbus_error_free(&self->priv->dbus_error);
#endif
  if(self->priv->devices) {
    GST_DEBUG("!!!! free devices: %d",g_list_length(self->priv->devices));
    GList* node;
    for(node=self->priv->devices;node;node=g_list_next(node)) {
      g_object_try_unref(node->data);
      node->data=NULL;
    }
  }

  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(parent_class)->dispose(object);
  GST_DEBUG("  done");
}

static void btic_registry_finalize(GObject * const object) {
  const BtIcRegistry * const self = BTIC_REGISTRY(object);

  GST_DEBUG("!!!! self=%p",self);

  if(self->priv->devices) {
    g_list_free(self->priv->devices);
    self->priv->devices=NULL;
  }

  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(parent_class)->finalize(object);
  GST_DEBUG("  done");
}

static GObject *btic_registry_constructor(GType type,guint n_construct_params,GObjectConstructParam *construct_params) {
  GObject *object;

  if(G_UNLIKELY(!singleton)) {
#ifdef USE_HAL
    gchar **devices;
    gint i,num_devices;
#endif
    object=G_OBJECT_CLASS(parent_class)->constructor(type,n_construct_params,construct_params);
    singleton=BTIC_REGISTRY(object);

#ifdef USE_HAL
    /* init dbus */
    dbus_error_init(&singleton->priv->dbus_error);
    singleton->priv->dbus_conn=dbus_bus_get(DBUS_BUS_SYSTEM,&singleton->priv->dbus_error);
    if(dbus_error_is_set(&singleton->priv->dbus_error)) {
      GST_WARNING("Could not connect to system bus %s", singleton->priv->dbus_error.message);
      return object;
    }
    dbus_connection_setup_with_g_main(singleton->priv->dbus_conn,NULL);
    dbus_connection_set_exit_on_disconnect(singleton->priv->dbus_conn,FALSE);
  
    /* init hal */
    if(!(singleton->priv->ctx=libhal_ctx_new())) {
      GST_WARNING("Could not create hal context");
      return object;
    }
    libhal_ctx_set_dbus_connection(singleton->priv->ctx,singleton->priv->dbus_conn);
    // register notify handler for add/remove
    libhal_ctx_set_device_added(singleton->priv->ctx,on_device_added);
    libhal_ctx_set_device_removed(singleton->priv->ctx,on_device_removed);
    if(!(libhal_ctx_init(singleton->priv->ctx,&singleton->priv->dbus_error))) {
      GST_WARNING("Could not init hal %s", singleton->priv->dbus_error.message);
      return object;
    }
    // scan already plugged devices via hal
    if((devices=libhal_find_device_by_capability(singleton->priv->ctx,"input",&num_devices,&singleton->priv->dbus_error))) {
      GST_INFO("%d input devices found, trying add..",num_devices);
      for(i=0;i<num_devices;i++) {
        on_device_added(singleton->priv->ctx,devices[i]);
      }
      libhal_free_string_array(devices);
    }
    if((devices=libhal_find_device_by_capability(singleton->priv->ctx,"alsa",&num_devices,&singleton->priv->dbus_error))) {
      GST_INFO("%d alsa devices found, trying to add..",num_devices);
      for(i=0;i<num_devices;i++) {
        on_device_added(singleton->priv->ctx,devices[i]);
      }
      libhal_free_string_array(devices);
    }
    if((devices=libhal_find_device_by_capability(singleton->priv->ctx,"alsa.sequencer",&num_devices,&singleton->priv->dbus_error))) {
      GST_INFO("%d alsa.sequencer devices found, trying to add..",num_devices);
      for(i=0;i<num_devices;i++) {
        on_device_added(singleton->priv->ctx,devices[i]);
      }
      libhal_free_string_array(devices);
    }
    if((devices=libhal_find_device_by_capability(singleton->priv->ctx,"oss",&num_devices,&singleton->priv->dbus_error))) {
      GST_INFO("%d oss devices found, trying to add..",num_devices);
      for(i=0;i<num_devices;i++) {
        on_device_added(singleton->priv->ctx,devices[i]);
      }
      libhal_free_string_array(devices);
    }
  
    GST_INFO("device registry initialized");
#else
    GST_INFO("no HAL support, not creating device registry");
#endif
  }
  else {
    object=g_object_ref(singleton);
  }
  return object;
}


static void btic_registry_init(const GTypeInstance * const instance, gconstpointer const g_class) {
  BtIcRegistry * const self = BTIC_REGISTRY(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BTIC_TYPE_REGISTRY, BtIcRegistryPrivate);
}

static void btic_registry_class_init(BtIcRegistryClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtIcRegistryPrivate));

  gobject_class->constructor  = btic_registry_constructor;
  gobject_class->set_property = btic_registry_set_property;
  gobject_class->get_property = btic_registry_get_property;
  gobject_class->dispose      = btic_registry_dispose;
  gobject_class->finalize     = btic_registry_finalize;

  g_object_class_install_property(gobject_class,REGISTRY_DEVICE_LIST,
                                  g_param_spec_pointer("devices",
                                     "device list prop",
                                     "A copy of the list of control devices",
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));
}

GType btic_registry_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      (guint16)(sizeof(BtIcRegistryClass)),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)btic_registry_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      (guint16)(sizeof(BtIcRegistry)),
      0,   // n_preallocs
      (GInstanceInitFunc)btic_registry_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(G_TYPE_OBJECT,"BtIcRegistry",&info,0);
  }
  return type;
}
