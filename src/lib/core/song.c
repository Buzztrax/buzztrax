/* $Id$
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
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
 * SECTION:btsong
 * @short_description: class of a song project object (contains #BtSongInfo,
 * #BtSetup, #BtSequence and #BtWavetable)
 *
 * A song is the top-level container object to manage all song-related objects.
 *
 * To load or save a song, use a #BtSongIO object. These implement loading and
 * saving for different file-formats.
 */

#define BT_CORE
#define BT_SONG_C

#include "core_private.h"

// if a state change not happens within this time, cancel playback
#define BT_SONG_STATE_CHANGE_TIMEOUT (30*1000)

//-- signal ids

enum {
  LAST_SIGNAL
};

//-- property ids

enum {
  SONG_APP=1,
  SONG_BIN,
  SONG_MASTER,
  SONG_SONG_INFO,
  SONG_SEQUENCE,
  SONG_SETUP,
  SONG_WAVETABLE,
  SONG_UNSAVED,
  SONG_PLAY_POS,
  SONG_IS_PLAYING,
  SONG_IO
};

struct _BtSongPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the parts of the song */
  BtSongInfo*  song_info;
  BtSequence*  sequence;
  BtSetup*     setup;
  BtWavetable* wavetable;

  /* whenever the song is changed, unsave should be set TRUE */
  gboolean unsaved;

  /* the playback position of the song */
  gulong play_pos;
  /* flag to signal playing and idle states */
  gboolean is_playing,is_idle,is_preparing;

  /* the application that currently uses the song */
  G_POINTER_ALIAS(BtApplication *,app);
  /* the main gstreamer container element */
  GstBin *bin,*master_bin;
  /* the element that has the clock */
  G_POINTER_ALIAS(BtSinkMachine *,master);

  /* the query is used in update_playback_position */
  GstQuery *position_query;
  /* seek events */
  GstEvent *play_seek_event;
  GstEvent *loop_seek_event;
  GstEvent *idle_seek_event;
  /* timeout handlers */
  guint paused_timeout_id,playback_timeout_id;
  
  /* the song-io plugin during i/o operations */
  BtSongIO *song_io;
};

static GObjectClass *parent_class=NULL;

//static guint signals[LAST_SIGNAL]={0,};

//-- helper

/* Ideally this would be 0. But unfortunately our songs stop too early for yet
 * unknown reason.
 */
//#define SEEK_END_EXTRA_BARS 8
#define SEEK_END_EXTRA_BARS 0

/*
 * bt_song_seek_to_play_pos:
 * @self: #BtSong to seek
 *
 * Send a new playback segment, that goes from the current playback position to
 * the new end position (loop or song end).
 */
static void bt_song_seek_to_play_pos(const BtSong * const self) {
  GstEvent *event;
  gboolean loop;
  glong loop_end,length;

  if(!self->priv->is_playing) return;

  g_object_get(self->priv->sequence,"loop",&loop,"loop-end",&loop_end,"length",&length,NULL);
  const GstClockTime bar_time=bt_sequence_get_bar_time(self->priv->sequence);

  GST_INFO("loop %d? %ld, length %ld, bar_time %lf",loop,loop_end,length,bar_time);

  // we need to flush, otheriwse mixing goes out of sync */
  if (loop) {
    event = gst_event_new_seek(1.0, GST_FORMAT_TIME,
        GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT,
        GST_SEEK_TYPE_SET, self->priv->play_pos*bar_time,
        GST_SEEK_TYPE_SET, (loop_end+0)*bar_time);
  }
  else {
    event = gst_event_new_seek(1.0, GST_FORMAT_TIME,
        GST_SEEK_FLAG_FLUSH,
        GST_SEEK_TYPE_SET, self->priv->play_pos*bar_time,
        GST_SEEK_TYPE_SET, (length+SEEK_END_EXTRA_BARS)*bar_time);
  }
  if(!(gst_element_send_event(GST_ELEMENT(self->priv->master_bin),event))) {
      GST_WARNING("element failed to seek to play_pos event");
  }
}

/*
 * bt_song_update_play_seek_event:
 * @self: #BtSong to seek
 *
 * Prepares a new playback segment, that goes from the new start position (loop
 * or song start) to the new end position (loop or song end).
 * Also calls bt_song_seek_to_play_pos() to update the current playback segment.
 */
static void bt_song_update_play_seek_event(const BtSong * const self) {
  gboolean loop;
  glong loop_start,loop_end,length;

  g_object_get(self->priv->sequence,"loop",&loop,"loop-start",&loop_start,"loop-end",&loop_end,"length",&length,NULL);
  const GstClockTime bar_time=bt_sequence_get_bar_time(self->priv->sequence);

  GST_DEBUG("loop %d? %ld ... %ld, length %ld bar_time %lf",loop,loop_start,loop_end,length,bar_time);

  if(self->priv->play_seek_event) gst_event_unref(self->priv->play_seek_event);
  if(self->priv->loop_seek_event) gst_event_unref(self->priv->loop_seek_event);
  /* we need to use FLUSH for play (due to prerolling), otherwise:
     0:00:00.866899000 15884 0x81cee70 DEBUG             basesink gstbasesink.c:1644:gst_base_sink_do_sync:<player> prerolling object 0x818ce90
     0:00:00.866948000 15884 0x81cee70 DEBUG             basesink gstbasesink.c:1493:gst_base_sink_wait_preroll:<player> waiting in preroll for flush or PLAYING
     but not for loop
   */
  if (loop) {
    self->priv->play_seek_event = gst_event_new_seek(1.0, GST_FORMAT_TIME,
        GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT,
        GST_SEEK_TYPE_SET, loop_start*bar_time,
        GST_SEEK_TYPE_SET, (loop_end+0)*bar_time);
    self->priv->loop_seek_event = gst_event_new_seek(1.0, GST_FORMAT_TIME,
        GST_SEEK_FLAG_SEGMENT,
        GST_SEEK_TYPE_SET, loop_start*bar_time,
        GST_SEEK_TYPE_SET, (loop_end+0)*bar_time);
  }
  else {
    self->priv->play_seek_event = gst_event_new_seek(1.0, GST_FORMAT_TIME,
        GST_SEEK_FLAG_FLUSH,
        GST_SEEK_TYPE_SET, G_GUINT64_CONSTANT(0),
        GST_SEEK_TYPE_SET, (length+SEEK_END_EXTRA_BARS)*bar_time);
    self->priv->loop_seek_event = gst_event_new_seek(1.0, GST_FORMAT_TIME,
        GST_SEEK_FLAG_NONE,
        GST_SEEK_TYPE_SET, G_GUINT64_CONSTANT(0),
        GST_SEEK_TYPE_SET, (length+SEEK_END_EXTRA_BARS)*bar_time);
  }
  /* the update needs to take the current play-position into account */
  bt_song_seek_to_play_pos(self);
  // DEBUG
#if 0
  {
    GstClockTime extra_time=8*bar_time;
    GST_WARNING("extra time %"GST_TIME_FORMAT,GST_TIME_ARGS(extra_time));
    // bar_time is what the pipeline calculates as LATENCY
  }
#endif
  // DEBUG
}

/*
 * bt_song_is_playable:
 * @song: the song to check
 *
 * Do some sanify check if a song an be played. GStreamer is a bit picky about
 * partially connected pipelines (see design/gst/connection1.c)
 *
 * Returns: %TRUE if the song seems to be playable
 */
