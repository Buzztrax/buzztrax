/* Buzztrax
 * Copyright (C) 2010 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * SECTION:bt-dec
 * @title: BtDec
 * @short_description: buzztrax song playback bin
 *
 * Element to play buzztrax songs.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 filesrc location=$HOME/buzztrax/share/buzztrax/songs/melo3.xml ! typefind ! buzztrax-dec ! autoaudiosink
 * ]| Play a buzztrax song.
 * </refsect2>
 */

/*
 * GST_DEBUG="*:3,bt*:4" gst-launch-1.0 -v filesrc location=$HOME/buzztrax/share/buzztrax/songs/melo3.xml ! buzztrax-dec ! fakesink
 * GST_DEBUG="*:3,bt*:4" gst-launch-1.0 playbin uri=file://$HOME/buzztrax/share/buzztrax/songs/melo3.xml
 * ~/projects/gstreamer/gst-plugins-base/tests/examples/seek/.libs/seek 16 file:///home/ensonic/buzztrax/share/buzztrax/songs/lac2010_01a.bzt
 *
 * GST_DEBUG="*:3,bt*:4,*type*:4" gst-launch-1.0 -v -m filesrc location=$HOME/buzztrax/share/buzztrax/songs/melo3.xml ! typefind ! fakesink
 * GST_DEBUG="*:2,bt*:4,*type*:5,default:5" gst-launch-1.0 filesrc location=$HOME/buzztrax/share/buzztrax/songs/melo3.xml ! typefind ! fakesink
 *
 * gst-typefind $HOME/buzztrax/share/buzztrax/songs/303.bzt
 */

/* description:
 * - we use an buffer to receive the whole song-data
 * - on EOS/buffer-filled we parse the song (and drop the eos).
 * - we use a fakesink in sink-bin
 * - we take the buffers from it and push them on our src pad
 * - this way we can keep the song-as a top-level pipeline.
 * todo
 * - check for stopped and send eos?
 * - change bt-bin to be a normal GstElement (no need to be a bin)
 * issues
 * - we depend on a running main-loop (for notify::is-playing)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "bt-dec.h"
#include <gio/gio.h>

#define GST_CAT_DEFAULT bt_dec_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

static GstStaticPadTemplate bt_dec_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-buzztrax")
    );

static GstStaticPadTemplate bt_dec_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " GST_AUDIO_NE (F32) ", "
        "layout = (string) interleaved, "
        "rate = (int) [ 1, MAX ], " "channels = (int) [1, 2] ")
    );

//-- helpers

static gchar *bt_dec_type_find_helper (const guint8 * data, gsize length);

//-- local application subclass

#define BT_TYPE_DEC_APPLICATION  (bt_dec_application_get_type ())

typedef struct _BtDecApplication
{
  const BtApplication parent;
} BtDecApplication;

typedef struct _BtDecApplicationClass
{
  const BtApplicationClass parent;
} BtDecApplicationClass;

G_DEFINE_TYPE (BtDecApplication, bt_dec_application, BT_TYPE_APPLICATION);

static void
bt_dec_application_init (BtDecApplication * self)
{
}

static void
bt_dec_application_class_init (BtDecApplicationClass * klass)
{
}

//-- the element class

G_DEFINE_TYPE (BtDec, bt_dec, GST_TYPE_ELEMENT);

static gboolean
bt_dec_src_query (GstPad * pad, GstObject * parent, GstQuery * query)
{
  gboolean res = TRUE;
  BtDec *self = BT_DEC (parent);

  if (!self->song) {
    gst_object_unref (self);
    return FALSE;
  }

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_DURATION:{
      gst_query_set_duration (query, self->segment.format,
          self->segment.duration);
      break;
    }
    default:
      res = gst_pad_query_default (pad, parent, query);
      break;
  }
  return res;
}

static gboolean
bt_dec_do_seek (BtDec * self, GstEvent * event)
{
  gdouble rate;
  GstFormat src_format;
  GstSeekFlags flags;
  GstSeekType start_type, stop_type;
  gint64 start, stop;
  GstEvent *tevent;
  guint32 seqnum;

  if (!self->song)
    return FALSE;

  gst_event_parse_seek (event, &rate, &src_format, &flags,
      &start_type, &start, &stop_type, &stop);

  if ((start_type == GST_SEEK_TYPE_SET) && (src_format == GST_FORMAT_TIME)) {
    BtSongInfo *song_info;
    gulong row;
    gboolean flush;
    GstSegment seeksegment;
    gboolean update;

    flush = ((flags & GST_SEEK_FLAG_FLUSH) == GST_SEEK_FLAG_FLUSH);
    seqnum = gst_event_get_seqnum (event);

    memcpy (&seeksegment, &self->segment, sizeof (GstSegment));

    if (flush) {
      GST_DEBUG_OBJECT (self, "flush start");
      tevent = gst_event_new_flush_start ();
      gst_event_set_seqnum (tevent, seqnum);
      gst_pad_push_event (self->srcpad, tevent);
    }

    /* seek */
    g_object_get (self->song, "song-info", &song_info, NULL);
    row = bt_song_info_time_to_tick (song_info, start);
    g_object_set (self->song, "play-pos", row, "play-rate", rate, NULL);
    GST_INFO_OBJECT (self, "seeked to sequence row %lu", row);
    start = bt_song_info_tick_to_time (song_info, row);
    g_object_unref (song_info);

    if (flush) {
      GST_DEBUG_OBJECT (self, "flush stop");
      tevent = gst_event_new_flush_stop (TRUE);
      gst_event_set_seqnum (tevent, seqnum);
      gst_pad_push_event (self->srcpad, tevent);
    }

    /* update our real segment */
    GST_OBJECT_LOCK (self);
    memcpy (&self->segment, &seeksegment, sizeof (GstSegment));
    GST_OBJECT_UNLOCK (self);

    /* prepare newsegment event */
    gst_segment_do_seek (&seeksegment, rate, src_format, flags, start_type,
        start, stop_type, stop, &update);

    /* for deriving a stop position for the playback segment from the seek
     * segment, we must take the duration when the stop is not set */
    if ((stop = seeksegment.stop) == -1)
      stop = seeksegment.duration;

    if (self->newsegment_event)
      gst_event_unref (self->newsegment_event);
    self->newsegment_event = gst_event_new_segment (&seeksegment);
    gst_event_set_seqnum (self->newsegment_event, seqnum);
    GST_INFO_OBJECT (self, "newsegment event prepared %" GST_PTR_FORMAT,
        self->newsegment_event);

    return TRUE;
  } else {
    GST_INFO_OBJECT (self, "not seeking seeking, wrong type %d or format %d",
        start_type, src_format);
  }
  return FALSE;
}

