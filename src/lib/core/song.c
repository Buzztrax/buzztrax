/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * SECTION:btsong
 * @short_description: class of a song project object (contains #BtSongInfo,
 * #BtSetup, #BtSequence and #BtWavetable)
 *
 * A song is the top-level container object to manage all song-related objects.
 * The #BtSetup contains the machines and their connections, the #BtSequence
 * contains the overall time-line, the #BtWavetable holds a list of audio
 * snippets and the #BtSongInfo has a couple of meta-data items for the song.
 *
 * To load or save a song, use a #BtSongIO object. These implement loading and
 * saving for different file-formats.
 *
 * When creating a new song, one needs to add exactly one #BtSinkMachine to make
 * the song playable.
 *
 * One can seek in a song by setting the #BtSong:play-pos property. Likewise one
 * can watch the property to display the playback position.
 *
 * The #BtSong:play-rate property can be used to change the playback speed and
 * direction.
 */
/* idle looping (needed for playing machines live)
 * - states:
 *   is_playing is_idle
 *   FALSE      FALSE    no playback
 *   FALSE      TRUE     play machines
 *   TRUE       FALSE    play song and machines
 *   true       true     not supported
 *
 * is_playing_changed: // in some of these case we could just seek
 *   if(is_playing) {
 *     if(is_idle)
 *       bt_song_idle_stop();
 *     bt_song_play();
 *   }
 *   else {
 *     bt_song_stop();
 *     if(is_idle)
 *       bt_song_idle_start();
 *   }
 * is_idle_change:
 *   if(is_idle) {
 *     if(!is_playing)
 *       bt_song_idle_start();
 *   }
 *   else {
 *     if(!is_playing)
 *       bt_song_idle_stop();
 *   }
 *
 * - unfortunately we have high latencies in live playback. This is due to the
 *   queues in the wires (see bt_wire_link_machines()). Ideally we skip the
 *   queue for one of wires connected to a src. We could optimize the live
 *   latencies of one machine, by disabling most of the wires on its way. That
 *   will not help though, when we play several machines live (using keyboards)
 *
 * - bt_song_play() and bt_song_idle_start() as well as the stop variants should
 *   be symetric, but they arent. Ideally we call common helpers with different
 *   seek-events for start.
 *
 * - for continous playback we'd like to change this to:
 *     bt_song_play(): call bt_song_idle_stop()
 *     bt_song_stop(): call bt_song_idle_start()
 *     - already is called on EOS
 *   and in another round, just make each of those ensure that the song is in
 *   PLAYING and send a seek. Then we can remove the is-idle property. We'd then
 *   also need to retry to start playback for each wire add/remove.
 *
 */
/* handle async-done when seeking
 * - whenever we send flushing seeks, we need to wait for GST_MESSAGE_ASYNC_DONE
 *   to ensure that the previous flushing seek is done (see design/gst/event.c)
 *   and on_song_async_done(). If we don't do this, the previous seeks can get
 *   canceled
 * - I added a pending_async_seek flag, but since we rate limmit from the UI we
 *   never skipped a seek
 */
/* TODO: make looping seamless:
 * - play with loop enabled:
 *   - apps sends flushing segmented seek
 *   - on segment_done, send segmented seek
 * - imagine this simplified graph and some of the events + msg, and this
 *   notation:
 *     p:xx = pipeline, b:xx = bin, e:xx = element
 *     e.xx = event, m.xx = message
 *       e.flush-start: pushed downstrean, not serialized
 *       e.flush-stop : pushed downstream, serialized
 *   TODO: add e.new_seg below
 * - playback starts:
 *   application        : e.seek
 *   - bus ------------------------------------------------------------------
 *   p:song             : e.seek
 *     b:src-machine    :  e.seek
 *       e:src          :   e.seek, e.flush-start, e.flush-stop, e.new_seg, buffer, buffer, ...
 *       e:tee          :            e.flush-start, e.flush-stop, e.new_seg, buffer, buffer, ...
 *     b:src->sink-wire :             e.flush-start, e.flush-stop, e.new_seg, buffer, buffer, ...
 *       e:queue        :              e.flush-start, e.flush-stop, e.new_seg, buffer, buffer, ...
 *       e:volume       :               e.flush-start, e.flush-stop, e.new_seg, buffer, buffer, ...
 *     b:sink-machine   :                e.flush-start, e.flush-stop, e.new_seg, buffer, buffer, ...
 *       e:adder        :                 e.flush-start, e.flush-stop, e.new_seg, buffer, buffer, ...
 *       e:sink         :                  e.flush-start, e.flush-stop, e.new_seg, buffer, buffer, ...
 * - looping:
 *   application        :    m.seg_done, e.seek
 *   - bus ------------------------------------------------------------------
 *   p:song             :   m.seg_done    e.seek
 *     b:src-machine    :  m.seg_done      e.seek
 *       e:src          : m.seg_done        e.seek, e.new_seg, buffer, buffer, ...
 *       e:tee          :                    e.seek, e.new_seg, buffer, buffer, ...
 *     b:src->sink-wire :                     e.seek, e.new_seg, buffer, buffer, ...
 *       e:queue        :                      e.seek, e.new_seg, buffer, buffer, ...
 *       e:volume       :                       e.seek, e.new_seg, buffer, buffer, ...
 *     b:sink-machine   :                        e.seek, e.new_seg, buffer, buffer, ...
 *       e:adder        :                         e.seek, e.new_seg, buffer, buffer, ...
 *       e:sink         :                          e.seek, e.new_seg, buffer, buffer, ...
 *
 * CHECKED:
 * - using "sync-message::segment-done" does not help
 * - making buffer-time more than twice latency-time does not help either
 *   see sink-bin.c::bt_sink_bin_configure_latency()
 * TODO:
 * - run with realtime scheduling - both help!
 *   sudo chrt -f -p 1 $(pidof lt-buzztrax-edit)
 *   sudo chrt -r -p 1 $(pidof lt-buzztrax-edit)
 * - run with different sinks:
 *   jack: issues with loops !
 * (buzztrax-edit:6784): GStreamer-CRITICAL **: gst_buffer_copy_into: assertion 'bufsize >= offset + size' failed
 * 0:00:35.996753504  6784      0x4384770 ERROR                  audio audio.c:237:gst_audio_buffer_clip: copy_region failed
 *     - only happens if we change the loop after playing,
 *       if we load a song, change the loop and then play, it is good
 *     - still also suffers from the jumpy loops
 *   alsa/pulse: no difference
 * - figure how to check buffers on the sink for:
 *   - discontinuities
 *   - late buffers
 *   - over/under-runs
 *
 * GST_DEBUG_FILE="trace.log" GST_DEBUG_NO_COLOR=1 GST_DEBUG="GST_TRACER:7" GST_TRACERS=stats ./buzztrax-edit
 */
#define BT_CORE
#define BT_SONG_C

#include "core_private.h"
#include <glib/gprintf.h>
#include "gst/tempo.h"

/* if a state change does not happen within this time, cancel playback
 * this time includes prerolling, set to 30 seconds for now */
#define STATE_CHANGE_TIMEOUT 30

/* Something changed in gstreamer to break seek in READY again. The pipeline
 * plays from the right pos, but reports position starting from 0 instead of
 * seek-pos.
 *
 * We've filed the bug on 2014-07-10 (short before 1.4.0?) and switched to seek
 * in paused. This wastes some CPU for preroll and then flushing with the actual
 * seek :/ It also breaks some file formats (e.g. ogg which seems to have
 * duplicated headers) (e.g. on ubuntu trusty which has 1.2.3). But this was
 * fixed in never versions (at least in 1.4.3 (ubuntu ppa)).
 *
 * git log --tags --simplify-by-decoration --pretty="format:%ai %d"
 */
#define GST_BUG_733031

//-- property ids

enum
{
  SONG_APP = 1,
  SONG_BIN,
  SONG_MASTER,
  SONG_SONG_INFO,
  SONG_SEQUENCE,
  SONG_SETUP,
  SONG_WAVETABLE,
  SONG_PLAY_POS,
  SONG_PLAY_RATE,
  SONG_IS_PLAYING,
  SONG_IS_IDLE,
  SONG_IO
};

struct _BtSongPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the parts of the song */
  BtSongInfo *song_info;
  BtSequence *sequence;
  BtSetup *setup;
  BtWavetable *wavetable;

  /* the playback position of the song */
  gulong play_pos, play_beg, play_end;
  gdouble play_rate;

  /* flag to signal playing and idle states */
  gboolean is_playing, is_preparing;
  gboolean is_idle, is_idle_active;

  /* the application that currently uses the song */
  BtApplication *app;
  /* the main gstreamer container element */
  GstBin *bin, *master_bin;
  /* the element that has the clock */
  BtSinkMachine *master;

  /* the query is used in update_playback_position */
  GstQuery *position_query;
  /* seek events */
  GstEvent *play_seek_event;
  GstEvent *loop_seek_event;
  GstEvent *idle_seek_event;
  GstEvent *idle_loop_seek_event;
  /* timeout handlers */
  guint paused_timeout_id, playback_timeout_id;

  /* the song-io plugin during i/o operations */
  BtSongIO *song_io;
};

