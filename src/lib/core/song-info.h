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

#ifndef BT_SONG_INFO_H
#define BT_SONG_INFO_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_SONG_INFO             (bt_song_info_get_type ())
#define BT_SONG_INFO(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SONG_INFO, BtSongInfo))
#define BT_SONG_INFO_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SONG_INFO, BtSongInfoClass))
#define BT_IS_SONG_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SONG_INFO))
#define BT_IS_SONG_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SONG_INFO))
#define BT_SONG_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SONG_INFO, BtSongInfoClass))

/* type macros */

typedef struct _BtSongInfo BtSongInfo;
typedef struct _BtSongInfoClass BtSongInfoClass;
typedef struct _BtSongInfoPrivate BtSongInfoPrivate;

/**
 * BtSongInfo:
 *
 * holds song metadata
 */
struct _BtSongInfo {
  const GObject parent;
  
  /*< private >*/
  BtSongInfoPrivate *priv;
};

struct _BtSongInfoClass {
  const GObjectClass parent;
};

GType bt_song_info_get_type(void) G_GNUC_CONST;

BtSongInfo *bt_song_info_new(const BtSong * const song);

gint bt_song_info_get_seconds_since_last_saved(const BtSongInfo * const self);
const gchar* bt_song_info_get_change_dts_in_local_tz(const BtSongInfo * const self);

GstClockTime bt_song_info_tick_to_time(const BtSongInfo * const self, const gulong tick);
gulong bt_song_info_time_to_tick(const BtSongInfo * const self, const GstClockTime ts);

void bt_song_info_time_to_m_s_ms(const BtSongInfo * const self, gulong ts, gulong *m, gulong *s, gulong *ms);
void bt_song_info_tick_to_m_s_ms(const BtSongInfo * const self, const gulong tick, gulong *m, gulong *s, gulong *ms);

gchar *bt_song_info_get_name(const BtSongInfo * const self);

#endif // BT_SONG_INFO_H
