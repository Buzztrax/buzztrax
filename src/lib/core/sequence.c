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
 * SECTION:btsequence
 * @short_description: class for the event timeline of a #BtSong instance
 *
 * A sequence holds grid of #BtCmdPatterns, with labels on the time axis and
 * #BtMachine instances on the track axis. It tracks first and last use of
 * patterns and provides two signals for notification -
 * #BtSequence::pattern-added and #BtSequence::pattern-removed.
 * The labels are exported as a #GstToc, readable through the #BtSequence:toc
 * property.
 *
 * It supports looping a section of the sequence (see #BtSequence:loop,
 * #BtSequence:loop-start, #BtSequence:loop-end).
 *
 * The sequence is not aware of timing related information; for this take a look
 * at #BtSongInfo.
 */
/* TODO(ensonic): introduce a BtTrack object
 * - the sequence will have a array of tracks
 * - each track has a machine and a array with patterns
 * - this makes it easier to e.g. pass old track data to track-removed
 * - also insert/delete can be done per track
 * - need to be careful to make getting the pattern not slow
 * - this would also make it easier to have BtTrackGroups
 *   - essentially an array of BtTracks per machine (in the order of appearance
 *     in the sequence)
 *   - this would make it more easy to determine the effective pattern cells for
 *     the controller
 *   - when reordering tracks, the per machine lists would need to be updated as
 *     well
 *
 * Sequence -+- Track 1 -+- TrackGroup 1
 *           +- Track 2 -´
 *           +- Track 3 --- TrackGroup 2
 *           +- Track 4 -+- TrackGroup 3
 *           `- Track 5 -´
 */

#define BT_CORE
#define BT_SEQUENCE_C

#include "core_private.h"

//-- signal ids

enum
{
  PATTERN_ADDED_EVENT,
  PATTERN_REMOVED_EVENT,
  SEQUENCE_ROWS_CHANGED_EVENT,
  TRACK_ADDED_EVENT,
  TRACK_REMOVED_EVENT,
  LAST_SIGNAL
};

//-- property ids

enum
{
  SEQUENCE_SONG = 1,
  SEQUENCE_LENGTH,
  SEQUENCE_TRACKS,
  SEQUENCE_LOOP,
  SEQUENCE_LOOP_START,
  SEQUENCE_LOOP_END,
  SEQUENCE_PROPERTIES,
  SEQUENCE_TOC
};

struct _BtSequencePrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the song the sequence belongs to */
  BtSong *song;

  /* the number of timeline entries */
  gulong length;

  /* the number of tracks */
  gulong tracks;

  /* loop mode on/off */
  gboolean loop;

  /* the timeline entries where the loop starts and ends
   * -1 for both mean to use 0...length
   */
  glong loop_start, loop_end;

  /* <tracks> machine entries that are the heading of the sequence */
  BtMachine **machines;
  /* <length> label entries that are the description of the time axis */
  gchar **labels;
  /* <length>*<tracks> BtCmdPattern pointers */
  BtCmdPattern **patterns;

  /* playback range variables */
  gulong play_start, play_end;

  /* we cache the number of time a pattern is referenced. This way way we have a
   * fast bt_sequence_is_pattern_used() and we can lower the ref-count of the
   * pattern. We will also add singals to notify on first use and after last
   * use. We only track data patterns, for pure command patterns it does not
   * matter if they are used or not (they always exist).
   */
  GHashTable *pattern_usage;

  /* (ui) properties */
  GHashTable *properties;

  /* toc containing the labels */
  GstToc *toc;
};

static guint signals[LAST_SIGNAL] = { 0, };

//-- the class

static void bt_sequence_persistence_interface_init (gpointer const g_iface,
    gpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtSequence, bt_sequence, G_TYPE_OBJECT,
    G_ADD_PRIVATE(BtSequence)
    G_IMPLEMENT_INTERFACE (BT_TYPE_PERSISTENCE,
        bt_sequence_persistence_interface_init));


//-- helper methods

/*
 * bt_sequence_get_toc:
 * @self: the sequence
 *
 * Get the toc containing a cue sheet of the labels. The toc is build as needed.
 *
 * Returns: the toc, unref when done.
 */
static GstToc *
bt_sequence_get_toc (const BtSequence * const self)
{
  if (!self->priv->toc) {
    GstTocEntry *entry, *subentry, *prev_entry = NULL;
    GstTagList *tags;
    GstClockTime duration, tick_duration, start = G_GUINT64_CONSTANT (0), stop;
    gchar *id;
    gulong i;

    self->priv->toc = gst_toc_new (GST_TOC_SCOPE_GLOBAL);

    bt_child_proxy_get (self->priv->song, "song-info::tick-duration",
        &tick_duration, NULL);
    duration = tick_duration * self->priv->length;

    entry = gst_toc_entry_new (GST_TOC_ENTRY_TYPE_EDITION, "cue");
    gst_toc_entry_set_start_stop_times (entry, 0, duration);
    gst_toc_append_entry (self->priv->toc, entry);

    for (i = 0; i < self->priv->length; i++) {
      const gchar *const label = self->priv->labels[i];
      if (label) {
        id = g_strdup_printf ("%08lx", i);
        subentry = gst_toc_entry_new (GST_TOC_ENTRY_TYPE_TRACK, id);
        g_free (id);
        tags = gst_tag_list_new_empty ();
        gst_tag_list_add (tags, GST_TAG_MERGE_APPEND, GST_TAG_TITLE, label,
            NULL);
        gst_toc_entry_set_tags (subentry, tags);
        gst_toc_entry_append_sub_entry (entry, subentry);

        stop = tick_duration * i;
        if (prev_entry) {
          gst_toc_entry_set_start_stop_times (prev_entry, start, stop);
        }
        prev_entry = subentry;
        start = stop;
      }
    }
    if (prev_entry) {
      gst_toc_entry_set_start_stop_times (prev_entry, start, duration);
    }
  }
  return gst_toc_ref (self->priv->toc);
}

/*
 * bt_sequence_release_toc:
 * @self: the sequence
 *
 * Release the cached toc. Use this when the sequence labels change.
 */
static void
bt_sequence_release_toc (const BtSequence * const self)
{
  if (self->priv->toc) {
    gst_toc_unref (self->priv->toc);
    self->priv->toc = NULL;
  }
}

/*
 * bt_sequence_get_number_of_pattern_uses:
 * @self: the sequence to count the patterns in
 * @pattern: the pattern to check for
 *
 * Determine the number of times a pattern is in use.
 *
 * Returns: the pattern count
 */
static inline gulong
bt_sequence_get_number_of_pattern_uses (const BtSequence * const self,
    const BtPattern * const pattern)
{
  return GPOINTER_TO_UINT (g_hash_table_lookup (self->priv->pattern_usage,
          pattern));
}

/*
 * bt_sequence_test_pattern:
 * @self: the #BtSequence that holds the patterns
 * @time: the requested time position
 * @track: the requested track index
 *
 * Checks if there is any pattern at the given location.
 * Avoides the reffing overhead of bt_sequence_get_pattern().
 *
 * Returns: %TRUE if there is a pattern at the given location
 */
static inline gboolean
bt_sequence_test_pattern (const BtSequence * const self, const gulong time,
    const gulong track)
{
  /*g_return_val_if_fail(BT_IS_SEQUENCE(self),FALSE);
     g_return_val_if_fail(time<self->priv->length,FALSE);
     g_return_val_if_fail(track<self->priv->tracks,FALSE);
   */
  return (self->priv->patterns[time * self->priv->tracks + track] != NULL);
}