//-- the class

static void bt_song_persistence_interface_init (gpointer const g_iface,
    gpointer const iface_data);


G_DEFINE_TYPE_WITH_CODE (BtSong, bt_song, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (BT_TYPE_PERSISTENCE,
        bt_song_persistence_interface_init));


//-- helper

#define MAKE_SEEK_EVENT_FL(r,s,e) \
  gst_event_new_seek ((r), GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | \
      GST_SEEK_FLAG_SEGMENT, GST_SEEK_TYPE_SET, (s), GST_SEEK_TYPE_SET, (e))

// If we flush, we drop part of the audio that hae not been rendered
#define MAKE_SEEK_EVENT_L(r,s,e) \
  gst_event_new_seek ((r), GST_FORMAT_TIME, GST_SEEK_FLAG_SEGMENT, \
      GST_SEEK_TYPE_SET, (s), GST_SEEK_TYPE_SET, (e))

#define MAKE_SEEK_EVENT_F(r,s,e) \
  gst_event_new_seek ((r), GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, \
      GST_SEEK_TYPE_SET, (s), GST_SEEK_TYPE_SET, (e))

static void
bt_song_update_play_seek_event (const BtSong * const self)
{
  BtSongPrivate *p = self->priv;
  gboolean loop;
  glong loop_start, loop_end, length;
  gulong play_pos = p->play_pos;
  GstClockTime tick_duration;

  g_object_get (p->sequence, "loop", &loop, "loop-start", &loop_start,
      "loop-end", &loop_end, "length", &length, NULL);
  g_object_get (p->song_info, "tick-duration", &tick_duration, NULL);

  if (loop_start == -1)
    loop_start = 0;
  if (loop_end == -1)
    loop_end = length + 1;
  // can't use bt_sequence_limit_play_pos here as that only clamps and we need
  // to wrap around instead
  if (play_pos >= loop_end)
    play_pos = loop_start;

  // remember end for play and eos
  p->play_beg = play_pos;
  p->play_end = loop_end;

  GST_INFO ("loop %d? %ld ... %ld, length %ld, pos %lu, tick_duration %"
      GST_TIME_FORMAT, loop, loop_start, loop_end, length, play_pos,
      GST_TIME_ARGS (tick_duration));

  if (p->play_seek_event)
    gst_event_unref (p->play_seek_event);
  if (p->loop_seek_event)
    gst_event_unref (p->loop_seek_event);

  /* we need to use FLUSH for play (due to prerolling), otherwise:
     0:00:00.866899000 15884 0x81cee70 DEBUG             basesink gstbasesink.c:1644:gst_base_sink_do_sync:<player> prerolling object 0x818ce90
     0:00:00.866948000 15884 0x81cee70 DEBUG             basesink gstbasesink.c:1493:gst_base_sink_wait_preroll:<player> waiting in preroll for flush or PLAYING
     but not for loop
   */
  if (loop) {
    p->play_seek_event = MAKE_SEEK_EVENT_FL (p->play_rate,
        play_pos * tick_duration, loop_end * tick_duration);
    p->loop_seek_event = MAKE_SEEK_EVENT_L (p->play_rate,
        loop_start * tick_duration, loop_end * tick_duration);
  } else {
    p->play_seek_event = MAKE_SEEK_EVENT_F (p->play_rate,
        play_pos * tick_duration, loop_end * tick_duration);
    p->loop_seek_event = NULL;
  }
}

/*
 * bt_song_seek_to_play_pos:
 * @self: #BtSong to seek
 *
 * Send a new playback segment, that goes from the current playback position to
 * the new end position (loop or song end).
 */
static void
bt_song_seek_to_play_pos (const BtSong * const self)
{
  BtSongPrivate *p = self->priv;
  GstEvent *event;
  gboolean loop;
  glong loop_end, length;
  GstClockTime tick_duration;

  if (!p->is_playing)
    return;

  g_object_get (p->sequence, "loop", &loop, "loop-end", &loop_end,
      "length", &length, NULL);
  g_object_get (p->song_info, "tick-duration", &tick_duration, NULL);

  GST_INFO ("loop %d? %ld, length %ld, tick_duration %" GST_TIME_FORMAT, loop,
      loop_end, length, GST_TIME_ARGS (tick_duration));

  // we need to flush, otherwise mixing goes out of sync
  if (loop) {
    event = MAKE_SEEK_EVENT_FL (1.0, p->play_pos * tick_duration,
        loop_end * tick_duration);
  } else {
    event = MAKE_SEEK_EVENT_F (1.0, p->play_pos * tick_duration,
        (length + 1) * tick_duration);
  }
  if (!(gst_element_send_event (GST_ELEMENT (p->master_bin), event))) {
    GST_WARNING ("element failed to seek to play_pos event");
  }
}

static void
bt_song_change_play_rate (const BtSong * const self)
{
  BtSongPrivate *p = self->priv;
  GstEvent *event;
  gboolean loop;
  glong loop_start, loop_end, length;
  GstClockTime tick_duration;

  if (!p->is_playing)
    return;

  g_object_get (p->sequence, "loop", &loop, "loop-start", &loop_start,
      "loop-end", &loop_end, "length", &length, NULL);
  g_object_get (p->song_info, "tick-duration", &tick_duration, NULL);

  GST_INFO ("rate %lf, loop %d?", self->priv->play_rate, loop);
  bt_song_update_play_seek_event (self);

  /* since 0.10.24 we do have step events (gst_event_new_step), they require an
   * initial seek if the direction has changed */

  // changing the playback rate should mostly affect sinks
  // still we need to flush to avoid adder locking up (fixed in 0.10.35.1 - no?)
  // and we need to give the position to workaround basesrc starting from 0
  if (loop) {
    if (self->priv->play_rate > 0.0) {
      event = MAKE_SEEK_EVENT_FL (p->play_rate, p->play_pos * tick_duration,
          loop_end * tick_duration);
    } else {
      event = MAKE_SEEK_EVENT_FL (p->play_rate, loop_start * tick_duration,
          p->play_pos * tick_duration);
    }
  } else {
    if (self->priv->play_rate > 0.0) {
      event = MAKE_SEEK_EVENT_F (p->play_rate, p->play_pos * tick_duration,
          (length + 1) * tick_duration);
    } else {
      event = MAKE_SEEK_EVENT_F (p->play_rate, G_GINT64_CONSTANT (0),
          p->play_pos * tick_duration);
    }
  }
  if (!(gst_element_send_event (GST_ELEMENT (p->master_bin), event))) {
    GST_WARNING ("element failed to change playback rate");
  }
  GST_INFO ("rate updated");
}

/*
 * bt_song_update_play_seek_event_and_play_pos:
 * @self: #BtSong to seek
 *
 * Prepares a new playback segment, that goes from the new start position (loop
 * or song start) to the new end position (loop or song end).
 * Also calls bt_song_seek_to_play_pos() to update the current playback segment.
 */
static void
bt_song_update_play_seek_event_and_play_pos (const BtSong * const self)
{
  bt_song_update_play_seek_event (self);
  /* the update needs to take the current play-position into account */
  bt_song_seek_to_play_pos (self);
}

/* IDEA(ensonic): dynamically handle stpb (subticks per beat)
 * - we'd like to set stpb=1 for non interactive playback and recording
 * - we'd like to set stpb>1 for:
 *   - idle-loop play
 *   - open machine windows (with controllers assigned)
 *
 * (GST_SECOND*60)
 * ------------------- = 30
 * (bpm*tpb*stpb*1000)
 *
 * (GST_SECOND*60)
 * ------------------- = stpb
 * (bpm*tpb*30*1000000)
 *
 * - maybe we get set target-latency=-1 for no subticks (=1) e.g. when recording
 * - make this a property on the song that merges latency setting + playback mode?
 */
