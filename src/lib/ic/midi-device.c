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
 * SECTION:bticmididevice
 * @short_description: buzztraxs interaction controller midi device
 *
 * Event handling for midi devices.
 */
/*
 * Linux MIDI-HOWTO: http://www.midi-howto.com/
 *
 * use sysex to get device ids (see _constructed)
 * http://www.borg.com/~jglatt/tech/midispec/identity.htm
 * http://en.wikipedia.org/wiki/MIDI_Machine_Control#Identity_Request
 *
 * to test midi inputs
 * amidi -l
 * amidi -d -p hw:2,0,0
 *
 * support reason midi-profiles:
 * http://www.propellerheads.se/substance/discovering-reason/index.cfm?fuseaction=get_article&article=part33
 */
#define BTIC_CORE
#define BTIC_MIDI_DEVICE_C

#include "ic_private.h"
#include <glib/gprintf.h>

enum
{
  DEVICE_DEVNODE = 1,
  DEVICE_CONTROLCHANGE
};

struct _BtIcMidiDevicePrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  gchar *devnode;

  /* io channel */
  GIOChannel *io_channel;
  guint io_source;

  /* learn-mode members */
  gboolean learn_mode;
  guint learn_key;
  guint learn_bits;
  gchar *control_change;
};

//-- the class

static void btic_midi_device_interface_init (gpointer const g_iface,
    gpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtIcMidiDevice, btic_midi_device, BTIC_TYPE_DEVICE,
    G_IMPLEMENT_INTERFACE (BTIC_TYPE_LEARN, btic_midi_device_interface_init));

//-- defines

#define MIDI_CMD_MASK            0xf0
#define MIDI_CH_MASK             0x0f
#define MIDI_NOTE_OFF            0x80
#define MIDI_NOTE_ON             0x90
#define MIDI_POLY_AFTER_TOUCH    0xa0
#define MIDI_CONTROL_CHANGE      0xb0
#define MIDI_CHANNEL_AFTER_TOUCH 0xd0
#define MIDI_PITCH_WHEEL_CHANGE  0xe0
#define MIDI_SYS_EX_START        0xf0
#define MIDI_SYS_EX_END          0xf7
#define MIDI_TIMING_CLOCK        0xf8
#define MIDI_TRANSPORT_START     0xfa
#define MIDI_TRANSPORT_CONTINUE  0xfb
#define MIDI_TRANSPORT_STOP      0xfc
#define MIDI_ACTIVE_SENSING      0xfe
#define MIDI_NON_REALTIME        0x7e

#define MIDI_CTRL_PITCH_WHEEL        128
#define MIDI_CTRL_NOTE_KEY           129
#define MIDI_CTRL_NOTE_VELOCITY      130
#define MIDI_CTRL_TRANSPORT_START    140
#define MIDI_CTRL_TRANSPORT_CONTINUE 141
#define MIDI_CTRL_TRANSPORT_STOP     142

//-- helper

static void
update_learn_info (BtIcMidiDevice * self, gchar * name, guint key, guint bits)
{

  if (self->priv->learn_key != key) {
    self->priv->learn_key = key;
    self->priv->learn_bits = bits;
    g_object_set (self, "device-controlchange", name, NULL);
  }
}

//-- handler

static void
log_io_error (GError * error, gchar * event)
{
  GST_WARNING ("iochannel error when reading '%s': %s", event, error->message);
  g_error_free (error);
}