static gboolean bt_song_is_playable(const BtSong * const self) {
  GList *wires;

  GST_INFO("check playability");

  // do not play an empty song
  if(!GST_BIN_NUMCHILDREN(self->priv->bin)) {
    GST_INFO("bin has no children");
    return(FALSE);
  }

  // do not play a song with no wires linked to sink
  wires=bt_setup_get_wires_by_dst_machine(self->priv->setup,BT_MACHINE(self->priv->master));
  if(!wires) {
    GST_INFO("song has no wires");
    return(FALSE);
  }
  else {
    GList *node;

    for(node=wires;node;node=g_list_next(node)) {
      /* @todo: do not play a song with no wires linked to effects that are linked to the sink
       * recursively do:
       * subwires=bt_setup_get_wires_by_dst_machine(self->priv->setup,BT_MACHINE(BT_WIRE(node->data)->src));
       */
      g_object_try_unref(node->data);
    }
    g_list_free(wires);
  }

  /* what about iterating all elements in the bin and iterating their always pads
   * and complaining about unlinked pads
   */
  {
    GstIterator *element_iter,*pad_iter;
    gboolean elements_done,pads_done,all_linked=TRUE;
    guint element_ct=0,pad_ct=0;
    GstElement *element;
    GstPad *pad;

    element_iter=gst_bin_iterate_elements(self->priv->bin);
    elements_done = FALSE;
    while (!elements_done) {
      switch (gst_iterator_next (element_iter, (gpointer)&element)) {
        case GST_ITERATOR_OK:
          pad_iter=gst_element_iterate_pads(element);
          pads_done=FALSE;
          while (!pads_done) {
            switch (gst_iterator_next (pad_iter, (gpointer)&pad)) {
              case GST_ITERATOR_OK:
                if(!gst_pad_is_linked(pad)) {
                  all_linked=FALSE;
                  GST_INFO("%s.%s is not linked",
                    GST_OBJECT_NAME(element), GST_OBJECT_NAME(pad));
                  /* @todo: now we'd like to which machine it belongs
                   * 1:
                   * iterate over machines from setup
                   *   get elements with bt_machine_get_element_list(machine)
                   *     check if its there
                   * 2:
                   * if machine would be a bin, we can just get the parent
                   * 3:
                   * we can set the machine as a qdata in debug versions
                   */
                }
                gst_object_unref(pad);
                pad_ct++;
                break;
              case GST_ITERATOR_RESYNC:
                gst_iterator_resync (pad_iter);
                break;
              case GST_ITERATOR_ERROR:
              case GST_ITERATOR_DONE:
                pads_done = TRUE;
                break;
            }
          }
          gst_iterator_free(pad_iter);
          gst_object_unref(element);
          element_ct++;
          break;
        case GST_ITERATOR_RESYNC:
          gst_iterator_resync (element_iter);
          break;
        case GST_ITERATOR_ERROR:
        case GST_ITERATOR_DONE:
          elements_done = TRUE;
          break;
      }
    }
    gst_iterator_free(element_iter);
    if(all_linked) {
      GST_INFO ("checked %u elementes and %u pads, all linked",element_ct, pad_ct);
    }
    else {
      GST_WARNING ("checked %u elementes and %u pads, not all linked",element_ct, pad_ct);
      bt_song_write_to_lowlevel_dot_file(self);
    }
  }

  // unconnected sources will throw an bus-error-message
  // unconnected effects don't harm

  return(TRUE);
}

static void bt_song_send_tags(const BtSong * const self) {
  GstTagList * const taglist;
  GstIterator *it;
  GstEvent *tag_event;
  gboolean done;
  gpointer item;

  // send tags
  g_object_get(self->priv->song_info,"taglist",&taglist,NULL);
  GST_DEBUG("about to send metadata to tagsetters, taglist=%p",taglist);
  it=gst_bin_iterate_all_by_interface(self->priv->bin,GST_TYPE_TAG_SETTER);
  done=FALSE;
  while(!done) {
    switch(gst_iterator_next(it, &item)) {
      case GST_ITERATOR_OK:
        GST_INFO("sending tags to '%s' element",GST_OBJECT_NAME(GST_ELEMENT(item)));
        gst_tag_setter_merge_tags(GST_TAG_SETTER(item),taglist,GST_TAG_MERGE_REPLACE);
        gst_object_unref(item);
        break;
      case GST_ITERATOR_RESYNC:
        gst_iterator_resync(it);
        break;
      case GST_ITERATOR_ERROR:
        GST_WARNING("wrong parameter for iterator");
        done=TRUE;
        break;
      case GST_ITERATOR_DONE:
        done=TRUE;
        break;
    }
  }
  gst_iterator_free (it);

#if 1
  done=FALSE;
  // should be fixed by:
  // http://freedesktop.org/cgi-bin/viewcvs.cgi/gstreamer/gstreamer/libs/gst/base/gstbasesrc.c.diff?r1=1.139&r2=1.140
  // but is not :/
  //GST_DEBUG("about to send metadata to all sources");
  //it=gst_bin_iterate_sources(self->priv->bin);
  GST_DEBUG("about to send metadata to all elements");
  it=gst_bin_iterate_elements(self->priv->bin);
  tag_event=gst_event_new_tag(gst_structure_copy(taglist));
  while(!done) {
    switch(gst_iterator_next(it, &item)) {
      case GST_ITERATOR_OK:
        GST_DEBUG("sending tags to '%s' element",GST_OBJECT_NAME(GST_ELEMENT(item)));
        if(!(gst_element_send_event(GST_ELEMENT(item),gst_event_ref(tag_event)))) {
          //GST_WARNING("element '%s' failed to handle tag event",GST_OBJECT_NAME(GST_ELEMENT(item)));
        }
        gst_object_unref(item);
        break;
      case GST_ITERATOR_RESYNC:
        gst_iterator_resync(it);
        break;
      case GST_ITERATOR_ERROR:
        GST_WARNING("wrong parameter for iterator");
        done=TRUE;
        break;
      case GST_ITERATOR_DONE:
        done=TRUE;
        break;
    }
  }
  gst_iterator_free (it);
  gst_event_unref(tag_event);
#endif

  gst_tag_list_free(taglist);
}


//-- handler

static void on_song_segment_done(const GstBus * const bus, const GstMessage * const message, gconstpointer user_data) {
  const BtSong * const self = BT_SONG(user_data);
  //GstStateChangeReturn res;

  GST_INFO("received SEGMENT_DONE bus message: %s", GST_OBJECT_NAME (GST_MESSAGE_SRC (message)));

  if(self->priv->is_playing) {
    if(!(gst_element_send_event(GST_ELEMENT(self->priv->master_bin),gst_event_ref(self->priv->loop_seek_event)))) {
      GST_WARNING("element failed to handle continuing play seek event");
    }
    else {
      GST_INFO("-> loop");
      /*
      gst_pipeline_set_new_stream_time (GST_PIPELINE (self->priv->bin), 0);
      gst_element_get_state (GST_ELEMENT (self->priv->bin), NULL, NULL, 40 * GST_MSECOND);
      */
    }
  }
  else {
    GST_WARNING("song isn't playing ?!?");
    /*
    if(!(gst_element_send_event(GST_ELEMENT(self->priv->master_bin),gst_event_ref(self->priv->idle_seek_event)))) {
      GST_WARNING("element failed to handle continuing idle seek event");
    }
    */
  }
}