static void
bt_song_send_audio_context (const BtSong * const self)
{
  BtSongPrivate *p = self->priv;
  BtSettings *settings = bt_settings_make ();
  GstContext *ctx;
  gulong bpm, tpb;
  guint latency;
  glong stpb = 0;

  g_object_get (p->song_info, "bpm", &bpm, "tpb", &tpb, NULL);
  g_object_get (settings, "latency", &latency, NULL);
  g_object_unref (settings);

  stpb = (glong) ((GST_SECOND * 60) / (bpm * tpb * latency * GST_MSECOND));
  stpb = MAX (1, stpb);
  GST_INFO ("chosing subticks=%ld from bpm=%lu,tpb=%lu,latency=%u", stpb, bpm,
      tpb, latency);

  ctx = gstbt_audio_tempo_context_new (bpm, tpb, stpb);
  gst_element_set_context ((GstElement *) p->bin, ctx);
  gst_context_unref (ctx);
}

static void
bt_song_send_tags (const BtSong * const self)
{
  GstTagList *const taglist;
  GstTagSetter *tag_setter;
  GstIterator *it;
  gboolean done;
  GValue item = { 0, };

  g_object_get (self->priv->song_info, "taglist", &taglist, NULL);
  GST_DEBUG ("about to send metadata to tagsetters, taglist=%p", taglist);
  it = gst_bin_iterate_all_by_interface (self->priv->bin, GST_TYPE_TAG_SETTER);
  done = FALSE;
  while (!done) {
    switch (gst_iterator_next (it, &item)) {
      case GST_ITERATOR_OK:
        tag_setter = GST_TAG_SETTER (g_value_get_object (&item));
        GST_DEBUG_OBJECT (tag_setter, "sending tags");
        gst_tag_setter_merge_tags (tag_setter, taglist, GST_TAG_MERGE_REPLACE);
        g_value_reset (&item);
        break;
      case GST_ITERATOR_RESYNC:
        gst_iterator_resync (it);
        break;
      case GST_ITERATOR_ERROR:
        GST_WARNING ("wrong parameter for iterator");
        done = TRUE;
        break;
      case GST_ITERATOR_DONE:
        done = TRUE;
        break;
    }
  }
  g_value_unset (&item);
  gst_iterator_free (it);

  gst_element_send_event (GST_ELEMENT (self->priv->bin),
      gst_event_new_tag (taglist));
}

static void
bt_song_send_toc (const BtSong * const self)
{
  GstToc *const toc;
  GstTocSetter *toc_setter;
  GstIterator *it;
  gboolean done;
  GValue item = { 0, };

  g_object_get (self->priv->sequence, "toc", &toc, NULL);
  GST_DEBUG ("about to send metadata to tocsetters, toc=%p", toc);
  it = gst_bin_iterate_all_by_interface (self->priv->bin, GST_TYPE_TOC_SETTER);
  done = FALSE;
  while (!done) {
    switch (gst_iterator_next (it, &item)) {
      case GST_ITERATOR_OK:
        toc_setter = GST_TOC_SETTER (g_value_get_object (&item));
        GST_DEBUG_OBJECT (toc_setter, "sending toc");
        gst_toc_setter_set_toc (toc_setter, toc);
        g_value_reset (&item);
        break;
      case GST_ITERATOR_RESYNC:
        gst_iterator_resync (it);
        break;
      case GST_ITERATOR_ERROR:
        GST_WARNING ("wrong parameter for iterator");
        done = TRUE;
        break;
      case GST_ITERATOR_DONE:
        done = TRUE;
        break;
    }
  }
  g_value_unset (&item);
  gst_iterator_free (it);

  gst_element_send_event (GST_ELEMENT (self->priv->bin),
      gst_event_new_toc (toc, FALSE));
  gst_toc_unref (toc);
}

#ifndef GST_BUG_733031
/*
 * bt_song_send_initial_seek:
 *
 * Only seeks on elements. This is a workaround for making seek-in-ready work.
 */
static gboolean
bt_song_send_initial_seek (const BtSong * const self, GstBin * bin,
    GstEvent * ev)
{
  GstIterator *it;
  GstElement *e;
  gboolean done = FALSE, res = TRUE;
  GValue item = { 0, };

  // gst_bin_iterate_elements (bin) does not fix seek-in-ready
  it = gst_bin_iterate_sources (bin);
  while (!done) {
    switch (gst_iterator_next (it, &item)) {
      case GST_ITERATOR_OK:
        e = GST_ELEMENT (g_value_get_object (&item));
        if (GST_IS_BIN (e)) {
          res &= bt_song_send_initial_seek (self, (GstBin *) e, ev);
        } else {
          GST_INFO_OBJECT (e, "sending initial seek");
          if (!(gst_element_send_event (e, gst_event_ref (ev)))) {
            bt_song_write_to_lowlevel_dot_file (self);
            GST_WARNING_OBJECT (e, "failed to handle seek event");
            res = FALSE;
          }
        }
        g_value_reset (&item);
        break;
      case GST_ITERATOR_RESYNC:
        gst_iterator_resync (it);
        break;
      case GST_ITERATOR_ERROR:
        GST_WARNING ("wrong parameter for iterator");
        done = TRUE;
        break;
      case GST_ITERATOR_DONE:
        done = TRUE;
        break;
    }
  }
  g_value_unset (&item);
  gst_iterator_free (it);
  return res;
}
#endif

//-- handler

static gboolean
on_song_paused_timeout (gpointer user_data)
{
  const BtSong *const self = BT_SONG (user_data);

  if (self->priv->is_preparing) {
    GST_WARNING ("->PAUSED timeout occurred");
    bt_song_write_to_lowlevel_dot_file (self);
    bt_song_stop (self);
  }
  self->priv->paused_timeout_id = 0;
  return FALSE;
}

static void
on_song_segment_done (const GstBus * const bus,
    const GstMessage * const message, gconstpointer user_data)
{
  const BtSong *const self = BT_SONG (user_data);
#ifndef GST_DISABLE_GST_DEBUG
  GstFormat format;
  gint64 position;
  guint32 seek_seqnum = gst_message_get_seqnum ((GstMessage *) message);
  // check how regullar the SEGMENT DONE comes
  static GstClockTime last_ts = 0;
  GstClockTime this_ts = gst_util_get_timestamp ();
  GstClockTimeDiff ts_diff = last_ts ? (this_ts - last_ts) : 0;
  last_ts = this_ts;

  gst_message_parse_segment_done ((GstMessage *) message, &format, &position);
#endif

  GST_INFO
      ("received SEGMENT_DONE (%u) bus message: from %s, with fmt=%s, ts=%"
      GST_TIME_FORMAT " after %" GST_TIME_FORMAT, seek_seqnum,
      GST_OBJECT_NAME (GST_MESSAGE_SRC (message)), gst_format_get_name (format),
      GST_TIME_ARGS (position), GST_TIME_ARGS (ts_diff));

  if (self->priv->is_playing || self->priv->is_idle_active) {
    GstEvent *event;

    if (self->priv->is_playing) {
      event = gst_event_copy (self->priv->loop_seek_event);
    } else {
      event = gst_event_copy (self->priv->idle_loop_seek_event);
    }
    g_assert (event);
#ifndef GST_DISABLE_GST_DEBUG
    seek_seqnum = gst_util_seqnum_next ();
    gst_event_set_seqnum (event, seek_seqnum);
#else
    gst_event_set_seqnum (event, gst_util_seqnum_next ());
#endif
    if (!(gst_element_send_event (GST_ELEMENT (self->priv->master_bin), event))) {
      GST_WARNING ("element failed to handle loop seek event");
    } else {
      GST_INFO ("-> loop (%u)", seek_seqnum);
      /*
         gst_pipeline_set_new_stream_time (GST_PIPELINE (self->priv->bin), 0);
         gst_element_get_state (GST_ELEMENT (self->priv->bin), NULL, NULL, 40 * GST_MSECOND);
       */
    }
  } else {
    GST_WARNING ("song isn't playing/idling ?!?");
  }
}

static void
on_song_eos (const GstBus * const bus, const GstMessage * const message,
    gconstpointer user_data)
{
  const BtSong *const self = BT_SONG (user_data);

  GST_INFO ("received EOS bus message from: %s",
      GST_OBJECT_NAME (GST_MESSAGE_SRC (message)));

  if (GST_MESSAGE_SRC (message) == GST_OBJECT (self->priv->bin)) {
    self->priv->play_pos = self->priv->play_end;
    g_object_notify (G_OBJECT (self), "play-pos");
    GST_INFO ("stopping");
    bt_song_stop (self);
  }
}

