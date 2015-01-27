/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic@users.sf.net>
 *
 * tempo.h: helper interface header for tempo synced gstreamer elements
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

#ifndef __GSTBT_TEMPO_H__
#define __GSTBT_TEMPO_H__

#include <glib-object.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define GSTBT_TYPE_TEMPO               (gstbt_tempo_get_type())
#define GSTBT_TEMPO(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSTBT_TYPE_TEMPO, GstBtTempo))
#define GSTBT_IS_TEMPO(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSTBT_TYPE_TEMPO))
#define GSTBT_TEMPO_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GSTBT_TYPE_TEMPO, GstBtTempoInterface))


typedef struct _GstBtTempo GstBtTempo; /* dummy object */
typedef struct _GstBtTempoInterface GstBtTempoInterface;

/**
 * GstBtTempo:
 *
 * Opaque interface handle.
 */
/**
 * GstBtTempoInterface:
 * @parent: parent type
 * @change_tempo: vmethod for changing the song tempo
 *
 * Interface structure.
 */
struct _GstBtTempoInterface
{
  GTypeInterface parent;

  void (*change_tempo) (GstBtTempo *self, glong beats_per_minute, glong ticks_per_beat, glong subticks_per_tick);
};

GType gstbt_tempo_get_type(void);

void gstbt_tempo_change_tempo (GstBtTempo *self, glong beats_per_minute, glong ticks_per_beat, glong subticks_per_tick);

G_END_DECLS

#endif /* __GSTBT_TEMPO_H__ */
