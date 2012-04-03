/* Buzztard
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
 * Manages a dynamic list of controller devices. It uses a discoverer helper to
 * scan devices. Right now GUdev is supported.
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

#if USE_GUDEV
  BtIcGudevDiscoverer *gudev_discoverer;
#endif
};

static BtIcRegistry *singleton=NULL;

//-- the class

G_DEFINE_TYPE (BtIcRegistry, btic_registry, G_TYPE_OBJECT);

//-- helper

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

/**
 * btic_registry_remove_device_by_udi:
 * @udi: device id
 *
 * Remove device identified by the given @udi from the registry.
 *
 * Only to be used by discoverers.
 */
void btic_registry_remove_device_by_udi(const gchar *udi) {
  BtIcRegistry *self=singleton;
  GList *node;
  BtIcDevice *device;
  gchar *device_udi;

  g_return_if_fail(self);

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

/**
 * btic_registry_add_device:
 * @device: new device
 *
 * Add the given device to the registry.
 *
 * Only to be used by discoverers.
 */
void btic_registry_add_device(BtIcDevice *device) {
  BtIcRegistry *self=singleton;

  g_return_if_fail(self);

  if(btic_device_has_controls(device) || BTIC_IS_LEARN(device)) {
    // add devices to our list and trigger notify
    self->priv->devices=g_list_prepend(self->priv->devices,(gpointer)device);
    g_object_notify(G_OBJECT(self),"devices");
  }
  else {
    GST_DEBUG("device has no controls, not adding");
    g_object_unref(device);
  }
}

//-- wrapper

//-- class internals

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

static void btic_registry_dispose(GObject * const object) {
  const BtIcRegistry * const self = BTIC_REGISTRY(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p, self->ref_ct=%d",self,G_OBJECT_REF_COUNT(self));
#if USE_GUDEV
  g_object_try_unref(self->priv->gudev_discoverer);
#endif
  if(self->priv->devices) {
    GList* node;
    GST_DEBUG("!!!! free devices: %d",g_list_length(self->priv->devices));
    for(node=self->priv->devices;node;node=g_list_next(node)) {
      g_object_try_unref(node->data);
      node->data=NULL;
    }
  }

  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(btic_registry_parent_class)->dispose(object);
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
  G_OBJECT_CLASS(btic_registry_parent_class)->finalize(object);
  GST_DEBUG("  done");
}

static GObject *btic_registry_constructor(GType type,guint n_construct_params,GObjectConstructParam *construct_params) {
  GObject *object;

  if(G_UNLIKELY(!singleton)) {
    object=G_OBJECT_CLASS(btic_registry_parent_class)->constructor(type,n_construct_params,construct_params);
    singleton=BTIC_REGISTRY(object);
    g_object_add_weak_pointer(object,(gpointer*)(gpointer)&singleton);

    GST_INFO("new device registry created");
#if USE_GUDEV
    singleton->priv->gudev_discoverer=btic_gudev_discoverer_new();
#else
    GST_INFO("no GUDev support, empty device registry");
#endif
  }
  else {
    object=g_object_ref(singleton);
  }
  return object;
}


static void btic_registry_init(BtIcRegistry *self) {
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BTIC_TYPE_REGISTRY, BtIcRegistryPrivate);
}

static void btic_registry_class_init(BtIcRegistryClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  g_type_class_add_private(klass,sizeof(BtIcRegistryPrivate));

  gobject_class->constructor  = btic_registry_constructor;
  gobject_class->get_property = btic_registry_get_property;
  gobject_class->dispose      = btic_registry_dispose;
  gobject_class->finalize     = btic_registry_finalize;

  g_object_class_install_property(gobject_class,REGISTRY_DEVICE_LIST,
                                  g_param_spec_pointer("devices",
                                     "device list prop",
                                     "A copy of the list of control devices",
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));
}

