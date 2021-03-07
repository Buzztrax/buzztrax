/* Buzztrax
 * Copyright (C) 2011 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * SECTION:bticgudevdiscoverer
 * @short_description: gudev based device discovery
 *
 * Discover input and midi devices using gudev.
 */

#define BTIC_CORE
#define BTIC_GUDEV_DISCOVERER_C

#include "ic_private.h"
#if USE_GUDEV
#include <gudev/gudev.h>
#endif

struct _BtIcGudevDiscovererPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  GUdevClient *client;
};

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtIcGudevDiscoverer, btic_gudev_discoverer, G_TYPE_OBJECT,
						 G_ADD_PRIVATE(BtIcGudevDiscoverer));

//-- helper

/*

remove: subsys=   input, devtype=       (null), name=    event1, number= 1, devnode=/dev/input/event1
remove: subsys=   input, devtype=       (null), name=       js0, number= 0, devnode=/dev/input/js0
remove: subsys=   input, devtype=       (null), name=   input11, number=11, devnode=(null)
remove: subsys=  hidraw, devtype=       (null), name=   hidraw0, number= 0, devnode=/dev/hidraw0
remove: subsys=     hid, devtype=       (null), name=0003:06A3:0502.0008, number=0008, devnode=(null)
remove: subsys=     usb, devtype=usb_interface, name=   5-5:1.0, number= 0, devnode=(null)
remove: subsys=     usb, devtype=usb_device,    name=       5-5, number= 5, devnode=/dev/bus/usb/005/014
   add: subsys=     usb, devtype=usb_device,    name=       5-5, number= 5, devnode=/dev/bus/usb/005/015
   add: subsys=     usb, devtype=usb_interface, name=   5-5:1.0, number= 0, devnode=(null)
   add: subsys=     hid, devtype=       (null), name=0003:06A3:0502.0009, number=0009, path=(null)
   add: subsys=  hidraw, devtype=       (null), name=   hidraw0, number= 0, devnode=/dev/hidraw0
   add: subsys=   input, devtype=       (null), name=   input12, number=12, devnode=(null)
   add: subsys=   input, devtype=       (null), name=       js0, number= 0, devnode=/dev/input/js0
   add: subsys=   input, devtype=       (null), name=    event1, number= 1, devnode=/dev/input/event1


old HAL output:
registry.c:305:hal_scan: 20 alsa devices found, trying add..
registry.c:275:on_device_added: midi device added: product=Hoontech SoundTrack Audio DSP24 ALSA MIDI Device, devnode=/dev/snd/midiC2D1
registry.c:275:on_device_added: midi device added: product=Hoontech SoundTrack Audio DSP24 ALSA MIDI Device, devnode=/dev/snd/midiC2D0
registry.c:305:hal_scan: 0 alsa.sequencer devices found, trying add..
registry.c:305:hal_scan: 14 oss devices found, trying add..
registry.c:249:on_device_added: midi device added: product=ICE1712 multi OSS MIDI Device, devnode=/dev/midi2
registry.c:249:on_device_added: midi device added: product=ICE1712 multi OSS MIDI Device, devnode=/dev/amidi2

*/

