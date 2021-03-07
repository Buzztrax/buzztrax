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
 * SECTION:btsongionativebzt
 * @short_description: class for song input and output in builtin native format
 *
 * This internal #BtSongIONative module implements loading and saving of an own
 * xml format with externals.
 * The format is an archive, that contains an XML file and optionally binary
 * data, such as audio samples.
 */

#define BT_CORE
#define BT_SONG_IO_NATIVE_BZT_C

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "core_private.h"
#include "song-io-native-bzt.h"
#include <glib/gprintf.h>

#ifdef USE_GSF
#include <gsf/gsf-utils.h>
#include <gsf/gsf-input-stdio.h>
#include <gsf/gsf-input-memory.h>
#include <gsf/gsf-infile.h>
#include <gsf/gsf-infile-zip.h>
#include <gsf/gsf-output-memory.h>
#include <gsf/gsf-output-stdio.h>
#include <gsf/gsf-outfile.h>
#include <gsf/gsf-outfile-zip.h>
#endif

//-- common helpers

gboolean bt_song_io_native_load (xmlDocPtr song_doc, const BtSong * const song,
    GError ** err);
void bt_song_io_native_load_set_error (xmlParserCtxtPtr const ctxt,
    gchar * const file_name, GError ** err);

struct _BtSongIONativeBZTPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

#ifdef USE_GSF
  GsfInput *input;
  GsfInfile *infile;
  GsfOutput *output;
  GsfOutfile *outfile;
#endif
};

static GQuark error_domain = 0;

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtSongIONativeBZT, bt_song_io_native_bzt,
    BT_TYPE_SONG_IO_NATIVE,
    G_ADD_PRIVATE(BtSongIONativeBZT));

//-- public methods

/**
 * bt_song_io_native_bzt_copy_to_fd:
 * @self: the song-plugin
 * @file_name: the path to the file inside the song
 * @fd: a file-descriptor of an opened file to copy @file_name to
 *
 * Copies the file specified by @file_name from the song file to the @fd.
 *
 * This is a helper for #BtSong persistence.
 *
 * Returns: %TRUE on success
 */
