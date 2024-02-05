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

#ifndef BT_WAVE_H
#define BT_WAVE_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_WAVE            (bt_wave_get_type ())
#define BT_WAVE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_WAVE, BtWave))
#define BT_WAVE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_WAVE, BtWaveClass))
#define BT_IS_WAVE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_WAVE))
#define BT_IS_WAVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_WAVE))
#define BT_WAVE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_WAVE, BtWaveClass))

/* type macros */

typedef struct _BtWave BtWave;
typedef struct _BtWaveClass BtWaveClass;
typedef struct _BtWavePrivate BtWavePrivate;

/**
 * BtWave:
 *
 * A single waveform.
 */
struct _BtWave {
  const GObject parent;
  
  /*< private >*/
  BtWavePrivate *priv;
};

struct _BtWaveClass {
  const GObjectClass parent;
};

#define BT_TYPE_WAVE_LOOP_MODE       (bt_wave_loop_mode_get_type())

/**
 * BtWaveLoopMode:
 * @BT_WAVE_LOOP_MODE_OFF: no loop
 * @BT_WAVE_LOOP_MODE_FORWARD: forward looping
 * @BT_WAVE_LOOP_MODE_PINGPONG: forward/backward looping
 *
 * #BtWave clips can be played using several loop modes.
 */
typedef enum {
  BT_WAVE_LOOP_MODE_OFF=0,
  BT_WAVE_LOOP_MODE_FORWARD,
  BT_WAVE_LOOP_MODE_PINGPONG
} BtWaveLoopMode;

GType bt_wave_loop_mode_get_type(void) G_GNUC_CONST;

GType bt_wave_get_type(void) G_GNUC_CONST;

#include "song.h"
#include "wavelevel.h"

BtWave *bt_wave_new(const BtSong * const song, const gchar * const name, const gchar * const uri, const gulong index, const gdouble volume, const  BtWaveLoopMode loop_mode, const guint channels);

gboolean bt_wave_add_wavelevel(const BtWave * const self, BtWavelevel * const wavelevel);
BtWavelevel *bt_wave_get_level_by_index(const BtWave * const self,const gulong index);
const gchar* bt_wave_get_name(const BtWave* self);

#endif // BT_WAVE_H
