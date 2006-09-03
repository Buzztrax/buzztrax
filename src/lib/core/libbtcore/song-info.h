/* $Id: song-info.h,v 1.16 2006-09-03 13:21:44 ensonic Exp $
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
/* structure of the SongInfo class */
struct _BtSongInfoClass {
  const GObjectClass parent;
};

/* used by SONG_INFO_TYPE */
GType bt_song_info_get_type(void) G_GNUC_CONST;

#endif // BT_SONG_INFO_H