static gboolean
bt_dec_src_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  gboolean res = FALSE;
  BtDec *self = BT_DEC (parent);

  GST_INFO_OBJECT (pad, "event received %" GST_PTR_FORMAT, event);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEEK:
      res = bt_dec_do_seek (self, event);
      break;
    default:
      res = gst_pad_event_default (pad, parent, event);
      break;
  }

  return res;
}

static void
on_song_is_playing_notify (const BtSong * song, GParamSpec * arg,
    gpointer user_data)
{
  BtDec *self = BT_DEC (user_data);
  gboolean is_playing;

  g_object_get ((gpointer) song, "is-playing", &is_playing, NULL);
  GST_INFO_OBJECT (self, "is_playing: %d", is_playing);
  if (!is_playing && self->srcpad) {
    GST_INFO_OBJECT (self, "sending eos");
    gst_pad_push_event (self->srcpad, gst_event_new_eos ());
  }
}

static GstPadProbeReturn
bt_dec_move_buffer (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  BtDec *self = BT_DEC (user_data);
  GstBuffer *buf = (GstBuffer *) info->data;
  GstClockTime start, duration;
  gint64 position;

  if (G_UNLIKELY (self->newsegment_event)) {
    GST_INFO_OBJECT (self, "sending newsegment");
    gst_pad_push_event (self->srcpad, self->newsegment_event);
    self->newsegment_event = NULL;
  }

  /* update segment */
  start = GST_BUFFER_TIMESTAMP (buf);
  duration = GST_BUFFER_DURATION (buf);

  if (GST_CLOCK_TIME_IS_VALID (start))
    position = start;
  else
    position = self->segment.position;

  if (GST_CLOCK_TIME_IS_VALID (duration)) {
    if (self->segment.rate >= 0.0)
      position += duration;
    else if (position > duration)
      position -= duration;
    else
      position = 0;
  }

  if (G_LIKELY (position < self->segment.duration)) {
    GST_OBJECT_LOCK (self);
    self->segment.position = position;
    GST_OBJECT_UNLOCK (self);

    gst_pad_push (self->srcpad, gst_buffer_ref (buf));
  } else {
    gst_pad_push_event (self->srcpad, gst_event_new_eos ());
  }

  /* don't push further */
  return GST_PAD_PROBE_DROP;
}