static void
on_song_state_changed (const GstBus * const bus, GstMessage * message,
    gconstpointer user_data)
{
  const BtSong *const self = BT_SONG (user_data);
  BtSongPrivate *p = self->priv;

  //GST_WARNING("user_data=%p,<%s>, bin=%p, msg->src=%p,<%s>",
  //  user_data, G_OBJECT_TYPE_NAME(G_OBJECT(user_data)),
  //  p->bin,GST_MESSAGE_SRC(message),G_OBJECT_TYPE_NAME(GST_MESSAGE_SRC(message)));

  if (p->is_idle_active)
    return;

  if (GST_MESSAGE_SRC (message) == GST_OBJECT (p->bin)) {
    GstState oldstate, newstate, pending;

    gst_message_parse_state_changed (message, &oldstate, &newstate, &pending);
    GST_INFO_OBJECT (p->bin, "state change on the bin: %s -> %s, pending: %s",
        gst_element_state_get_name (oldstate),
        gst_element_state_get_name (newstate),
        gst_element_state_get_name (pending));

    switch (GST_STATE_TRANSITION (oldstate, newstate)) {
      case GST_STATE_CHANGE_READY_TO_PAUSED:
        if (!p->is_preparing)
          break;
#ifdef GST_BUG_733031
        // meh, we preroll twice now, this also breaks recording as we send the
        // preroll part twice :/
        if (!(gst_element_send_event (GST_ELEMENT (p->master_bin),
                    gst_event_ref (p->play_seek_event)))) {
          GST_WARNING_OBJECT (p->master_bin,
              "element failed to handle seek event");
        }
#endif
        // ensure that sources set their durations
        g_object_notify (G_OBJECT (p->sequence), "length");

        GstStateChangeReturn res;
        if ((res = gst_element_set_state (GST_ELEMENT (p->bin),
                    GST_STATE_PLAYING)) == GST_STATE_CHANGE_FAILURE) {
          GST_WARNING_OBJECT (p->bin, "can't go to playing state");
        }
        GST_DEBUG_OBJECT (p->bin, "->PLAYING state change returned '%s'",
            gst_element_state_change_return_get_name (res));
        // we're prepared to play
        p->is_preparing = FALSE;
        break;
      case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
        if (!p->is_playing) {
          GST_INFO_OBJECT (p->bin, "playback started");
          p->is_playing = TRUE;
          g_object_notify (G_OBJECT (self), "is-playing");
          // if the song is empty playback is done
          if (!GST_BIN_NUMCHILDREN (p->bin)) {
            GST_INFO_OBJECT (p->bin, "song is empty - stopping playback");
            bt_song_stop (self);
          }
        } else {
          GST_INFO_OBJECT (p->bin, "looping");
        }
        bt_song_update_playback_position (self);
        if (p->paused_timeout_id) {
          g_source_remove (p->paused_timeout_id);
          p->paused_timeout_id = 0;
        }
        break;
      default:
        break;
    }
  }
}

static void
on_song_async_done (const GstBus * const bus, GstMessage * message,
    gconstpointer user_data)
{
  const BtSong *const self = BT_SONG (user_data);

  if (GST_MESSAGE_SRC (message) == GST_OBJECT (self->priv->bin)) {
    GST_INFO ("async operation done");
    if (self->priv->paused_timeout_id) {
      g_source_remove (self->priv->paused_timeout_id);
      self->priv->paused_timeout_id = 0;
    }
  }
}

static void
on_song_clock_lost (const GstBus * const bus, GstMessage * message,
    gconstpointer user_data)
{
  const BtSong *const self = BT_SONG (user_data);

  if (GST_MESSAGE_SRC (message) == GST_OBJECT (self->priv->bin)) {
    if (self->priv->is_playing) {
      GST_INFO ("clock·lost!·PAUSE·and·PLAY·to·select·a·new·clock");

      gst_element_set_state (GST_ELEMENT (self->priv->bin), GST_STATE_PAUSED);
      gst_element_set_state (GST_ELEMENT (self->priv->bin), GST_STATE_PLAYING);
    }
  }
}

static void
on_song_latency (const GstBus * const bus, GstMessage * message,
    gconstpointer user_data)
{
  const BtSong *const self = BT_SONG (user_data);

  if (GST_MESSAGE_SRC (message) == GST_OBJECT (self->priv->bin)) {
    GST_INFO ("latency changed, redistributing ...");

    gst_bin_recalculate_latency (self->priv->bin);
  }
}

static void
on_song_request_state (const GstBus * const bus, GstMessage * message,
    gconstpointer user_data)
{
  const BtSong *const self = BT_SONG (user_data);

  //if(GST_MESSAGE_SRC(message) == GST_OBJECT(self->priv->bin)) {
  GstState state;

  gst_message_parse_request_state (message, &state);
  GST_WARNING ("requesting state-change to '%s'",
      gst_element_state_get_name (state));

  switch (state) {
    case GST_STATE_NULL:
    case GST_STATE_READY:
    case GST_STATE_PAUSED:
      bt_song_stop (self);
      break;
    case GST_STATE_PLAYING:
      bt_song_play (self);
      break;
    default:
      break;
  }
  //}
}

#ifdef DETAILED_CPU_LOAD
static void
on_song_stream_status (const GstBus * const bus, GstMessage * message,
    gconstpointer user_data)
{
  //const BtSong * const self = BT_SONG(user_data);
  GstStreamStatusType type;
  GstElement *owner;

  gst_message_parse_stream_status (message, &type, &owner);

  GST_WARNING_OBJECT (GST_MESSAGE_SRC (message), "new stream status '%d' on %s",
      type, GST_OBJECT_NAME (owner));
  /* we could use this to determine CPU usage per thread
   * - for that we need to periodically call getrusage(RUSAGE_THREAD,&ru);
   *   from each thread
   * - this could be done from a dataprobe on the pad that starts the thread or even
   *   all pads in that thread
   * - the dataprobe would
   *   - need a previous ClockTime, elapsed wallclock time and a pointer to the
   *     BtMachine, we could have a global hashtable that maps thread-id to a
   *     BtPerfData struct
   *   - set the cpu-load value on the BtMachine (no notify!)
   * - the application can poll the value from a machine property
   *   and show load-meters in the UI
   */

  switch (type) {
    case GST_STREAM_STATUS_TYPE_CREATE:
      // TODO(ensonic): add to thread-id -> perf-data map
      break;
    case GST_STREAM_STATUS_TYPE_DESTROY:
      // TODO(ensonic): remove from thread-id -> perf data map
      break;
    default:
      break;
  }
}
#endif

static void
bt_song_on_loop_changed (BtSequence * const sequence, GParamSpec * const arg,
    gconstpointer user_data)
{
  GST_DEBUG ("loop mode changed");
  bt_song_update_play_seek_event_and_play_pos (BT_SONG (user_data));
}

static void
bt_song_on_loop_start_changed (BtSequence * const sequence,
    GParamSpec * const arg, gconstpointer user_data)
{
  GST_DEBUG ("loop start changed");
  bt_song_update_play_seek_event_and_play_pos (BT_SONG (user_data));
}

static void
bt_song_on_loop_end_changed (BtSequence * const sequence,
    GParamSpec * const arg, gconstpointer user_data)
{
  GST_DEBUG ("loop end changed");
  bt_song_update_play_seek_event_and_play_pos (BT_SONG (user_data));
}

static void
bt_song_on_length_changed (BtSequence * const sequence, GParamSpec * const arg,
    gconstpointer user_data)
{
  GST_DEBUG ("song length changed");
  bt_song_update_play_seek_event_and_play_pos (BT_SONG (user_data));
}

static void
bt_song_on_tempo_changed (BtSongInfo * const song_info, GParamSpec * const arg,
    gconstpointer user_data)
{
  GST_DEBUG ("tempo changed");
  bt_song_update_play_seek_event_and_play_pos (BT_SONG (user_data));
  bt_song_send_audio_context (BT_SONG (user_data));
}

static void
bt_song_on_latency_changed (BtSongInfo * const song_info,
    GParamSpec * const arg, gconstpointer user_data)
{
  bt_song_send_audio_context (BT_SONG (user_data));
}

/* required for live mode */

