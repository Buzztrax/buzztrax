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

#ifndef BT_WAVETABLE_H
#define BT_WAVETABLE_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_WAVETABLE             (bt_wavetable_get_type ())
#define BT_WAVETABLE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_WAVETABLE, BtWavetable))
#define BT_WAVETABLE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_WAVETABLE, BtWavetableClass))
#define BT_IS_WAVETABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_WAVETABLE))
#define BT_IS_WAVETABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_WAVETABLE))
#define BT_WAVETABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_WAVETABLE, BtWavetableClass))

/* type macros */

typedef struct _BtWavetable BtWavetable;
typedef struct _BtWavetableClass BtWavetableClass;
typedef struct _BtWavetablePrivate BtWavetablePrivate;

/**
 * BtWavetable:
 *
 * A table of #BtWave objects.
 */
struct _BtWavetable {
  const GObject parent;

  /*< private >*/
  BtWavetablePrivate *priv;
};

struct _BtWavetableClass {
  const GObjectClass parent;
};

GType bt_wavetable_get_type(void) G_GNUC_CONST;

#include "song.h"
#include "wave.h"

BtWavetable *bt_wavetable_new(const BtSong * const song);

gboolean bt_wavetable_add_wave(const BtWavetable * const self, const BtWave * const wave);
gboolean bt_wavetable_remove_wave(const BtWavetable * const self, const BtWave * const wave);

BtWave *bt_wavetable_get_wave_by_index(const BtWavetable * const self, const gulong index);

void bt_wavetable_remember_missing_wave(const BtWavetable * const self, const gchar * const str);

gpointer bt_wavetable_get_callbacks(BtWavetable * self);

#endif // BT_WAVETABLE_H