static void on_song_eos(const GstBus * const bus, const GstMessage * const message, gconstpointer user_data) {
  const BtSong * const self = BT_SONG(user_data);

  if(GST_MESSAGE_SRC(message) == GST_OBJECT(self->priv->bin)) {
    GST_WARNING("received EOS bus message from: %s", 
      GST_OBJECT_NAME (GST_MESSAGE_SRC (message)));
    // @todo: we're getting this too early
    // gst_base_audio_sink_callback() post eos, this runs
    // gst_base_audio_sink_drain()
    bt_song_stop(self);
  }
}

static gboolean on_song_paused_timeout(gpointer user_data) {
  const BtSong * const self = BT_SONG(user_data);

  if(self->priv->is_preparing) {
    GST_WARNING("->PAUSED timeout occurred");
    bt_song_write_to_lowlevel_dot_file(self);
    bt_song_stop(self);
  }
  self->priv->paused_timeout_id=0;
  return(FALSE);
}

static void on_song_state_changed(const GstBus * const bus, GstMessage *message, gconstpointer user_data) {
  const BtSong * const self = BT_SONG(user_data);

  g_assert(user_data);
  //GST_INFO("user_data=%p,<%s>, bin=%p, msg->src=%p,<%s>",
  //  user_data, G_OBJECT_TYPE_NAME(G_OBJECT(user_data)),
  //  self->priv->bin,GST_MESSAGE_SRC(message),G_OBJECT_TYPE_NAME(GST_MESSAGE_SRC(message)));

  if(GST_MESSAGE_SRC(message) == GST_OBJECT(self->priv->bin)) {
    GstState oldstate,newstate,pending;

    gst_message_parse_state_changed(message,&oldstate,&newstate,&pending);
    GST_WARNING("state change on the bin: %s -> %s", 
      gst_element_state_get_name(oldstate),gst_element_state_get_name(newstate));
    switch(GST_STATE_TRANSITION(oldstate,newstate)) {
      /* if we do this in READY_TO_PAUSED, we get two PAUSED -> PAUSED transitions
       * seems to be not the case with gstreamer core.
       */
      //case GST_STATE_CHANGE_READY_TO_PAUSED:
      case GST_STATE_CHANGE_NULL_TO_READY:
        // here the formats are negotiated
        //bt_song_write_to_lowlevel_dot_file(self);

        // we're prepared to play
        self->priv->is_preparing=FALSE;
        // this should be sequence->play_start
        self->priv->play_pos=0;
        // seek to start time
        GST_DEBUG("seek event");
        if(!(gst_element_send_event(GST_ELEMENT(self->priv->master_bin),gst_event_ref(self->priv->play_seek_event)))) {
          GST_WARNING("bin failed to handle seek event");
        }
        break;
      case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
        if(!self->priv->is_playing) {
          GST_INFO("playback started");
          self->priv->is_playing=TRUE;
          g_object_notify(G_OBJECT(self),"is-playing");
        }
        else {
          GST_INFO("looping");
        }
        break;
      default:
        break;
    }
  }
}

static void on_song_async_done(const GstBus * const bus, GstMessage *message, gconstpointer user_data) {
  const BtSong * const self = BT_SONG(user_data);

  g_assert(user_data);
  
  if(GST_MESSAGE_SRC(message) == GST_OBJECT(self->priv->bin)) {
    GST_WARNING("async state-change done");
  }
}

static void bt_song_on_loop_changed(BtSequence * const sequence, GParamSpec * const arg, gconstpointer user_data) {
  bt_song_update_play_seek_event(BT_SONG(user_data));
}

static void bt_song_on_loop_start_changed(BtSequence * const sequence, GParamSpec * const arg, gconstpointer user_data) {
  bt_song_update_play_seek_event(BT_SONG(user_data));
}

static void bt_song_on_loop_end_changed(BtSequence * const sequence, GParamSpec * const arg, gconstpointer user_data) {
  bt_song_update_play_seek_event(BT_SONG(user_data));
}

static void bt_song_on_length_changed(BtSequence * const sequence, GParamSpec * const arg, gconstpointer user_data) {
  bt_song_update_play_seek_event(BT_SONG(user_data));
}

static void bt_song_on_tempo_changed(BtSongInfo * const song_info, GParamSpec * const arg, gconstpointer user_data) {
  bt_song_update_play_seek_event(BT_SONG(user_data));
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
BtSong *bt_song_new(const BtApplication * const app) {
  BtSong *self=NULL;
  GstBin * const bin;

  g_return_val_if_fail(BT_IS_APPLICATION(app),NULL);

  g_object_get(G_OBJECT(app),"bin",&bin,NULL);
  g_assert(bin);

  if(!(self=BT_SONG(g_object_new(BT_TYPE_SONG,"app",app,"bin",bin,NULL)))) {
    goto Error;
  }

  GstBus * const bus=gst_element_get_bus(GST_ELEMENT(bin));
  GST_DEBUG("listen to bus messages (%p)",bus);
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);
  g_signal_connect(bus, "message::segment-done", G_CALLBACK(on_song_segment_done), (gpointer)self);
  g_signal_connect(bus, "message::eos", G_CALLBACK(on_song_eos), (gpointer)self);
  g_signal_connect(bus, "message::state-changed", G_CALLBACK(on_song_state_changed), (gpointer)self);
  g_signal_connect(bus, "message::async-done", G_CALLBACK(on_song_async_done), (gpointer)self);
  gst_object_unref(bus);
  gst_object_unref(bin);

  g_signal_connect(self->priv->sequence,"notify::loop",G_CALLBACK(bt_song_on_loop_changed),(gpointer)self);
  g_signal_connect(self->priv->sequence,"notify::loop-start",G_CALLBACK(bt_song_on_loop_start_changed),(gpointer)self);
  g_signal_connect(self->priv->sequence,"notify::loop-end",G_CALLBACK(bt_song_on_loop_end_changed),(gpointer)self);
  g_signal_connect(self->priv->sequence,"notify::length",G_CALLBACK(bt_song_on_length_changed),(gpointer)self);
  GST_DEBUG("  loop-signals connected");
  g_signal_connect(self->priv->song_info,"notify::tpb",G_CALLBACK(bt_song_on_tempo_changed),(gpointer)self);
  g_signal_connect(self->priv->song_info,"notify::bpm",G_CALLBACK(bt_song_on_tempo_changed),(gpointer)self);
  GST_DEBUG("  tempo-signals connected");

  bt_song_update_play_seek_event(BT_SONG(self));
  bt_song_idle_start(self);
  GST_INFO("  new song created: %p",self);
  return(self);
Error:
  g_object_try_unref(self);
  gst_object_unref(bin);
  return(NULL);
}

//-- methods

/**
 * bt_song_set_unsaved:
 * @self: the song that should be changed
 * @unsaved: the new state of the songs unsaved flag
 *
 * Use this method instead of directly setting the state via g_object_set() to
 * avoid double notifies, if the state is unchanged.
 */
void bt_song_set_unsaved(const BtSong * const self, const gboolean unsaved) {
  g_return_if_fail(BT_IS_SONG(self));

  if(self->priv->unsaved!=unsaved) {
    self->priv->unsaved=unsaved;
    GST_INFO("unsaved = %d",unsaved);
    g_object_notify(G_OBJECT(self),"unsaved");
  }
}