static gboolean
io_handler (GIOChannel * channel, GIOCondition condition, gpointer user_data)
{
  BtIcMidiDevice *self = BTIC_MIDI_DEVICE (user_data);
  BtIcControl *control;
  GError *error = NULL;
  gsize bytes_read;
  guchar midi_event[3], cmd;
  static guchar prev_cmd = 0;
  guint key;
  gboolean res = TRUE;

  if (condition & (G_IO_IN | G_IO_PRI)) {
    g_io_channel_read_chars (self->priv->io_channel, (gchar *) midi_event, 1,
        &bytes_read, &error);
    if (error) {
      GST_WARNING ("iochannel error when reading: %s", error->message);
      g_error_free (error);
      //res=FALSE;
    } else {
      gint have_read = 0;
      guchar *midi_data = &midi_event[1];

      GST_LOG ("command: %02x", midi_event[0]);
      cmd = midi_event[0] & MIDI_CMD_MASK;
      if (cmd < 0x80 && prev_cmd) {
        have_read = 1;
        midi_event[1] = midi_event[0];
        midi_event[0] = prev_cmd;
        midi_data = &midi_event[2];
        cmd = prev_cmd & MIDI_CMD_MASK;
      }
      // http://www.midi.org/techspecs/midimessages.php
      // http://www.cs.cf.ac.uk/Dave/Multimedia/node158.html
      switch (cmd) {
        case MIDI_NOTE_OFF:
          g_io_channel_read_chars (self->priv->io_channel, (gchar *) midi_data,
              2 - have_read, &bytes_read, &error);
          if (error)
            log_io_error (error, "NOTE-OFF");
          else {
            GST_DEBUG ("note-off: %02x %02x %02x", midi_event[0], midi_event[1],
                midi_event[2]);

#if 0
            // we probably need to make this another controller that just sends
            // note-off, sending this as part of the key-controler causes trouble
            // for the enum scaling
            // we still need a way to ensure note-off is send to the voice
            // matching the note, maybe a user_data field in the
            // BtPolyControlData struct
            key = MIDI_CTRL_NOTE_KEY;
            if ((control =
                    btic_device_get_control_by_id (BTIC_DEVICE (self), key))) {
              g_object_set (control, "value", (glong) 255 /* GSTBT_NOTE_OFF */ ,
                  NULL);
            }
            // also handle note-off velocity
#endif
            prev_cmd = midi_event[0];
          }
          break;
        case MIDI_NOTE_ON:
          g_io_channel_read_chars (self->priv->io_channel, (gchar *) midi_data,
              2 - have_read, &bytes_read, &error);
          if (error)
            log_io_error (error, "NOTE-ON");
          else {
            /* this CMD drives two controllers, key and velocity, thus we need
             * to do the lean in two steps
             * TODO(ensonic): maybe we can add a callback and a extra info message
             * to update_learn_info. The info message can tell, that this will name
             * multiple controllers. The callback can actually register them.
             * TODO(ensonic): we could also define one regular abs-range controller
             * for each key - then we can play drums with the key - each drum
             * controlled by one key. The downside is, that this will cause the
             * controller menu to explode.
             *
             * Maybe we should change the machine-window to have tabs:
             * - properties
             * - interactions
             * - settings (the preferences)
             * This would fold the preferences window into the machine-window as
             * a tab. The downside is that some of the toolbar items (about and
             * help) would be related to all tabs, while the preset, randomize,
             * reset ones would be related to the properties tab only.
             * The 'interaction' tab would have a list/tree of parameters
             * and a list/tree of controls. The control list could show if a
             * control is bound already (e.g. to another machine). We would need
             * a drawable between the lists to show the connections with lines.
             *
             * An alternative would be to only list the devices in the
             * controller menu. When selecting a device we ensure its running
             * and set the learn mode. As soon as controls changed, we list them
             * and let the user pick one. For each control we could show where
             * it is bound currently. If we go that route, we would actually not
             * need the controller profiles.
             *
             * If we keep the menu, we should show where to a controller is
             * bound.
             */
            gboolean learn_1st = FALSE;
            GST_DEBUG ("note-on: %02x %02x %02x", midi_event[0], midi_event[1],
                midi_event[2]);

            key = MIDI_CTRL_NOTE_KEY;
            if ((control =
                    btic_device_get_control_by_id (BTIC_DEVICE (self), key))) {
              g_object_set (control, "value", (gint32) (midi_event[1]), NULL);
            } else if (G_UNLIKELY (self->priv->learn_mode)) {
              update_learn_info (self, "note-key", key, 7);
              learn_1st = TRUE;
            }
            key = MIDI_CTRL_NOTE_VELOCITY;
            if ((control =
                    btic_device_get_control_by_id (BTIC_DEVICE (self), key))) {
              g_object_set (control, "value", (gint32) (midi_event[2]), NULL);
            } else if (G_UNLIKELY (self->priv->learn_mode) && !learn_1st) {
              update_learn_info (self, "note-velocity", key, 7);
            }
            prev_cmd = midi_event[0];
          }
          break;
        case MIDI_CONTROL_CHANGE:
          g_io_channel_read_chars (self->priv->io_channel, (gchar *) midi_data,
              2 - have_read, &bytes_read, &error);
          if (error)
            log_io_error (error, "CONTROL-CHANGE");
          else {
            GST_DEBUG ("control-change: %02x %02x %02x", midi_event[0],
                midi_event[1], midi_event[2]);

            key = (guint) midi_event[1];        // 0-119 (normal controls), 120-127 (channel mode message)
            if ((control =
                    btic_device_get_control_by_id (BTIC_DEVICE (self), key))) {
              g_object_set (control, "value", (gint32) (midi_event[2]), NULL);
            } else if (G_UNLIKELY (self->priv->learn_mode)) {
              static gchar name[20];

              snprintf (name, sizeof(name), "control-change %u", key);
              update_learn_info (self, name, key, 7);
            }
            prev_cmd = midi_event[0];
          }
          break;
        case MIDI_PITCH_WHEEL_CHANGE:
          g_io_channel_read_chars (self->priv->io_channel, (gchar *) midi_data,
              2 - have_read, &bytes_read, &error);
          if (error)
            log_io_error (error, "PITCH-WHEEL-CHANGE");
          else {
            GST_DEBUG ("pitch-wheel-change: %02x %02x %02x", midi_event[0],
                midi_event[1], midi_event[2]);

            key = MIDI_CTRL_PITCH_WHEEL;
            if ((control =
                    btic_device_get_control_by_id (BTIC_DEVICE (self), key))) {
              gint32 v = (((gint32) midi_event[2]) << 7) | (midi_event[1]);

              g_object_set (control, "value", v, NULL);
            } else if (G_UNLIKELY (self->priv->learn_mode)) {
              update_learn_info (self, "pitch-wheel-change", key, 14);
            }
            prev_cmd = midi_event[0];
          }
          break;
#if 0
        case MIDI_TRANSPORT_START:
          GST_DEBUG ("transport-start");

          key = MIDI_CTRL_TRANSPORT_START;
          if ((control =
                  btic_device_get_control_by_id (BTIC_DEVICE (self), key))) {
            gint32 v = 1;

            g_object_set (control, "value", v, NULL);
          } else if (G_UNLIKELY (self->priv->learn_mode)) {
            update_learn_info (self, "transport-start", key, 1);
          }
          break;
        case MIDI_TRANSPORT_CONTINUE:
          break;
        case MIDI_TRANSPORT_STOP:
          break;
#endif
        case 0xf0:             /* system common/realtime */
          break;
        default:
          GST_LOG ("unhandled message: %02x", midi_event[0]);
          break;
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
 * btic_midi_device_new:
 * @udi: the udi of the device
 * @name: human readable name
 * @devnode: device node in filesystem
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtIcMidiDevice *
btic_midi_device_new (const gchar * udi, const gchar * name,
    const gchar * devnode)
{
  return BTIC_MIDI_DEVICE (g_object_new (BTIC_TYPE_MIDI_DEVICE, "udi", udi,
          "name", name, "devnode", devnode, NULL));
}

//-- methods

static gboolean
btic_midi_device_start (gconstpointer _self)
{
  BtIcMidiDevice *self = BTIC_MIDI_DEVICE (_self);
  GError *error = NULL;

  GST_INFO ("starting the midi device");

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
    GST_WARNING ("iochannel error for setting encoding to NULL: %s",
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
btic_midi_device_stop (gconstpointer _self)
{
  BtIcMidiDevice *self = BTIC_MIDI_DEVICE (_self);

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

//-- learn interface

static gboolean
btic_midi_device_learn_start (gconstpointer _self)
{
  BtIcMidiDevice *self = BTIC_MIDI_DEVICE (_self);

  self->priv->learn_mode = TRUE;
  btic_device_start (BTIC_DEVICE (self));

  return TRUE;
}

static gboolean
btic_midi_device_learn_stop (gconstpointer _self)
{
  BtIcMidiDevice *self = BTIC_MIDI_DEVICE (_self);

  self->priv->learn_mode = FALSE;
  btic_device_stop (BTIC_DEVICE (self));

  return TRUE;
}

static BtIcControl *
btic_midi_device_register_learned_control (gconstpointer _self,
    const gchar * name)
{
  BtIcControl *control = NULL;
  BtIcMidiDevice *self = BTIC_MIDI_DEVICE (_self);

  GST_INFO ("registering midi control as %s", name);

  /* this avoids that we learn a key again */
  if (!(control =
          btic_device_get_control_by_id (BTIC_DEVICE (self),
              self->priv->learn_key))) {
    control =
        BTIC_CONTROL (btic_abs_range_control_new (BTIC_DEVICE (self), name,
            self->priv->learn_key, 0, (1 << self->priv->learn_bits) - 1, 0));
    btic_learn_store_controller_map (BTIC_LEARN (self));
  }
  return control;
}

static void
btic_midi_device_interface_init (gpointer const g_iface,
    gpointer const iface_data)
{
  BtIcLearnInterface *iface = (BtIcLearnInterface *) g_iface;

  iface->learn_start = btic_midi_device_learn_start;
  iface->learn_stop = btic_midi_device_learn_stop;
  iface->register_learned_control = btic_midi_device_register_learned_control;
}

//-- wrapper

//-- class internals

static void
btic_midi_device_constructed (GObject * object)
{
  const BtIcMidiDevice *const self = BTIC_MIDI_DEVICE (object);

  if (G_OBJECT_CLASS (btic_midi_device_parent_class)->constructed)
    G_OBJECT_CLASS (btic_midi_device_parent_class)->constructed (object);

  btic_learn_load_controller_map (BTIC_LEARN (self));

#if 0
  // also see design/midi/rawmidi.c
  gint io;
  if ((io = open (self->priv->devnode, O_NONBLOCK | O_RDWR | O_SYNC)) > 0) {
    gchar data[17] = { 0, };
    gsize ct;

    data[0] = MIDI_SYS_EX_START;
    data[1] = MIDI_NON_REALTIME;
    data[2] = 0x7f;             // SysEx channel, set to "disregard"
    data[3] = 0x06;             // General Information
    data[4] = 0x01;             // Identity Request
    data[5] = MIDI_SYS_EX_END;
    GST_INFO ("send identity request to: %s", self->priv->devnode);
    if ((ct = write (io, data, 6)) < 6)
      goto done;
    do {
      ct = read (io, data, 15);
    } while (ct == -1);
    GST_MEMDUMP ("reply", (guint8 *) data, 15);
    // 5:   manufacturer id (if 0, then id is next two bytes)
    // 6,7: family code
    // 8,9: model code
    // 10,11,12,13: version number
  done:
    close (io);
  }
#endif
}

static void
btic_midi_device_get_property (GObject * const object, const guint property_id,
    GValue * const value, GParamSpec * const pspec)
{
  const BtIcMidiDevice *const self = BTIC_MIDI_DEVICE (object);
  return_if_disposed ();
  switch (property_id) {
    case DEVICE_DEVNODE:
      g_value_set_string (value, self->priv->devnode);
      break;
    case DEVICE_CONTROLCHANGE:
      g_value_set_string (value, self->priv->control_change);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
btic_midi_device_set_property (GObject * const object, const guint property_id,
    const GValue * const value, GParamSpec * const pspec)
{
  const BtIcMidiDevice *const self = BTIC_MIDI_DEVICE (object);
  return_if_disposed ();
  switch (property_id) {
    case DEVICE_DEVNODE:
      self->priv->devnode = g_value_dup_string (value);
      break;
    case DEVICE_CONTROLCHANGE:
      g_free (self->priv->control_change);
      self->priv->control_change = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
btic_midi_device_dispose (GObject * const object)
{
  const BtIcMidiDevice *const self = BTIC_MIDI_DEVICE (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);
  btic_midi_device_stop (self);

  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (btic_midi_device_parent_class)->dispose (object);
  GST_DEBUG ("  done");
}

static void
btic_midi_device_finalize (GObject * const object)
{
  const BtIcMidiDevice *const self = BTIC_MIDI_DEVICE (object);

  GST_DEBUG ("!!!! self=%p", self);

  g_free (self->priv->devnode);

  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (btic_midi_device_parent_class)->finalize (object);
  GST_DEBUG ("  done");
}

static void
btic_midi_device_init (BtIcMidiDevice * self)
{
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, BTIC_TYPE_MIDI_DEVICE,
      BtIcMidiDevicePrivate);
}

static void
btic_midi_device_class_init (BtIcMidiDeviceClass * const klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);
  BtIcDeviceClass *const bticdevice_class = BTIC_DEVICE_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BtIcMidiDevicePrivate));

  gobject_class->constructed = btic_midi_device_constructed;
  gobject_class->set_property = btic_midi_device_set_property;
  gobject_class->get_property = btic_midi_device_get_property;
  gobject_class->dispose = btic_midi_device_dispose;
  gobject_class->finalize = btic_midi_device_finalize;

  bticdevice_class->start = btic_midi_device_start;
  bticdevice_class->stop = btic_midi_device_stop;

  g_object_class_install_property (gobject_class, DEVICE_DEVNODE,
      g_param_spec_string ("devnode", "devnode prop", "device node path", NULL,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  // override learn interface
  g_object_class_override_property (gobject_class, DEVICE_CONTROLCHANGE,
      "device-controlchange");
}