static void
bt_sequence_use_pattern (const BtSequence * const self,
    BtCmdPattern * cmd_pattern)
{
  BtPattern *pattern;
  guint count;

  if (!BT_IS_PATTERN (cmd_pattern))
    return;
  pattern = (BtPattern *) cmd_pattern;

  GST_DEBUG ("init for new pattern %p", pattern);

  // update use count
  count = bt_sequence_get_number_of_pattern_uses (self, pattern);
  g_hash_table_insert (self->priv->pattern_usage, (gpointer) pattern,
      GUINT_TO_POINTER (count + 1));
  // check if this is the first usage
  if (count == 0) {
    // take one shared ref
    g_object_ref (pattern);

    GST_DEBUG ("first use of pattern %p", pattern);
    g_signal_emit ((gpointer) self, signals[PATTERN_ADDED_EVENT], 0, pattern);
  }
}

static void
bt_sequence_unuse_pattern (const BtSequence * const self,
    BtCmdPattern * cmd_pattern)
{
  BtPattern *pattern;
  guint count;

  if (!BT_IS_PATTERN (cmd_pattern))
    return;
  pattern = (BtPattern *) cmd_pattern;

  GST_DEBUG ("clean up for pattern %p", pattern);

  count = bt_sequence_get_number_of_pattern_uses (self, pattern);
  // check if this is the last usage
  if (count == 1) {
    g_signal_emit ((gpointer) self, signals[PATTERN_REMOVED_EVENT], 0, pattern);
    // release the shared ref
    g_object_unref (pattern);
  }
  // update use count
  if (count > 0) {
    g_hash_table_insert (self->priv->pattern_usage, pattern,
        GUINT_TO_POINTER (count - 1));
  } else {
    GST_WARNING
        ("use count for pattern %p, is 0 while we were expecting it to be >0",
        pattern);
  }
}

static BtCmdPattern *
bt_sequence_get_pattern_unchecked (const BtSequence * const self,
    const gulong time, const gulong track)
{
  //GST_DEBUG("get pattern at time %d, track %d",time, track);
  return self->priv->patterns[time * self->priv->tracks + track];
}

static BtMachine *
bt_sequence_get_machine_unchecked (const BtSequence * const self,
    const gulong track)
{
  //GST_DEBUG("getting machine : %" G_OBJECT_REF_COUNT_FMT,
  //    G_OBJECT_LOG_REF_COUNT(self->priv->machines[track]));
  return self->priv->machines[track];
}

/*
 * bt_sequence_resize_data_length:
 * @self: the sequence to resize the length
 * @length: the old length
 *
 * Resizes the pattern data grid to the new length. Keeps previous values.
 */
static void
bt_sequence_resize_data_length (const BtSequence * const self,
    const gulong old_length)
{
  const gulong tracks = self->priv->tracks;
  const gulong new_length = self->priv->length;
  const gulong old_data_count = old_length * tracks;
  const gulong new_data_count = new_length * tracks;
  BtCmdPattern **const patterns = self->priv->patterns;
  gchar **const labels = self->priv->labels;

  // allocate new space
  if ((self->priv->patterns =
          (BtCmdPattern **) g_try_new0 (gpointer, new_data_count))) {
    if (patterns) {
      const gulong count = MIN (old_data_count, new_data_count);
      // copy old values over
      memcpy (self->priv->patterns, patterns, count * sizeof (gpointer));
      // free old data
      if (old_length > new_length) {
        gulong i, j, k;
        k = new_data_count;
        for (i = new_length; i < old_length; i++) {
          for (j = 0; j < tracks; j++, k++) {
            if (patterns[k])
              bt_sequence_unuse_pattern (self, patterns[k]);
          }
        }
      }
      g_free (patterns);
    }
  } else {
    GST_INFO
        ("extending sequence length from %lu to %lu failed : data_count=%lu = length=%lu * tracks=%lu",
        old_length, new_length, new_data_count, new_length, self->priv->tracks);
  }
  // allocate new space
  if ((self->priv->labels = (gchar **) g_try_new0 (gpointer, new_length))) {
    if (labels) {
      const gulong count = MIN (old_length, new_length);
      // copy old values over
      memcpy (self->priv->labels, labels, count * sizeof (gpointer));
      // free old data
      if (old_length > new_length) {
        gulong i;
        for (i = new_length; i < old_length; i++) {
          g_free (labels[i]);
        }
      }
      g_free (labels);
    }
    bt_sequence_release_toc (self);
  } else {
    GST_INFO ("extending sequence labels from %lu to %lu failed", old_length,
        new_length);
  }
}

/*
 * bt_sequence_resize_data_tracks:
 * @self: the sequence to resize the tracks number
 * @old_tracks: the old number of tracks
 *
 * Resizes the pattern data grid to the new number of tracks (adds and removes
 * at the end). Keeps previous values.
 */
static void
bt_sequence_resize_data_tracks (const BtSequence * const self,
    const gulong old_tracks)
{
  const gulong length = self->priv->length;
  const gulong new_tracks = self->priv->tracks;
  //gulong old_data_count=length*old_tracks;
  const gulong new_data_count = length * new_tracks;
  BtCmdPattern **const patterns = self->priv->patterns;
  BtMachine **const machines = self->priv->machines;
  const gulong count = MIN (old_tracks, self->priv->tracks);

  GST_DEBUG ("resize tracks %lu -> %lu to new_data_count=%lu", old_tracks,
      new_tracks, new_data_count);

  // allocate new space
  if ((self->priv->patterns =
          (BtCmdPattern **) g_try_new0 (GValue, new_data_count))) {
    if (patterns) {
      gulong i;
      BtCmdPattern **src, **dst;

      // copy old values over
      src = patterns;
      dst = self->priv->patterns;
      for (i = 0; i < length; i++) {
        memcpy (dst, src, count * sizeof (gpointer));
        src = &src[old_tracks];
        dst = &dst[self->priv->tracks];
      }
      // free old data
      if (old_tracks > new_tracks) {
        gulong j, k;
        for (i = 0; i < length; i++) {
          k = i * old_tracks + new_tracks;
          for (j = new_tracks; j < old_tracks; j++, k++) {
            if (patterns[k])
              bt_sequence_unuse_pattern (self, patterns[k]);
          }
        }
      }
      g_free (patterns);
    }
  } else {
    GST_INFO
        ("extending sequence tracks from %lu to %lu failed : data_count=%lu = length=%lu * tracks=%lu",
        old_tracks, new_tracks, new_data_count, self->priv->length, new_tracks);
  }
  // allocate new space
  if (new_tracks) {
    if ((self->priv->machines =
            (BtMachine **) g_try_new0 (gpointer, new_tracks))) {
      if (machines) {
        // copy old values over
        memcpy (self->priv->machines, machines, count * sizeof (gpointer));
        // free old data
        if (old_tracks > new_tracks) {
          gulong i;
          for (i = new_tracks; i < old_tracks; i++) {
            GST_INFO_OBJECT (machines[i],
                "release machine %" G_OBJECT_REF_COUNT_FMT " for track %lu",
                G_OBJECT_LOG_REF_COUNT (machines[i]), i);
            g_object_try_unref (machines[i]);
          }
        }
        g_free (machines);
      }
    } else
      self->priv->machines = NULL;
  } else {
    GST_INFO ("extending sequence machines from %lu to %lu failed", old_tracks,
        new_tracks);
  }
}

/*
 * bt_sequence_limit_play_pos_internal:
 * @self: the sequence to trim the play position of
 *
 * Enforce the playback position to be within loop start and end or the song
 * bounds if there is no loop.
 */
static void
bt_sequence_limit_play_pos_internal (const BtSequence * const self)
{
  gulong old_play_pos, new_play_pos;

  g_object_get (self->priv->song, "play-pos", &old_play_pos, NULL);
  new_play_pos = bt_sequence_limit_play_pos (self, old_play_pos);
  if (new_play_pos != old_play_pos) {
    GST_DEBUG ("limit play pos: %lu -> %lu : [%lu ... %lu]", old_play_pos,
        new_play_pos, self->priv->play_start, self->priv->play_end);
    g_object_set (self->priv->song, "play-pos", new_play_pos, NULL);
  }
}