static GstPadProbeReturn
bt_dec_move_event (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  BtDec *self = BT_DEC (user_data);
  GstEvent *event = (GstEvent *) info->data;

  GST_INFO_OBJECT (pad, "forwarding event %" GST_PTR_FORMAT, event);

  if (GST_EVENT_IS_DOWNSTREAM (event)) {
    switch (GST_EVENT_TYPE (event)) {
        /*
           case GST_EVENT_FLUSH_START:
           case GST_EVENT_FLUSH_STOP:
           // eat flush events to avoid duplicates ?
           break;
         */
      default:
        gst_pad_push_event (self->srcpad, gst_event_ref (event));
        break;
    }
  }
  return GST_PAD_PROBE_OK;
}

static gboolean
bt_dec_load_song (BtDec * self)
{
  gboolean res = FALSE;
  BtSongIO *loader = NULL;
  GstMapInfo map;
  GstCaps *caps;
  GstStructure *s;
  GError *err = NULL;
  const gchar *media_type = "audio/x-buzztrax";

  GST_DEBUG_OBJECT (self, "song loaded");

  gst_buffer_map (self->buffer, &map, GST_MAP_READ);

  caps = gst_pad_get_current_caps (self->sinkpad);
  GST_INFO_OBJECT (self, "input caps %" GST_PTR_FORMAT, caps);
  if (caps) {
    if (!GST_CAPS_IS_SIMPLE (caps))
      goto Error;

    s = gst_caps_get_structure (caps, 0);
    media_type = gst_structure_get_string (s, "format");
    gst_caps_unref (caps);
  } else {
    /* it seems that our type-finder was not run */
    media_type = bt_dec_type_find_helper (map.data, self->song_size);
  }
  GST_INFO_OBJECT (self, "about to load buzztrax song in %s format",
      media_type);

  /* create song-loader */
  if ((loader = bt_song_io_from_data ((gpointer) map.data, self->song_size,
              media_type, &err))) {
    if (bt_song_io_load (loader, self->song, &err)) {
      BtSetup *setup;
      BtSequence *sequence;
      BtSongInfo *song_info;
      BtMachine *machine;

      g_object_get (self->song, "setup", &setup, "sequence", &sequence,
          "song-info", &song_info, NULL);
      /* turn off loops in any case */
      g_object_set (sequence, "loop", FALSE, NULL);
      GST_OBJECT_LOCK (self);
      self->segment.duration = bt_song_info_tick_to_time (song_info,
          bt_sequence_get_loop_length (sequence));
      GST_OBJECT_UNLOCK (self);

      if ((machine =
              bt_setup_get_machine_by_type (setup, BT_TYPE_SINK_MACHINE))) {
        BtSinkBin *sink_bin;
        GstPad *target_pad;
        GstPad *probe_pad;
        GstElementClass *klass = GST_ELEMENT_GET_CLASS (self);
        GstElement *fakesink;
        GstPadLinkReturn plr;

        g_object_get (machine, "machine", &sink_bin, NULL);
        g_object_set (sink_bin, "mode", BT_SINK_BIN_MODE_PASS_THRU, NULL);

        target_pad = gst_element_get_static_pad (GST_ELEMENT (sink_bin), "src");
        /* bahh, dirty ! */
        fakesink = gst_element_factory_make ("fakesink", NULL);
        /* otherwise the song is not starting .. */
        g_object_set (fakesink,
            "async", FALSE, "enable-last-sample", FALSE, "silent", TRUE,
            /*"sync", TRUE, */
            NULL);
        gst_bin_add (GST_BIN (machine), fakesink);
        probe_pad = gst_element_get_static_pad (fakesink, "sink");
        if (GST_PAD_LINK_FAILED (plr = gst_pad_link (target_pad, probe_pad))) {
          GST_WARNING_OBJECT (self, "can't link %s:%s with %s:%s: %d",
              GST_DEBUG_PAD_NAME (target_pad), GST_DEBUG_PAD_NAME (probe_pad),
              plr);
        }
        gst_pad_add_probe (probe_pad, GST_PAD_PROBE_TYPE_BUFFER,
            bt_dec_move_buffer, (gpointer) self, NULL);
        gst_pad_add_probe (probe_pad, GST_PAD_PROBE_TYPE_EVENT_BOTH,
            bt_dec_move_event, (gpointer) self, NULL);
        gst_object_unref (probe_pad);
        gst_object_unref (target_pad);

        self->srcpad =
            gst_pad_new_from_template (gst_element_class_get_pad_template
            (klass, "src"), "src");
        gst_pad_set_query_function (self->srcpad, bt_dec_src_query);
        gst_pad_set_event_function (self->srcpad, bt_dec_src_event);
        gst_pad_set_active (self->srcpad, TRUE);
        gst_element_add_pad (GST_ELEMENT (self), self->srcpad);

        gst_element_no_more_pads (GST_ELEMENT (self));

        GST_INFO_OBJECT (self, "ghost pad connected");

        self->newsegment_event = gst_event_new_segment (&self->segment);

        GST_INFO_OBJECT (self, "prepared initial new segment");

        gst_object_unref (sink_bin);
        g_object_unref (machine);
        res = TRUE;
      }

      g_object_unref (song_info);
      g_object_unref (sequence);
      g_object_unref (setup);
    } else {
      GST_WARNING_OBJECT (self, "could not load song: %s", err->message);
      g_error_free (err);
    }
  } else {
    GST_WARNING_OBJECT (self, "could not create song-io: %s", err->message);
    g_error_free (err);
  }
  gst_buffer_unmap (self->buffer, &map);
  gst_buffer_unref (self->buffer);
  self->buffer = NULL;
  self->offset = 0;

Error:
  if (loader) {
    g_object_unref (loader);
  }
  return res;
}

