/* Buzztard
 * Copyright (C) 2011 Buzztard team <buzztard-devel@lists.sf.net>
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
 * SECTION:btichaldiscoverer
 * @short_description: hal based device discovery
 *
 * Discover input and midi devices using hal.
 */
/*
 * http://webcvs.freedesktop.org/hal/hal/doc/spec/hal-spec.html?view=co
 */
#define BTIC_CORE
#define BTIC_HAL_DISCOVERER_C

#include "ic_private.h"

struct _BtIcHalDiscovererPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  LibHalContext *ctx;
  DBusError dbus_error;
  DBusConnection *dbus_conn;
};

//-- the class

G_DEFINE_TYPE (BtIcHalDiscoverer, btic_hal_discoverer, G_TYPE_OBJECT);

//-- helper

static void on_device_added(LibHalContext *ctx, const gchar *udi) {
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
    // TODO(ensonic): try alsa.sequencer
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
    btic_registry_add_device(device);
  }
  else {
    GST_DEBUG("unknown device found, not added: name=%s",name);
  }

  libhal_free_string(hal_category);
  libhal_free_string(name);
}

static void on_device_removed(LibHalContext *ctx, const gchar *udi) {
  btic_registry_remove_device_by_udi(udi);
}

static void hal_scan(BtIcHalDiscoverer *self, const gchar *subsystem) {
  gchar **devices;
  gint i,num_devices;

  if((devices=libhal_find_device_by_capability(self->priv->ctx,subsystem,&num_devices,&self->priv->dbus_error))) {
    GST_INFO("%d %s devices found, trying add..",num_devices,subsystem);
    for(i=0;i<num_devices;i++) {
      on_device_added(self->priv->ctx,devices[i]);
    }
    libhal_free_string_array(devices);
  }
}

//-- constructor methods

/**
 * btic_hal_discoverer_new:
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtIcHalDiscoverer *btic_hal_discoverer_new(void) {
  return(g_object_new(BTIC_TYPE_HAL_DISCOVERER,NULL));
}

//-- methods

//-- wrapper

//-- class internals

static void btic_hal_discoverer_dispose(GObject * const object) {
  const BtIcHalDiscoverer * const self = BTIC_HAL_DISCOVERER(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p, self->ref_ct=%d",self,G_OBJECT_REF_COUNT(self));
  libhal_ctx_free(self->priv->ctx);
  dbus_error_free(&self->priv->dbus_error);

  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(btic_hal_discoverer_parent_class)->dispose(object);
  GST_DEBUG("  done");
}

static GObject *btic_hal_discoverer_constructor(GType type,guint n_construct_params,GObjectConstructParam *construct_params) {
  BtIcHalDiscoverer *self;

  self=BTIC_HAL_DISCOVERER(G_OBJECT_CLASS(btic_hal_discoverer_parent_class)->constructor(type,n_construct_params,construct_params));

  // init dbus
  dbus_error_init(&self->priv->dbus_error);
  self->priv->dbus_conn=dbus_bus_get(DBUS_BUS_SYSTEM,&self->priv->dbus_error);
  if(dbus_error_is_set(&self->priv->dbus_error)) {
    GST_WARNING("Could not connect to system bus %s", self->priv->dbus_error.message);
    goto done;
  }
  dbus_connection_setup_with_g_main(self->priv->dbus_conn,NULL);
  dbus_connection_set_exit_on_disconnect(self->priv->dbus_conn,FALSE);

  GST_DEBUG("dbus init okay");

  // init hal
  if(!(self->priv->ctx=libhal_ctx_new())) {
    GST_WARNING("Could not create hal context");
    goto done;
  }
  if(!libhal_ctx_set_dbus_connection(self->priv->ctx,self->priv->dbus_conn)) {
    GST_WARNING("Failed to set dbus connection to hal ctx");
    goto done;
  }

  GST_DEBUG("hal init okay");

  // register notify handler for add/remove
  libhal_ctx_set_device_added(self->priv->ctx,on_device_added);
  libhal_ctx_set_device_removed(self->priv->ctx,on_device_removed);
  if(!(libhal_ctx_init(self->priv->ctx,&self->priv->dbus_error))) {
    if(dbus_error_is_set(&self->priv->dbus_error)) {
      GST_WARNING("Could not init hal %s", singleton->priv->dbus_error.message);
    }
    goto done;
  }
  // scan already plugged devices via hal
  hal_scan(self,"input");
  hal_scan(self,"alsa");
  hal_scan(self,"alsa.sequencer");
  hal_scan(self,"oss");

done:
  return((GObject *)self);
}

static void btic_hal_discoverer_init(BtIcHalDiscoverer *self) {
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BTIC_TYPE_HAL_DISCOVERER, BtIcHalDiscovererPrivate);
}

static void btic_hal_discoverer_class_init(BtIcHalDiscovererClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  g_type_class_add_private(klass,sizeof(BtIcHalDiscovererPrivate));

  gobject_class->constructor  = btic_hal_discoverer_constructor;
  gobject_class->dispose      = btic_hal_discoverer_dispose;
}