/* @todo required for live mode */

/**
 * bt_song_idle_start:
 * @self: a #BtSong
 *
 * Works like bt_song_play(), but sends a segmented-seek that loops from
 * G_MAXUINT64-GST_SECONF to G_MAXUINT64-1.
 * This is needed to do state changes (mute, solo, bypass) and to play notes
 * live.
 *
 * The application should not be concered about this internal detail. Stopping
 * and restarting the idle loop should only be done, when massive changes are
 * about (e.g. loading a song).
 *
 * Returns: %TRUE for success
 */
gboolean bt_song_idle_start(const BtSong * const self) {
#ifdef __ENABLE_IDLE_LOOP_
  GstStateChangeReturn res;

  // do not idle again
  if(self->priv->is_idle) return(TRUE);

  GST_INFO("prepare idle loop");
  // prepare idle loop
  if((res=gst_element_set_state(GST_ELEMENT(self->priv->bin),GST_STATE_PAUSED))==GST_STATE_CHANGE_FAILURE) {
    GST_WARNING("can't go to paused state");
    return(FALSE);
  }
  GST_DEBUG("state change returned %d",res);

  // seek to start time
  if(!(gst_element_send_event(GST_ELEMENT(self->priv->master_bin),gst_event_ref(self->priv->idle_seek_event)))) {
    GST_WARNING("element failed to handle seek event");
  }

  // start idling
  if((res=gst_element_set_state(GST_ELEMENT(self->priv->bin),GST_STATE_PLAYING))==GST_STATE_CHANGE_FAILURE) {
    GST_WARNING("can't go to playing state");
    return(FALSE);
  }
  GST_DEBUG("state change returned %d",res);
  self->priv->is_idle=TRUE;
  //g_object_notify(G_OBJECT(self),"is-idle");
#endif
  return(TRUE);
}

/**
 * bt_song_idle_stop:
 * @self: a #BtSong
 *
 * Stops the idle loop.
 *
 * Returns: %TRUE for success
 */
gboolean bt_song_idle_stop(const BtSong * const self) {
#ifdef __ENABLE_IDLE_LOOP_
  GstStateChangeReturn res;

  GST_INFO("stopping idle loop");
  // do not stop if not idle
  if(!self->priv->is_idle) return(TRUE);

  if((res=gst_element_set_state(GST_ELEMENT(self->priv->bin),GST_STATE_NULL))==GST_STATE_CHANGE_FAILURE) {
    GST_WARNING("can't go to null state");
    return(FALSE);
  }
  GST_DEBUG("state change returned %d",res);
  self->priv->is_idle=FALSE;
  //g_object_notify(G_OBJECT(self),"is-idle");
#endif
  return(TRUE);
}

/**
 * bt_song_play:
 * @self: the song that should be played
 *
 * Starts to play the specified song instance from beginning.
 * This methods emits the "play" signal.
 *
 * Returns: %TRUE for success
 */
gboolean bt_song_play(const BtSong * const self) {
  GstStateChangeReturn res;

  g_return_val_if_fail(BT_IS_SONG(self),FALSE);

  if(!bt_song_is_playable(self)) return(FALSE);

  // do not play again
  if(self->priv->is_playing) return(TRUE);
  bt_song_idle_stop(self);

  GST_INFO("prepare playback");

  // send tags
  bt_song_send_tags(self);

  // prepare playback
  self->priv->is_preparing=TRUE;
  res=gst_element_set_state(GST_ELEMENT(self->priv->bin),GST_STATE_PLAYING);
  GST_DEBUG("->PAUSED state change returned '%s'",gst_element_state_change_return_get_name(res));
  switch(res) {
    case GST_STATE_CHANGE_FAILURE:
      GST_WARNING("can't go to paused state");
      bt_machine_dbg_print_parts(BT_MACHINE(self->priv->master));
      return(FALSE);
      break;
    case GST_STATE_CHANGE_ASYNC:
      GST_INFO("->PAUSED needs async wait");
      // start a short timeout that aborts playback if if get not started
      self->priv->paused_timeout_id=g_timeout_add(BT_SONG_STATE_CHANGE_TIMEOUT, on_song_paused_timeout, (gpointer)self);
      break;
    default:
      GST_WARNING("unexpected state-change-return %d",res);
      break;
  }
  GST_INFO("playback started");
  return(TRUE);
}

/**
 * bt_song_stop:
 * @self: the song that should be stopped
 *
 * Stops the playback of the specified song instance.
 *
 * Returns: %TRUE for success
 */
gboolean bt_song_stop(const BtSong * const self) {
  GstStateChangeReturn res;

  g_return_val_if_fail(BT_IS_SONG(self),FALSE);

  GST_INFO("stopping playback, is_playing: %d, is_preparing: %d",self->priv->is_playing,self->priv->is_preparing);

  if(!(self->priv->is_preparing || self->priv->is_playing)) {
    return(TRUE);
  }

  if((res=gst_element_set_state(GST_ELEMENT(self->priv->bin),GST_STATE_READY))==GST_STATE_CHANGE_FAILURE) {
    GST_WARNING("can't go to ready state");
  }
  GST_DEBUG("->READY state change returned '%s'",gst_element_state_change_return_get_name(res));
  if((res=gst_element_set_state(GST_ELEMENT(self->priv->bin),GST_STATE_NULL))==GST_STATE_CHANGE_FAILURE) {
    GST_WARNING("can't go to NULL state");
  }
  GST_DEBUG("->NULL state change returned '%s'",gst_element_state_change_return_get_name(res));

  // do not stop if not playing or not preparing
  if(self->priv->is_preparing) {
    self->priv->is_preparing=FALSE;
    goto done;
  }
  if(!self->priv->is_playing)
    goto done;

  self->priv->is_playing=FALSE;

done:
  g_object_notify(G_OBJECT(self),"is-playing");
  bt_song_idle_start(self);

  return(TRUE);
}

/**
 * bt_song_pause:
 * @self: the song that should be paused
 *
 * Pauses the playback of the specified song instance.
 *
 * Returns: %TRUE for success
 */
gboolean bt_song_pause(const BtSong * const self) {
  g_return_val_if_fail(BT_IS_SONG(self),FALSE);
  return(gst_element_set_state(GST_ELEMENT(self->priv->bin),GST_STATE_PAUSED)!=GST_STATE_CHANGE_FAILURE);
}

/**
 * bt_song_continue:
 * @self: the song that should be paused
 *
 * Continues the playback of the specified song instance.
 *
 * Returns: %TRUE for success
 */
gboolean bt_song_continue(const BtSong * const self) {
  g_return_val_if_fail(BT_IS_SONG(self),FALSE);
  return(gst_element_set_state(GST_ELEMENT(self->priv->bin),GST_STATE_PLAYING)!=GST_STATE_CHANGE_FAILURE);
}

/**
 * bt_song_update_playback_position:
 * @self: the song that should update its playback-pos counter
 *
 * Updates the playback-position counter to fire all notify::playback-pos
 * handlers.
 *
 * Returns: %FALSE if the song is not playing
 */
