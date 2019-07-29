/* Buzztrax
 * Copyright (C) 2014 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * SECTION:bticaseqdevice
 * @short_description: buzztraxs interaction controller alsa sequencer device
 *
 * Event handling for alsa sequencer devices.
 */
#define BTIC_CORE
#define BTIC_ASEQ_DEVICE_C

#include "ic_private.h"
#include <glib/gprintf.h>

enum
{
  DEVICE_CLIENT = 1,
  DEVICE_PORT,
  DEVICE_CONTROLCHANGE
};

struct _BtIcASeqDevicePrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  gint src_port;
  gint dst_client, dst_port;

  /* io channel */
  GHashTable *io;

  /* learn-mode members */
  gboolean learn_mode;
  guint learn_key;
  guint learn_bits;
  gchar *control_change;
};

//-- the class

static void btic_aseq_device_interface_init (gpointer const g_iface,
    gpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtIcASeqDevice, btic_aseq_device, BTIC_TYPE_DEVICE,
    G_IMPLEMENT_INTERFACE (BTIC_TYPE_LEARN, btic_aseq_device_interface_init));

//-- defines

/* We need something outside of the usual values in snd_seq_ev_ctrl_t->param as
 * as key. Both are guint. Traditional midi controler numbers are 7bit. */
#define MIDI_CTRL_NOTE_KEY           128
#define MIDI_CTRL_NOTE_VELOCITY      129

//-- helper

static void
update_learn_info (BtIcASeqDevice * self, gchar * name, guint key, guint bits)
{
  if (self->priv->learn_key != key) {
    self->priv->learn_key = key;
    self->priv->learn_bits = bits;
    g_object_set (self, "device-controlchange", name, NULL);
  }
}

//-- handler

static gboolean
io_handler (GIOChannel * channel, GIOCondition condition, gpointer user_data)
{
  BtIcASeqDevice *self = BTIC_ASEQ_DEVICE (user_data);
  BtIcASeqDeviceClass *klass = BTIC_ASEQ_DEVICE_GET_CLASS (self);
  BtIcASeqDevicePrivate *p = self->priv;

  gboolean res = TRUE;

  if (condition & (G_IO_IN | G_IO_PRI)) {
    snd_seq_event_t *ev;
    gint err;

    // TODO(ensonic): how does alsa now know from which port we'd like to read?
    while ((err = snd_seq_event_input (klass->seq, &ev)) >= 0) {
      BtIcControl *control;
      snd_seq_ev_note_t *note = &ev->data.note;
      snd_seq_ev_ctrl_t *ctrl = &ev->data.control;
      guint key;
      gboolean learn_1st;

      GST_DEBUG ("data: %3d:%-3d : %d", ev->source.client, ev->source.port,
          ev->type);

      switch (ev->type) {
        case SND_SEQ_EVENT_NOTE:
          GST_FIXME ("note: %02x %02x %02x %02x %02x", note->channel,
              note->note, note->velocity, note->off_velocity, note->duration);
          break;
        case SND_SEQ_EVENT_NOTEON:
          GST_DEBUG ("note-on: %02x %02x %02x", note->channel, note->note,
              note->velocity);
          learn_1st = FALSE;

          key = MIDI_CTRL_NOTE_KEY;
          if ((control =
                  btic_device_get_control_by_id (BTIC_DEVICE (self), key))) {
            g_object_set (control, "value", (gint32) (note->note), NULL);
          } else if (G_UNLIKELY (self->priv->learn_mode)) {
            update_learn_info (self, "note-key", key, 7);
            learn_1st = TRUE;
          }
          key = MIDI_CTRL_NOTE_VELOCITY;
          if ((control =
                  btic_device_get_control_by_id (BTIC_DEVICE (self), key))) {
            g_object_set (control, "value", (gint32) (note->velocity), NULL);
          } else if (G_UNLIKELY (self->priv->learn_mode) && !learn_1st) {
            update_learn_info (self, "note-velocity", key, 7);
          }
          break;
        case SND_SEQ_EVENT_NOTEOFF:
          GST_FIXME ("note-off: %02x %02x %02x", note->channel, note->note,
              note->velocity);
          break;
        case SND_SEQ_EVENT_CHANPRESS:
          GST_DEBUG ("channel-pressure: %02x %02x %02x",
              ctrl->channel, ctrl->param, ctrl->value);
          key = ctrl->param;
          if ((control =
                  btic_device_get_control_by_id (BTIC_DEVICE (self), key))) {
            g_object_set (control, "value", (gint32) (ctrl->value), NULL);
          } else if (G_UNLIKELY (self->priv->learn_mode)) {
            update_learn_info (self, "channel-pressure", key, 7);
          }
          break;
        case SND_SEQ_EVENT_PITCHBEND:
          GST_DEBUG ("pitch-wheel-change: %02x %02x %04x",
              ctrl->channel, ctrl->param, ctrl->value);
          key = ctrl->param;
          if ((control =
                  btic_device_get_control_by_id (BTIC_DEVICE (self), key))) {
            g_object_set (control, "value", (gint32) (ctrl->value), NULL);
          } else if (G_UNLIKELY (self->priv->learn_mode)) {
            update_learn_info (self, "pitch-wheel-change", key, 14);
          }
          break;
        case SND_SEQ_EVENT_CONTROLLER:
          GST_DEBUG ("control-change (7): %02x %02x %02x",
              ctrl->channel, ctrl->param, ctrl->value);
          key = ctrl->param;
          if ((control =
                  btic_device_get_control_by_id (BTIC_DEVICE (self), key))) {
            g_object_set (control, "value", (gint32) (ctrl->value), NULL);
          } else if (G_UNLIKELY (self->priv->learn_mode)) {
            static gchar name[30];

            snprintf (name, sizeof(name), "control-change-7bit %u", key);
            update_learn_info (self, name, key, 7);
          }
          break;
        case SND_SEQ_EVENT_CONTROL14:
          GST_DEBUG ("control-change (14): %02x %02x %02x",
              ctrl->channel, ctrl->param, ctrl->value);
          key = ctrl->param;
          if ((control =
                  btic_device_get_control_by_id (BTIC_DEVICE (self), key))) {
            g_object_set (control, "value", (gint32) (ctrl->value), NULL);
          } else if (G_UNLIKELY (self->priv->learn_mode)) {
            static gchar name[30];

            snprintf (name, sizeof(name), "control-change-14bit %u", key);
            update_learn_info (self, name, key, 14);
          }
          break;
        default:
          break;
      }
    }
  }
  if (condition & (G_IO_HUP | G_IO_ERR | G_IO_NVAL)) {
    res = FALSE;
  }
  if (!res) {
    GST_INFO ("closing connection");
    g_hash_table_remove (p->io, channel);
  }
  return res;
}


