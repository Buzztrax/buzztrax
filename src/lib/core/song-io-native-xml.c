/* Buzztrax
 * Copyright (C) 2008 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * SECTION:btsongionativexml
 * @short_description: class for song input and output in builtin native format
 *
 * This internal #BtSongIONative module implements loading and saving of an own
 * xml format without externals.
 */

#define BT_CORE
#define BT_SONG_IO_NATIVE_XML_C

#include "core_private.h"
#include "song-io-native-xml.h"
#include <glib/gprintf.h>

static GQuark error_domain = 0;

//-- the class

G_DEFINE_TYPE (BtSongIONativeXML, bt_song_io_native_xml,
    BT_TYPE_SONG_IO_NATIVE);


//-- methods

static gboolean
bt_song_io_native_xml_load (gconstpointer const _self,
    const BtSong * const song, GError ** err)
{
  const BtSongIONativeXML *const self = BT_SONG_IO_NATIVE_XML (_self);
  gboolean result = FALSE;

  xmlParserCtxtPtr const ctxt = xmlNewParserCtxt ();
  if (ctxt) {
    xmlDocPtr song_doc;
    gchar *const file_name;
    guint len;
    gpointer data;

    g_object_get ((gpointer) self, "file-name", &file_name, "data", &data,
        "data-len", &len, NULL);
    GST_INFO ("native io xml will now load song from \"%s\"",
        file_name ? file_name : "data");

    if (data && len) {
      // parse the file from the memory block
      song_doc = xmlCtxtReadMemory (ctxt, data, len, NULL, NULL, 0L);
    } else {
      // open the file from the file_name argument
      song_doc = xmlCtxtReadFile (ctxt, file_name, NULL, 0L);
    }
    if (song_doc) {
      if (!ctxt->valid) {
        GST_WARNING ("is not a XML/Buzztrax document");
        g_set_error (err, BT_SONG_IO_ERROR, BT_SONG_IO_ERROR_INVALID_FORMAT,
            _("Is not a XML/Buzztrax document."));
      } else if (!ctxt->wellFormed) {
        GST_WARNING ("is not a wellformed XML document");
        g_set_error (err, BT_SONG_IO_ERROR, BT_SONG_IO_ERROR_INVALID_FORMAT,
            _("Is not a wellformed XML document."));
      } else {
        xmlNodePtr const root_node = xmlDocGetRootElement (song_doc);

        if (root_node == NULL) {
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
            result = TRUE;
          }
        }
      }
      if (song_doc)
        xmlFreeDoc (song_doc);
    } else {
      GST_WARNING ("failed to read song file '%s'", file_name);
      g_set_error_literal (err, G_IO_ERROR, g_io_error_from_errno (errno),
          g_strerror (errno));
    }
    g_free (file_name);
    xmlFreeParserCtxt (ctxt);
  } else {
    GST_WARNING ("failed to create parser context");
    g_set_error (err, G_IO_ERROR, G_IO_ERROR_FAILED,
        "Failed to create parser context.");
  }
  return result;
}

static gboolean
bt_song_io_native_xml_save (gconstpointer const _self,
    const BtSong * const song, GError ** err)
{
  const BtSongIONativeXML *const self = BT_SONG_IO_NATIVE_XML (_self);
  gboolean result = FALSE;
  gchar *const file_name;

  g_object_get ((gpointer) self, "file-name", &file_name, NULL);
  GST_INFO ("native io xml will now save song to \"%s\"", file_name);

  xmlDocPtr const song_doc = xmlNewDoc (XML_CHAR_PTR ("1.0"));
  if (song_doc) {
    xmlNodePtr const root_node =
        bt_persistence_save (BT_PERSISTENCE (song), NULL);
    if (root_node) {
      xmlDocSetRootElement (song_doc, root_node);
      if (xmlSaveFile (file_name, song_doc) != -1) {
        result = TRUE;
        GST_INFO ("xml saved okay");
      } else {
        GST_WARNING ("failed to write song file \"%s\"", file_name);
        g_set_error_literal (err, G_IO_ERROR, g_io_error_from_errno (errno),
            g_strerror (errno));
      }
    } else {
      g_set_error (err, G_IO_ERROR, G_IO_ERROR_FAILED,
          "Failed to serialize XML doc.");
    }
  } else {
    g_set_error (err, G_IO_ERROR, G_IO_ERROR_FAILED,
        "Failed to create XML doc.");
  }

  g_free (file_name);
  return result;
}

//-- wrapper

//-- class internals

static void
bt_song_io_native_xml_init (BtSongIONativeXML * self)
{
}

static void
bt_song_io_native_xml_class_init (BtSongIONativeXMLClass * klass)
{
  BtSongIOClass *const btsongio_class = BT_SONG_IO_CLASS (klass);

  error_domain = g_type_qname (BT_TYPE_SONG_IO_NATIVE_XML);
  //g_type_class_add_private(klass,sizeof(BtSongIONativeXMLPrivate));

  btsongio_class->load = bt_song_io_native_xml_load;
  btsongio_class->save = bt_song_io_native_xml_save;
}
