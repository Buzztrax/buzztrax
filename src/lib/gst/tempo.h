/* Buzztrax
 * Copyright (C) 2005 Stefan Kost <ensonic@users.sf.net>
 *
 * tempo.h: helper for tempo synced gstreamer elements
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

#define GSTBT_AUDIO_TEMPO_TYPE "gstbt.audio.Tempo"

GstContext *gstbt_audio_tempo_context_new (guint bpm, guint tpb, guint stpb);
gboolean gstbt_audio_tempo_context_get_tempo (GstContext *ctx, guint *bpm, guint *tpb, guint *stpb);

G_END_DECLS

#endif /* __GSTBT_TEMPO_H__ */