gboolean bt_song_update_playback_position(const BtSong * const self) {
  gint64 pos_cur;

  g_return_val_if_fail(BT_IS_SONG(self),FALSE);
  g_assert(GST_IS_BIN(self->priv->bin));
  g_assert(GST_IS_QUERY(self->priv->position_query));
  //GST_INFO("query playback-pos");

  if(!self->priv->is_playing) return(FALSE);

  // query playback position and update self->priv->play-pos;
  gst_element_query(GST_ELEMENT(self->priv->master_bin),self->priv->position_query);
  gst_query_parse_position(self->priv->position_query,NULL,&pos_cur);
  if(pos_cur!=-1) {
    // calculate new play-pos (in ticks)
    const GstClockTime bar_time=bt_sequence_get_bar_time(self->priv->sequence);
    const gulong play_pos=(gulong)(pos_cur/bar_time);
    if(play_pos!=self->priv->play_pos) {
      self->priv->play_pos=play_pos;
      GST_DEBUG("query playback-pos: cur=%"G_GINT64_FORMAT", tick=%ul",pos_cur,self->priv->play_pos);
      g_object_notify(G_OBJECT(self),"play-pos");
    }
  }
  return(TRUE);
}

/**
 * bt_song_write_to_xml_file:
 * @self: the song that should be written
 *
 * To aid debugging applications one can use this method to write out the whole
 * network of gstreamer elements that form the song into an XML file.
 * The file will be written to '/tmp' and will be named according the 'name'
 * property of the #BtSongInfo.
 * This XML file can be loaded into gst-editor.
 */
