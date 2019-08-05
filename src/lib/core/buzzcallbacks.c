/* Buzztrax
 * Copyright (C) 2008 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#define BT_CORE
#define BT_BUZZCALLBACKS_C

#include "core_private.h"
// for CHostCallbacks
#include "bml/bml.h"

/*
 * we could use some static hashtables to cache the returned structs
 * if bt_buzz_callbacks_get() is called with a song different from what we have
 * set there, we flush the hash tables
 */

// structs

typedef struct
{
  int Flags;
  float Volume;
} BuzzWaveInfo;

typedef struct
{
  int numSamples;
  short *pSamples;
  int RootNote;
  int SamplesPerSec;
  int LoopStart;
  int LoopEnd;
} BuzzWaveLevel;

// callbacks

static void const *
GetWave (CHostCallbacks * self, int const i)
{
  // 200 is WAVE_MAX
  static BuzzWaveInfo res[200] = { {0,}, };
  BtSong *song = BT_SONG (self->user_data);
  BtWavetable *wavetable;
  BtWave *wave;
  BtWaveLoopMode loop_mode;
  gdouble volume;
  guint channels;

  GST_DEBUG ("(%p,%d)", self, i);
  if (G_UNLIKELY (!song))
    return NULL;

  g_object_get (song, "wavetable", &wavetable, NULL);
  if (G_UNLIKELY (!wavetable))
    return NULL;
  if ((wave = bt_wavetable_get_wave_by_index (wavetable, i))) {
    g_object_get (wave, "volume", &volume, "loop-mode", &loop_mode, "channels",
        &channels, NULL);
    res[i].Volume = volume;
    res[i].Flags = 0;
    switch (loop_mode) {
      case BT_WAVE_LOOP_MODE_OFF:
        break;
      case BT_WAVE_LOOP_MODE_FORWARD:
        res[i].Flags |= 1;
        break;
      case BT_WAVE_LOOP_MODE_PINGPONG:
        res[i].Flags |= 1 + 16;
        break;
    }
    if (channels == 2) {
      res[i].Flags |= 8;
    }
    g_object_unref (wave);
    GST_DEBUG ("= %d, %f", res[i].Flags, res[i].Volume);
  } else {
    GST_WARNING ("no wave for index %d", i);
  }
  g_object_unref (wavetable);

  return &res[i];
}

// TODO(ensonic): this is still racy if multiple levels are used

static void const *
GetWaveLevel (CHostCallbacks * self, int const i, int const level)
{
  // 200 is WAVE_MAX
  static BuzzWaveLevel res[200] = { {0,}, };
  BtSong *song = BT_SONG (self->user_data);
  BtWavetable *wavetable;
  BtWave *wave;
  BtWavelevel *wavelevel;

  GST_DEBUG ("(%p,%d,%d)", self, i, level);
  if (G_UNLIKELY (!song))
    return NULL;

  g_object_get (song, "wavetable", &wavetable, NULL);
  if (G_UNLIKELY (!wavetable))
    return NULL;
  if ((wave = bt_wavetable_get_wave_by_index (wavetable, i))) {
    if ((wavelevel = bt_wave_get_level_by_index (wave, level))) {
      gulong length, rate;
      glong ls, le;
      GstBtNote root_note;

      // fill BuzzWaveLevel
      g_object_get (wavelevel, "length", &length, "rate", &rate,
          "loop-start", &ls, "loop-end", &le,
          "root-note", &root_note, "data", &res[i].pSamples, NULL);
      res[i].numSamples = length;
      res[i].SamplesPerSec = rate;
      res[i].RootNote = root_note;
      res[i].LoopStart = ls;
      res[i].LoopEnd = le;
      g_object_unref (wavelevel);
    } else {
      GST_WARNING ("no wavelevel for index %d", level);
    }
    g_object_unref (wave);
  } else {
    GST_WARNING ("no wave for index %d", i);
  }
  g_object_unref (wavetable);

  return &res[i];
}

static void const *
GetNearestWaveLevel (CHostCallbacks * self, int const i, int const note)
{
  // 200 is WAVE_MAX
  static BuzzWaveLevel res[200] = { {0,}, };
  BtSong *song = BT_SONG (self->user_data);
  BtWavetable *wavetable;
  BtWave *wave;

  GST_DEBUG ("(%p,%d,%d)", self, i, note);
  if (G_UNLIKELY (!song))
    return NULL;

  g_object_get (song, "wavetable", &wavetable, NULL);
  if (G_UNLIKELY (!wavetable))
    return NULL;
  if ((wave = bt_wavetable_get_wave_by_index (wavetable, i))) {
    GList *list, *node;
    BtWavelevel *wavelevel, *best = NULL;
    gint max_diff = /*NOTE_MAX */ 200 + 1;
    GstBtNote root_note;

    g_object_get (wave, "wavelevels", &list, NULL);
    for (node = list; node; node = g_list_next (node)) {
      wavelevel = BT_WAVELEVEL (node->data);
      g_object_get (wavelevel, "root-note", &root_note, NULL);
      GST_DEBUG ("  note=%d, diff=%d, max_diff=%d",
          (gint) root_note, abs (note - (gint) root_note), max_diff);

      if (abs (note - (gint) root_note) < max_diff) {
        best = wavelevel;
        max_diff = abs (note - (gint) root_note);
      }
    }
    if (best) {
      gulong length, rate;
      glong ls, le;

      GST_DEBUG ("  wavelevel found");

      // fill BuzzWaveLevel
      g_object_get (best, "length", &length, "rate", &rate,
          "loop-start", &ls, "loop-end", &le,
          "root-note", &root_note, "data", &res[i].pSamples, NULL);
      res[i].numSamples = length;
      res[i].SamplesPerSec = rate;
      res[i].RootNote = root_note;
      res[i].LoopStart = ls;
      res[i].LoopEnd = le;
    } else {
      GST_WARNING ("no nearest wavelevel");
    }
    g_list_free (list);
    g_object_unref (wave);
  } else {
    GST_WARNING ("no wave for index %d", i);
  }
  g_object_unref (wavetable);

  return &res[i];
}

// callback bundle

static CHostCallbacks callbacks = {
  /* user-data */
  NULL,
  /* callbacks */
  GetWave,
  GetWaveLevel,
  GetNearestWaveLevel
};

/**
 * bt_buzz_callbacks_get:
 * @song: the song for the callback context
 *
 * Store the song in the callback structure and return it.
 *
 * Returns: the callbacks
 */
gpointer
bt_buzz_callbacks_get (BtSong * song)
{
  callbacks.user_data = (gpointer) song;
  return &callbacks;
}
