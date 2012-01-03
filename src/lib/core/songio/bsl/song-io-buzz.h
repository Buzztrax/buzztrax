/* $Id$
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
 *
 * class for buzz song input and output
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
 

#ifndef BT_SONG_IO_BUZZ_H
#define BT_SONG_IO_BUZZ_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_SONG_IO_BUZZ		        (bt_song_io_buzz_get_type ())
#define BT_SONG_IO_BUZZ(obj)		        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SONG_IO_BUZZ, BtSongIOBuzz))
#define BT_SONG_IO_BUZZ_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SONG_IO_BUZZ, BtSongIOBuzzClass))
#define BT_IS_SONG_IO_BUZZ(obj)	        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SONG_IO_BUZZ))
#define BT_IS_SONG_IO_BUZZ_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SONG_IO_BUZZ))
#define BT_SONG_IO_BUZZ_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SONG_IO_BUZZ, BtSongIOBuzzClass))

/* type macros */

typedef struct _BtSongIOBuzz BtSongIOBuzz;
typedef struct _BtSongIOBuzzClass BtSongIOBuzzClass;
typedef struct _BtSongIOBuzzPrivate BtSongIOBuzzPrivate;

/**
 * BtSongIOBuzz:
 *
 * object for song input and output in buzz zip/xml format
 */
struct _BtSongIOBuzz {
  BtSongIO parent;
  
  /*< private >*/
  BtSongIOBuzzPrivate *priv;
};
/**
 * BtSongIOBuzzClass:
 *
 * class for song input and output in buzz binary format
 */
struct _BtSongIOBuzzClass {
  BtSongIOClass parent_class;

	/* class data */
};

/* used by SONG_IO_BUZZ_TYPE */
GType bt_song_io_buzz_get_type(void);

#endif // BT_SONG_IO_BUZZ_H