void bt_song_write_to_xml_file(const BtSong * const self) {
#ifndef GST_DISABLE_LOADSAVE
  FILE *out;
  BtSongInfo * const song_info;
  gchar * const song_name;
  char ts[10];
  time_t t;

  g_return_if_fail(BT_IS_SONG(self));

  g_object_get(G_OBJECT(self),"song-info",&song_info,NULL);
  g_object_get(song_info,"name",&song_name,NULL);
  gchar * const file_name=g_alloca(strlen(song_name)+20);
  // not overwrite files during a run by adding current time
  t = time(NULL);
  strftime(ts, sizeof(ts), "%T", localtime(&t));
  g_sprintf(file_name,"/tmp/%s_%s.xml",song_name,ts);

  if((out=fopen(file_name,"wb"))) {
    gst_xml_write_file(GST_ELEMENT(self->priv->bin),out);
    fclose(out);
  }
  g_free(song_name);
  g_object_unref(song_info);
#endif
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
void bt_song_write_to_highlevel_dot_file(const BtSong * const self) {
  FILE *out;
  gchar * const song_name;

  g_return_if_fail(BT_IS_SONG(self));

  g_object_get(self->priv->song_info,"name",&song_name,NULL);
  gchar * const file_name=g_alloca(strlen(song_name)+10);
  g_sprintf(file_name,"/tmp/%s_highlevel.dot",song_name);

  /* @idea: improve dot output
   * - make border of main element in machine black, other borders gray
   * - get caps for each wire segment
   *   - use colors for float/int
   *   - use line style for mono/stereo/quadro
   */

  if((out=fopen(file_name,"wb"))) {
    GList * const list,*node,*sublist,*subnode;
    BtMachine * const src,* const dst;
    GstElement *elem;
    GstElementFactory *factory;
    gchar * const id,*label;
    gchar *this_name=NULL,*last_name,*src_name,*dst_name;
    gulong index,count;

    // write header
    fprintf(out,
      "digraph buzztard {\n"
      "  fontname=\"Arial\";"
      "  node [style=filled, shape=box, labelfontsize=\"8\", fontsize=\"8\", fontname=\"Arial\"];\n"
      "\n"
    );

    // iterate over machines list
    g_object_get(self->priv->setup,"machines",&list,NULL);
    for(node=list;node;node=g_list_next(node)) {
      BtMachine * const machine=BT_MACHINE(node->data);
      g_object_get(machine,"id",&id,NULL);
      fprintf(out,
        "  subgraph cluster_%s {\n"
        "    style=filled;\n"
        "    label=\"%s\";\n"
        "    fillcolor=\"%s\";\n"
        "    color=black\n\n"
        ,id,id,BT_IS_SOURCE_MACHINE(machine)?"#FFAAAA":(BT_IS_SINK_MACHINE(machine)?"#AAAAFF":"#AAFFAA"));

      // query internal element of each machine
      sublist=bt_machine_get_element_list(machine);
      last_name=NULL;
      for(subnode=sublist;subnode;subnode=g_list_next(subnode)) {
        elem=GST_ELEMENT(subnode->data);
        this_name=gst_element_get_name(elem);
        factory=gst_element_get_factory(elem);
        label=(gchar *)gst_element_factory_get_longname(factory);
        fprintf(out,"    %s [color=black, fillcolor=white, label=\"%s\"];\n",this_name,label);
        if(last_name) {
          fprintf(out,"    %s -> %s\n",last_name,this_name);
          g_free(last_name);
        }
        last_name=this_name;
      }
      g_list_free(sublist);
      g_free(id);
      if(last_name) g_free(last_name);
      fprintf(out,"  }\n\n");
    }
    g_list_free(list);

    // iterate over wire list
    g_object_get(self->priv->setup,"wires",&list,NULL);
    for(node=list;node;node=g_list_next(node)) {
      BtWire * const wire=BT_WIRE(node->data);
      g_object_get(wire,"src",&src,"dst",&dst,NULL);

      // get last_name of src
      sublist=bt_machine_get_element_list(src);
      subnode=g_list_last(sublist);
      elem=GST_ELEMENT(subnode->data);
      src_name=gst_element_get_name(elem);
      g_list_free(sublist);
      // get first_name of dst
      sublist=bt_machine_get_element_list(dst);
      subnode=g_list_first(sublist);
      elem=GST_ELEMENT(subnode->data);
      dst_name=gst_element_get_name(elem);
      g_list_free(sublist);

      // query internal element of each wire
      sublist=bt_wire_get_element_list(wire);
      count=g_list_length(sublist);
      GST_INFO("wire %s->%s has %d elements",src_name,dst_name,count);
      index=0;
      last_name=NULL;
      for(subnode=sublist;subnode;subnode=g_list_next(subnode)) {
        // skip first and last
        if((index>0) && (index<(count-1))) {
          elem=GST_ELEMENT(subnode->data);
          this_name=gst_element_get_name(elem);
          factory=gst_element_get_factory(elem);
          label=(gchar *)gst_element_factory_get_longname(factory);
          fprintf(out,"    %s [color=black, fillcolor=white, label=\"%s\"];\n",this_name,label);
        }
        else if(index==0) {
          this_name=src_name;
        }
        else if ((index==(count-1))) {
          this_name=dst_name;
        }
        if(last_name) {
          fprintf(out,"    %s -> %s\n",last_name,this_name);
        }
        last_name=this_name;
        index++;
      }
      g_list_free(sublist);

      /*
      fprintf(out,"  %s -> %s\n",src_name,dst_name);
      */

      g_object_unref(src);
      g_object_unref(dst);
    }
    g_list_free(list);

    // write footer
    fprintf(out,"}\n");
    fclose(out);
  }
  g_free(song_name);
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
void bt_song_write_to_lowlevel_dot_file(const BtSong * const self) {
#if !GST_CHECK_VERSION(0,10,15)
  FILE *out;
  gchar * const song_name;

  g_return_if_fail(BT_IS_SONG(self));

  g_object_get(self->priv->song_info,"name",&song_name,NULL);
  g_strcanon(song_name, G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "-_", '_');

  gchar * const file_name=g_alloca(strlen(song_name)+10);
  g_sprintf(file_name,"/tmp/%s_lowlevel.dot",song_name);

  if((out=fopen(file_name,"wb"))) {
    GstIterator *element_iter,*pad_iter;
    gboolean elements_done,pads_done;
    GstElement *element,*peer_element;
    GstPad *pad,*peer_pad;
    GstPadDirection dir;
    GstCaps *caps;
    GstStructure *structure;
    guint src_pads,sink_pads;
    const gchar *src_media,*dst_media;
    gchar *pad_name,*element_name,*peer_pad_name,*peer_element_name;

    // write header
    fprintf(out,
      "digraph buzztard {\n"
      "  rankdir=LR;\n"
      "  fontname=\"Arial\";\n"
      "  fontsize=\"9\";\n"
      "  node [style=filled, shape=box, fontsize=\"8\", fontname=\"Arial\"];\n"
      "  edge [labelfontsize=\"7\", fontsize=\"8\", labelfontname=\"Arial\", fontname=\"Arial\"];\n"
      "\n"
    );

    element_iter=gst_bin_iterate_elements(self->priv->bin);
    elements_done = FALSE;
    while (!elements_done) {
      switch (gst_iterator_next (element_iter, (gpointer)&element)) {
        case GST_ITERATOR_OK:
          element_name=g_strcanon(g_strdup(GST_OBJECT_NAME(element)), G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "-_", '_');
          fprintf(out,
            "  subgraph cluster_%s {\n"
            "    fontname=\"Arial\";\n"
            "    fontsize=\"9\";\n"
            "    style=filled;\n"
            "    label=\"<%s>\\n%s\";\n"
            "    color=black;\n\n",
            element_name,G_OBJECT_TYPE_NAME(element),GST_OBJECT_NAME(element));
          g_free(element_name);

          pad_iter=gst_element_iterate_pads(element);
          pads_done=FALSE;
          src_pads=sink_pads=0;
          while (!pads_done) {
            switch (gst_iterator_next (pad_iter, (gpointer)&pad)) {
              case GST_ITERATOR_OK:
                dir=gst_pad_get_direction(pad);
                pad_name=g_strcanon(g_strdup(GST_OBJECT_NAME(pad)), G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "-_", '_');
                element_name=g_strcanon(g_strdup(GST_OBJECT_NAME(element)), G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "-_", '_');
                fprintf(out,"    %s_%s [color=black, fillcolor=\"%s\", label=\"%s\"];\n",
                  element_name,pad_name,
                  ((dir==GST_PAD_SRC)?"#FFAAAA":((dir==GST_PAD_SINK)?"#AAAAFF":"#FFFFFF")),
                  GST_OBJECT_NAME(pad));
                if(dir==GST_PAD_SRC) src_pads++;
                else if(dir==GST_PAD_SINK) sink_pads++;
                g_free(pad_name);
                g_free(element_name);
                gst_object_unref(pad);
                break;
              case GST_ITERATOR_RESYNC:
                gst_iterator_resync (pad_iter);
                break;
              case GST_ITERATOR_ERROR:
              case GST_ITERATOR_DONE:
                pads_done = TRUE;
                break;
            }
          }
          if(src_pads && !sink_pads)
            fprintf(out,"    fillcolor=\"#FFAAAA\";\n");
          else if (!src_pads && sink_pads)
            fprintf(out,"    fillcolor=\"#AAAAFF\";\n");
          else if (src_pads && sink_pads)
            fprintf(out,"    fillcolor=\"#AAFFAA\";\n");
          else
            fprintf(out,"    fillcolor=\"#FFFFFF\";\n");
          gst_iterator_free(pad_iter);
          fprintf(out,"  }\n\n");
          pad_iter=gst_element_iterate_pads(element);
          pads_done=FALSE;
          while (!pads_done) {
            switch (gst_iterator_next (pad_iter, (gpointer)&pad)) {
              case GST_ITERATOR_OK:
                if(gst_pad_is_linked(pad) && gst_pad_get_direction(pad)==GST_PAD_SRC) {
                  if((peer_pad=gst_pad_get_peer(pad))) {
                    if((caps=gst_pad_get_negotiated_caps(pad))) {
                      if(GST_CAPS_IS_SIMPLE(caps)) {
                        structure=gst_caps_get_structure(caps,0);
                        src_media=gst_structure_get_name(structure);
                      }
                      else src_media="!";
                      gst_caps_unref(caps);
                      //src_media=gst_caps_to_string(caps);
                      //needs to be formatted/filtered and freed later
                    }
                    else src_media="?";
                    if((caps=gst_pad_get_negotiated_caps(peer_pad))) {
                      if(GST_CAPS_IS_SIMPLE(caps)) {
                        structure=gst_caps_get_structure(caps,0);
                        dst_media=gst_structure_get_name(structure);
                      }
                      else dst_media="!";
                      gst_caps_unref(caps);
                    }
                    else dst_media="?";

                    peer_element=gst_pad_get_parent_element(peer_pad);
                    pad_name=g_strcanon(g_strdup(GST_OBJECT_NAME(pad)), G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "-_", '_');
                    element_name=g_strcanon(g_strdup(GST_OBJECT_NAME(element)), G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "-_", '_');
                    peer_pad_name=g_strcanon(g_strdup(GST_OBJECT_NAME(peer_pad)), G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "-_", '_');
                    peer_element_name=g_strcanon(g_strdup(GST_OBJECT_NAME(peer_element)), G_CSET_A_2_Z G_CSET_a_2_z G_CSET_DIGITS "-_", '_');
                    fprintf(out,"  %s_%s -> %s_%s [taillabel=\"%s\",headlabel=\"%s\"]\n",
                      element_name,pad_name, peer_element_name,peer_pad_name,
                      src_media,dst_media);

                    g_free(pad_name);
                    g_free(element_name);
                    g_free(peer_pad_name);
                    g_free(peer_element_name);
                    gst_object_unref(peer_element);
                    gst_object_unref(peer_pad);
                  }
                }
                gst_object_unref(pad);
                break;
              case GST_ITERATOR_RESYNC:
                gst_iterator_resync (pad_iter);
                break;
              case GST_ITERATOR_ERROR:
              case GST_ITERATOR_DONE:
                pads_done = TRUE;
                break;
            }
          }
          gst_iterator_free(pad_iter);
          gst_object_unref(element);
          break;
        case GST_ITERATOR_RESYNC:
          gst_iterator_resync (element_iter);
          break;
        case GST_ITERATOR_ERROR:
        case GST_ITERATOR_DONE:
          elements_done = TRUE;
          break;
      }
    }
    gst_iterator_free(element_iter);

    // write footer
    fprintf(out,"}\n");
    fclose(out);
  }
  g_free(song_name);
#else
  GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(self->priv->bin,
    /*GST_DEBUG_GRAPH_SHOW_ALL,*/
    GST_DEBUG_GRAPH_SHOW_CAPS_DETAILS|GST_DEBUG_GRAPH_SHOW_STATES,
    PACKAGE_NAME);
#endif
}

//-- child proxy interface

#if 0
/* @todo: this only works if I turn the children into GstObject
 * the reason is that gst_child_proxy_get_child_by_name() is not a virtual method
 * in the below case we could still map names to objects
 */
static GstObject *bt_song_child_proxy_get_child_by_index(GstChildProxy * child_proxy, guint index) {
  const BtSong * const self = BT_SONG(persistence);
  GstObject *res = NULL;

  switch(index) {
    case 0: res=self->priv->song_info;
    case 1: res=self->priv->sequence;
    case 2: res=self->priv->setup;
    case 3: res=self->priv->wavtable;
  }
  if(res)
    g_object_ref (res);
  return res;
}

guint bt_song_child_proxy_get_children_count(GstChildProxy * child_proxy) {
  return 4;
}

static void
bt_song_child_proxy_init (gpointer g_iface, gpointer iface_data)
{
  GstChildProxyInterface *iface = g_iface;

  iface->get_children_count = bt_song_child_proxy_get_children_count;
  iface->get_child_by_index = bt_song_child_proxy_get_child_by_index;
}
#endif

//-- io interface

static xmlNodePtr bt_song_persistence_save(const BtPersistence * const persistence, xmlNodePtr const parent_node, const BtPersistenceSelection * const selection) {
  const BtSong * const self = BT_SONG(persistence);
  xmlNodePtr node=NULL;

  GST_DEBUG("PERSISTENCE::song");

  if((node=xmlNewNode(NULL,XML_CHAR_PTR("buzztard")))) {
    xmlNewProp(node,XML_CHAR_PTR("xmlns"),(const xmlChar *)"http://www.buzztard.org/");
    xmlNewProp(node,XML_CHAR_PTR("xmlns:xsd"),XML_CHAR_PTR("http://www.w3.org/2001/XMLSchema-instance"));
    xmlNewProp(node,XML_CHAR_PTR("xsd:noNamespaceSchemaLocation"),XML_CHAR_PTR("buzztard.xsd"));

    bt_persistence_save(BT_PERSISTENCE(self->priv->song_info),node,NULL);
    bt_persistence_save(BT_PERSISTENCE(self->priv->setup),node,NULL);
    bt_persistence_save(BT_PERSISTENCE(self->priv->sequence),node,NULL);
    bt_persistence_save(BT_PERSISTENCE(self->priv->wavetable),node,NULL);
  }
  return(node);
}

static gboolean bt_song_persistence_load(const BtPersistence * const persistence, xmlNodePtr node, const BtPersistenceLocation * const location) {
  const BtSong * const self = BT_SONG(persistence);
  gboolean res=TRUE;

  GST_DEBUG("PERSISTENCE::song");
  g_assert(node);

  res=TRUE;
  for(node=node->children;(node && res);node=node->next) {
    if(!xmlNodeIsText(node)) {
      if(!strncmp((gchar *)node->name,"meta\0",5)) {
        res&=bt_persistence_load(BT_PERSISTENCE(self->priv->song_info),node,NULL);
      }
      else if(!strncmp((gchar *)node->name,"setup\0",6)) {
        res&=bt_persistence_load(BT_PERSISTENCE(self->priv->setup),node,NULL);
      }
      else if(!strncmp((gchar *)node->name,"sequence\0",9)) {
        res&=bt_persistence_load(BT_PERSISTENCE(self->priv->sequence),node,NULL);
      }
      else if(!strncmp((gchar *)node->name,"wavetable\0",10)) {
        res&=bt_persistence_load(BT_PERSISTENCE(self->priv->wavetable),node,NULL);
      }
    }
  }
  if(!res && node) {
    GST_INFO("failed to load %s",(gchar *)node->name);
  }
  return(res);
}

static void bt_song_persistence_interface_init(gpointer const g_iface, gpointer const iface_data) {
  BtPersistenceInterface * const iface = g_iface;

  iface->load = bt_song_persistence_load;
  iface->save = bt_song_persistence_save;
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_song_get_property(GObject      * const object,
                               const guint         property_id,
                               GValue       * const value,
                               GParamSpec   * const pspec)
{
  const BtSong * const self = BT_SONG(object);
  return_if_disposed();
  switch (property_id) {
    case SONG_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    case SONG_BIN: {
      g_value_set_object(value, self->priv->bin);
    } break;
    case SONG_MASTER: {
      g_value_set_object(value, self->priv->master);
    } break;
    case SONG_SONG_INFO: {
      g_value_set_object(value, self->priv->song_info);
    } break;
    case SONG_SEQUENCE: {
      g_value_set_object(value, self->priv->sequence);
    } break;
    case SONG_SETUP: {
      g_value_set_object(value, self->priv->setup);
    } break;
    case SONG_WAVETABLE: {
      g_value_set_object(value, self->priv->wavetable);
    } break;
    case SONG_UNSAVED: {
      g_value_set_boolean(value, self->priv->unsaved);
    } break;
    case SONG_PLAY_POS: {
      g_value_set_ulong(value, self->priv->play_pos);
    } break;
    case SONG_IS_PLAYING: {
      g_value_set_boolean(value, self->priv->is_playing);
    } break;
    case SONG_IO: {
      g_value_set_object(value, self->priv->song_io);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_song_set_property(GObject      * const object,
                              const guint         property_id,
                              const GValue * const value,
                              GParamSpec   * const pspec)
{
  const BtSong * const self = BT_SONG(object);
  return_if_disposed();
  switch (property_id) {
    case SONG_APP: {
      g_object_try_weak_unref(self->priv->app);
      self->priv->app = BT_APPLICATION(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->app);
      GST_DEBUG("set the app for the song: %p",self->priv->app);
    } break;
    case SONG_BIN: {
      if(self->priv->bin) gst_object_unref(self->priv->bin);
      self->priv->bin=GST_BIN(g_value_dup_object(value));
      GST_DEBUG("set the bin for the song: %p",self->priv->bin);
    } break;
    case SONG_MASTER: {
      g_object_try_weak_unref(self->priv->master);
      self->priv->master = BT_SINK_MACHINE(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->master);
      g_object_get(G_OBJECT(self->priv->master),"machine",&self->priv->master_bin,NULL);
      GST_DEBUG("set the master for the song: %p (machine-refs: %d)",self->priv->master,(G_OBJECT(self->priv->master))->ref_count);
    } break;
    case SONG_UNSAVED: {
      self->priv->unsaved = g_value_get_boolean(value);
      GST_WARNING("set the unsaved flag for the song: %d (please use bt_song_set_unsaved())",self->priv->unsaved);
    } break;
    case SONG_PLAY_POS: {
      self->priv->play_pos=bt_sequence_limit_play_pos(self->priv->sequence,g_value_get_ulong(value));
      GST_DEBUG("set the play-pos for sequence: %lu",self->priv->play_pos);
      // seek on playpos changes (if playing)
      bt_song_seek_to_play_pos(self);
    } break;
    case SONG_IO: {
      if(self->priv->song_io) g_object_unref(self->priv->song_io);
      self->priv->song_io=BT_SONG_IO(g_value_dup_object(value));
      GST_DEBUG("set the song-io plugin for the song: %p",self->priv->song_io);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_song_dispose(GObject * const object) {
  const BtSong * const self = BT_SONG(object);
  GstStateChangeReturn res;

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  //DEBUG
  //bt_song_write_to_xml_file(self);
  //DEBUG

  GST_DEBUG("!!!! self=%p",self);
  
  if(self->priv->playback_timeout_id)
    g_source_remove(self->priv->playback_timeout_id);
  if(self->priv->paused_timeout_id)
    g_source_remove(self->priv->paused_timeout_id);

  if(self->priv->is_playing) bt_song_stop(self);
  else if(self->priv->is_idle) bt_song_idle_stop(self);

  if((res=gst_element_set_state(GST_ELEMENT(self->priv->bin),GST_STATE_NULL))==GST_STATE_CHANGE_FAILURE) {
    GST_WARNING("can't go to null state");
  }
  GST_DEBUG("->NULL state change returned '%s'",gst_element_state_change_return_get_name(res));

  g_signal_handlers_disconnect_matched(self->priv->sequence,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,bt_song_on_loop_changed,(gpointer)self);
  g_signal_handlers_disconnect_matched(self->priv->sequence,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,bt_song_on_loop_start_changed,(gpointer)self);
  g_signal_handlers_disconnect_matched(self->priv->sequence,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,bt_song_on_loop_end_changed,(gpointer)self);
  g_signal_handlers_disconnect_matched(self->priv->sequence,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,bt_song_on_length_changed,(gpointer)self);
  g_signal_handlers_disconnect_matched(self->priv->song_info,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,bt_song_on_tempo_changed,(gpointer)self);

  GstBus * const bus=gst_element_get_bus(GST_ELEMENT(self->priv->bin));
  g_signal_handlers_disconnect_matched(bus,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_song_state_changed,(gpointer)self);
  g_signal_handlers_disconnect_matched(bus,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_song_segment_done,(gpointer)self);
  g_signal_handlers_disconnect_matched(bus,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_song_eos,(gpointer)self);
  g_signal_handlers_disconnect_matched(bus,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_song_async_done,(gpointer)self);
  gst_bus_remove_signal_watch(bus);
  gst_object_unref(bus);

  if(self->priv->master) GST_DEBUG("sink-machine-refs: %d",(G_OBJECT(self->priv->master))->ref_count);
  g_object_try_unref(self->priv->master_bin);
  g_object_try_weak_unref(self->priv->master);
  GST_DEBUG("refs: song_info: %d, sequence: %d, setup: %d, wavetable: %d",
    (G_OBJECT(self->priv->song_info))->ref_count,
    (G_OBJECT(self->priv->sequence))->ref_count,
    (G_OBJECT(self->priv->setup))->ref_count,
    (G_OBJECT(self->priv->wavetable))->ref_count
  );
  g_object_try_unref(self->priv->song_info);
  g_object_try_unref(self->priv->sequence);
  g_object_try_unref(self->priv->setup);
  g_object_try_unref(self->priv->wavetable);
  gst_query_unref(self->priv->position_query);
  if(self->priv->play_seek_event) gst_event_unref(self->priv->play_seek_event);
  if(self->priv->loop_seek_event) gst_event_unref(self->priv->loop_seek_event);
  if(self->priv->idle_seek_event) gst_event_unref(self->priv->idle_seek_event);
  gst_object_unref(self->priv->bin);
  g_object_try_weak_unref(self->priv->app);

  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(parent_class)->dispose(object);
  GST_DEBUG("  done");
}

static void bt_song_finalize(GObject * const object) {
  const BtSong * const self = BT_SONG(object);

  GST_DEBUG("!!!! self=%p",self);

  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(parent_class)->finalize(object);
  GST_DEBUG("  done");
}

static void bt_song_init(const GTypeInstance * const instance, gconstpointer const g_class) {
  BtSong * const self = BT_SONG(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_SONG, BtSongPrivate);

  /* don't change the order */
  self->priv->song_info=bt_song_info_new(self);
  self->priv->setup    =bt_setup_new(self);
  self->priv->sequence =bt_sequence_new(self);
  self->priv->wavetable=bt_wavetable_new(self);

  self->priv->position_query=gst_query_new_position(GST_FORMAT_TIME);

  self->priv->idle_seek_event = gst_event_new_seek(1.0, GST_FORMAT_TIME,
    GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT,
    GST_SEEK_TYPE_SET, (GstClockTime)(G_MAXUINT64-GST_SECOND),
    GST_SEEK_TYPE_SET, (GstClockTime)(G_MAXUINT64-1));
  GST_DEBUG("  done");
}

static void bt_song_class_init(BtSongClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtSongPrivate));

  gobject_class->set_property = bt_song_set_property;
  gobject_class->get_property = bt_song_get_property;
  gobject_class->dispose      = bt_song_dispose;
  gobject_class->finalize     = bt_song_finalize;

  g_object_class_install_property(gobject_class,SONG_APP,
                                  g_param_spec_object("app",
                                     "app contruct prop",
                                     "set application object, the song belongs to",
                                     BT_TYPE_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SONG_BIN,
                                  g_param_spec_object("bin",
                                     "bin construct prop",
                                     "songs top-level GstElement container",
                                     GST_TYPE_BIN, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SONG_MASTER,
                                  g_param_spec_object("master",
                                     "master prop",
                                     "songs audio_sink",
                                     BT_TYPE_SINK_MACHINE, /* object type */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SONG_SONG_INFO,
                                  g_param_spec_object("song-info",
                                     "song-info prop",
                                     "songs metadata sub object",
                                     BT_TYPE_SONG_INFO, /* object type */
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SONG_SEQUENCE,
                                  g_param_spec_object("sequence",
                                     "sequence prop",
                                     "songs sequence sub object",
                                     BT_TYPE_SEQUENCE, /* object type */
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SONG_SETUP,
                                  g_param_spec_object("setup",
                                     "setup prop",
                                     "songs setup sub object",
                                     BT_TYPE_SETUP, /* object type */
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SONG_WAVETABLE,
                                  g_param_spec_object("wavetable",
                                     "wavetable prop",
                                     "songs wavetable sub object",
                                     BT_TYPE_WAVETABLE, /* object type */
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SONG_UNSAVED,
                                  g_param_spec_boolean("unsaved",
                                     "unsaved prop",
                                     "tell wheter the current state of the song has been saved",
                                     TRUE,
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SONG_PLAY_POS,
                                  g_param_spec_ulong("play-pos",
                                     "play-pos prop",
                                     "position of the play cursor of the sequence in timeline bars",
                                     0,
                                     G_MAXLONG,  // loop-positions are LONG as well
                                     0,
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SONG_IS_PLAYING,
                                  g_param_spec_boolean("is-playing",
                                     "is-playing prop",
                                     "tell wheter the song is playing right now or not",
                                     FALSE,
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SONG_IO,
                                  g_param_spec_object("song-io",
                                     "song-io prop",
                                     "the song-io plugin during i/o operations",
                                     BT_TYPE_SONG_IO, /* object type */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));
}

GType bt_song_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof(BtSongClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_song_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtSong),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_song_init, // instance_init
      NULL // value_table
    };
    const GInterfaceInfo persistence_interface_info = {
      (GInterfaceInitFunc) bt_song_persistence_interface_init,  // interface_init
      NULL, // interface_finalize
      NULL  // interface_data
    };
#if 0
    static const GInterfaceInfo child_proxy_info = {
      (GInterfaceInitFunc) bt_song_child_proxy_init,
      NULL,
      NULL
    };
#endif

    type = g_type_register_static(G_TYPE_OBJECT,"BtSong",&info,0);
    g_type_add_interface_static(type, BT_TYPE_PERSISTENCE, &persistence_interface_info);
#if 0
    g_type_add_interface_static(type, GST_TYPE_CHILD_PROXY, &child_proxy_info);
#endif
  }
  return type;
}
