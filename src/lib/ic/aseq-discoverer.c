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
 * SECTION:bticaseqdiscoverer
 * @short_description: alsa based device discovery
 *
 * Discover midi devices using alsa sequencer.
 */

#define BTIC_CORE
#define BTIC_ASEQ_DISCOVERER_C

#include "ic_private.h"
#if USE_ALSA
#include <alsa/asoundlib.h>
#endif

struct _BtIcASeqDiscovererPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  snd_seq_t *seq;
  GHashTable *io;
};

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtIcASeqDiscoverer, btic_aseq_discoverer, G_TYPE_OBJECT,
						 G_ADD_PRIVATE(BtIcASeqDiscoverer));

//-- helper

static gboolean
io_handler (GIOChannel * channel, GIOCondition condition, gpointer user_data)
{
  BtIcASeqDiscoverer *self = BTIC_ASEQ_DISCOVERER (user_data);
  BtIcASeqDiscovererPrivate *p = self->priv;
  gboolean res = TRUE;

  if (condition & (G_IO_IN | G_IO_PRI)) {
    snd_seq_event_t *ev;
    gint err;

    while ((err = snd_seq_event_input (p->seq, &ev)) >= 0) {
      const gchar *cname = "", *pname = "";
      snd_seq_addr_t *addr = NULL;

      GST_INFO ("got event %d, remaining bytes: %d", ev->type, err);

      switch (ev->type) {
        case SND_SEQ_EVENT_PORT_START:
        case SND_SEQ_EVENT_PORT_EXIT:
        case SND_SEQ_EVENT_PORT_CHANGE:{
          snd_seq_client_info_t *cinfo;
          snd_seq_port_info_t *pinfo;
          guint pcaps = 0;

          addr = &ev->data.addr;

          // meh: we don't get names on exit
          snd_seq_client_info_alloca (&cinfo);
          if ((err = snd_seq_get_any_client_info (p->seq, addr->client, cinfo))
              >= 0) {
            cname = snd_seq_client_info_get_name (cinfo);
          } else {
            GST_INFO ("failed to get client info: %s", snd_strerror (err));
          }

          snd_seq_port_info_alloca (&pinfo);
          if ((err = snd_seq_get_any_port_info (p->seq, addr->client,
                      addr->port, pinfo)) >= 0) {
            pname = snd_seq_port_info_get_name (pinfo);
            pcaps = snd_seq_port_info_get_capability (pinfo);
            if ((pcaps & (SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ))
                != (SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ)) {
              GST_INFO ("skipping port event due to caps: 0x%08x", pcaps);
              addr = NULL;
            }
          } else {
            GST_INFO ("failed to get port info: %s", snd_strerror (err));
          }
          break;
        }
          // DEBUG
        case SND_SEQ_EVENT_CLIENT_START:
          GST_INFO ("Client start: %d", ev->data.addr.client);
          break;
        case SND_SEQ_EVENT_CLIENT_EXIT:
          GST_INFO ("Client exit: %d", ev->data.addr.client);
          break;
        case SND_SEQ_EVENT_CLIENT_CHANGE:
          GST_INFO ("Client changed: %d", ev->data.addr.client);
          break;
        case SND_SEQ_EVENT_PORT_SUBSCRIBED:
          GST_INFO ("Port subscribed:  %d:%d -> %d:%d",
              ev->data.connect.sender.client, ev->data.connect.sender.port,
              ev->data.connect.dest.client, ev->data.connect.dest.port);
          break;
        case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
          GST_INFO ("Port unsubscribed:  %d:%d -> %d:%d",
              ev->data.connect.sender.client, ev->data.connect.sender.port,
              ev->data.connect.dest.client, ev->data.connect.dest.port);
          break;
          // DEBUG
        default:
          GST_INFO ("Event %d", ev->type);
          break;
      }
      if (addr) {
        gchar *device_name;
        gchar *udi;

        udi = g_strdup_printf ("/aseq/%d:%d", addr->client, addr->port);
        switch (ev->type) {
          case SND_SEQ_EVENT_PORT_START:
            GST_INFO ("Port start: %d:%d - %s:%s - %s",
                addr->client, addr->port, cname, pname, udi);
            device_name = g_strconcat ("alsa sequencer: ", pname, NULL);
            btic_registry_add_device (BTIC_DEVICE (btic_aseq_device_new (udi,
                        device_name, addr->client, addr->port)));
            g_free (device_name);
            break;
          case SND_SEQ_EVENT_PORT_EXIT:
            GST_INFO ("Port exit: %d:%d - %s:%s - %s",
                addr->client, addr->port, cname, pname, udi);
            btic_registry_remove_device_by_udi (udi);
            break;
          case SND_SEQ_EVENT_PORT_CHANGE:
            GST_INFO ("Port changed: %d:%d - %s:%s - %s",
                addr->client, addr->port, cname, pname, udi);
            // TODO: we need to change the udi, but we can't look up the old
            // udi if we don't know the old port number
            // device = btic_registry_find_device_by_udi (udi);
            break;
          default:
            break;
        }
        g_free (udi);
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

static void
scan_clients (BtIcASeqDiscoverer * self)
{
  BtIcASeqDiscovererPrivate *p = self->priv;
  snd_seq_client_info_t *cinfo;
  snd_seq_port_info_t *pinfo;
  gchar *cat_full_name;
  gchar *udi;

  snd_seq_client_info_alloca (&cinfo);
  snd_seq_port_info_alloca (&pinfo);

  snd_seq_client_info_set_client (cinfo, -1);
  while (snd_seq_query_next_client (p->seq, cinfo) >= 0) {
    gint client = snd_seq_client_info_get_client (cinfo);

    // skip some internal clients
    if (client == SND_SEQ_CLIENT_SYSTEM) {
      continue;
    }

    snd_seq_port_info_set_client (pinfo, client);
    snd_seq_port_info_set_port (pinfo, -1);
    while (snd_seq_query_next_port (p->seq, pinfo) >= 0) {
      /* we need both READ and SUBS_READ */
      if ((snd_seq_port_info_get_capability (pinfo)
              & (SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ))
          == (SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ)) {
        gint port = snd_seq_port_info_get_port (pinfo);

        GST_INFO ("Port start: %d:%d - %s:%s", client, port,
            snd_seq_client_info_get_name (cinfo),
            snd_seq_port_info_get_name (pinfo));

        udi = g_strdup_printf ("/aseq/%d:%d", client, port);
        cat_full_name = g_strconcat ("alsa sequencer: ",
            snd_seq_port_info_get_name (pinfo), NULL);
        btic_registry_add_device (BTIC_DEVICE (btic_aseq_device_new (udi,
                    cat_full_name, client, port)));
        g_free (cat_full_name);
        g_free (udi);
      }
    }
  }
}

static void
alsa_error_handler (const char *file, int line, const char *function,
    int err, const char *fmt, ...)
{
#ifndef GST_DISABLE_GST_DEBUG
  va_list args;
  gchar *str;

  va_start (args, fmt);
  str = g_strdup_vprintf (fmt, args);
  va_end (args);
  gst_debug_log (GST_CAT_DEFAULT, GST_LEVEL_WARNING, file, function, line, NULL,
      "alsalib error: %s%s%s", str, err ? ": " : "",
      err ? snd_strerror (err) : "");
  g_free (str);
#endif
}

//-- constructor methods

/**
 * btic_aseq_discoverer_new:
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtIcASeqDiscoverer *
btic_aseq_discoverer_new (void)
{
  return g_object_new (BTIC_TYPE_ASEQ_DISCOVERER, NULL);
}

//-- methods

//-- wrapper

//-- class internals

static void
btic_aseq_discoverer_finalize (GObject * const object)
{
  const BtIcASeqDiscoverer *const self = BTIC_ASEQ_DISCOVERER (object);

  GST_DEBUG ("!!!! self=%p", self);
  if (self->priv->io) {
    g_hash_table_destroy (self->priv->io);
  }
  if (self->priv->seq) {
    snd_seq_close (self->priv->seq);
  }

  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (btic_aseq_discoverer_parent_class)->finalize (object);
  GST_DEBUG ("  done");
}

static GObject *
btic_aseq_discoverer_constructor (GType type, guint n_construct_params,
    GObjectConstructParam * construct_params)
{
  BtIcASeqDiscoverer *self;
  BtIcASeqDiscovererPrivate *p;
  gint err;
  gint i, npfds, port;
  struct pollfd *pfds;

  self =
      BTIC_ASEQ_DISCOVERER (G_OBJECT_CLASS
      (btic_aseq_discoverer_parent_class)->constructor (type,
          n_construct_params, construct_params));

  if (!btic_registry_active ())
    goto done;

  p = self->priv;

  if ((err = snd_lib_error_set_handler (alsa_error_handler)) < 0) {
    GST_WARNING ("failed to set alsa error handler");
  }

  if ((err = snd_seq_open (&p->seq, "default", SND_SEQ_OPEN_DUPLEX, 0)) < 0) {
    GST_WARNING ("open sequencer failed: %s", snd_strerror (err));
    goto done;
  }

  if ((err = snd_seq_set_client_name (p->seq, PACKAGE "-discoverer")) < 0) {
    GST_WARNING ("set client name failed: %s", snd_strerror (err));
    goto done;
  }

  if ((err = snd_seq_create_simple_port (p->seq, "discoverer",
              SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
              SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION)) <
      0) {
    GST_WARNING ("create port failed: %s", snd_strerror (err));
    goto done;
  }
  port = err;

  if ((err = snd_seq_connect_from (p->seq, port, SND_SEQ_CLIENT_SYSTEM,
              SND_SEQ_PORT_SYSTEM_ANNOUNCE)) < 0) {
    GST_WARNING ("connecting to sys:announce failed: %s", snd_strerror (err));
    goto done;
  }

  if ((err = snd_seq_nonblock (p->seq, 1)) < 0) {
    GST_WARNING ("set nonblock mode failed: %s", snd_strerror (err));
    goto done;
  }
  // npfds is usually 1
  npfds = snd_seq_poll_descriptors_count (p->seq, POLLIN);
  GST_INFO ("alsa sequencer api ready: nr_poll_fds=%d", npfds);

  pfds = alloca (sizeof (*pfds) * npfds);
  snd_seq_poll_descriptors (p->seq, pfds, npfds, POLLIN);

  p->io = g_hash_table_new_full (NULL, NULL,
      (GDestroyNotify) g_io_channel_unref, (GDestroyNotify) g_source_remove);

  // scan already connected clients
  scan_clients (self);

  // watch for new and exiting clients
  for (i = 0; i < npfds; i++) {
    GIOChannel *io_channel = g_io_channel_unix_new (pfds[i].fd);
    gint io_source = g_io_add_watch_full (io_channel,
        G_PRIORITY_LOW,
        G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL,
        io_handler, (gpointer) self, NULL);
    g_hash_table_insert (p->io, io_channel, GINT_TO_POINTER (io_source));
  }

  /*
     ./aseqdump -l
     Port    Client name                      Port name
     0:0    System                           Timer
     0:1    System                           Announce
     14:0    Midi Through                     Midi Through Port-0
     128:0    Virtual Keyboard                 Virtual Keyboard

     ./aseqdump -p0:1
     Source  Event                  Ch  Data
     0:1   Port subscribed            0:1 -> 128:0
     0:1   Client start               client 129
     0:1   Port start                 129:0
     0:1   Port exit                  129:0
     0:1   Client exit                client 129
   */
done:
  return (GObject *) self;
}

static void
btic_aseq_discoverer_init (BtIcASeqDiscoverer * self)
{
  self->priv = btic_aseq_discoverer_get_instance_private(self);
}

static void
btic_aseq_discoverer_class_init (BtIcASeqDiscovererClass * const klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructor = btic_aseq_discoverer_constructor;
  gobject_class->finalize = btic_aseq_discoverer_finalize;
}
