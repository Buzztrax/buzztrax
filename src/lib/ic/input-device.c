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
 * SECTION:bticinputdevice
 * @short_description: buzztraxs interaction controller input device
 *
 * Event handling for input devices (joystick,hdaps,wiimote).
 */
/*
 * http://linuxconsole.cvs.sourceforge.net/linuxconsole/ruby/utils/
 * http://www.frogmouth.net/hid-doco/linux-hid.html
 *
 * http://gentoo-wiki.com/HOWTO_Joystick_Setup
 * http://www.wiili.org/index.php/Wiimote
 *
 * This is lowlevel
 * http://libhid.alioth.debian.org/
 */
#define BTIC_CORE
#define BTIC_INPUT_DEVICE_C

#include "ic_private.h"

#include <fcntl.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/input.h>

enum
{
  DEVICE_DEVNODE = 1
};

struct _BtIcInputDevicePrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  gchar *devnode;

  /* io channel */
  GIOChannel *io_channel;
  guint io_source;
};

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtIcInputDevice, btic_input_device, BTIC_TYPE_DEVICE, G_ADD_PRIVATE(BtIcInputDevice));

//-- helper
#define test_bit(bit, array)    (array[bit>>3] & (1<<(bit&0x7)))

static gboolean
register_trigger_controls (const BtIcInputDevice * const self, int fd)
{
  guint ix;
  guint8 key_bitmask[KEY_MAX / 8 + 1];
  const gchar *name = NULL;
  gulong key;

  memset (key_bitmask, 0, sizeof (key_bitmask));
  if (ioctl (fd, EVIOCGBIT (EV_KEY, sizeof (key_bitmask)), key_bitmask) < 0) {
    GST_WARNING ("evdev ioctl : %s", g_strerror (errno));
    return FALSE;
  }
  // read details
  for (ix = 0; ix < KEY_MAX; ix++) {
    if (test_bit (ix, key_bitmask)) {
      GST_DEBUG ("Key  0x%02x", ix);
      switch (ix) {
        case BTN_0:
          name = "Button 0";
          break;
        case BTN_1:
          name = "Button 1";
          break;
        case BTN_2:
          name = "Button 2";
          break;
        case BTN_3:
          name = "Button 3";
          break;
        case BTN_4:
          name = "Button 4";
          break;
        case BTN_5:
          name = "Button 5";
          break;
        case BTN_6:
          name = "Button 6";
          break;
        case BTN_7:
          name = "Button 7";
          break;
        case BTN_8:
          name = "Button 8";
          break;
        case BTN_9:
          name = "Button 9";
          break;
        case BTN_LEFT:
          name = "Left Button";
          break;
        case BTN_RIGHT:
          name = "Right Button";
          break;
        case BTN_MIDDLE:
          name = "Middle Button";
          break;
        case BTN_SIDE:
          name = "Side Button";
          break;
        case BTN_EXTRA:
          name = "Extra Button";
          break;
        case BTN_FORWARD:
          name = "Forward Button";
          break;
        case BTN_BACK:
          name = "Back Button";
          break;
        case BTN_TRIGGER:
          name = "Trigger Button";
          break;
        case BTN_THUMB:
          name = "Thumb Button";
          break;
        case BTN_THUMB2:
          name = "Second Thumb Button";
          break;
        case BTN_TOP:
          name = "Top Button";
          break;
        case BTN_TOP2:
          name = "Second Top Button";
          break;
        case BTN_PINKIE:
          name = "Pinkie Button";
          break;
        case BTN_BASE:
          name = "Base Button";
          break;
        case BTN_BASE2:
          name = "Second Base Button";
          break;
        case BTN_BASE3:
          name = "Third Base Button";
          break;
        case BTN_BASE4:
          name = "Fourth Base Button";
          break;
        case BTN_BASE5:
          name = "Fifth Base Button";
          break;
        case BTN_BASE6:
          name = "Sixth Base Button";
          break;
        case BTN_DEAD:
          name = "Dead Button";
          break;
        case BTN_A:
          name = "Button A";
          break;
        case BTN_B:
          name = "Button B";
          break;
        case BTN_C:
          name = "Button C";
          break;
        case BTN_X:
          name = "Button X";
          break;
        case BTN_Y:
          name = "Button Y";
          break;
        case BTN_Z:
          name = "Button Z";
          break;
        case BTN_TL:
          name = "Thumb Left Button";
          break;
        case BTN_TR:
          name = "Thumb Right Button ";
          break;
        case BTN_TL2:
          name = "Second Thumb Left Button";
          break;
        case BTN_TR2:
          name = "Second Thumb Right Button ";
          break;
        case BTN_SELECT:
          name = "Select Button";
          break;
        case BTN_MODE:
          name = "Mode Button";
          break;
        case BTN_THUMBL:
          name = "Another Left Thumb Button ";
          break;
        case BTN_THUMBR:
          name = "Another Right Thumb Button ";
          break;
        case BTN_TOOL_PEN:
          name = "Digitiser Pen Tool";
          break;
        case BTN_TOOL_RUBBER:
          name = "Digitiser Rubber Tool";
          break;
        case BTN_TOOL_BRUSH:
          name = "Digitiser Brush Tool";
          break;
        case BTN_TOOL_PENCIL:
          name = "Digitiser Pencil Tool";
          break;
        case BTN_TOOL_AIRBRUSH:
          name = "Digitiser Airbrush Tool";
          break;
        case BTN_TOOL_FINGER:
          name = "Digitiser Finger Tool";
          break;
        case BTN_TOOL_MOUSE:
          name = "Digitiser Mouse Tool";
          break;
        case BTN_TOOL_LENS:
          name = "Digitiser Lens Tool";
          break;
        case BTN_TOUCH:
          name = "Digitiser Touch Button";
          break;
        case BTN_STYLUS:
          name = "Digitiser Stylus Button";
          break;
        case BTN_STYLUS2:
          name = "Second Digitiser Stylus Button";
          break;
        default:
          GST_INFO ("Unknown key: %d", ix);
      }
      if (name) {
        // BtIcTriggerControl *control;
        // create controller instances and register them
        key = (((guint) EV_KEY) << 16) | (guint) ix;
        /*control = */ btic_trigger_control_new (BTIC_DEVICE (self), name, key);
      }
    }
  }
  return TRUE;
}