static void
on_uevent (GUdevClient * client, gchar * action, GUdevDevice * udevice,
    gpointer user_data)
{
  //BtIcGudevDiscoverer *self=BTIC_GUDEV_DISCOVERER(user_data);
  const gchar *name = g_udev_device_get_name (udevice);
  const gchar *subsystem = g_udev_device_get_subsystem (udevice);
  const gchar *udi = g_udev_device_get_sysfs_path (udevice);
  const gchar *devnode = g_udev_device_get_device_file (udevice);

  GST_INFO
      ("action=%6s: subsys=%8s, devtype=%15s, name=%10s, number=%2s, devnode=%s, driver=%s, udi=%s",
      action, subsystem, g_udev_device_get_devtype (udevice), name,
      g_udev_device_get_number (udevice), devnode,
      g_udev_device_get_driver (udevice), udi);

  if (!devnode || !udi)
    return;

  if (!strcmp (action, "remove") || !strcmp (action, "change")) {
    btic_registry_remove_device_by_udi (udi);
  }

  if (!strcmp (action, "add") || !strcmp (action, "change")) {
    BtIcDevice *device = NULL;
    GUdevDevice *uparent, *t;
    const gchar *full_name = NULL;
    const gchar *vendor_name, *model_name;
    gboolean free_full_name = FALSE;
    gchar *cat_full_name;

    // skip unreadble device descriptors quickly
    if (access (devnode, R_OK) == -1) {
      return;
    }

    /* FIXME(ensonic): this is a udev bug, the address changes and this causes valgrind
     * warnings: http://www.pastie.org/1589552
     * we copy it for now */
    if (devnode)
      devnode = g_strdup (devnode);

    /* dump properties, also available as:
     * $(which udevadm) info -qall -p /sys/class/sound/card0
     *
     const gchar* const *props=g_udev_device_get_property_keys(udevice);
     while(*props) {
     GST_INFO("  %s: %s", *props, g_udev_device_get_property(udevice,*props));
     props++;
     }
     */

    /* get human readable device name */
    uparent = udevice;
    vendor_name = g_udev_device_get_property (uparent, "ID_VENDOR");
    if (!vendor_name)
      vendor_name =
          g_udev_device_get_property (uparent, "ID_VENDOR_FROM_DATABASE");
    model_name = g_udev_device_get_property (uparent, "ID_MODEL");
    if (!model_name)
      model_name =
          g_udev_device_get_property (uparent, "ID_MODEL_FROM_DATABASE");

    GST_INFO ("  v m:  '%s' '%s'", vendor_name, model_name);
    while (uparent && !(vendor_name && model_name)) {
      t = uparent;
      if ((uparent = g_udev_device_get_parent (t))) {
        if (!vendor_name)
          vendor_name = g_udev_device_get_property (uparent, "ID_VENDOR");
        if (!vendor_name)
          vendor_name =
              g_udev_device_get_property (uparent, "ID_VENDOR_FROM_DATABASE");
        if (!model_name)
          model_name = g_udev_device_get_property (uparent, "ID_MODEL");
        if (!model_name)
          model_name =
              g_udev_device_get_property (uparent, "ID_MODEL_FROM_DATABASE");
      }
      if (t != udevice) {
        g_object_unref (t);
      }
    }
    if (uparent && uparent != udevice) {
      g_object_unref (uparent);
    }
    GST_INFO ("  v m:  '%s' '%s'", vendor_name, model_name);
    if (vendor_name && model_name) {
      full_name = g_strconcat (vendor_name, " ", model_name, NULL);
      free_full_name = TRUE;
    } else {
      full_name = name;
    }

    /* FIXME(ensonic): we got different names with HAL (we save those in songs :/):
     * http://cgit.freedesktop.org/hal/tree/hald/linux/device.c#n3400
     * http://cgit.freedesktop.org/hal/tree/hald/linux/device.c#n3363
     */

    if (!strcmp (subsystem, "input")) {
      cat_full_name = g_strconcat ("input: ", full_name, NULL);
      device =
          BTIC_DEVICE (btic_input_device_new (udi, cat_full_name, devnode));
      g_free (cat_full_name);
#ifndef USE_ALSA
    } else if (!strcmp (subsystem, "sound")) {
      /* http://cgit.freedesktop.org/hal/tree/hald/linux/device.c#n3509 */
      if (!strncmp (name, "midiC", 5)) {
        /* alsa */
        cat_full_name = g_strconcat ("alsa midi: ", full_name, NULL);
        device =
            BTIC_DEVICE (btic_midi_device_new (udi, cat_full_name, devnode));
        g_free (cat_full_name);
      } else if (!strcmp (name, "midi2") || !strcmp (name, "amidi2")) {
        /* oss */
        cat_full_name = g_strconcat ("oss midi: ", full_name, NULL);
        device =
            BTIC_DEVICE (btic_midi_device_new (udi, cat_full_name, devnode));
        g_free (cat_full_name);
      }
#endif
    }

    if (free_full_name) {
      g_free ((gchar *) full_name);
    }

    if (device) {
      btic_registry_add_device (device);
    } else {
      GST_DEBUG ("unknown device found, not added: name=%s", name);
    }

    /* FIXME(ensonic): see above */
    g_free ((gchar *) devnode);
  }
}

static void
gudev_scan (BtIcGudevDiscoverer * self, const gchar * subsystem)
{
  GList *list, *node;
  GUdevDevice *device;

  if ((list = g_udev_client_query_by_subsystem (self->priv->client, subsystem))) {
    for (node = list; node; node = g_list_next (node)) {
      device = (GUdevDevice *) node->data;
      on_uevent (self->priv->client, "add", device, (gpointer) self);
      g_object_unref (device);
    }
    g_list_free (list);
  }
}

//-- constructor methods

/**
 * btic_gudev_discoverer_new:
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtIcGudevDiscoverer *
btic_gudev_discoverer_new (void)
{
  return g_object_new (BTIC_TYPE_GUDEV_DISCOVERER, NULL);
}

//-- methods

//-- wrapper

//-- class internals

static void
btic_gudev_discoverer_dispose (GObject * const object)
{
  const BtIcGudevDiscoverer *const self = BTIC_GUDEV_DISCOVERER (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);
  g_object_try_unref (self->priv->client);

  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (btic_gudev_discoverer_parent_class)->dispose (object);
  GST_DEBUG ("  done");
}

static GObject *
btic_gudev_discoverer_constructor (GType type, guint n_construct_params,
    GObjectConstructParam * construct_params)
{
  BtIcGudevDiscoverer *self;
  /* check with 'udevadm monitor' */
  const gchar *const subsystems[] = {
    /*
       "input",
       "sound", */
    NULL
  };

  self =
      BTIC_GUDEV_DISCOVERER (G_OBJECT_CLASS
      (btic_gudev_discoverer_parent_class)->constructor (type,
          n_construct_params, construct_params));

  if (!btic_registry_active ())
    goto done;

  // get a gudev client
  if (!(self->priv->client = g_udev_client_new (subsystems))) {
    GST_WARNING ("Could not create gudev client context");
    goto done;
  }
  // register notifications
  g_signal_connect (self->priv->client, "uevent", G_CALLBACK (on_uevent),
      (gpointer) self);

  // check already known devices
  gudev_scan (self, "input");
  gudev_scan (self, "sound");

done:
  return (GObject *) self;
}

static void
btic_gudev_discoverer_init (BtIcGudevDiscoverer * self)
{
  self->priv = btic_gudev_discoverer_get_instance_private(self);
}

static void
btic_gudev_discoverer_class_init (BtIcGudevDiscovererClass * const klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructor = btic_gudev_discoverer_constructor;
  gobject_class->dispose = btic_gudev_discoverer_dispose;
}