/*
 * bt_song_idle_start:
 * @self: a #BtSong
 *
 * Works like bt_song_play(), but sends a segmented-seek that loops from
 * for a few seconds close to G_MAXINT64.
 * This is needed to do state changes (mute, solo, bypass) and to play notes
 * live.
 *
 * The application should not be concerned about this internal detail. Stopping
 * and restarting the idle loop should only be done, when massive changes are
 * about (e.g. loading a song).
 *
 * Returns: %TRUE for success
 */
static gboolean
bt_song_idle_start (const BtSong * const self)
{
  GstStateChangeReturn res;

  GST_INFO ("prepare idle loop");
  self->priv->is_idle_active = TRUE;
  // prepare idle loop
  if ((res =
          gst_element_set_state (GST_ELEMENT (self->priv->bin),
              GST_STATE_PAUSED)) == GST_STATE_CHANGE_FAILURE) {
    GST_WARNING ("can't go to paused state");
    self->priv->is_idle_active = FALSE;
    return FALSE;
  }
  GST_DEBUG ("->PAUSED state change returned '%s'",
      gst_element_state_change_return_get_name (res));

  // seek to start time
  if (!(gst_element_send_event (GST_ELEMENT (self->priv->master_bin),
              gst_event_ref (self->priv->idle_seek_event)))) {
    GST_WARNING ("element failed to handle idle seek event");
  }
  // start idling
  if ((res =
          gst_element_set_state (GST_ELEMENT (self->priv->bin),
              GST_STATE_PLAYING)) == GST_STATE_CHANGE_FAILURE) {
    GST_WARNING ("can't go to playing state");
    self->priv->is_idle_active = FALSE;
    return FALSE;
  }
  GST_DEBUG (">PLAYING state change returned '%s'",
      gst_element_state_change_return_get_name (res));
  GST_INFO ("idle loop running");
  return TRUE;
}

/*
 * bt_song_idle_stop:
 * @self: a #BtSong
 *
 * Stops the idle loop.
 *
 * Returns: %TRUE for success
 */
static gboolean
bt_song_idle_stop (const BtSong * const self)
{
  GstStateChangeReturn res;

  GST_INFO ("stopping idle loop");

  if ((res =
          gst_element_set_state (GST_ELEMENT (self->priv->bin),
              GST_STATE_READY)) == GST_STATE_CHANGE_FAILURE) {
    GST_WARNING ("can't go to ready state");
    return FALSE;
  }
  GST_DEBUG ("state change returned '%s'",
      gst_element_state_change_return_get_name (res));
  self->priv->is_idle_active = FALSE;
  return TRUE;
}


//-- constructor methods

/**
 * bt_song_new:
 * @app: the application object the songs belongs to.
 *
 * Create a new instance.
 * The new song instance automatically has one instance of #BtSetup, #BtSequence
 * and #BtSongInfo. These instances can be retrieved via the respecting
 * properties.
 *
 * For example use following code to retrive a BtSequence from the song class:
 * <informalexample><programlisting language="c">
 * BtSequence *sequence;
 * ...
 * g_object_get(BT_SONG(song), "sequence", &amp;sequence, NULL);</programlisting>
 * </informalexample>
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtSong *
bt_song_new (const BtApplication * const app)
{
  return BT_SONG (g_object_new (BT_TYPE_SONG, "app", app, NULL));
}

//-- methods

/**
 * bt_song_play:
 * @self: the song that should be played
 *
 * Starts to play the specified song instance from beginning.
 * This methods toggles the #BtSong:is-playing property.
 *
 * Returns: %TRUE for success
 */
gboolean
bt_song_play (const BtSong * const self)
{
  GstStateChangeReturn res;

  g_return_val_if_fail (BT_IS_SONG (self), FALSE);
  // this is the sink machine, and there should always be one
  g_return_val_if_fail (GST_IS_BIN (self->priv->master_bin), FALSE);

  // do not play again
  if (self->priv->is_playing)
    return TRUE;
  if (self->priv->is_idle)
    bt_song_idle_stop (self);

  GST_INFO ("prepare playback");
  // update play-pos
  bt_song_update_play_seek_event_and_play_pos (self);
  // prepare playback
  self->priv->is_preparing = TRUE;

  // this should be sequence->play_start
  self->priv->play_pos = self->priv->play_beg;
  g_object_notify (G_OBJECT (self), "play-pos");
  GST_DEBUG_OBJECT (self->priv->master_bin, "seek event: %" GST_PTR_FORMAT,
      self->priv->play_seek_event);

#ifndef GST_BUG_733031
  // send seek
  bt_song_send_initial_seek (self, self->priv->bin,
      self->priv->play_seek_event);
#endif
  // send tags && toc
  bt_song_send_tags (self);
  bt_song_send_toc (self);

  res = gst_element_set_state (GST_ELEMENT (self->priv->bin), GST_STATE_PAUSED);
  GST_DEBUG ("->PAUSED state change returned '%s'",
      gst_element_state_change_return_get_name (res));
  switch (res) {
    case GST_STATE_CHANGE_SUCCESS:
    case GST_STATE_CHANGE_NO_PREROLL:
      GST_INFO ("playback started");
      break;
    case GST_STATE_CHANGE_FAILURE:
      GST_WARNING ("can't go to playing state");
      bt_song_write_to_lowlevel_dot_file (self);
      return FALSE;
      break;
    case GST_STATE_CHANGE_ASYNC:
      GST_INFO ("->PLAYING needs async wait");
      // start a short timeout that aborts playback if if get not started
      self->priv->paused_timeout_id =
          g_timeout_add_seconds (STATE_CHANGE_TIMEOUT, on_song_paused_timeout,
          (gpointer) self);
      break;
    default:
      GST_WARNING ("unexpected state-change-return %d:%s", res,
          gst_element_state_change_return_get_name (res));
      bt_song_write_to_lowlevel_dot_file (self);
      break;
  }
  return TRUE;
}

/**
 * bt_song_stop:
 * @self: the song that should be stopped
 *
 * Stops the playback of the specified song instance.
 *
 * Returns: %TRUE for success
 */
gboolean
bt_song_stop (const BtSong * const self)
{
  GstStateChangeReturn res;

  g_return_val_if_fail (BT_IS_SONG (self), FALSE);

  GST_INFO ("stopping playback, is_playing: %d, is_preparing: %d",
      self->priv->is_playing, self->priv->is_preparing);

  if (!(self->priv->is_preparing || self->priv->is_playing)
      && (GST_STATE (self->priv->bin) <= GST_STATE_READY)) {
    GST_INFO ("not playing");
    return TRUE;
  }

  if ((res =
          gst_element_set_state (GST_ELEMENT (self->priv->bin),
              GST_STATE_READY)) == GST_STATE_CHANGE_FAILURE) {
    GST_WARNING ("can't go to ready state");
  }
  GST_DEBUG ("->READY state change returned '%s'",
      gst_element_state_change_return_get_name (res));

  // kill a pending timeout
  if (self->priv->paused_timeout_id) {
    g_source_remove (self->priv->paused_timeout_id);
    self->priv->paused_timeout_id = 0;
  }
  // do not stop if not playing or not preparing
  if (self->priv->is_preparing) {
    self->priv->is_preparing = FALSE;
    goto done;
  }
  if (!self->priv->is_playing)
    goto done;

  GST_INFO ("playback stopped");
  self->priv->is_playing = FALSE;

done:
  g_object_notify (G_OBJECT (self), "is-playing");
  if (self->priv->is_idle)
    bt_song_idle_start (self);

  return TRUE;
}

/**
 * bt_song_pause:
 * @self: the song that should be paused
 *
 * Pauses the playback of the specified song instance.
 *
 * Returns: %TRUE for success
 */
gboolean
bt_song_pause (const BtSong * const self)
{
  g_return_val_if_fail (BT_IS_SONG (self), FALSE);
  return (gst_element_set_state (GST_ELEMENT (self->priv->bin),
          GST_STATE_PAUSED) != GST_STATE_CHANGE_FAILURE);
}

/**
 * bt_song_continue:
 * @self: the song that should be paused
 *
 * Continues the playback of the specified song instance.
 *
 * Returns: %TRUE for success
 */