gboolean
bt_song_io_native_bzt_copy_to_fd (const BtSongIONativeBZT * const self,
    const gchar * file_name, gint fd)
{
  gboolean res = FALSE;
#ifdef USE_GSF
  GsfInput *data;
#if 1
  GsfInfile *infile = self->priv->infile, *tmp_infile = NULL;
  gchar **parts;
  gint i = 0, num;

  // bahh, we need to get the dir for the file_name
  // or use gsf_infile_child_by_vname(infile,parts[0],parts[1],...NULL);
  parts = g_strsplit (file_name, G_DIR_SEPARATOR_S, 0);
  num = gsf_infile_num_children (infile);
  while (parts[i] && (num > 0)) {
    GST_INFO ("%2d : %s", i, parts[i]);
    if ((tmp_infile = GSF_INFILE (gsf_infile_child_by_name (infile, parts[i])))) {
      num = gsf_infile_num_children (tmp_infile);
      i++;
    } else {
      GST_WARNING ("error opening \"%s\"", parts[i]);
      num = 0;
    }
    if (infile != self->priv->infile)
      g_object_unref (infile);
    infile = tmp_infile;
  }
  g_strfreev (parts);

  // get file from zip
  if ((data = GSF_INPUT (tmp_infile))) {
#else
  // this won't work : http://bugzilla.gnome.org/show_bug.cgi?id=540521
  // there will be new api in libgsf-1.14.9
  // opensuse-11.0 has 1.14.8
  // gsf_infile_child_by_aname / gsf_infile_child_by_vaname
  if ((data = gsf_infile_child_by_name (self->priv->infile, file_name))) {
#endif
    const guint8 *bytes;
    size_t len = (size_t) gsf_input_size (data);

    GST_INFO ("'%s' size: %" G_GSIZE_FORMAT, gsf_input_name (data), len);

    if ((bytes = gsf_input_read (data, len, NULL))) {
      // write to fd
      if ((write (fd, bytes, len))) {
        if (lseek (fd, 0, SEEK_SET) == 0) {
          res = TRUE;
        } else {
          GST_ERROR ("error seeking back to 0 \"%s\"", file_name);
        }
      } else {
        GST_ERROR ("error writing data \"%s\"", file_name);
      }
    } else {
      GST_WARNING ("error reading data \"%s\"", file_name);
    }
    g_object_unref (data);
  } else {
    GST_WARNING ("error opening \"%s\"", file_name);
  }
#endif
  return res;
}

/**
 * bt_song_io_native_bzt_copy_from_uri:
 * @self: the song-plugin
 * @file_name: the path to the file inside the song
 * @uri: location of the source file
 *
 * Copies the file specified by @uri to @file_name into the song file.
 *
 * This is a helper for #BtSong persistence.
 *
 * Returns: %TRUE on success
 */

gboolean
bt_song_io_native_bzt_copy_from_uri (const BtSongIONativeBZT * const self,
    const gchar * file_name, const gchar * uri)
{
  gboolean res = FALSE;
#ifdef USE_GSF
  GsfOutput *data;

  if ((data = gsf_outfile_new_child (self->priv->outfile, file_name, FALSE))) {
    GError *err = NULL;
    gchar *src_file_name;
    gchar *bytes;
    gsize size;
    gboolean have_data = FALSE;

    GST_INFO ("src uri : %s", uri);

    // IDEA(ensonic): what about using gio here
    src_file_name = g_filename_from_uri (uri, NULL, NULL);
    if (src_file_name) {
      if (g_file_get_contents (src_file_name, &bytes, &size, &err)) {
        have_data = TRUE;
      } else {
        GST_ERROR ("error reading data \"%s\" : %s", file_name, err->message);
        g_error_free (err);
      }
    } else {
      if (g_str_has_prefix (uri, "fd://")) {
        struct stat buf;
        gint fd;

        sscanf (uri, "fd://%d", &fd);
        GST_INFO ("read data from file-deskriptor: fd=%d", fd);

        if (!(fstat (fd, &buf))) {
          if (lseek (fd, 0, SEEK_SET) == 0) {
            if ((bytes = g_try_malloc (buf.st_size))) {
              size = read (fd, bytes, buf.st_size);
              if (size != buf.st_size) {
                GST_WARNING ("did not write all data");
              }
              have_data = TRUE;
            }
          } else {
            GST_WARNING ("can't seek in filedeskriptor");
          }
        }
      }
    }

    if (have_data) {
      GST_INFO ("write %" G_GSIZE_FORMAT " bytes to sample file", size);
      if (gsf_output_write (data, (size_t) size, (guint8 const *) bytes)) {
        res = TRUE;
      } else {
        GST_ERROR ("error writing data \"%s\"", file_name);
      }
      g_free (bytes);
    }
    g_free (src_file_name);
    gsf_output_close (data);
    g_object_unref (data);
  }
#endif
  return res;
}

//-- methods

static gboolean
bt_song_io_native_bzt_load (gconstpointer const _self,
    const BtSong * const song, GError ** err)
{
  gboolean result = FALSE;
#ifdef USE_GSF
  const BtSongIONativeBZT *const self = BT_SONG_IO_NATIVE_BZT (_self);
  xmlDocPtr song_doc = NULL;
  GError *e = NULL;
  gchar *const file_name;
  guint len;
  gpointer data;

  xmlParserCtxtPtr const ctxt = xmlNewParserCtxt ();
  if (!ctxt) {
    GST_WARNING ("failed to create parser context");
    g_set_error (err, G_IO_ERROR, G_IO_ERROR_FAILED,
        "Failed to create parser context.");
    return FALSE;
  }

  g_object_get ((gpointer) self, "file-name", &file_name, "data", &data,
      "data-len", &len, NULL);
  GST_INFO ("native io bzt will now load song from \"%s\"",
      file_name ? file_name : "data");

  if (data && len) {
    // parse the file from the memory block
    self->priv->input = gsf_input_memory_new (data, len, FALSE);
  } else {
    // open the file from the file_name argument
    self->priv->input = gsf_input_stdio_new (file_name, &e);
  }
  if (self->priv->input) {
    // create an gsf input file
    if ((self->priv->infile = gsf_infile_zip_new (self->priv->input, &e))) {
      GsfInput *data;

      GST_INFO ("'%s' size: %" GSF_OFF_T_FORMAT ", files: %d",
          gsf_input_name (self->priv->input),
          gsf_input_size (self->priv->input),
          gsf_infile_num_children (self->priv->infile));

      // get file from zip
      if ((data = gsf_infile_child_by_name (self->priv->infile, "song.xml"))) {
        const guint8 *bytes;
        size_t len = (size_t) gsf_input_size (data);

        GST_INFO ("'%s' size: %" G_GSIZE_FORMAT, gsf_input_name (data), len);

        if ((bytes = gsf_input_read (data, len, NULL))) {
          song_doc =
              xmlCtxtReadMemory (ctxt, (const char *) bytes, len,
              "http://www.buzztrax.org", NULL, 0L);
        } else {
          GST_WARNING ("'%s': error reading data",
              (file_name ? file_name : "data"));
        }
        g_object_unref (data);
      }
    } else {
      GST_WARNING ("'%s' is not a zip file: %s",
          (file_name ? file_name : "data"), e->message);
      g_propagate_error (err, e);
    }
  } else {
    if (e) {
      GST_WARNING ("'%s' error: %s", (file_name ? file_name : "data"),
          e->message);
      g_propagate_error (err, e);
    } else {
      GST_WARNING ("'%s' error", (file_name ? file_name : "data"));
      g_set_error (err, G_IO_ERROR, G_IO_ERROR_FAILED,
          _("Failed to create memory input."));
    }
  }

  if (song_doc) {
    result = bt_song_io_native_load (song_doc, song, err);
    xmlFreeDoc (song_doc);
  } else {
    bt_song_io_native_load_set_error (ctxt, file_name, err);
  }

  if (self->priv->infile) {
    g_object_unref (self->priv->infile);
    self->priv->infile = NULL;
  }
  if (self->priv->input) {
    g_object_unref (self->priv->input);
    self->priv->input = NULL;
  }
  g_free (file_name);
  xmlFreeParserCtxt (ctxt);
#endif
  return result;
}

static gboolean
bt_song_io_native_bzt_save (gconstpointer const _self,
    const BtSong * const song, GError ** err)
{
  gboolean result = FALSE;
#ifdef USE_GSF
  const BtSongIONativeBZT *const self = BT_SONG_IO_NATIVE_BZT (_self);
  gchar *const file_name;

  g_object_get ((gpointer) self, "file-name", &file_name, NULL);
  GST_INFO ("native io bzt will now save song to \"%s\"",
      file_name ? file_name : "data");

  xmlDocPtr const song_doc = xmlNewDoc (XML_CHAR_PTR ("1.0"));
  if (song_doc) {
    GError *e = NULL;

    if (file_name) {
      // open the file from the file_name argument
      if (!(self->priv->output = gsf_output_stdio_new (file_name, &e))) {
        GST_WARNING ("failed to write song file \"%s\" : %s", file_name,
            e->message);
        g_propagate_error (err, e);
        goto Error;
      }
    } else {
      if (!(self->priv->output = gsf_output_memory_new ())) {
        GST_WARNING ("failed to create song buffer");
        g_set_error (err, G_IO_ERROR, G_IO_ERROR_FAILED,
            "Failed to create song buffer.");
        goto Error;
      }
    }
    // create an gsf output file
    if (!(self->priv->outfile = gsf_outfile_zip_new (self->priv->output, &e))) {
      GST_WARNING ("failed to create zip song file \"%s\" : %s", file_name,
          e->message);
      g_propagate_error (err, e);
      goto Error;
    }

    xmlNodePtr const root_node =
        bt_persistence_save (BT_PERSISTENCE (song), NULL);
    if (root_node) {
      GsfOutput *data;

      xmlDocSetRootElement (song_doc, root_node);

      // create file in zip
      if ((data =
              gsf_outfile_new_child (self->priv->outfile, "song.xml", FALSE))) {
        xmlChar *bytes;
        gint size;

        xmlDocDumpMemory (song_doc, &bytes, &size);
        if (gsf_output_write (data, (size_t) size, (guint8 const *) bytes)) {
          if (!file_name) {
            gsf_off_t len = gsf_output_size (self->priv->output);
            const guint8 *mem = gsf_output_memory_get_bytes (
                (GsfOutputMemory *) self->priv->output);
            gpointer data = g_memdup (mem, (guint) len);
            g_object_set ((gpointer) self, "data", data, "data-len",
                (guint) len, NULL);
          }
          result = TRUE;
          GST_INFO ("bzt saved okay");
        } else {
          GST_WARNING ("failed to write song file \"%s\"", file_name);
          g_set_error_literal (err, G_IO_ERROR, g_io_error_from_errno (errno),
              g_strerror (errno));
        }
        xmlFree (bytes);
        gsf_output_close (data);
        g_object_unref (data);
      } else {
        g_set_error (err, G_IO_ERROR, G_IO_ERROR_FAILED,
            "Failed to create child object in output file.");
      }
    } else {
      g_set_error (err, G_IO_ERROR, G_IO_ERROR_FAILED,
          "Failed to serialize XML doc.");
    }
  } else {
    g_set_error (err, G_IO_ERROR, G_IO_ERROR_FAILED,
        "Failed to create XML doc.");
  }

Error:
  if (self->priv->outfile) {
    gsf_output_close (GSF_OUTPUT (self->priv->outfile));
    g_object_unref (self->priv->outfile);
    self->priv->outfile = NULL;
  }
  if (self->priv->output) {
    gsf_output_close (self->priv->output);
    g_object_unref (self->priv->output);
    self->priv->output = NULL;
  }

  g_free (file_name);
#endif
  return result;
}

//-- wrapper

//-- class internals

static void
bt_song_io_native_bzt_dispose (GObject * const object)
{
  const BtSongIONativeBZT *const self = BT_SONG_IO_NATIVE_BZT (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);
  G_OBJECT_CLASS (bt_song_io_native_bzt_parent_class)->dispose (object);
}

static void
bt_song_io_native_bzt_init (BtSongIONativeBZT * self)
{
  self->priv = bt_song_io_native_bzt_get_instance_private(self);
}

static void
bt_song_io_native_bzt_class_init (BtSongIONativeBZTClass * const klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);
  BtSongIOClass *const btsongio_class = BT_SONG_IO_CLASS (klass);

  error_domain = g_type_qname (BT_TYPE_SONG_IO_NATIVE_BZT);

  gobject_class->dispose = bt_song_io_native_bzt_dispose;

  btsongio_class->load = bt_song_io_native_bzt_load;
  btsongio_class->save = bt_song_io_native_bzt_save;
}
