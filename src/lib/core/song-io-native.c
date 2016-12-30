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
#include "song-io-native-bzt.h"
#include "song-io-native-xml.h"
#include "song-io-native.h"

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
  return TRUE;
}

//-- common helpers

gboolean
bt_song_io_native_load (xmlDocPtr song_doc, const BtSong * const song,
    GError ** err)
{
  xmlNodePtr const root_node = xmlDocGetRootElement (song_doc);

  if (root_node == NULL) {
    // this cannot really happen, since a missing root tag would fail the
    // validity checks
    GST_WARNING ("XML document is empty");
    g_set_error (err, BT_SONG_IO_ERROR, BT_SONG_IO_ERROR_INVALID_FORMAT,
        _("XML document is empty."));
  } else if (xmlStrcmp (root_node->name, (const xmlChar *) "buzztrax") &&
      xmlStrcmp (root_node->name, (const xmlChar *) "buzztard")) {
    GST_WARNING ("wrong XML document root");
    g_set_error (err, BT_SONG_IO_ERROR, BT_SONG_IO_ERROR_INVALID_FORMAT,
        _("Wrong XML document root."));
  } else {
    GError *e = NULL;
    bt_persistence_load (BT_TYPE_SONG, BT_PERSISTENCE (song), root_node,
        &e, NULL);
    if (e != NULL) {
      GST_WARNING ("deserialisation failed: %s", e->message);
      g_propagate_error (err, e);
    } else {
      return TRUE;
    }
  }
  return FALSE;
}

void
bt_song_io_native_load_set_error (xmlParserCtxtPtr const ctxt,
    gchar * const file_name, GError ** err)
{
  /* other things that can be checked:
   * ctxt->valid: DTD validation issue, we don't use a DTD anymore, also
   *              set if !wellFormed
   * ctxt->nsWellFormed:Namespace issue
   */
  if (!ctxt->wellFormed) {
    GST_WARNING ("is not a wellformed XML document");
    g_set_error (err, BT_SONG_IO_ERROR, BT_SONG_IO_ERROR_INVALID_FORMAT,
        _("Is not a wellformed XML document."));
  } else {
    GST_WARNING ("failed to read song file '%s'",
        (file_name ? file_name : "data"));
    g_set_error_literal (err, G_IO_ERROR, g_io_error_from_errno (errno),
        g_strerror (errno));
  }
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