gboolean
bt_song_continue (const BtSong * const self)
{
  g_return_val_if_fail (BT_IS_SONG (self), FALSE);
  return (gst_element_set_state (GST_ELEMENT (self->priv->bin),
          GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);
}

/**
 * bt_song_update_playback_position:
 * @self: the song that should update its playback-pos counter
 *
 * Updates the playback-position counter to fire all #BtSong:play-pos notify
 * handlers.
 *
 * Returns: %FALSE if the song is not playing
 */
gboolean
bt_song_update_playback_position (const BtSong * const self)
{
  g_return_val_if_fail (BT_IS_SONG (self), FALSE);
  g_assert (GST_IS_BIN (self->priv->master_bin));
  g_assert (GST_IS_QUERY (self->priv->position_query));
  //GST_INFO("query playback-pos");

  if (G_UNLIKELY (!self->priv->is_playing)) {
    GST_WARNING ("not playing");
    return FALSE;
  }
  // query playback position and update self->priv->play-pos;
  if (gst_element_query (GST_ELEMENT (self->priv->master_bin),
          self->priv->position_query)) {
    gint64 pos_cur;
    gst_query_parse_position (self->priv->position_query, NULL, &pos_cur);
    if (GST_CLOCK_TIME_IS_VALID (pos_cur)) {
      // calculate new play-pos (in ticks)
      const gulong play_pos = bt_song_info_time_to_tick (self->priv->song_info,
          pos_cur);
      if (play_pos != self->priv->play_pos) {
        GST_INFO ("query playback-pos: cur=%" G_GINT64_FORMAT ", tick=%lu",
            pos_cur, play_pos);
        self->priv->play_pos = play_pos;
        g_object_notify (G_OBJECT (self), "play-pos");
      }
    } else {
      GST_WARNING ("query playback-pos: invalid pos");
    }
  } else {
    GST_WARNING ("query playback-pos: failed");
  }
  // don't return FALSE in the WARNING case above, we use the return value to
  // return from time-out handlers (e.g. toolbar)
  return TRUE;
}

/**
 * bt_song_write_to_highlevel_dot_file:
 * @self: the song that should be written
 *
 * To aid debugging applications one can use this method to write out the whole
 * network of gstreamer elements from the songs perspective into an dot file.
 * The file will be written to '/tmp' and will be named according the 'name'
 * property of the #BtSongInfo.
 * This file can be processed with graphviz to get an image.
 * <informalexample><programlisting>
 *  dot -Tpng -oimage.png graph_highlevel.dot
 * </programlisting></informalexample>
 */
void
bt_song_write_to_highlevel_dot_file (const BtSong * const self)
{
  FILE *out;
  gchar *const song_name;

  g_return_if_fail (BT_IS_SONG (self));

  g_object_get (self->priv->song_info, "name", &song_name, NULL);
  const gint len = strlen (song_name) + 10;
  gchar *const file_name = g_alloca (len);
  g_snprintf (file_name, len, "/tmp/%s_highlevel.dot", song_name);

  if ((out = fopen (file_name, "wb"))) {
    GList *const list, *node, *sublist, *subnode;
    BtMachine *const src, *const dst;
    GstElement *elem;
    GstElementFactory *factory;
    gchar *id, *label;
    gchar *this_name = NULL, *last_name, *src_name, *dst_name;
    gulong index, count;

    // write header
    fprintf (out,
        "digraph buzztrax {\n"
        "  rankdir=LR;\n"
        "  fontname=\"sans\";\n"
        "  node [style=filled, shape=box, labelfontsize=\"8\", fontsize=\"8\", fontname=\"Arial\"];\n"
        "\n");

    // iterate over machines list
    g_object_get (self->priv->setup, "machines", &list, NULL);
    for (node = list; node; node = g_list_next (node)) {
      BtMachine *const machine = BT_MACHINE (node->data);
      g_object_get (machine, "id", &id, NULL);
      fprintf (out,
          "  subgraph cluster_%s {\n"
          "    style=filled;\n"
          "    label=\"%s\\n%d\";\n"
          "    fillcolor=\"%s\";\n"
          "    color=black\n\n", id, id, G_OBJECT_REF_COUNT (machine),
          BT_IS_SOURCE_MACHINE (machine) ? "#FFAAAA"
          : (BT_IS_SINK_MACHINE (machine) ? "#AAAAFF" : "#AAFFAA"));

      // query internal element of each machine
      sublist = bt_machine_get_element_list (machine);
      last_name = NULL;
      for (subnode = sublist; subnode; subnode = g_list_next (subnode)) {
        elem = GST_ELEMENT (subnode->data);
        this_name = g_strdelimit (gst_element_get_name (elem), ":", '_');
        factory = gst_element_get_factory (elem);
        label = (gchar *) gst_element_factory_get_metadata (factory,
            GST_ELEMENT_METADATA_LONGNAME);
        fprintf (out,
            "    %s [color=black, fillcolor=white, label=\"%s\\n%d\"];\n",
            this_name, label, G_OBJECT_REF_COUNT (elem));
        if (last_name) {
          fprintf (out, "    %s -> %s\n", last_name, this_name);
          g_free (last_name);
        }
        last_name = this_name;
      }
      g_list_free (sublist);
      g_free (id);
      if (last_name)
        g_free (last_name);
      fprintf (out, "  }\n\n");
    }
    g_list_free (list);

    // iterate over wire list
    g_object_get (self->priv->setup, "wires", &list, NULL);
    for (node = list; node; node = g_list_next (node)) {
      BtWire *const wire = BT_WIRE (node->data);
      g_object_get (wire, "src", &src, "dst", &dst, NULL);
      id = g_strdelimit (gst_element_get_name (wire), ":", '_');

      fprintf (out,
          "  subgraph cluster_%s {\n"
          "    label=\"%s\\n%d\";\n"
          "    color=black\n\n", id, id, G_OBJECT_REF_COUNT (wire));

      // get last_name of src
      sublist = bt_machine_get_element_list (src);
      subnode = g_list_last (sublist);
      elem = GST_ELEMENT (subnode->data);
      src_name = g_strdelimit (gst_element_get_name (elem), ":", '_');
      g_list_free (sublist);
      // get first_name of dst
      sublist = bt_machine_get_element_list (dst);
      subnode = g_list_first (sublist);
      elem = GST_ELEMENT (subnode->data);
      dst_name = g_strdelimit (gst_element_get_name (elem), ":", '_');
      g_list_free (sublist);

      // query internal element of each wire
      sublist = bt_wire_get_element_list (wire);
      count = g_list_length (sublist);
      GST_INFO ("wire %s->%s has %lu elements", src_name, dst_name, count);
      index = 0;
      last_name = NULL;
      for (subnode = sublist; subnode; subnode = g_list_next (subnode)) {
        // skip first and last
        if ((index > 0) && (index < (count - 1))) {
          elem = GST_ELEMENT (subnode->data);
          this_name = g_strdelimit (gst_element_get_name (elem), ":", '_');
          factory = gst_element_get_factory (elem);
          label = (gchar *) gst_element_factory_get_metadata (factory,
              GST_ELEMENT_METADATA_LONGNAME);
          fprintf (out,
              "    %s [color=black, fillcolor=white, label=\"%s\\n%d\"];\n",
              this_name, label, G_OBJECT_REF_COUNT (elem));
        } else if (index == 0) {
          this_name = src_name;
        } else if (index == (count - 1)) {
          this_name = dst_name;
        }
        if (last_name) {
          fprintf (out, "    %s -> %s\n", last_name, this_name);
        }
        last_name = this_name;
        index++;
      }
      g_list_free (sublist);
      g_object_unref (src);
      g_object_unref (dst);
      g_free (id);

      fprintf (out, "  }\n\n");
    }
    g_list_free (list);

    // write footer
    fprintf (out, "}\n");
    fclose (out);
  }
  g_free (song_name);
}

/**
 * bt_song_write_to_lowlevel_dot_file:
 * @self: the song that should be written
 *
 * To aid debugging applications one can use this method to write out the whole
 * network of gstreamer elements that form the song into an dot file.
 * The file will be written to '/tmp' and will be named according the 'name'
 * property of the #BtSongInfo.
 * This file can be processed with graphviz to get an image.
 * <informalexample><programlisting>
 *  dot -Tpng -oimage.png graph_lowlevel.dot
 * </programlisting></informalexample>
 */
void
bt_song_write_to_lowlevel_dot_file (const BtSong * const self)
{
  GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS (self->priv->bin, GST_DEBUG_GRAPH_SHOW_ALL,
      /*GST_DEBUG_GRAPH_SHOW_CAPS_DETAILS|GST_DEBUG_GRAPH_SHOW_STATES, */
      PACKAGE_NAME);
}

//-- io interface

static xmlNodePtr
bt_song_persistence_save (const BtPersistence * const persistence,
    xmlNodePtr const parent_node)
{
  const BtSong *const self = BT_SONG (persistence);
  xmlNodePtr node = NULL;

  GST_DEBUG ("PERSISTENCE::song");

  if ((node = xmlNewNode (NULL, XML_CHAR_PTR ("buzztrax")))) {
    xmlNewProp (node, XML_CHAR_PTR ("xmlns"),
        (const xmlChar *) "http://www.buzztrax.org/");
    xmlNewProp (node, XML_CHAR_PTR ("xmlns:xsd"),
        XML_CHAR_PTR ("http://www.w3.org/2001/XMLSchema-instance"));
    xmlNewProp (node, XML_CHAR_PTR ("xsd:noNamespaceSchemaLocation"),
        XML_CHAR_PTR ("buzztrax.xsd"));

    bt_persistence_save (BT_PERSISTENCE (self->priv->song_info), node);
    bt_persistence_save (BT_PERSISTENCE (self->priv->setup), node);
    bt_persistence_save (BT_PERSISTENCE (self->priv->sequence), node);
    bt_persistence_save (BT_PERSISTENCE (self->priv->wavetable), node);
  }
  return node;
}

static BtPersistence *
bt_song_persistence_load (const GType type,
    const BtPersistence * const persistence, xmlNodePtr node, GError ** err,
    va_list var_args)
{
  const BtSong *const self = BT_SONG (persistence);
  // TODO(ensonic): this is a bit inconsistent
  // gtk is getting labels with progressbar, we could use this then
  //const gchar * const msg=_("Loading file: '%s'");
  //gchar * const status=g_alloca(1+strlen(msg)+40);

  GST_DEBUG ("PERSISTENCE::song");
  g_assert (node);

  for (node = node->children; node; node = node->next) {
    if (!xmlNodeIsText (node)) {
      if (!strncmp ((gchar *) node->name, "meta\0", 5)) {
        if (!bt_persistence_load (BT_TYPE_SONG_INFO,
                BT_PERSISTENCE (self->priv->song_info), node, NULL, NULL))
          goto Error;
      } else if (!strncmp ((gchar *) node->name, "setup\0", 6)) {
        if (!bt_persistence_load (BT_TYPE_SETUP,
                BT_PERSISTENCE (self->priv->setup), node, NULL, NULL))
          goto Error;
      } else if (!strncmp ((gchar *) node->name, "sequence\0", 9)) {
        if (!bt_persistence_load (BT_TYPE_SEQUENCE,
                BT_PERSISTENCE (self->priv->sequence), node, NULL, NULL))
          goto Error;
      } else if (!strncmp ((gchar *) node->name, "wavetable\0", 10)) {
        if (!bt_persistence_load (BT_TYPE_WAVETABLE,
                BT_PERSISTENCE (self->priv->wavetable), node, NULL, NULL))
          goto Error;
      }
    }
  }

  return BT_PERSISTENCE (persistence);
Error:
  if (node) {
    /* TODO(ensonic): set the GError? */
    GST_WARNING ("failed to load %s", (gchar *) node->name);
  }
  return NULL;
}

static void
bt_song_persistence_interface_init (gpointer const g_iface,
    gpointer const iface_data)
{
  BtPersistenceInterface *const iface = g_iface;

  iface->load = bt_song_persistence_load;
  iface->save = bt_song_persistence_save;
}

//-- wrapper

//-- g_object overrides

static void
bt_song_constructed (GObject * object)
{
  BtSong *self = BT_SONG (object);
  BtSettings *settings = bt_settings_make ();
  GstStateChangeReturn res;

  if (G_OBJECT_CLASS (bt_song_parent_class)->constructed)
    G_OBJECT_CLASS (bt_song_parent_class)->constructed (object);

  g_return_if_fail (BT_IS_APPLICATION (self->priv->app));

  g_object_get ((gpointer) (self->priv->app), "bin", &self->priv->bin, NULL);

  GstBus *const bus = gst_element_get_bus (GST_ELEMENT (self->priv->bin));
  if (bus) {
    GST_DEBUG ("listen to bus messages (%p)", bus);
    gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);
    bt_g_signal_connect_object (bus, "message::segment-done",
        G_CALLBACK (on_song_segment_done), (gpointer) self, 0);
    bt_g_signal_connect_object (bus, "message::eos", G_CALLBACK (on_song_eos),
        (gpointer) self, 0);
    bt_g_signal_connect_object (bus, "message::state-changed",
        G_CALLBACK (on_song_state_changed), (gpointer) self, 0);
    bt_g_signal_connect_object (bus, "message::async-done",
        G_CALLBACK (on_song_async_done), (gpointer) self, 0);
    bt_g_signal_connect_object (bus, "message::clock-lost",
        G_CALLBACK (on_song_clock_lost), (gpointer) self, 0);
    bt_g_signal_connect_object (bus, "message::latency",
        G_CALLBACK (on_song_latency), (gpointer) self, 0);
    bt_g_signal_connect_object (bus, "message::request-state",
        G_CALLBACK (on_song_request_state), (gpointer) self, 0);
#ifdef DETAILED_CPU_LOAD
    bt_g_signal_connect_object (bus, "message::stream-status",
        G_CALLBACK (on_song_stream_status), (gpointer) self, 0);
#endif

    gst_bus_set_flushing (bus, FALSE);
    gst_object_unref (bus);
  }

  /* don't change the order */
  self->priv->song_info = bt_song_info_new (self);
  self->priv->setup = bt_setup_new (self);
  self->priv->sequence = bt_sequence_new (self);
  self->priv->wavetable = bt_wavetable_new (self);

  g_signal_connect_object (self->priv->sequence, "notify::loop",
      G_CALLBACK (bt_song_on_loop_changed), (gpointer) self, 0);
  g_signal_connect_object (self->priv->sequence, "notify::loop-start",
      G_CALLBACK (bt_song_on_loop_start_changed), (gpointer) self, 0);
  g_signal_connect_object (self->priv->sequence, "notify::loop-end",
      G_CALLBACK (bt_song_on_loop_end_changed), (gpointer) self, 0);
  g_signal_connect_object (self->priv->sequence, "notify::length",
      G_CALLBACK (bt_song_on_length_changed), (gpointer) self, 0);
  GST_DEBUG ("  sequence-signals connected");
  g_signal_connect_object (self->priv->song_info, "notify::tpb",
      G_CALLBACK (bt_song_on_tempo_changed), (gpointer) self, 0);
  g_signal_connect_object (self->priv->song_info, "notify::bpm",
      G_CALLBACK (bt_song_on_tempo_changed), (gpointer) self, 0);
  GST_DEBUG ("  song-info-signals connected");
  g_signal_connect_object (settings, "notify::latency",
      G_CALLBACK (bt_song_on_latency_changed), (gpointer) self, 0);
  g_object_unref (settings);

  bt_song_send_audio_context (self);
  bt_song_update_play_seek_event_and_play_pos (BT_SONG (self));
  if ((res =
          gst_element_set_state (GST_ELEMENT (self->priv->bin),
              GST_STATE_READY)) == GST_STATE_CHANGE_FAILURE) {
    GST_WARNING_OBJECT (self->priv->bin, "can't go to ready state");
  }
  GST_INFO_OBJECT (self->priv->bin, "->READY state change returned '%s'",
      gst_element_state_change_return_get_name (res));
  GST_INFO ("  new song created: %p", self);
}