static gboolean
register_abs_range_controls (const BtIcInputDevice * const self, int fd)
{
  guint ix;
  guint8 abs_bitmask[ABS_MAX / 8 + 1];
  struct input_absinfo abs_features;
  const gchar *name = NULL;
  gulong key;

  memset (abs_bitmask, 0, sizeof (abs_bitmask));
  if (ioctl (fd, EVIOCGBIT (EV_ABS, sizeof (abs_bitmask)), abs_bitmask) < 0) {
    GST_WARNING ("evdev ioctl : %s", g_strerror (errno));
    return FALSE;
  }
  // read details
  for (ix = 0; ix < ABS_MAX; ix++) {
    if (test_bit (ix, abs_bitmask)) {
      GST_DEBUG ("Absolute Axis 0x%02x", ix);
      switch (ix) {
        case ABS_X:
          name = "X axis";
          break;
        case ABS_Y:
          name = "Y axis";
          break;
        case ABS_Z:
          name = "Z axis";
          break;
        case ABS_RX:
          name = "X rate axis";
          break;
        case ABS_RY:
          name = "Y rate axis";
          break;
        case ABS_RZ:
          name = "Z rate axis";
          break;
        case ABS_THROTTLE:
          name = "Throttle";
          break;
        case ABS_RUDDER:
          name = "Rudder";
          break;
        case ABS_WHEEL:
          name = "Wheel";
          break;
        case ABS_GAS:
          name = "Accelerator";
          break;
        case ABS_BRAKE:
          name = "Brake";
          break;
        case ABS_HAT0X:
          name = "Hat zero, x axis";
          break;
        case ABS_HAT0Y:
          name = "Hat zero, y axis";
          break;
        case ABS_HAT1X:
          name = "Hat one, x axis";
          break;
        case ABS_HAT1Y:
          name = "Hat one, y axis";
          break;
        case ABS_HAT2X:
          name = "Hat two, x axis";
          break;
        case ABS_HAT2Y:
          name = "Hat two, y axis";
          break;
        case ABS_HAT3X:
          name = "Hat three, x axis";
          break;
        case ABS_HAT3Y:
          name = "Hat three, y axis";
          break;
        case ABS_PRESSURE:
          name = "Pressure";
          break;
        case ABS_DISTANCE:
          name = "Distance";
          break;
        case ABS_TILT_X:
          name = "Tilt, X axis";
          break;
        case ABS_TILT_Y:
          name = "Tilt, Y axis";
          break;
        case ABS_MISC:
          name = "Miscellaneous";
          break;
        default:
          GST_INFO ("Unknown absolute axis : 0x%02x ", ix);
      }
      memset (&abs_features, 0, sizeof (abs_features));
      if (ioctl (fd, EVIOCGABS (ix), &abs_features) < 0) {
        GST_WARNING ("evdev EVIOCGABS ioctl : %s", g_strerror (errno));
      } else {
        GST_DEBUG ("min: %d, max: %d, flatness: %d, fuzz: %d", abs_features.minimum, abs_features.maximum, abs_features.flat,   // default = middle value
            abs_features.fuzz   // tolerance
            );
        if (name) {
          // BtIcAbsRangeControl *control;
          // create controller instances and register them
          key = (((guint) EV_ABS) << 16) | (guint) ix;
          /*control = */ btic_abs_range_control_new (BTIC_DEVICE (self), name,
              key,
              (glong) abs_features.minimum, (glong) abs_features.maximum,
              (glong) abs_features.flat);
        }
      }
    }
  }
  // create controller instances and register them
  return TRUE;
}