static gboolean
bt_dec_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  gboolean res = FALSE;
  BtDec *self = BT_DEC (parent);

  GST_INFO_OBJECT (pad, "event received %" GST_PTR_FORMAT, event);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_EOS:
      GST_DEBUG_OBJECT (self, "song loaded");
      /* don't forward the event */
      gst_event_unref (event);
      res = TRUE;
      break;
    default:
      if (self->srcpad) {
        res = gst_pad_push_event (self->srcpad, event);
      } else {
        res = gst_pad_event_default (pad, parent, event);
      }
      break;
  }

  return res;
}

static void
bt_dec_append_data (BtDec * self, GstBuffer * buffer)
{
  GstMapInfo map;

  gst_buffer_map (buffer, &map, GST_MAP_READ);
  GST_DEBUG_OBJECT (self, "storing buffer, size = %" G_GSIZE_FORMAT, map.size);
  gst_buffer_fill (self->buffer, self->offset, map.data, map.size);
  self->offset += map.size;
  gst_buffer_unmap (buffer, &map);
  gst_buffer_unref (buffer);
}

static gboolean
bt_dec_get_upstream_size (BtDec * self, gint64 * length)
{
  gboolean res = FALSE;
  GstPad *peer;

  peer = gst_pad_get_peer (self->sinkpad);
  if (peer == NULL)
    return FALSE;

  if (gst_pad_query_duration (peer, GST_FORMAT_BYTES, length) && *length >= 0) {
    res = TRUE;
  }

  gst_object_unref (peer);
  return res;
}

static gboolean
bt_dec_init_song_buffer (BtDec * self)
{
  GST_DEBUG_OBJECT (self, "checking size");
  if (!bt_dec_get_upstream_size (self, &self->song_size)) {
    GST_ELEMENT_ERROR (self, STREAM, DECODE, (NULL), ("Unable to load song"));
    return FALSE;
  }
  GST_DEBUG_OBJECT (self, "got size: %" G_GINT64_FORMAT, self->song_size);

  if (self->buffer) {
    gst_buffer_unref (self->buffer);
  }
  self->buffer = gst_buffer_new_and_alloc (self->song_size);
  self->offset = 0;
  return TRUE;
}