//-- constructor methods

/**
 * btic_aseq_device_new:
 * @udi: the udi of the device
 * @name: human readable name
 * @client: alsa sequencer client id
 * @port: alsa sequencer port id
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtIcASeqDevice *
btic_aseq_device_new (const gchar * udi, const gchar * name,
    gint client, gint port)
{
  return BTIC_ASEQ_DEVICE (g_object_new (BTIC_TYPE_ASEQ_DEVICE, "udi", udi,
          "name", name, "client", client, "port", port, NULL));
}

//-- methods

static gboolean
btic_aseq_device_start (gconstpointer _self)
{
  BtIcASeqDevice *self = BTIC_ASEQ_DEVICE (_self);
  BtIcASeqDeviceClass *klass = BTIC_ASEQ_DEVICE_GET_CLASS (self);
  BtIcASeqDevicePrivate *p = self->priv;
  gchar *dev_name;
  gint err;
  gint i, npfds;
  struct pollfd *pfds;

  GST_INFO ("starting the alsa sequencer device");

  g_object_get (self, "name", &dev_name, NULL);

  if ((err = snd_seq_create_simple_port (klass->seq, dev_name,
              SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
              SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION)) <
      0) {
    GST_WARNING ("create port failed: %s", snd_strerror (err));
    goto done;
  }
  p->src_port = err;

  if ((err =
          snd_seq_connect_from (klass->seq, p->src_port, p->dst_client,
              p->dst_port)) < 0) {
    GST_WARNING ("connecting to %d:%d failed: %s", p->dst_client, p->dst_port,
        snd_strerror (err));
    goto done;
  }
  // npfds is usually 1
  npfds = snd_seq_poll_descriptors_count (klass->seq, POLLIN);
  GST_INFO ("alsa sequencer api ready: nr_poll_fds=%d", npfds);

  pfds = alloca (sizeof (*pfds) * npfds);
  snd_seq_poll_descriptors (klass->seq, pfds, npfds, POLLIN);

  p->io = g_hash_table_new_full (NULL, NULL,
      (GDestroyNotify) g_io_channel_unref, (GDestroyNotify) g_source_remove);

  // listen to events
  for (i = 0; i < npfds; i++) {
    GIOChannel *io_channel = g_io_channel_unix_new (pfds[i].fd);
    gint io_source = g_io_add_watch_full (io_channel,
        G_PRIORITY_LOW,
        G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL,
        io_handler, (gpointer) self, NULL);
    g_hash_table_insert (p->io, io_channel, GINT_TO_POINTER (io_source));
  }

done:
  g_free (dev_name);
  return TRUE;
}

static gboolean
btic_aseq_device_stop (gconstpointer _self)
{
  BtIcASeqDevice *self = BTIC_ASEQ_DEVICE (_self);
  BtIcASeqDeviceClass *klass = BTIC_ASEQ_DEVICE_GET_CLASS (self);
  BtIcASeqDevicePrivate *p = self->priv;

  // stop the io-loop
  if (p->io) {
    g_hash_table_destroy (p->io);
    p->io = NULL;
  }
  if (p->src_port >= 0) {
    snd_seq_delete_simple_port (klass->seq, p->src_port);
    p->src_port = -1;
  }

  return TRUE;
}

//-- learn interface

static gboolean
btic_aseq_device_learn_start (gconstpointer _self)
{
  BtIcASeqDevice *self = BTIC_ASEQ_DEVICE (_self);

  self->priv->learn_mode = TRUE;
  btic_device_start (BTIC_DEVICE (self));

  return TRUE;
}

static gboolean
btic_aseq_device_learn_stop (gconstpointer _self)
{
  BtIcASeqDevice *self = BTIC_ASEQ_DEVICE (_self);

  self->priv->learn_mode = FALSE;
  btic_device_stop (BTIC_DEVICE (self));

  return TRUE;
}

static BtIcControl *
btic_aseq_device_register_learned_control (gconstpointer _self,
    const gchar * name)
{
  BtIcControl *control = NULL;
  BtIcASeqDevice *self = BTIC_ASEQ_DEVICE (_self);

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
btic_aseq_device_interface_init (gpointer const g_iface,
    gpointer const iface_data)
{
  BtIcLearnInterface *iface = (BtIcLearnInterface *) g_iface;

  iface->learn_start = btic_aseq_device_learn_start;
  iface->learn_stop = btic_aseq_device_learn_stop;
  iface->register_learned_control = btic_aseq_device_register_learned_control;
}

//-- wrapper

//-- class internals

static void
btic_aseq_device_constructed (GObject * object)
{
  const BtIcASeqDevice *const self = BTIC_ASEQ_DEVICE (object);

  if (G_OBJECT_CLASS (btic_aseq_device_parent_class)->constructed)
    G_OBJECT_CLASS (btic_aseq_device_parent_class)->constructed (object);

  btic_learn_load_controller_map (BTIC_LEARN (self));
}

static void
btic_aseq_device_get_property (GObject * const object, const guint property_id,
    GValue * const value, GParamSpec * const pspec)
{
  const BtIcASeqDevice *const self = BTIC_ASEQ_DEVICE (object);
  return_if_disposed ();
  switch (property_id) {
    case DEVICE_CLIENT:
      g_value_set_int (value, self->priv->dst_client);
      break;
    case DEVICE_PORT:
      g_value_set_int (value, self->priv->dst_port);
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
btic_aseq_device_set_property (GObject * const object, const guint property_id,
    const GValue * const value, GParamSpec * const pspec)
{
  const BtIcASeqDevice *const self = BTIC_ASEQ_DEVICE (object);
  return_if_disposed ();
  switch (property_id) {
    case DEVICE_CLIENT:
      self->priv->dst_client = g_value_get_int (value);
      break;
    case DEVICE_PORT:
      self->priv->dst_port = g_value_get_int (value);
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
btic_aseq_device_dispose (GObject * const object)
{
  const BtIcASeqDevice *const self = BTIC_ASEQ_DEVICE (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);
  btic_aseq_device_stop (self);

  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (btic_aseq_device_parent_class)->dispose (object);
  GST_DEBUG ("  done");
}

static void
btic_aseq_device_finalize (GObject * const object)
{
  const BtIcASeqDevice *const self = BTIC_ASEQ_DEVICE (object);

  GST_DEBUG ("!!!! self=%p", self);

  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (btic_aseq_device_parent_class)->finalize (object);
  GST_DEBUG ("  done");
}

static void
btic_aseq_device_init (BtIcASeqDevice * self)
{
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, BTIC_TYPE_ASEQ_DEVICE,
      BtIcASeqDevicePrivate);
  self->priv->src_port = -1;
}

static void
btic_aseq_device_class_init (BtIcASeqDeviceClass * const klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);
  BtIcDeviceClass *const bticdevice_class = BTIC_DEVICE_CLASS (klass);
  gint err;

  g_type_class_add_private (klass, sizeof (BtIcASeqDevicePrivate));

  gobject_class->constructed = btic_aseq_device_constructed;
  gobject_class->set_property = btic_aseq_device_set_property;
  gobject_class->get_property = btic_aseq_device_get_property;
  gobject_class->dispose = btic_aseq_device_dispose;
  gobject_class->finalize = btic_aseq_device_finalize;

  bticdevice_class->start = btic_aseq_device_start;
  bticdevice_class->stop = btic_aseq_device_stop;

  g_object_class_install_property (gobject_class, DEVICE_CLIENT,
      g_param_spec_int ("client", "client prop", "alsa sequencer client id",
          0, G_MAXINT, 0,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, DEVICE_PORT,
      g_param_spec_int ("port", "port prop", "alsa sequencer port id",
          0, G_MAXINT, 0,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  // override learn interface
  g_object_class_override_property (gobject_class, DEVICE_CONTROLCHANGE,
      "device-controlchange");

  if ((err = snd_seq_open (&klass->seq, "default", SND_SEQ_OPEN_DUPLEX, 0)) < 0) {
    GST_WARNING ("open sequencer failed: %s", snd_strerror (err));
    goto done;
  }

  if ((err = snd_seq_set_client_name (klass->seq, PACKAGE "-controller")) < 0) {
    GST_WARNING ("set client name failed: %s", snd_strerror (err));
    goto done;
  }

  if ((err = snd_seq_nonblock (klass->seq, 1)) < 0) {
    GST_WARNING ("set nonblock mode failed: %s", snd_strerror (err));
    goto done;
  }

done:
  return;
}