static gboolean
register_controls (const BtIcInputDevice * const self)
{
  int fd;
  guint ix;
  guint8 evtype_bitmask[EV_MAX / 8 + 1];

  GST_INFO ("checking controls : %s", self->priv->devnode);

  if ((fd = open (self->priv->devnode, O_RDONLY)) < 0) {
    GST_WARNING ("evdev open failed on device %s: %s", self->priv->devnode,
        g_strerror (errno));
    return FALSE;
  }
  GST_INFO ("opened device : %s", self->priv->devnode);

  // query capabillities and register controllers
  // FIXME(ensonic): doing that for e.g. js0/js1 fails slowly
  memset (evtype_bitmask, 0, sizeof (evtype_bitmask));
  if (ioctl (fd, EVIOCGBIT (0, sizeof (evtype_bitmask)), evtype_bitmask) < 0) {
    GST_WARNING ("evdev ioctl : %s", g_strerror (errno));
  } else {
    // check supported event types
    for (ix = 0; ix < EV_MAX; ix++) {
      if (test_bit (ix, evtype_bitmask)) {
        switch (ix) {
          case EV_SYN:
            break;
          case EV_KEY:
            GST_INFO ("Keys or Buttons");
            register_trigger_controls (self, fd);
            break;
          case EV_REL:
            GST_INFO ("Relative Axes");
            break;
          case EV_ABS:
            GST_INFO ("Absolute Axes");
            register_abs_range_controls (self, fd);
            break;
          case EV_MSC:
            GST_INFO ("Miscellaneous");
            break;
          case EV_LED:
            GST_INFO ("LEDs");
            break;
          case EV_SND:
            GST_INFO ("Sounds");
            break;
          case EV_REP:
            GST_INFO ("Repeat");
            break;
          case EV_FF:
            GST_INFO ("Force Feedback");
            break;
          default:
            GST_INFO ("Unknown event type: 0x%04hx", ix);
        }
      }
    }
  }
  close (fd);
  return TRUE;
}

//-- handler

static gboolean
io_handler (GIOChannel * channel, GIOCondition condition, gpointer user_data)
{
  BtIcInputDevice *self = BTIC_INPUT_DEVICE (user_data);
  BtIcControl *control;
  GError *error = NULL;
  struct input_event ev;
  guint key;
  gboolean res = TRUE;

  //GST_INFO("io handler : %d",condition);
  if (condition & (G_IO_IN | G_IO_PRI)) {
    g_io_channel_read_chars (self->priv->io_channel, (gchar *) & ev,
        sizeof (struct input_event), NULL, &error);
    if (error) {
      GST_WARNING ("iochannel error when reading: %s", error->message);
      g_error_free (error);
    } else {
      key = (((guint) ev.type) << 16) | (guint) ev.code;
      if ((control = btic_device_get_control_by_id (BTIC_DEVICE (self), key))) {
        switch (ev.type) {
          case EV_KEY:
            //GST_INFO("key/button event: value %d, code 0x%x",ev.value,ev.code);
            g_object_set (control, "value", (gboolean) ev.value, NULL);
            break;
          case EV_ABS:
            //GST_INFO("abs axis event: value %d, code 0x%x",ev.value,ev.code);
            g_object_set (control, "value", (glong) ev.value, NULL);
            break;
          default:
            GST_INFO ("unhandled control event: type 0x%x, value %d, code 0x%x",
                ev.type, ev.value, ev.code);
        }
      }
    }
  }
  if (condition & (G_IO_HUP | G_IO_ERR | G_IO_NVAL)) {
    res = FALSE;
  }
  if (!res) {
    GST_INFO ("closing connection");
    self->priv->io_source = 0;
  }
  return res;
}