static GstFlowReturn
bt_dec_chain (GstPad * sinkpad, GstObject * parent, GstBuffer * buffer)
{
  BtDec *self = BT_DEC (GST_PAD_PARENT (sinkpad));

  GST_DEBUG_OBJECT (self, "loading song");

  /* first, get the size of the song */
  if (!self->song_size) {
    if (!bt_dec_init_song_buffer (self)) {
      return GST_FLOW_ERROR;
    }
  }

  /* just append data to internal buffer, we play when we get eos */
  bt_dec_append_data (self, buffer);
  if (self->offset >= self->song_size) {
    if (bt_dec_load_song (self)) {
      GST_DEBUG_OBJECT (self, "start to play");
      bt_song_play (self->song);
    } else {
      GST_ELEMENT_ERROR (self, STREAM, WRONG_TYPE, (NULL),
          ("failed to load the song"));
    }
  }

  return GST_FLOW_OK;
}

static void
bt_dec_loop (GstPad * sinkpad)
{
  BtDec *self = BT_DEC (GST_PAD_PARENT (sinkpad));
  GstFlowReturn ret;
  GstBuffer *buffer = NULL;

  /* first, get the size of the song */
  if (!self->song_size) {
    if (!bt_dec_init_song_buffer (self)) {
      ret = GST_FLOW_ERROR;
      goto pause;
    }
  }

  if (self->buffer) {
    ret = gst_pad_pull_range (self->sinkpad, self->offset, -1, &buffer);
    if (ret == GST_FLOW_EOS) {
      if (bt_dec_load_song (self)) {
        GST_DEBUG_OBJECT (self, "start to play");
        bt_song_play (self->song);
        ret = GST_FLOW_OK;
      } else {
        GST_ELEMENT_ERROR (self, STREAM, WRONG_TYPE, (NULL),
            ("failed to load the song"));
      }
      goto pause;
    } else if (ret != GST_FLOW_OK) {
      GST_ELEMENT_ERROR (self, STREAM, DECODE, (NULL), ("Unable to read song"));
      goto pause;
    } else {
      bt_dec_append_data (self, buffer);
      /* actually load it */
      if (self->offset >= self->song_size) {
        if (bt_dec_load_song (self)) {
          GST_DEBUG_OBJECT (self, "start to play");
          bt_song_play (self->song);
        } else {
          GST_ELEMENT_ERROR (self, STREAM, WRONG_TYPE, (NULL),
              ("failed to load the song"));
        }
      }
    }
  }

  return;

pause:
  {
    const gchar *reason = gst_flow_get_name (ret);
    GstEvent *event;

    GST_DEBUG_OBJECT (self, "pausing task, reason %s", reason);
    gst_pad_pause_task (sinkpad);
#if 0
    /* this is never executed, see above */
    if (ret == GST_FLOW_EOS) {
      if (self->segment.flags & GST_SEEK_FLAG_SEGMENT) {
        gint64 stop;
        GstMessage *message;

        /* for segment playback we need to post when (in stream time)
         * we stopped, this is either stop (when set) or the duration. */
        if ((stop = self->segment.stop) == -1)
          stop = self->segment.duration;

        GST_LOG_OBJECT (self, "Sending segment done, at end of segment");
        message = gst_message_new_segment_done (GST_OBJECT (self),
            GST_FORMAT_TIME, stop);
        gst_element_post_message (GST_ELEMENT (self), message);
      } else {
        /* perform EOS logic */
        GST_LOG_OBJECT (self, "Sending EOS, at end of stream");
        event = gst_event_new_eos ();
        gst_pad_push_event (self->srcpad, event);
      }
    } else
#endif
    if (ret == GST_FLOW_NOT_LINKED || ret < GST_FLOW_EOS) {
      event = gst_event_new_eos ();
      /* for fatal errors we post an error message, post the error
       * first so the app knows about the error first. */
      GST_ELEMENT_ERROR (self, STREAM, FAILED,
          ("Internal data flow error."),
          ("streaming task paused, reason %s (%d)", reason, ret));
      gst_pad_push_event (self->srcpad, event);
    }
  }
}

