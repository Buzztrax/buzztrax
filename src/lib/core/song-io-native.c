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
/**
 * SECTION:btsongionative
 * @short_description: class for song input and output in builtin native format
 *
 * Buzztrax stores songs in an own file-format. This internal io-module 
 * implements loading and saving of this format.
 * The format is an archive, that contains an XML file and optionally binary
 * data, such as audio samples.
 */

#define BT_CORE
#define BT_SONG_IO_NATIVE_C

#include "core_private.h"

//-- the class

G_DEFINE_ABSTRACT_TYPE (BtSongIONative, bt_song_io_native, BT_TYPE_SONG_IO);

//-- plugin detect

static gboolean bt_song_io_init (void);

/**
 * bt_song_io_native_module_info:
 *
 * Buzztrax native format song loader/saver metadata.
 */
BtSongIOModuleInfo bt_song_io_native_module_info = {
  bt_song_io_init,
  {
#ifdef USE_GSF
        {0, "buzztrax song with externals", "audio/x-bzt", "bzt"},
#endif
        {0, "buzztrax song without externals", "audio/x-bzt-xml", "xml"},
        {0, NULL, NULL, NULL}
      }
};

static gboolean
bt_song_io_init (void)
{
  static gboolean first_run = TRUE;

  if (first_run) {
#ifdef ENABLE_NLS
    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

    GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "bt-bsl", 0,
        "music production environment / buzz song io plugin");

#ifdef USE_GSF
    bt_song_io_native_module_info.formats[0].type = BT_TYPE_SONG_IO_NATIVE_BZT;
    bt_song_io_native_module_info.formats[1].type = BT_TYPE_SONG_IO_NATIVE_XML;
#else
    bt_song_io_native_module_info.formats[0].type = BT_TYPE_SONG_IO_NATIVE_XML;
#endif
    first_run = FALSE;
  }
  return (TRUE);
}

//-- methods

//-- wrapper

//-- class internals

static void
bt_song_io_native_init (BtSongIONative * self)
{
}

static void
bt_song_io_native_class_init (BtSongIONativeClass * const klass)
{
  //g_type_class_add_private(klass,sizeof(BtSongIONativePrivate));
}