//-- constructor methods

/**
 * btic_input_device_new:
 * @udi: the udi of the device
 * @name: human readable name
 * @devnode: device node in filesystem
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtIcInputDevice *
btic_input_device_new (const gchar * udi, const gchar * name,
    const gchar * devnode)
{
  return BTIC_INPUT_DEVICE (g_object_new (BTIC_TYPE_INPUT_DEVICE, "udi", udi,
          "name", name, "devnode", devnode, NULL));
}

//-- methods

static gboolean
btic_input_device_start (gconstpointer _self)
{
  BtIcInputDevice *self = BTIC_INPUT_DEVICE (_self);
  GError *error = NULL;

  // start the io-loop
  self->priv->io_channel =
      g_io_channel_new_file (self->priv->devnode, "r", &error);
  if (error) {
    GST_WARNING ("iochannel error for open(%s): %s", self->priv->devnode,
        error->message);
    g_error_free (error);
    return FALSE;
  }
  g_io_channel_set_encoding (self->priv->io_channel, NULL, &error);
  if (error) {
    GST_WARNING ("iochannel error for settin encoding to NULL: %s",
        error->message);
    g_error_free (error);
    g_io_channel_unref (self->priv->io_channel);
    self->priv->io_channel = NULL;
    return FALSE;
  }
  self->priv->io_source = g_io_add_watch_full (self->priv->io_channel,
      G_PRIORITY_LOW,
      G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL,
      io_handler, (gpointer) self, NULL);
  return TRUE;
}

static gboolean
btic_input_device_stop (gconstpointer _self)
{
  BtIcInputDevice *self = BTIC_INPUT_DEVICE (_self);

  // stop the io-loop
  if (self->priv->io_channel) {
    if (self->priv->io_source) {
      g_source_remove (self->priv->io_source);
      self->priv->io_source = 0;
    }
    g_io_channel_unref (self->priv->io_channel);
    self->priv->io_channel = NULL;
  }
  return TRUE;
}

//-- wrapper

//-- class internals

static void
btic_input_device_get_property (GObject * const object, const guint property_id,
    GValue * const value, GParamSpec * const pspec)
{
  const BtIcInputDevice *const self = BTIC_INPUT_DEVICE (object);
  return_if_disposed ();
  switch (property_id) {
    case DEVICE_DEVNODE:
      g_value_set_string (value, self->priv->devnode);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
btic_input_device_set_property (GObject * const object, const guint property_id,
    const GValue * const value, GParamSpec * const pspec)
{
  const BtIcInputDevice *const self = BTIC_INPUT_DEVICE (object);
  return_if_disposed ();
  switch (property_id) {
    case DEVICE_DEVNODE:
      self->priv->devnode = g_value_dup_string (value);
      //g_assert(!btic_device_has_controls(BTIC_DEVICE(self)));
      register_controls (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
btic_input_device_dispose (GObject * const object)
{
  const BtIcInputDevice *const self = BTIC_INPUT_DEVICE (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);
  btic_input_device_stop (self);

  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (btic_input_device_parent_class)->dispose (object);
  GST_DEBUG ("  done");
}

static void
btic_input_device_finalize (GObject * const object)
{
  const BtIcInputDevice *const self = BTIC_INPUT_DEVICE (object);

  GST_DEBUG ("!!!! self=%p", self);

  g_free (self->priv->devnode);

  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (btic_input_device_parent_class)->finalize (object);
  GST_DEBUG ("  done");
}

static void
btic_input_device_init (BtIcInputDevice * self)
{
  self->priv = btic_input_device_get_instance_private(self);
}

static void
btic_input_device_class_init (BtIcInputDeviceClass * const klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);
  BtIcDeviceClass *const bticdevice_class = BTIC_DEVICE_CLASS (klass);

  gobject_class->set_property = btic_input_device_set_property;
  gobject_class->get_property = btic_input_device_get_property;
  gobject_class->dispose = btic_input_device_dispose;
  gobject_class->finalize = btic_input_device_finalize;

  bticdevice_class->start = btic_input_device_start;
  bticdevice_class->stop = btic_input_device_stop;

  g_object_class_install_property (gobject_class, DEVICE_DEVNODE,
      g_param_spec_string ("devnode", "devnode prop", "device node path", NULL,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}