static gboolean
bt_dec_activate (GstPad * sinkpad, GstObject * parent)
{
  GstSchedulingFlags sched_flags;
  GstQuery *query;
  gboolean pull_mode;

  query = gst_query_new_scheduling ();
  if (!gst_pad_peer_query (sinkpad, query)) {
    gst_query_unref (query);
    goto push;
  }

  gst_query_parse_scheduling (query, &sched_flags, NULL, NULL, NULL);

  pull_mode = gst_query_has_scheduling_mode (query, GST_PAD_MODE_PULL)
      && ((sched_flags & GST_SCHEDULING_FLAG_SEEKABLE) != 0);

  gst_query_unref (query);

  if (!pull_mode)
    goto push;

  GST_INFO_OBJECT (sinkpad, "activating in pull mode");
  return gst_pad_activate_mode (sinkpad, GST_PAD_MODE_PULL, TRUE);

push:
  GST_INFO_OBJECT (sinkpad, "activating in push mode");
  return gst_pad_activate_mode (sinkpad, GST_PAD_MODE_PUSH, TRUE);
}

static gboolean
bt_dec_activate_mode (GstPad * pad, GstObject * parent,
    GstPadMode mode, gboolean active)
{
  gboolean res;

  switch (mode) {
    case GST_PAD_MODE_PUSH:
      res = TRUE;
      break;
    case GST_PAD_MODE_PULL:
      if (active) {
        res = gst_pad_start_task (pad, (GstTaskFunction) bt_dec_loop, pad,
            NULL);
      } else {
        res = gst_pad_stop_task (pad);
      }
      break;
    default:
      res = FALSE;
      break;
  }
  return res;
}

static void
bt_dec_reset (BtDec * self)
{
  GST_INFO_OBJECT (self, "reset");

  self->offset = 0;
  self->song_size = 0;
  //self->discont = FALSE;

  if (self->buffer) {
    gst_buffer_unref (self->buffer);
    self->buffer = NULL;
  }
  gst_event_replace (&self->newsegment_event, NULL);

  if (self->srcpad) {
    gst_pad_set_active (self->srcpad, FALSE);
    gst_element_remove_pad (GST_ELEMENT (self), self->srcpad);
    self->srcpad = NULL;
  }
}

static GstStateChangeReturn
bt_dec_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  BtDec *self = BT_DEC (element);

  GST_INFO_OBJECT (self, "state change on the bin: %s -> %s",
      gst_element_state_get_name (GST_STATE_TRANSITION_CURRENT (transition)),
      gst_element_state_get_name (GST_STATE_TRANSITION_NEXT (transition)));

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      //bt_song_play (self->song);
      break;
    default:
      break;
  }

  ret =
      GST_ELEMENT_CLASS (bt_dec_parent_class)->change_state (element,
      transition);

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      bt_song_pause (self->song);
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      /* this causes a deadlock if called in PLAYING_TO_PAUSED */
      bt_song_stop (self->song);
      if ((gst_element_set_state (GST_ELEMENT (self->bin),
                  GST_STATE_NULL)) == GST_STATE_CHANGE_FAILURE) {
        GST_WARNING ("can't go to null state");
      }
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      bt_dec_reset (self);
      break;
    default:
      break;
  }

  return ret;
}


static void
bt_dec_dispose (GObject * object)
{
  BtDec *self = BT_DEC (object);

  bt_dec_reset (self);

  g_clear_object (&self->song);
  g_object_unref (self->app);

  G_OBJECT_CLASS (bt_dec_parent_class)->dispose (object);
}

static void
bt_dec_class_init (BtDecClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;

  gobject_class->dispose = bt_dec_dispose;

  gstelement_class->change_state = bt_dec_change_state;

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&bt_dec_sink_template));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&bt_dec_src_template));
  gst_element_class_set_static_metadata (gstelement_class,
      "BtDec",
      "Codec/Decoder/Audio",
      "Buzztrax song player", "Stefan Kost <ensonic@users.sf.net>");
}

static void
bt_dec_init (BtDec * self)
{
  GstElementClass *klass = GST_ELEMENT_GET_CLASS (self);

  gst_segment_init (&self->segment, GST_FORMAT_TIME);

  self->app = g_object_new (BT_TYPE_DEC_APPLICATION, NULL);
  self->song = bt_song_new (self->app);
  g_signal_connect_object (self->song, "notify::is-playing",
      G_CALLBACK (on_song_is_playing_notify), (gpointer) self, 0);
  g_object_get (self->app, "bin", &self->bin, NULL);

  self->sinkpad =
      gst_pad_new_from_template (gst_element_class_get_pad_template (klass,
          "sink"), "sink");
  gst_pad_set_activate_function (self->sinkpad, bt_dec_activate);
  gst_pad_set_activatemode_function (self->sinkpad, bt_dec_activate_mode);
  gst_pad_set_event_function (self->sinkpad, bt_dec_sink_event);
  gst_pad_set_chain_function (self->sinkpad, bt_dec_chain);
  gst_element_add_pad (GST_ELEMENT (self), self->sinkpad);

  /* we add the src-pad dynamically */
}


