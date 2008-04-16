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
 * virtual hardware setup
 * (contains #BtMachine and #BtWire objects)
 */
struct _BtWave {
  const GObject parent;
  
  /*< private >*/
  BtWavePrivate *priv;
};

struct _BtWaveClass {
  const GObjectClass parent;
};

GType bt_wave_get_type(void) G_GNUC_CONST;


#endif // BT_WAVE_H
