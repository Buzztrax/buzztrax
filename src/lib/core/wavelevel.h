/* Buzztard
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BT_WAVELEVEL_H
#define BT_WAVELEVEL_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_WAVELEVEL             (bt_wavelevel_get_type ())
#define BT_WAVELEVEL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_WAVELEVEL, BtWavelevel))
#define BT_WAVELEVEL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_WAVELEVEL, BtWavelevelClass))
#define BT_IS_WAVELEVEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_WAVELEVEL))
#define BT_IS_WAVELEVEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_WAVELEVEL))
#define BT_WAVELEVEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_WAVELEVEL, BtWavelevelClass))

/* type macros */

typedef struct _BtWavelevel BtWavelevel;
typedef struct _BtWavelevelClass BtWavelevelClass;
typedef struct _BtWavelevelPrivate BtWavelevelPrivate;

/**
 * BtWavelevel:
 *
 * A tone level for a #BtWave. In most cases a #BtWave has only one
 * #BtWavelevel.
 */
struct _BtWavelevel {
  const GObject parent;
  
  /*< private >*/
  BtWavelevelPrivate *priv;
};

struct _BtWavelevelClass {
  const GObjectClass parent;
};

/**
 * BT_WAVELEVEL_DEFAULT_ROOT_NOTE:
 *
 * Default base-note for a wavelevel (c-3).
 */
#define BT_WAVELEVEL_DEFAULT_ROOT_NOTE (1+(4*16))

GType bt_wavelevel_get_type(void) G_GNUC_CONST;

#include "song.h"
#include "wave.h"

BtWavelevel *bt_wavelevel_new(const BtSong * const song, const BtWave * const wave, const GstBtNote root_note, const gulong length, const glong loop_start, const glong loop_end, const gulong rate, gconstpointer sample);

#endif // BT_WAVELEVEL_H