static gchar *
bt_dec_type_find_helper (const guint8 * data, gsize length)
{
  gchar *tmp, *mimetype;
  const GList *plugins, *node;
  BtSongIOModuleInfo *info;
  guint ix;
  gboolean found_match = FALSE;

  // check it
  tmp = g_content_type_guess (NULL, data, length, NULL);
  if (tmp == NULL || g_content_type_is_unknown (tmp)) {
    GST_DEBUG ("content type is unknown or NULL (type=0x%p)", tmp);
    g_free (tmp);
    return NULL;
  }

  mimetype = g_content_type_get_mime_type (tmp);
  g_free (tmp);

  if (mimetype == NULL)
    return NULL;

  GST_INFO ("Got mimetype '%s'", mimetype);

  plugins = bt_song_io_get_module_info_list ();
  for (node = plugins; (!found_match && node); node = g_list_next (node)) {
    info = (BtSongIOModuleInfo *) node->data;
    ix = 0;
    while (!found_match && info->formats[ix].name) {
      GST_INFO ("  checking '%s'", info->formats[ix].name);
      found_match = !strcmp (mimetype, info->formats[ix].mime_type);
      ix++;
    }
  }
  return (found_match ? mimetype : NULL);
}

static void
bt_dec_type_find (GstTypeFind * tf, gpointer ignore)
{
  gsize length = 16384;
  guint64 tf_length;
  const guint8 *data;
  gchar *mimetype;

  GST_DEBUG ("checking type");

  if ((tf_length = gst_type_find_get_length (tf)) > 0)
    length = MIN (length, tf_length);

  if ((data = gst_type_find_peek (tf, 0, length)) == NULL)
    return;

  if ((mimetype = bt_dec_type_find_helper (data, length))) {
    GST_INFO ("Found a match");
    // just suggest one static type, we can internally differentiate between the
    // different formats we do support
    gst_type_find_suggest_simple (tf, GST_TYPE_FIND_LIKELY, "audio/x-buzztrax",
        "format", G_TYPE_STRING, mimetype, NULL);
  } else {
    GST_INFO ("No match!");
  }
}


static gboolean
plugin_init (GstPlugin * plugin)
{
  const GList *plugins;
  BtSongIOModuleInfo *info;
  gchar *exts = NULL;
  guint j;

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "bt-dec",
      GST_DEBUG_FG_WHITE | GST_DEBUG_BG_BLACK, "buzztrax song renderer");

  if (!bt_init_check (NULL, NULL, NULL, NULL)) {
    GST_WARNING ("failed to init buzztrax library");
    return FALSE;
  }
  if (!btic_init_check (NULL, NULL, NULL)) {
    GST_WARNING ("failed to init buzztrax interaction controller library");
    return FALSE;
  }

  plugins = bt_song_io_get_module_info_list ();
  for (; plugins; plugins = g_list_next (plugins)) {
    info = (BtSongIOModuleInfo *) plugins->data;
    j = 0;
    while (info->formats[j].name) {
      if (info->formats[j].extension) {
        if (exts) {
          gchar *t = g_strconcat (exts, ",", info->formats[j].extension, NULL);
          g_free (exts);
          exts = t;
        } else {
          exts = g_strdup (info->formats[j].extension);
        }
      }
      j++;
    }
  }

  gst_type_find_register (plugin, "audio/x-buzztrax", GST_RANK_SECONDARY,
      bt_dec_type_find, exts, GST_CAPS_ANY, NULL, NULL);
  g_free (exts);

  return gst_element_register (plugin, "bt_dec", GST_RANK_MARGINAL,
      BT_TYPE_DEC);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    buzztraxdec,
    "Buzztrax song renderer",
    plugin_init, VERSION, "LGPL", PACKAGE_NAME, "http://www.buzztrax.org");