//-- event handler

//-- helper methods

//-- constructor methods

/**
 * bt_sequence_new:
 * @song: the song the new instance belongs to
 *
 * Create a new instance. One would not call this directly, but rather get this
 * from a #BtSong instance.
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtSequence *
bt_sequence_new (const BtSong * const song)
{
  return BT_SEQUENCE (g_object_new (BT_TYPE_SEQUENCE, "song", song, NULL));
}

//-- methods

/**
 * bt_sequence_get_track_by_machine:
 * @self: the sequence to search in
 * @machine: the machine to find the next track for
 * @track: the track to start the search from
 *
 * Gets the next track >= @track this @machine is on.
 *
 * Returns: the track-index or -1 if there is no further track for this
 * @machine.
 *
 * Since: 0.6
 */
glong
bt_sequence_get_track_by_machine (const BtSequence * const self,
    const BtMachine * const machine, gulong track)
{
  const gulong tracks = self->priv->tracks;
  BtMachine **machines = self->priv->machines;

  for (; track < tracks; track++) {
    if (machines[track] == machine) {
      return (glong) track;
    }
  }
  return -1;
}

/**
 * bt_sequence_get_tick_by_pattern:
 * @self: the sequence to search in
 * @track: the track to search in
 * @pattern: the pattern to find the next track for
 * @tick: the tick position to start the search from
 *
 * Gets the next tick position >= @tick this @pattern is on.
 *
 * Returns: the tick position or -1 if there is no further tick for this
 * @pattern.
 *
 * Since: 0.6
 */
glong
bt_sequence_get_tick_by_pattern (const BtSequence * const self, gulong track,
    const BtCmdPattern * const pattern, gulong tick)
{
  const gulong length = self->priv->length;
  const gulong tracks = self->priv->tracks;
  gulong pos = tick * tracks + track;
  BtCmdPattern **patterns = self->priv->patterns;

  for (; tick < length; tick++, pos += tracks) {
    if (patterns[pos] == pattern) {
      return (glong) tick;
    }
  }
  return -1;
}

/**
 * bt_sequence_get_machine:
 * @self: the #BtSequence that holds the tracks
 * @track: the requested track index
 *
 * Fetches the #BtMachine for the given @track.
 *
 * Returns: (transfer full): a reference to the #BtMachine pointer or %NULL in
 * case of an error. Unref when done.
 */
BtMachine *
bt_sequence_get_machine (const BtSequence * const self, const gulong track)
{
  g_return_val_if_fail (BT_IS_SEQUENCE (self), NULL);

  if (track >= self->priv->tracks)
    return NULL;

  BtMachine *machine = bt_sequence_get_machine_unchecked (self, track);
  if (machine) {
    GST_DEBUG_OBJECT (machine, "getting machine : %" G_OBJECT_REF_COUNT_FMT
        " for track %lu", G_OBJECT_LOG_REF_COUNT (machine), track);
    return g_object_ref (machine);
  } else {
    /* TODO(ensonic): shouldn't we better make self->priv->tracks a readonly property
     * and offer methods to insert/remove tracks as it should not be allowed to change
     * the machine later on
     */
    return NULL;
  }
}

/**
 * bt_sequence_add_track:
 * @self: the #BtSequence that holds the tracks
 * @machine: the #BtMachine
 * @ix: position to add the track at, use -1 to append
 *
 * Adds a new track with the @machine at @ix or the end.
 *
 * Returns: %TRUE for success
 */
gboolean
bt_sequence_add_track (const BtSequence * const self,
    const BtMachine * const machine, const glong ix)
{
  g_return_val_if_fail (BT_IS_SEQUENCE (self), FALSE);
  g_return_val_if_fail (BT_IS_MACHINE (machine), FALSE);

  BtMachine **machines;
  gulong tracks = self->priv->tracks + 1;
  const gulong pos = (ix == -1) ? self->priv->tracks : ix;

  g_return_val_if_fail (ix <= (glong) self->priv->tracks, FALSE);

  GST_INFO_OBJECT (machine,
      "add track for machine %" G_OBJECT_REF_COUNT_FMT "at %lu",
      G_OBJECT_LOG_REF_COUNT (machine), pos);

  // enlarge
  g_object_set ((gpointer) self, "tracks", tracks, NULL);
  machines = self->priv->machines;
  if (pos != (tracks - 1)) {
    // shift tracks to the right
    BtCmdPattern **src, **dst;
    const gulong count = tracks - pos;
    const gulong length = self->priv->length;
    gulong i;

    src = &self->priv->patterns[pos];
    dst = &self->priv->patterns[pos + 1];
    for (i = 0; i < length; i++) {
      memmove (dst, src, count * sizeof (gpointer));
      src[count - 1] = NULL;
      src = &src[tracks];
      dst = &dst[tracks];
    }
    memmove (&machines[pos + 1], &machines[pos], count * sizeof (gpointer));
  }
  machines[pos] = g_object_ref ((gpointer) machine);

  g_signal_emit ((gpointer) self, signals[TRACK_ADDED_EVENT], 0, machine, pos);

  GST_INFO_OBJECT (machine,
      ".. added track for machine %" G_OBJECT_REF_COUNT_FMT " at %lu",
      G_OBJECT_LOG_REF_COUNT (machine), pos);
  return TRUE;
}

/**
 * bt_sequence_remove_track_by_ix:
 * @self: the #BtSequence that holds the tracks
 * @ix: the requested track index
 *
 * Removes the specified @track.
 *
 * Returns: %TRUE for success
 */