static void
bt_song_get_property (GObject * const object, const guint property_id,
    GValue * const value, GParamSpec * const pspec)
{
  const BtSong *const self = BT_SONG (object);
  return_if_disposed ();
  switch (property_id) {
    case SONG_APP:
      g_value_set_object (value, self->priv->app);
      break;
    case SONG_BIN:
      g_value_set_object (value, self->priv->bin);
      break;
    case SONG_MASTER:
      g_value_set_object (value, self->priv->master);
      break;
    case SONG_SONG_INFO:
      g_value_set_object (value, self->priv->song_info);
      break;
    case SONG_SEQUENCE:
      g_value_set_object (value, self->priv->sequence);
      break;
    case SONG_SETUP:
      g_value_set_object (value, self->priv->setup);
      break;
    case SONG_WAVETABLE:
      g_value_set_object (value, self->priv->wavetable);
      break;
    case SONG_PLAY_POS:
      g_value_set_ulong (value, self->priv->play_pos);
      break;
    case SONG_PLAY_RATE:
      g_value_set_double (value, self->priv->play_rate);
      break;
    case SONG_IS_PLAYING:
      g_value_set_boolean (value, self->priv->is_playing);
      break;
    case SONG_IS_IDLE:
      g_value_set_boolean (value, self->priv->is_idle);
      break;
    case SONG_IO:
      g_value_set_object (value, self->priv->song_io);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_song_set_property (GObject * const object, const guint property_id,
    const GValue * const value, GParamSpec * const pspec)
{
  const BtSong *const self = BT_SONG (object);
  return_if_disposed ();
  switch (property_id) {
    case SONG_APP:
      self->priv->app = BT_APPLICATION (g_value_get_object (value));
      g_object_try_weak_ref (self->priv->app);
      GST_DEBUG ("set the app for the song: %p", self->priv->app);
      break;
    case SONG_MASTER:
      g_object_try_weak_unref (self->priv->master);
      self->priv->master = BT_SINK_MACHINE (g_value_get_object (value));
      g_object_try_weak_ref (self->priv->master);
      g_object_get ((gpointer) (self->priv->master), "machine",
          &self->priv->master_bin, NULL);
      GST_DEBUG_OBJECT (self->priv->master, "set the master for the song: %"
          G_OBJECT_REF_COUNT_FMT, G_OBJECT_LOG_REF_COUNT (self->priv->master));
      break;
    case SONG_PLAY_POS:{
      gulong play_pos = bt_sequence_limit_play_pos (self->priv->sequence,
          g_value_get_ulong (value));
      if (play_pos != self->priv->play_pos) {
        GST_DEBUG ("set the play-pos for sequence: %lu", play_pos);
        self->priv->play_pos = play_pos;
        // seek on playpos changes (if playing)
        bt_song_seek_to_play_pos (self);
      }
      break;
    }
    case SONG_PLAY_RATE:
      self->priv->play_rate = g_value_get_double (value);
      GST_DEBUG ("set the play-rate: %lf", self->priv->play_rate);
      // update rate (if playing)
      bt_song_change_play_rate (self);
      break;
    case SONG_IS_IDLE:
      self->priv->is_idle = g_value_get_boolean (value);
      if (!self->priv->is_playing) {
        if (self->priv->is_idle)
          bt_song_idle_start (self);
        else
          bt_song_idle_stop (self);
      }
      GST_DEBUG ("idle flag song: %d", self->priv->is_idle);
      break;
    case SONG_IO:
      if (self->priv->song_io)
        g_object_unref (self->priv->song_io);
      self->priv->song_io = BT_SONG_IO (g_value_dup_object (value));
      GST_DEBUG ("set the song-io plugin for the song: %p",
          self->priv->song_io);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_song_dispose (GObject * const object)
{
  const BtSong *const self = BT_SONG (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  if (self->priv->playback_timeout_id)
    g_source_remove (self->priv->playback_timeout_id);
  if (self->priv->paused_timeout_id)
    g_source_remove (self->priv->paused_timeout_id);

  if (self->priv->bin) {
    if (self->priv->is_playing)
      bt_song_stop (self);
    else if (self->priv->is_idle)
      bt_song_idle_stop (self);

    GstBus *const bus = gst_element_get_bus (GST_ELEMENT (self->priv->bin));
    gst_bus_remove_signal_watch (bus);
    gst_object_unref (bus);
  }

  GST_DEBUG_OBJECT (self->priv->master, "sink-machine: %"
      G_OBJECT_REF_COUNT_FMT, G_OBJECT_LOG_REF_COUNT (self->priv->master));
  if (self->priv->master_bin)
    gst_object_unref (self->priv->master_bin);
  g_object_try_weak_unref (self->priv->master);

  if (self->priv->song_info) {
    GST_DEBUG ("song_info: %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (self->priv->song_info));
    g_object_unref (self->priv->song_info);
  }
  if (self->priv->sequence) {
    GST_DEBUG ("sequence: %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (self->priv->sequence));
    g_object_unref (self->priv->sequence);
  }
  if (self->priv->setup) {
    GST_DEBUG ("setup: %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (self->priv->setup));
    g_object_unref (self->priv->setup);
  }
  if (self->priv->wavetable) {
    GST_DEBUG ("wavetable: %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (self->priv->wavetable));
    g_object_unref (self->priv->wavetable);
  }

  gst_query_unref (self->priv->position_query);
  if (self->priv->play_seek_event)
    gst_event_unref (self->priv->play_seek_event);
  if (self->priv->loop_seek_event)
    gst_event_unref (self->priv->loop_seek_event);
  if (self->priv->idle_seek_event)
    gst_event_unref (self->priv->idle_seek_event);
  if (self->priv->idle_loop_seek_event)
    gst_event_unref (self->priv->idle_loop_seek_event);
  if (self->priv->bin) {
    GST_DEBUG ("bin->num_children=%d", GST_BIN_NUMCHILDREN (self->priv->bin));
    GST_DEBUG_OBJECT (self->priv->bin, "bin: %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (self->priv->bin));
    gst_object_unref (self->priv->bin);
  }
  g_object_try_weak_unref (self->priv->app);

  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (bt_song_parent_class)->dispose (object);
  GST_DEBUG ("  done");
}

#ifndef GST_DISABLE_GST_DEBUG
static void
bt_song_finalize (GObject * const object)
{
  const BtSong *const self = BT_SONG (object);

  GST_DEBUG ("!!!! self=%p", self);

  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (bt_song_parent_class)->finalize (object);
  GST_DEBUG ("  done");
}
#endif

//-- class internals

static void
bt_song_init (BtSong * self)
{
  GstClockTime s, e;

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, BT_TYPE_SONG, BtSongPrivate);

  self->priv->position_query = gst_query_new_position (GST_FORMAT_TIME);
  self->priv->play_rate = 1.0;

  s = (GstClockTime) (G_MAXINT64 - (11 * GST_SECOND));
  e = (GstClockTime) (G_MAXINT64 - (1 * GST_SECOND));
  self->priv->idle_seek_event = MAKE_SEEK_EVENT_FL (1.0, s, e);
  self->priv->idle_loop_seek_event = MAKE_SEEK_EVENT_L (1.0, s, e);
  GST_DEBUG ("  done");
}

static void
bt_song_class_init (BtSongClass * const klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BtSongPrivate));

  gobject_class->constructed = bt_song_constructed;
  gobject_class->set_property = bt_song_set_property;
  gobject_class->get_property = bt_song_get_property;
  gobject_class->dispose = bt_song_dispose;
#ifndef GST_DISABLE_GST_DEBUG
  gobject_class->finalize = bt_song_finalize;
#endif

  g_object_class_install_property (gobject_class, SONG_APP,
      g_param_spec_object ("app", "app contruct prop",
          "set application object, the song belongs to",
          BT_TYPE_APPLICATION, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE |
          G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SONG_BIN,
      g_param_spec_object ("bin", "bin construct prop",
          "songs top-level GstElement container",
          GST_TYPE_BIN, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SONG_MASTER,
      g_param_spec_object ("master", "master prop", "songs audio_sink",
          BT_TYPE_SINK_MACHINE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SONG_SONG_INFO,
      g_param_spec_object ("song-info", "song-info prop",
          "songs metadata sub object",
          BT_TYPE_SONG_INFO, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SONG_SEQUENCE,
      g_param_spec_object ("sequence", "sequence prop",
          "songs sequence sub object",
          BT_TYPE_SEQUENCE, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SONG_SETUP,
      g_param_spec_object ("setup", "setup prop", "songs setup sub object",
          BT_TYPE_SETUP, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SONG_WAVETABLE,
      g_param_spec_object ("wavetable", "wavetable prop",
          "songs wavetable sub object",
          BT_TYPE_WAVETABLE, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  // loop-positions are LONG as well
  g_object_class_install_property (gobject_class, SONG_PLAY_POS,
      g_param_spec_ulong ("play-pos", "play-pos prop",
          "position of the play cursor of the sequence in timeline bars", 0,
          G_MAXLONG, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SONG_PLAY_RATE,
      g_param_spec_double ("play-rate",
          "play-rate prop",
          "playback rate of the sequence",
          -5.0, 5.0, 1.0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SONG_IS_PLAYING,
      g_param_spec_boolean ("is-playing",
          "is-playing prop",
          "tell whether the song is playing right now or not",
          FALSE, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SONG_IS_IDLE,
      g_param_spec_boolean ("is-idle",
          "is-idle prop",
          "request that the song should idle-loop if not playing",
          FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SONG_IO,
      g_param_spec_object ("song-io", "song-io prop",
          "the song-io plugin during i/o operations",
          BT_TYPE_SONG_IO, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}
