/* $Id$
 *
 * Buzztard
 * Copyright (C) 2008 Buzztard team <buzztard-devel@lists.sf.net>
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
 
#define BT_CORE
#define BT_BUZZCALLBACKS_C

#include "core_private.h"
#ifdef USE_BML
#include <bml.h>

/*
 * we could use some static hashtables to cache the returned structs
 * if bt_buzz_callbacks_get() is called with a song different from what we have
 * set there, we flush the hash tables
 */

// structs

typedef struct {
	int Flags;
	float Volume;
} BuzzWaveInfo;

typedef struct {
	int numSamples;
	short *pSamples;
	int RootNote;
	int SamplesPerSec;
	int LoopStart;
	int LoopEnd;
} BuzzWaveLevel;

// callbacks

static void const *GetWave(CHostCallbacks *self, int const i) {
  static BuzzWaveInfo res;
  
  GST_WARNING("(%p,%d)",self,i);
  
  /*
  BtSong *song=BT_SONG(self->user_data);
  BtWaveTable *wavetable;
  BtWave *wave;
  
  g_object_get(song,"wavetable",&wavetable,NULL);
  if((wave=bt_wavetable_get_wave_by_index(wavetable,i))) {
    // we don't have volume and flags are per wavelevel right now ...
    g_object_unref(wave);
  }
  g_object_unref(wavetable);
  */
  res.Flags=0;
  res.Volume=1.0;

  return(&res);
}

static void const *GetWaveLevel(CHostCallbacks *self, int const i, int const level) {
  static BuzzWaveLevel res={0,};
  BtSong *song=BT_SONG(self->user_data);
  BtWavetable *wavetable;
  BtWave *wave;
  BtWavelevel *wavelevel;
  
  GST_WARNING("(%p,%d,%d)",self,i,level);
  
  g_object_get(song,"wavetable",&wavetable,NULL);
  if((wave=bt_wavetable_get_wave_by_index(wavetable,i-1))) {
    if((wavelevel=bt_wave_get_level_by_index(wave,level))) {
      gulong length,rate;
      glong ls, le;
      
      // fill BuzzWaveLevel
      g_object_get(wavelevel,"length",&length, "rate",&rate,
        "loop-start",&ls, "loop-end",&le,
        "root-note",&res.RootNote, "data",&res.pSamples,        
        NULL);
      res.numSamples=length;
      res.SamplesPerSec=rate;
      res.LoopStart=ls;
      res.LoopEnd=le;
      g_object_unref(wavelevel);
    }
    g_object_unref(wave);
  }
  g_object_unref(wavetable);

  return(&res);
}

static void const *GetNearestWaveLevel(CHostCallbacks *self, int const i, int const note) {
  static BuzzWaveLevel res={0,};
  BtSong *song=BT_SONG(self->user_data);
  BtWavetable *wavetable;
  BtWave *wave;
  BtWavelevel *wavelevel,*best=NULL;
  gint max_diff=/*NOTE_MAX*/200+1;
  guchar root_note;
  
  GST_WARNING("(%p,%d,%d)",self,i,note);

  g_object_get(song,"wavetable",&wavetable,NULL);
  if((wave=bt_wavetable_get_wave_by_index(wavetable,i-1))) {
    GList *list,*node;

    g_object_get(wave,"wavelevels",&list,NULL);
    for(node=list;node;node=g_list_next(node)) {
      wavelevel=BT_WAVELEVEL(node->data);
      g_object_get(wavelevel,"root-note",&root_note,NULL);
      GST_WARNING("  note=%d, diff=%d, max_diff=%d",
        (gint)root_note,abs((gint)note-root_note),max_diff);

      if(abs(note-(gint)root_note)<max_diff) {
        best=wavelevel;
        max_diff=abs(note-(gint)root_note);
      }
    }
    if(best) {
      gulong length,rate;
      glong ls, le;
      
      GST_WARNING("  wavelevel found");
      
      // fill BuzzWaveLevel
      g_object_get(best,"length",&length, "rate",&rate,
        "loop-start",&ls, "loop-end",&le,
        "root-note",&root_note, "data",&res.pSamples,        
        NULL);
      res.numSamples=length;
      res.SamplesPerSec=rate;
      res.RootNote=root_note;
      res.LoopStart=ls;
      res.LoopEnd=le;
    }
    g_list_free(list);
    g_object_unref(wave);
  }
  g_object_unref(wavetable);

  return(&res);
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

#endif

/**
 * bt_buzz_callbacks_get:
 * @song: the song for the callback context
 *
 * Store the song in the callback structure and return it.
 *
 * Returns: the callbacks
 */
void *bt_buzz_callbacks_get(BtSong *song) {
#ifdef USE_BML
  callbacks.user_data=(gpointer)song;
  return &callbacks;
#else
  return NULL;
#endif
}

