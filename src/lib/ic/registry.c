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
 * SECTION:bticregistry
 * @short_description: buzztraxs interaction controller registry
 *
 * Manages a dynamic list of controller devices. It uses a discoverer helper to
 * scan devices. Right now GUdev is supported.
 */
#define BTIC_CORE
#define BTIC_REGISTRY_C

#include "ic_private.h"

enum
{
  REGISTRY_DEVICES = 1
};

struct _BtIcRegistryPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* list of BtIcDevice objects */
  GList *devices;

#if USE_ALSA
  BtIcASeqDiscoverer *aseq_discoverer;
#endif
#if USE_GUDEV
  BtIcGudevDiscoverer *gudev_discoverer;
#endif
};

static BtIcRegistry *singleton = NULL;

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtIcRegistry, btic_registry, G_TYPE_OBJECT, G_ADD_PRIVATE(BtIcRegistry));

//-- helper

static GList *
find_device_node_by_property (const gchar * prop, const gchar * value)
{
  BtIcRegistry *self = singleton;
  GList *node, *res = NULL;
  gchar *device_value;

  GST_INFO ("searching for prop='%s' with value='%s'", prop, value);

  for (node = self->priv->devices; (node && !res); node = g_list_next (node)) {
    g_object_get (node->data, prop, &device_value, NULL);
    GST_INFO (".. value='%s'", device_value);
    if (!strcmp (value, device_value))
      res = node;
    g_free (device_value);
  }
  return res;
}

//-- constructor methods

/**
 * btic_registry_new:
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtIcRegistry *
btic_registry_new (void)
{
  return g_object_new (BTIC_TYPE_REGISTRY, NULL);
}

/*
 * btic_registry_active:
 *
 * Check if there is an active registry. Meant to be used by internal
 * API (e.g. discoverers to only run if created from the registry),
 *
 * Returns: %TRUE if there is an active registry instance
 */
gboolean
btic_registry_active (void)
{
  return singleton != NULL;
}

/**
 * btic_registry_start_discovery:
 *
 * Run discovery services (if available).
 *
 * Since: 0.11
 */
void
btic_registry_start_discovery (void)
{
#if USE_ALSA
  singleton->priv->aseq_discoverer = btic_aseq_discoverer_new ();
#endif
#if USE_GUDEV
  singleton->priv->gudev_discoverer = btic_gudev_discoverer_new ();
#endif
}

//-- methods

/**
 * btic_registry_get_device_by_name:
 * @name: device name
 *
 * Find the device identified by the given @name in the registry.
 *
 * Returns: (transfer full): a ref to the device or %NULL.
 *
 * Since: 0.9
 */
BtIcDevice *
btic_registry_get_device_by_name (const gchar * name)
{
  g_return_val_if_fail (singleton, NULL);
  GList *node = find_device_node_by_property ("name", name);
  return node ? g_object_ref (node->data) : NULL;
}

/**
 * btic_registry_remove_device_by_udi:
 * @udi: device id
 *
 * Remove device identified by the given @udi from the registry.
 *
 * Only to be used by discoverers.
 */
void
btic_registry_remove_device_by_udi (const gchar * udi)
{
  BtIcRegistry *self = singleton;
  g_return_if_fail (self);

  GList *node = find_device_node_by_property ("udi", udi);

  if (node) {
    g_object_unref (node->data);
    self->priv->devices = g_list_delete_link (self->priv->devices, node);
    g_object_notify (G_OBJECT (self), "devices");
    GST_INFO ("registry has %u devices", g_list_length (self->priv->devices));
  } else {
    GST_WARNING ("no device with udi=%s", udi);
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
void
btic_registry_add_device (BtIcDevice * device)
{
  BtIcRegistry *self = singleton;
  g_return_if_fail (self);

  if (btic_device_has_controls (device) || BTIC_IS_LEARN (device)) {
    // add devices to our list and trigger notify
    self->priv->devices =
        g_list_prepend (self->priv->devices, (gpointer) device);
    g_object_notify (G_OBJECT (self), "devices");
    GST_INFO ("registry has %u devices", g_list_length (self->priv->devices));
  } else {
    GST_DEBUG ("device has no controls, not adding");
    g_object_unref (device);
  }
}

//-- wrapper

//-- class internals

static void
btic_registry_get_property (GObject * const object, const guint property_id,
    GValue * const value, GParamSpec * const pspec)
{
  const BtIcRegistry *const self = BTIC_REGISTRY (object);
  return_if_disposed ();
  switch (property_id) {
    case REGISTRY_DEVICES:
      g_value_set_pointer (value, g_list_copy (self->priv->devices));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
btic_registry_dispose (GObject * const object)
{
  const BtIcRegistry *const self = BTIC_REGISTRY (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);
#if USE_ALSA
  g_object_try_unref (self->priv->aseq_discoverer);
#endif
#if USE_GUDEV
  g_object_try_unref (self->priv->gudev_discoverer);
#endif
  if (self->priv->devices) {
    GList *node;
    GST_DEBUG ("!!!! free devices: %d", g_list_length (self->priv->devices));
    for (node = self->priv->devices; node; node = g_list_next (node)) {
      g_object_try_unref (node->data);
      node->data = NULL;
    }
  }

  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (btic_registry_parent_class)->dispose (object);
  GST_DEBUG ("  done");
}

static void
btic_registry_finalize (GObject * const object)
{
  const BtIcRegistry *const self = BTIC_REGISTRY (object);

  GST_DEBUG ("!!!! self=%p", self);

  if (self->priv->devices) {
    g_list_free (self->priv->devices);
    self->priv->devices = NULL;
  }

  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (btic_registry_parent_class)->finalize (object);
  GST_DEBUG ("  done");
}

static GObject *
btic_registry_constructor (GType type, guint n_construct_params,
    GObjectConstructParam * construct_params)
{
  GObject *object;

  if (G_UNLIKELY (!singleton)) {
    object =
        G_OBJECT_CLASS (btic_registry_parent_class)->constructor (type,
        n_construct_params, construct_params);
    singleton = BTIC_REGISTRY (object);
    g_object_add_weak_pointer (object, (gpointer *) (gpointer) & singleton);

    GST_INFO ("new device registry created");
  } else {
    object = g_object_ref ((gpointer) singleton);
  }
  return object;
}


static void
btic_registry_init (BtIcRegistry * self)
{
  self->priv = btic_registry_get_instance_private(self);
}

static void
btic_registry_class_init (BtIcRegistryClass * const klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructor = btic_registry_constructor;
  gobject_class->get_property = btic_registry_get_property;
  gobject_class->dispose = btic_registry_dispose;
  gobject_class->finalize = btic_registry_finalize;

  g_object_class_install_property (gobject_class, REGISTRY_DEVICES,
      g_param_spec_pointer ("devices",
          "device list prop",
          "A copy of the list of control devices",
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}
