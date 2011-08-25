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

#ifndef BT_SONG_IO_NATIVE_XML_H
#define BT_SONG_IO_NATIVE_XML_H

#include <glib.h>
#include <glib-object.h>

#include "song-io-native.h"

#define BT_TYPE_SONG_IO_NATIVE_XML            (bt_song_io_native_xml_get_type ())
#define BT_SONG_IO_NATIVE_XML(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SONG_IO_NATIVE_XML, BtSongIONativeXML))
#define BT_SONG_IO_NATIVE_XML_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SONG_IO_NATIVE_XML, BtSongIONativeXMLClass))
#define BT_IS_SONG_IO_NATIVE_XML(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SONG_IO_NATIVE_XML))
#define BT_IS_SONG_IO_NATIVE_XML_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SONG_IO_NATIVE_XML))
#define BT_SONG_IO_NATIVE_XML_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SONG_IO_NATIVE_XML, BtSongIONativeXMLClass))

/* type macros */

typedef struct _BtSongIONativeXML BtSongIONativeXML;
typedef struct _BtSongIONativeXMLClass BtSongIONativeXMLClass;

/**
 * BtSongIONativeXML:
 *
 * object for song input and output in native zip/xml format
 */
struct _BtSongIONativeXML {
  const BtSongIONative parent;
};
/**
 * BtSongIONativeXMLClass:
 * @parent: parent class type
 *
 * Class for song input and output in native zip/xml format
 */
struct _BtSongIONativeXMLClass {
  const BtSongIONativeClass parent;

  /*< private >*/
};

GType bt_song_io_native_xml_get_type(void) G_GNUC_CONST;

#endif // BT_SONG_IO_NATIVE_XML_H