gboolean
bt_sequence_remove_track_by_ix (const BtSequence * const self, const gulong ix)
{
  const gulong tracks = self->priv->tracks;
  const gulong length = self->priv->length;
  BtMachine **machines = self->priv->machines;
  BtCmdPattern **src, **dst;
  BtMachine *machine;
  gulong i;

  g_return_val_if_fail (BT_IS_SEQUENCE (self), FALSE);
  g_return_val_if_fail (ix < tracks, FALSE);

  const gulong count = (tracks - 1) - ix;
  machine = machines[ix];
  GST_INFO ("remove track %lu/%lu (shift %lu tracks)", ix, tracks, count);

  g_signal_emit ((gpointer) self, signals[TRACK_REMOVED_EVENT], 0, machine, ix);

  src = &self->priv->patterns[ix + 1];
  dst = &self->priv->patterns[ix];
  for (i = 0; i < length; i++) {
    // unref patterns
    if (*dst) {
      GST_INFO ("unref pattern: %" G_OBJECT_REF_COUNT_FMT " at timeline %lu",
          G_OBJECT_LOG_REF_COUNT (*dst), i);
      bt_sequence_unuse_pattern (self, *dst);
    }
    if (count) {
      memmove (dst, src, count * sizeof (gpointer));
    }
    src[count - 1] = NULL;
    src = &src[tracks];
    dst = &dst[tracks];
  }
  if (count) {
    memmove (&machines[ix], &machines[ix + 1], count * sizeof (gpointer));
  }
  machines[tracks - 1] = NULL;

  // this will resize the arrays
  g_object_set ((gpointer) self, "tracks", (gulong) (tracks - 1), NULL);

  GST_INFO_OBJECT (machine, "release machine %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));
  g_object_unref (machine);
  return TRUE;
}

/**
 * bt_sequence_move_track_left:
 * @self: the #BtSequence that holds the tracks
 * @track: the track to move
 *
 * Move the selected track on column left.
 *
 * Returns: %TRUE for success
 */
gboolean
bt_sequence_move_track_left (const BtSequence * const self, const gulong track)
{
  const gulong tracks = self->priv->tracks;
  const gulong length = self->priv->length;
  BtCmdPattern **patterns = self->priv->patterns;
  BtMachine **machines = self->priv->machines;
  BtCmdPattern *pattern;
  BtMachine *machine;
  gulong i, ix = track;

  g_return_val_if_fail (track > 0, FALSE);

  for (i = 0; i < length; i++) {
    pattern = patterns[ix];
    patterns[ix] = patterns[ix - 1];
    patterns[ix - 1] = pattern;
    ix += tracks;
  }
  machine = machines[track];
  machines[track] = machines[track - 1];
  machines[track - 1] = machine;

  return TRUE;
}

/**
 * bt_sequence_move_track_right:
 * @self: the #BtSequence that holds the tracks
 * @track: the track to move
 *
 * Move the selected track on column left.
 *
 * Returns: %TRUE for success
 */
gboolean
bt_sequence_move_track_right (const BtSequence * const self, const gulong track)
{
  const gulong tracks = self->priv->tracks;
  const gulong length = self->priv->length;
  BtCmdPattern **patterns = self->priv->patterns;
  BtMachine **machines = self->priv->machines;
  BtCmdPattern *pattern;
  BtMachine *machine;
  gulong i, ix = track;

  g_return_val_if_fail (track < (tracks - 1), FALSE);

  for (i = 0; i < length; i++) {
    pattern = patterns[ix];
    patterns[ix] = patterns[ix + 1];
    patterns[ix + 1] = pattern;
    ix += tracks;
  }
  machine = machines[track];
  machines[track] = machines[track + 1];
  machines[track + 1] = machine;

  return TRUE;
}

/**
 * bt_sequence_remove_track_by_machine:
 * @self: the #BtSequence that holds the tracks
 * @machine: the #BtMachine
 *
 * Removes all tracks that belong the the given @machine.
 *
 * Returns: %TRUE for success
 */
gboolean
bt_sequence_remove_track_by_machine (const BtSequence * const self,
    const BtMachine * const machine)
{
  gboolean res = TRUE;
  glong track = 0;

  g_return_val_if_fail (BT_IS_SEQUENCE (self), FALSE);
  g_return_val_if_fail (BT_IS_MACHINE (machine), FALSE);

  GST_INFO_OBJECT (machine, "remove tracks for machine %"
      G_OBJECT_REF_COUNT_FMT, G_OBJECT_LOG_REF_COUNT (machine));

  // do bt_sequence_remove_track_by_ix() for each occurance
  while (((track =
              bt_sequence_get_track_by_machine (self, machine, track)) > -1)
      && res) {
    res = bt_sequence_remove_track_by_ix (self, (gulong) track);
  }
  GST_INFO_OBJECT (machine, "removed tracks for machine %"
      G_OBJECT_REF_COUNT_FMT " res=%d", G_OBJECT_LOG_REF_COUNT (machine), res);
  return res;
}

/**
 * bt_sequence_get_label:
 * @self: the #BtSequence that holds the labels
 * @time: the requested time position
 *
 * Fetches the label for the given @time position. Free when done.
 *
 * Returns: a copy of the label or %NULL in case of an error
 */
gchar *
bt_sequence_get_label (const BtSequence * const self, const gulong time)
{
  g_return_val_if_fail (BT_IS_SEQUENCE (self), NULL);
  g_return_val_if_fail (time < self->priv->length, NULL);

  return g_strdup (self->priv->labels[time]);
}

/**
 * bt_sequence_set_label:
 * @self: the #BtSequence that holds the labels
 * @time: the requested time position
 * @label: the new label
 *
 * Sets a new label for the respective @time position.
 */
void
bt_sequence_set_label (const BtSequence * const self, const gulong time,
    const gchar * const label)
{
  g_return_if_fail (BT_IS_SEQUENCE (self));
  g_return_if_fail (time < self->priv->length);

  GST_DEBUG ("set label for time %lu", time);

  g_free (self->priv->labels[time]);
  self->priv->labels[time] = g_strdup (label);
  bt_sequence_release_toc (self);

  g_signal_emit ((gpointer) self, signals[SEQUENCE_ROWS_CHANGED_EVENT], 0, time,
      time);
}

/**
 * bt_sequence_get_pattern:
 * @self: the #BtSequence that holds the patterns
 * @time: the requested time position
 * @track: the requested track index
 *
 * Fetches the pattern for the given @time and @track position.
 *
 * Returns: (transfer full): a reference to the #BtCmdPattern or %NULL when
 * empty. Unref when done.
 */
BtCmdPattern *
bt_sequence_get_pattern (const BtSequence * const self, const gulong time,
    const gulong track)
{
  g_return_val_if_fail (BT_IS_SEQUENCE (self), NULL);
  g_return_val_if_fail (time < self->priv->length, NULL);
  g_return_val_if_fail (track < self->priv->tracks, NULL);

  //GST_DEBUG("get pattern at time %d, track %d",time, track);
  return (g_object_try_ref (bt_sequence_get_pattern_unchecked (self, time,
              track)));
}

/**
 * bt_sequence_set_pattern_quick:
 * @self: the #BtSequence that holds the patterns
 * @time: the requested time position
 * @track: the requested track index
 * @pattern: the #BtCmdPattern or %NULL to unset
 *
 * A quick version of bt_sequence_set_pattern() that does not check parameters.
 * Useful when doing mass updates.
 *
 * Returns: %TRUE if a change has been made.
 *
 * Since: 0.5
 */
gboolean
bt_sequence_set_pattern_quick (const BtSequence * const self, const gulong time,
    const gulong track, const BtCmdPattern * const pattern)
{
  gboolean changed = FALSE;
  const gulong index = time * self->priv->tracks + track;
  BtCmdPattern *old_pattern = self->priv->patterns[index];

  GST_DEBUG ("set pattern from %p to %p for time %lu, track %lu",
      self->priv->patterns[index], pattern, time, track);

  // take out the old pattern
  if (old_pattern) {
    bt_sequence_unuse_pattern (self, old_pattern);

    changed = TRUE;
    self->priv->patterns[index] = NULL;
  }
  if (pattern) {
    bt_sequence_use_pattern (self, (BtCmdPattern *) pattern);
    // enter the new pattern
    self->priv->patterns[index] = g_object_ref ((gpointer) pattern);
    //g_object_add_weak_pointer((gpointer)pattern,(gpointer *)(&self->priv->patterns[index]));
    changed = TRUE;
  }
  g_signal_emit ((gpointer) self, signals[SEQUENCE_ROWS_CHANGED_EVENT], 0, time,
      time);
  GST_DEBUG ("done: %d", changed);
  return changed;
}

/**
 * bt_sequence_set_pattern:
 * @self: the #BtSequence that holds the patterns
 * @time: the requested time position
 * @track: the requested track index
 * @pattern: the #BtCmdPattern or %NULL to unset
 *
 * Sets the #BtCmdPattern for the respective @time and @track position.
 */
void
bt_sequence_set_pattern (const BtSequence * const self, const gulong time,
    const gulong track, const BtCmdPattern * const pattern)
{
  g_return_if_fail (BT_IS_SEQUENCE (self));
  g_return_if_fail (time < self->priv->length);
  g_return_if_fail (track < self->priv->tracks);
  g_return_if_fail (self->priv->machines[track]);

#ifndef G_DISABLE_ASSERT
  if (pattern) {
    BtMachine *const machine;

    g_return_if_fail (BT_IS_CMD_PATTERN (pattern));
    g_object_get ((gpointer) pattern, "machine", &machine, NULL);
    if (self->priv->machines[track] != machine) {
      GST_WARNING ("adding a pattern to a track with different machine!");
      g_object_unref (machine);
      return;
    }
    g_object_unref (machine);
  }
#endif

  bt_sequence_set_pattern_quick (self, time, track, pattern);
//TODO(ensonic): change bt_sequence_set_pattern_quick() to bt_sequence_set_pattern_unchecked
}

/**
 * bt_sequence_get_loop_length:
 * @self: the #BtSequence of the song
 *
 * Calculates the length of the song loop in ticks.
 *
 * Returns: the length of the song loop in ticks
 */
gulong
bt_sequence_get_loop_length (const BtSequence * const self)
{
  g_return_val_if_fail (BT_IS_SEQUENCE (self), 0);
  BtSequencePrivate *p = self->priv;

  GST_DEBUG ("%lu .. %lu = %lu", p->play_start, p->play_end,
      (p->play_end - p->play_start));

  return p->play_end - p->play_start;
}

/**
 * bt_sequence_limit_play_pos:
 * @self: the sequence to trim the play position of
 * @play_pos: the time position to lock inbetween loop-boundaries
 *
 * Enforce the playback position to be within loop start and end or the song
 * bounds if there is no loop.
 *
 * Returns: the new @play_pos
 */
gulong
bt_sequence_limit_play_pos (const BtSequence * const self, gulong play_pos)
{
  if (play_pos > self->priv->play_end) {
    play_pos = self->priv->play_end;
  }
  if (play_pos < self->priv->play_start) {
    play_pos = self->priv->play_start;
  }
  return play_pos;
}

/**
 * bt_sequence_is_pattern_used:
 * @self: the sequence to check for pattern use
 * @pattern: the pattern to check for
 *
 * Checks if the @pattern is used in the sequence.
 *
 * Returns: %TRUE if @pattern is used.
 */
gboolean
bt_sequence_is_pattern_used (const BtSequence * const self,
    const BtPattern * const pattern)
{
  g_return_val_if_fail (BT_IS_SEQUENCE (self), 0);
  g_return_val_if_fail (BT_IS_PATTERN (pattern), 0);

  return (bt_sequence_get_number_of_pattern_uses (self, pattern) > 0);
}

/*
 * insert_rows:
 * @self: the sequence
 * @time: the postion to insert at
 * @track: the track
 * @rows: the number of rows to insert
 *
 * Insert empty @rows for given @track.
 */
static void
insert_rows (const BtSequence * const self, const gulong time,
    const gulong track, const gulong rows)
{
  BtSequencePrivate *p = self->priv;
  const gulong tracks = p->tracks;
  const gulong length = p->length;
  BtCmdPattern **src = &p->patterns[track + tracks * (length - (1 + rows))];
  BtCmdPattern **dst = &p->patterns[track + tracks * (length - 1)];
  BtCmdPattern **clr = &src[tracks];
  gulong i;

  /* ins 3     0 1
   * 0 a       a a
   * 1 b       b b
   * 2 c       c c
   * 3 d       d d
   * 4 e src   e .
   * 5 f    \  - .
   * 6 g     v - .
   * 7 h dst . - e
   */

  /* we're pushing out the last @rows rows */
  for (i = 0; i < rows; i++) {
    if (*clr) {
      bt_sequence_unuse_pattern (self, *clr);
      *clr = NULL;
    }
    clr += tracks;
  }
  /* copy patterns and move upwards */
  for (i = (length - 1); i >= (time + rows); i--) {
    *dst = *src;
    *src = NULL;
    src -= tracks;
    dst -= tracks;
  }
}

/**
 * bt_sequence_insert_rows:
 * @self: the sequence
 * @time: the postion to insert at
 * @track: the track
 * @rows: the number of rows to insert
 *
 * Insert @rows empty rows for given @track at position given by @time.
 *
 * Since: 0.3
 */
void
bt_sequence_insert_rows (const BtSequence * const self, const gulong time,
    const glong track, const gulong rows)
{
  g_return_if_fail (BT_IS_SEQUENCE (self));

  const gulong length = self->priv->length;

  GST_INFO ("insert %lu rows at %lu,%ld", rows, time, track);

  if (track > -1) {
    insert_rows (self, time, track, rows);
  } else {
    gulong j = 0;
    gchar **const labels = self->priv->labels;

    // shift label down
    for (j = length - rows; j < length; j++) {
      g_free (labels[j]);
    }
    memmove (&labels[time + rows], &labels[time],
        ((length - rows) - time) * sizeof (gpointer));
    for (j = 0; j < rows; j++) {
      labels[time + j] = NULL;
    }
    bt_sequence_release_toc (self);
  }
  g_signal_emit ((gpointer) self, signals[SEQUENCE_ROWS_CHANGED_EVENT], 0, time,
      length);
}

/**
 * bt_sequence_insert_full_rows:
 * @self: the sequence
 * @time: the postion to insert at
 * @rows: the number of rows to insert
 *
 * Insert one empty row for all tracks.
 *
 * Since: 0.3
 */
void
bt_sequence_insert_full_rows (const BtSequence * const self, const gulong time,
    const gulong rows)
{
  g_return_if_fail (BT_IS_SEQUENCE (self));

  const gulong tracks = self->priv->tracks;
  const gulong length = self->priv->length;
  gulong j = 0;
  gchar **labels;               // don't take the pointer yet, we realloc

  GST_DEBUG ("insert %lu full-rows at %lu / %lu", rows, time, length);

  g_object_set ((gpointer) self, "length", length + rows, NULL);

  // shift label down
  labels = self->priv->labels;
  memmove (&labels[time + rows], &labels[time],
      ((length - rows) - time) * sizeof (gpointer));
  for (j = 0; j < rows; j++) {
    labels[time + j] = NULL;
  }
  for (j = 0; j < tracks; j++) {
    insert_rows (self, time, j, rows);
  }
  bt_sequence_release_toc (self);
  g_signal_emit ((gpointer) self, signals[SEQUENCE_ROWS_CHANGED_EVENT], 0, time,
      length + rows);
}

/*
 * delete_rows:
 * @self: the sequence
 * @time: the postion to delete
 * @track: the track
 * @rows: the number of rows to remove
 *
 * Delete @rows for given @track.
 */
static void
delete_rows (const BtSequence * const self, const gulong time,
    const gulong track, const gulong rows)
{
  const gulong tracks = self->priv->tracks;
  const gulong length = self->priv->length;
  BtCmdPattern **src = &self->priv->patterns[track + tracks * (time + rows)];
  BtCmdPattern **dst = &self->priv->patterns[track + tracks * time];
  gulong i;

  /* del 3     0 1 2 3 4 5
   * 0 a dst d d d d d d d
   * 1 b     ^ b e e e e e
   * 2 c    /  c c f f f r
   * 3 d src   d d d g g g
   * 4 e       e e e e h h
   * 5 f       f f f f f .
   * 6 g       g g g g g .
   * 7 h       h h h h h .
   */
  // not sure why we need the final loop and can't use this scheme
  /* del 3     0 1 2 3 4
   * 0 a dst d d d d d d
   * 1 b     ^ b e e e e
   * 2 c    /  c c f f f
   * 3 d src   . . . g g
   * 4 e       e . . . h
   * 5 f       f f . . .
   * 6 g       g g g . .
   * 7 h       h h h h .
   */

  /* we're overwriting these */
  for (i = 0; i < rows; i++) {
    if (dst[i * tracks])
      bt_sequence_unuse_pattern (self, dst[i * tracks]);
  }
  /* copy patterns and move downwards */
  for (i = time; i < (length - rows); i++) {
    *dst = *src;
    //*src=NULL;
    src += tracks;
    dst += tracks;
  }
  for (i = 0; i < rows; i++) {
    *dst = NULL;
    dst += tracks;
  }
}

/**
 * bt_sequence_delete_rows:
 * @self: the sequence
 * @time: the postion to delete
 * @track: the track
 * @rows: the number of rows to remove
 *
 * Delete row for given @track.
 *
 * Since: 0.3
 */
void
bt_sequence_delete_rows (const BtSequence * const self, const gulong time,
    const glong track, const gulong rows)
{
  g_return_if_fail (BT_IS_SEQUENCE (self));

  const gulong length = self->priv->length;

  GST_INFO ("delete %lu rows at %lu,%ld", rows, time, track);

  if (track > -1) {
    delete_rows (self, time, track, rows);
  } else {
    gulong j = 0;
    gchar **const labels = self->priv->labels;

    // shift label up
    for (j = 0; j < rows; j++) {
      g_free (labels[time + j]);
    }
    memmove (&labels[time], &labels[time + rows],
        ((length - rows) - time) * sizeof (gpointer));
    for (j = length - rows; j < length; j++) {
      labels[j] = NULL;
    }
    bt_sequence_release_toc (self);
  }
  g_signal_emit ((gpointer) self, signals[SEQUENCE_ROWS_CHANGED_EVENT], 0, time,
      rows);
}

/**
 * bt_sequence_delete_full_rows:
 * @self: the sequence
 * @time: the postion to delete
 * @rows: the number of rows to remove
 *
 * Delete row for all tracks.
 *
 * Since: 0.3
 */
void
bt_sequence_delete_full_rows (const BtSequence * const self, const gulong time,
    const gulong rows)
{
  g_return_if_fail (BT_IS_SEQUENCE (self));

  const gulong tracks = self->priv->tracks;
  const gulong length = self->priv->length;
  gulong j = 0;
  gchar **const labels = self->priv->labels;

  GST_DEBUG ("delete %lu full-rows at %lu / %lu", rows, time, length);

  // shift label up
  for (j = 0; j < rows; j++) {
    g_free (labels[time + j]);
  }
  memmove (&labels[time], &labels[time + rows],
      ((length - rows) - time) * sizeof (gpointer));
  for (j = length - rows; j < length; j++) {
    labels[j] = NULL;
  }
  for (j = 0; j < tracks; j++) {
    bt_sequence_delete_rows (self, time, j, rows);
  }

  // don't make it shorter because of loop-end ?
  g_object_set ((gpointer) self, "length", length - rows, NULL);

  bt_sequence_release_toc (self);
  g_signal_emit ((gpointer) self, signals[SEQUENCE_ROWS_CHANGED_EVENT], 0, time,
      length - rows);
}

//-- io interface

static xmlNodePtr
bt_sequence_persistence_save (const BtPersistence * const persistence,
    xmlNodePtr const parent_node)
{
  BtSequence *const self = BT_SEQUENCE (persistence);
  const gulong tracks = self->priv->tracks;
  const gulong length = self->priv->length;
  BtCmdPattern **patterns = self->priv->patterns;
  BtMachine **machines = self->priv->machines;
  gchar **const labels = self->priv->labels;
  xmlNodePtr node = NULL;
  xmlNodePtr child_node, child_node2, child_node3;
  gulong i, j;

  GST_DEBUG ("PERSISTENCE::sequence");

  if ((node = xmlNewChild (parent_node, NULL, XML_CHAR_PTR ("sequence"), NULL))) {
    xmlNewProp (node, XML_CHAR_PTR ("length"),
        XML_CHAR_PTR (bt_str_format_ulong (self->priv->length)));
    xmlNewProp (node, XML_CHAR_PTR ("tracks"),
        XML_CHAR_PTR (bt_str_format_ulong (self->priv->tracks)));
    if (self->priv->loop) {
      xmlNewProp (node, XML_CHAR_PTR ("loop"), XML_CHAR_PTR ("on"));
      xmlNewProp (node, XML_CHAR_PTR ("loop-start"),
          XML_CHAR_PTR (bt_str_format_long (self->priv->loop_start)));
      xmlNewProp (node, XML_CHAR_PTR ("loop-end"),
          XML_CHAR_PTR (bt_str_format_long (self->priv->loop_end)));
    }
    if ((child_node = xmlNewChild (node, NULL, XML_CHAR_PTR ("labels"), NULL))) {
      // iterate over timelines
      for (i = 0; i < length; i++) {
        const gchar *const label = labels[i];
        if (label) {
          child_node2 =
              xmlNewChild (child_node, NULL, XML_CHAR_PTR ("label"), NULL);
          xmlNewProp (child_node2, XML_CHAR_PTR ("name"), XML_CHAR_PTR (label));
          xmlNewProp (child_node2, XML_CHAR_PTR ("time"),
              XML_CHAR_PTR (bt_str_format_ulong (i)));
        }
      }
    } else
      goto Error;
    if ((child_node = xmlNewChild (node, NULL, XML_CHAR_PTR ("tracks"), NULL))) {
      gchar *const machine_id, *const pattern_name;
      BtMachine *machine;
      BtCmdPattern *pattern;

      // iterate over tracks
      for (j = 0; j < tracks; j++) {
        child_node2 =
            xmlNewChild (child_node, NULL, XML_CHAR_PTR ("track"), NULL);
        machine = machines[j];
        g_object_get (machine, "id", &machine_id, NULL);
        xmlNewProp (child_node2, XML_CHAR_PTR ("index"),
            XML_CHAR_PTR (bt_str_format_ulong (j)));
        xmlNewProp (child_node2, XML_CHAR_PTR ("machine"),
            XML_CHAR_PTR (machine_id));
        g_free (machine_id);
        // iterate over timelines
        for (i = 0; i < length; i++) {
          // get pattern
          pattern = patterns[i * tracks + j];
          if (pattern) {
            g_object_get (pattern, "name", &pattern_name, NULL);
            child_node3 =
                xmlNewChild (child_node2, NULL, XML_CHAR_PTR ("position"),
                NULL);
            xmlNewProp (child_node3, XML_CHAR_PTR ("time"),
                XML_CHAR_PTR (bt_str_format_ulong (i)));
            xmlNewProp (child_node3, XML_CHAR_PTR ("pattern"),
                XML_CHAR_PTR (pattern_name));
            g_free (pattern_name);
          }
        }
      }
    } else
      goto Error;
    if (g_hash_table_size (self->priv->properties)) {
      if ((child_node =
              xmlNewChild (node, NULL, XML_CHAR_PTR ("properties"), NULL))) {
        if (!bt_persistence_save_hashtable (self->priv->properties, child_node))
          goto Error;
      } else
        goto Error;
    }
  }
Error:
  return node;
}

static BtPersistence *
bt_sequence_persistence_load (const GType type,
    const BtPersistence * const persistence, xmlNodePtr node, GError ** err,
    va_list var_args)
{
  BtSequence *const self = BT_SEQUENCE (persistence);
  xmlNodePtr child_node, child_node2;

  GST_DEBUG ("PERSISTENCE::sequence");
  g_assert (node);

  xmlChar *const length_str = xmlGetProp (node, XML_CHAR_PTR ("length"));
  xmlChar *const tracks_str = xmlGetProp (node, XML_CHAR_PTR ("tracks"));
  xmlChar *const loop_str = xmlGetProp (node, XML_CHAR_PTR ("loop"));
  xmlChar *const loop_start_str =
      xmlGetProp (node, XML_CHAR_PTR ("loop-start"));
  xmlChar *const loop_end_str = xmlGetProp (node, XML_CHAR_PTR ("loop-end"));

  const gulong length = length_str ? atol ((char *) length_str) : 0;
  const gulong tracks = tracks_str ? atol ((char *) tracks_str) : 0;
  const gulong loop_start =
      loop_start_str ? atol ((char *) loop_start_str) : -1;
  const gulong loop_end = loop_end_str ? atol ((char *) loop_end_str) : -1;
  const gboolean loop =
      loop_str ? !strncasecmp ((char *) loop_str, "on\0", 3) : FALSE;

  g_object_set (self, "length", length, "tracks", tracks,
      "loop", loop, "loop-start", loop_start, "loop-end", loop_end, NULL);
  xmlFree (length_str);
  xmlFree (tracks_str);
  xmlFree (loop_str);
  xmlFree (loop_start_str);
  xmlFree (loop_end_str);

  for (node = node->children; node; node = node->next) {
    if (!xmlNodeIsText (node)) {
      if (!strncmp ((gchar *) node->name, "labels\0", 7)) {
        for (child_node = node->children; child_node;
            child_node = child_node->next) {
          if ((!xmlNodeIsText (child_node))
              && (!strncmp ((char *) child_node->name, "label\0", 6))) {
            xmlChar *const time_str =
                xmlGetProp (child_node, XML_CHAR_PTR ("time"));
            xmlChar *const name =
                xmlGetProp (child_node, XML_CHAR_PTR ("name"));
            bt_sequence_set_label (self, atol ((const char *) time_str),
                (const gchar *) name);
            xmlFree (time_str);
            xmlFree (name);
          }
        }
      } else if (!strncmp ((const gchar *) node->name, "tracks\0", 7)) {
        BtSetup *const setup;

        g_object_get (self->priv->song, "setup", &setup, NULL);

        for (child_node = node->children; child_node;
            child_node = child_node->next) {
          if ((!xmlNodeIsText (child_node))
              && (!strncmp ((char *) child_node->name, "track\0", 6))) {
            xmlChar *const machine_id =
                xmlGetProp (child_node, XML_CHAR_PTR ("machine"));
            xmlChar *const index_str =
                xmlGetProp (child_node, XML_CHAR_PTR ("index"));
            const gulong index = index_str ? atol ((char *) index_str) : 0;
            BtMachine *const machine =
                bt_setup_get_machine_by_id (setup, (gchar *) machine_id);

            if (machine) {
              GST_INFO ("add track for machine %" G_OBJECT_REF_COUNT_FMT
                  " at position %lu", G_OBJECT_LOG_REF_COUNT (machine), index);
              if (index < tracks) {
                self->priv->machines[index] = machine;
                g_signal_emit ((gpointer) self, signals[TRACK_ADDED_EVENT], 0,
                    machine, index);
                GST_DEBUG ("loading track with index=%s for machine=\"%s\"",
                    index_str, machine_id);
                for (child_node2 = child_node->children; child_node2;
                    child_node2 = child_node2->next) {
                  if ((!xmlNodeIsText (child_node2))
                      && (!strncmp ((char *) child_node2->name, "position\0",
                              9))) {
                    xmlChar *const time_str =
                        xmlGetProp (child_node2, XML_CHAR_PTR ("time"));
                    xmlChar *const pattern_name =
                        xmlGetProp (child_node2, XML_CHAR_PTR ("pattern"));
                    GST_DEBUG ("  at %s, machinepattern \"%s\"", time_str,
                        safe_string (pattern_name));
                    if (pattern_name) {
                      BtCmdPattern *pattern =
                          bt_machine_get_pattern_by_name (machine,
                          (gchar *) pattern_name);
                      // support for pre-0.7 songs
                      if (!pattern) {
                        pattern = bt_machine_get_pattern_by_id (machine,
                            (gchar *) pattern_name);
                      }
                      if (pattern) {
                        // this refs the pattern
                        bt_sequence_set_pattern_quick (self,
                            atol ((char *) time_str), index, pattern);
                        g_object_unref (pattern);
                      } else {
                        GST_WARNING ("  unknown pattern \"%s\"", pattern_name);
                      }
                      xmlFree (pattern_name);
                    }
                    xmlFree (time_str);
                  }
                }
                // we keep the ref in self->priv->machines[index]
              } else {
                GST_WARNING ("index beyond tracks: %lu>=%lu", index, tracks);
                g_object_unref (machine);
              }
            } else {
              GST_INFO ("invalid or missing machine %s referenced at track %lu",
                  (gchar *) machine_id, index);
            }
            xmlFree (index_str);
            xmlFree (machine_id);
          }
        }
        g_object_unref (setup);
      } else if (!strncmp ((gchar *) node->name, "properties\0", 11)) {
        bt_persistence_load_hashtable (self->priv->properties, node);
      }
    }
  }

  return BT_PERSISTENCE (persistence);
}

static void
bt_sequence_persistence_interface_init (gpointer const g_iface,
    gpointer const iface_data)
{
  BtPersistenceInterface *const iface = g_iface;

  iface->load = bt_sequence_persistence_load;
  iface->save = bt_sequence_persistence_save;
}

//-- wrapper

//-- default signal handler

//-- g_object overrides

static void
bt_sequence_get_property (GObject * const object, const guint property_id,
    GValue * const value, GParamSpec * const pspec)
{
  const BtSequence *const self = BT_SEQUENCE (object);
  return_if_disposed ();
  switch (property_id) {
    case SEQUENCE_SONG:
      g_value_set_object (value, self->priv->song);
      break;
    case SEQUENCE_LENGTH:
      g_value_set_ulong (value, self->priv->length);
      break;
    case SEQUENCE_TRACKS:
      g_value_set_ulong (value, self->priv->tracks);
      break;
    case SEQUENCE_LOOP:
      g_value_set_boolean (value, self->priv->loop);
      break;
    case SEQUENCE_LOOP_START:
      g_value_set_long (value, self->priv->loop_start);
      break;
    case SEQUENCE_LOOP_END:
      g_value_set_long (value, self->priv->loop_end);
      break;
    case SEQUENCE_PROPERTIES:
      g_value_set_pointer (value, self->priv->properties);
      break;
    case SEQUENCE_TOC:
      g_value_set_pointer (value, bt_sequence_get_toc (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_sequence_set_property (GObject * const object, const guint property_id,
    const GValue * const value, GParamSpec * const pspec)
{
  const BtSequence *const self = BT_SEQUENCE (object);

  return_if_disposed ();
  switch (property_id) {
    case SEQUENCE_SONG:
      self->priv->song = BT_SONG (g_value_get_object (value));
      g_object_try_weak_ref (self->priv->song);
      GST_DEBUG ("set the song: %p", self->priv->song);
      break;
    case SEQUENCE_LENGTH:{
      // TODO(ensonic): remove or better stop the song
      // if(self->priv->is_playing) bt_sequence_stop(self);
      // prepare new data
      const gulong length = self->priv->length;
      self->priv->length = g_value_get_ulong (value);
      if (length != self->priv->length) {
        GST_DEBUG ("set the length for sequence: %lu", self->priv->length);
        bt_sequence_resize_data_length (self, length);
        if (self->priv->loop_end != -1) {
          // clip loopend to length or extend loop-end as well if loop_end was
          // old length
          if ((self->priv->loop_end > self->priv->length) ||
              (self->priv->loop_end == length)) {
            self->priv->play_end = self->priv->loop_end = self->priv->length;
            g_object_notify ((GObject *) self, "loop-end");
            if (self->priv->loop_end <= self->priv->loop_start) {
              self->priv->loop_start = self->priv->loop_end = -1;
              self->priv->loop = FALSE;
              g_object_notify ((GObject *) self, "loop-start");
              g_object_notify ((GObject *) self, "loop");
            }
          }
        } else {
          self->priv->play_end = self->priv->length;
        }
        bt_sequence_limit_play_pos_internal (self);
      }
      break;
    }
    case SEQUENCE_TRACKS:{
      // TODO(ensonic): remove or better stop the song
      //if(self->priv->is_playing) bt_sequence_stop(self);
      // prepare new data
      const gulong tracks = self->priv->tracks;
      self->priv->tracks = g_value_get_ulong (value);
      if (tracks != self->priv->tracks) {
        GST_DEBUG ("set the tracks for sequence: %lu -> %lu", tracks,
            self->priv->tracks);
        bt_sequence_resize_data_tracks (self, tracks);
      }
      break;
    }
    case SEQUENCE_LOOP:
      self->priv->loop = g_value_get_boolean (value);
      GST_DEBUG ("set the loop for sequence: %d", self->priv->loop);
      if (self->priv->loop) {
        if (self->priv->loop_start == -1) {
          self->priv->loop_start = 0;
          g_object_notify (G_OBJECT (self), "loop-start");
        }
        self->priv->play_start = self->priv->loop_start;
        if (self->priv->loop_end == -1) {
          self->priv->loop_end = self->priv->length;
          g_object_notify (G_OBJECT (self), "loop-end");
        }
        self->priv->play_end = self->priv->loop_end;
        bt_sequence_limit_play_pos_internal (self);
      } else {
        self->priv->play_start = 0;
        self->priv->play_end = self->priv->length;
        bt_sequence_limit_play_pos_internal (self);
      }
      break;
    case SEQUENCE_LOOP_START:
      self->priv->loop_start = g_value_get_long (value);
      if (self->priv->loop_start != -1) {
        // make sure its less then loop_end/length
        if (self->priv->loop_end > 0) {
          if (self->priv->loop_start >= self->priv->loop_end)
            self->priv->loop_start = self->priv->loop_end - 1;
        } else if (self->priv->length > 0) {
          if (self->priv->loop_start >= self->priv->length)
            self->priv->loop_start = self->priv->length - 1;
        } else
          self->priv->loop_start = -1;
      }
      GST_DEBUG ("set the loop-start for sequence: %ld",
          self->priv->loop_start);
      self->priv->play_start =
          (self->priv->loop_start != -1) ? self->priv->loop_start : 0;
      bt_sequence_limit_play_pos_internal (self);
      break;
    case SEQUENCE_LOOP_END:
      self->priv->loop_end = g_value_get_long (value);
      if (self->priv->loop_end != -1) {
        // make sure its more then loop-start
        if (self->priv->loop_start > -1) {
          if (self->priv->loop_end < self->priv->loop_start)
            self->priv->loop_end = self->priv->loop_start + 1;
        }
        // make sure its less then or equal to length
        if (self->priv->length > 0) {
          if (self->priv->loop_end > self->priv->length)
            self->priv->loop_end = self->priv->length;
        } else
          self->priv->loop_end = -1;
      }
      GST_DEBUG ("set the loop-end for sequence: %ld", self->priv->loop_end);
      self->priv->play_end =
          (self->priv->loop_end !=
          -1) ? self->priv->loop_end : self->priv->length;
      bt_sequence_limit_play_pos_internal (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_sequence_dispose (GObject * const object)
{
  BtSequence *const self = BT_SEQUENCE (object);
  const gulong tracks = self->priv->tracks;
  const gulong length = self->priv->length;
  BtCmdPattern **patterns = self->priv->patterns;
  BtMachine **machines = self->priv->machines;
  gchar **const labels = self->priv->labels;
  gulong i, j, k;

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);
  g_object_try_weak_unref (self->priv->song);
  // unref the machines
  GST_DEBUG ("unref %lu machines", tracks);
  for (i = 0; i < tracks; i++) {
    GST_INFO ("releasing machine %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (machines[i]));
    g_object_try_unref (machines[i]);
  }
  // free the labels
  for (i = 0; i < length; i++) {
    g_free (labels[i]);
  }
  // unref the patterns
  for (k = i = 0; i < length; i++) {
    for (j = 0; j < tracks; j++, k++) {
      if (patterns[k])
        bt_sequence_unuse_pattern (self, patterns[k]);
    }
  }

  bt_sequence_release_toc (self);

  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (bt_sequence_parent_class)->dispose (object);
  GST_DEBUG ("  done");
}

static void
bt_sequence_finalize (GObject * const object)
{
  const BtSequence *const self = BT_SEQUENCE (object);

  GST_DEBUG ("!!!! self=%p", self);

  g_free (self->priv->machines);
  g_free (self->priv->labels);
  g_free (self->priv->patterns);
  g_hash_table_destroy (self->priv->pattern_usage);
  g_hash_table_destroy (self->priv->properties);

  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (bt_sequence_parent_class)->finalize (object);
  GST_DEBUG ("  done");
}

//-- class internalsbt_sequence_get_toc

static void
bt_sequence_init (BtSequence * self)
{
  self->priv = bt_sequence_get_instance_private(self);
  self->priv->loop_start = -1;
  self->priv->loop_end = -1;
  self->priv->pattern_usage = g_hash_table_new (NULL, NULL);
  self->priv->properties =
      g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
}

static void
bt_sequence_class_init (BtSequenceClass * const klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = bt_sequence_set_property;
  gobject_class->get_property = bt_sequence_get_property;
  gobject_class->dispose = bt_sequence_dispose;
  gobject_class->finalize = bt_sequence_finalize;

  /**
   * BtSequence::pattern-added:
   * @self: the sequence object that emitted the signal
   * @pattern: the new pattern
   *
   * A pattern has been used in the sequence for the first time.
   */
  signals[PATTERN_ADDED_EVENT] =
      g_signal_new ("pattern-added", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,
      NULL, g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, BT_TYPE_PATTERN);

  /**
   * BtSequence::pattern-removed:
   * @self: the sequence object that emitted the signal
   * @pattern: the old pattern
   *
   * The last occurance of pattern has been removed from the sequence.
   */
  signals[PATTERN_REMOVED_EVENT] =
      g_signal_new ("pattern-removed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,
      NULL, g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, BT_TYPE_PATTERN);

  /**
   * BtSequence::rows-changed:
   * @self: the sequence object that emitted the signal
   * @begin: start row that changed
   * @end: last row that changed
   *
   * The content of the given rows in the sequence has changed.
   *
   * Since: 0.6
   */
  signals[SEQUENCE_ROWS_CHANGED_EVENT] =
      g_signal_new ("rows-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,
      NULL, bt_marshal_VOID__ULONG_ULONG, G_TYPE_NONE, 2, G_TYPE_ULONG,
      G_TYPE_ULONG);

  /**
   * BtSequence::track-added:
   * @self: the sequence object that emitted the signal
   * @machine: the machine for the track
   * @track: the track index
   *
   * A new track for @machine has been added with the @track index.
   *
   * Since: 0.6
   */
  signals[TRACK_ADDED_EVENT] =
      g_signal_new ("track-added", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,
      NULL, bt_marshal_VOID__OBJECT_ULONG, G_TYPE_NONE, 2, BT_TYPE_MACHINE,
      G_TYPE_ULONG);

  /**
   * BtSequence::track-removed:
   * @self: the sequence object that emitted the signal
   * @machine: the machine for the track
   * @track: the track index
   *
   * A track for @machine has been removed at the @track index.
   *
   * Since: 0.6
   */
  signals[TRACK_REMOVED_EVENT] = g_signal_new ("track-removed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL, NULL,  // acc data
      bt_marshal_VOID__OBJECT_ULONG, G_TYPE_NONE,
      2, BT_TYPE_MACHINE, G_TYPE_ULONG);

  g_object_class_install_property (gobject_class, SEQUENCE_SONG,
      g_param_spec_object ("song", "song contruct prop",
          "Set song object, the sequence belongs to", BT_TYPE_SONG,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  // loop-pos are LONG as well
  g_object_class_install_property (gobject_class, SEQUENCE_LENGTH,
      g_param_spec_ulong ("length", "length prop",
          "length of the sequence in timeline bars", 0, G_MAXLONG, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SEQUENCE_TRACKS,
      g_param_spec_ulong ("tracks",
          "tracks prop",
          "number of tracks in the sequence",
          0, G_MAXULONG, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SEQUENCE_LOOP,
      g_param_spec_boolean ("loop",
          "loop prop",
          "is loop activated",
          FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SEQUENCE_LOOP_START,
      g_param_spec_long ("loop-start",
          "loop-start prop",
          "start of the repeat sequence on the timeline",
          -1, G_MAXLONG, -1, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SEQUENCE_LOOP_END,
      g_param_spec_long ("loop-end",
          "loop-end prop",
          "end of the repeat sequence on the timeline",
          -1, G_MAXLONG, -1, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SEQUENCE_PROPERTIES,
      g_param_spec_pointer ("properties",
          "properties prop",
          "hashtable of sequence properties",
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SEQUENCE_TOC,
      g_param_spec_pointer ("toc", "toc prop",
          "TOC containing the labels",
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}
