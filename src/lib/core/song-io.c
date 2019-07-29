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
 * SECTION:btsongio
 * @short_description: base class for song input and output
 *
 * A base class for #BtSong loader and saver implementations. A #BtSongIO module
 * needs to be installed as a shared library into LIBDIR/songio. It is
 * recognized, if it exports a #BtSongIOModuleInfo structure. At runtime the
 * detect method of each module is called with the chosen file-name. The module
 * should return its #GType if it can handle the format or %NULL else.
 *
 * Such a module should overwrite the bt_song_io_load() and/or bt_song_io_save()
 * default implementations.
 *
 * There is an internal subclass of this called #BtSongIONative.
 *
 * <note><para>
 * This API is not yet fully stable. Please discuss with the deverloper team if
 * you intend to write a io plugin.
 * </para></note>
 */

#define BT_CORE
#define BT_SONG_IO_C

#include "core_private.h"
#include "song-io-native.h"
#include <glib/gprintf.h>

//-- property ids

enum
{
  SONG_IO_FILE_NAME = 1,
  SONG_IO_DATA,
  SONG_IO_DATA_LEN,
  SONG_IO_STATUS
};

struct _BtSongIOPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* used to load or save the song file */
  gchar *file_name;
  gpointer data;
  guint len;

  /* informs about the progress of the loader */
  gchar *status;
};

/* list of registered io-classes, each entry points to a BtSongIOModuleInfo
 * structure for each plugin
 */
static GList *plugins = NULL;

//-- the class

G_DEFINE_ABSTRACT_TYPE (BtSongIO, bt_song_io, G_TYPE_OBJECT);

//-- error codes

GQuark
bt_song_io_error_quark (void)
{
  static GQuark error_quark = 0;
  if (!error_quark) {
    error_quark = g_quark_from_static_string ("bt-song-io-error-quark");
  }
  return error_quark;
}

//-- helper methods

static gboolean
bt_song_io_scan_dir (const gchar * libdir)
{
  GDir *dir;
  const gchar *entry_name;
  gchar plugin_name[FILENAME_MAX];
  gboolean found_new_plugins = FALSE;

  GST_INFO ("  scanning external song-io plugins in '%s'", libdir);

  if (!(dir = g_dir_open (libdir, 0, NULL))) {
    GST_WARNING ("can't open song-io dir '%s'", libdir);
    return FALSE;
  }
  // 1.) scan plugin-folder
  while ((entry_name = g_dir_read_name (dir))) {
    // skip names starting with a dot
    if (*entry_name == '.')
      continue;
    g_snprintf (plugin_name, sizeof(plugin_name), "%s" G_DIR_SEPARATOR_S "%s", libdir, entry_name);
    // skip symlinks and directories
    if (g_file_test (plugin_name, G_FILE_TEST_IS_SYMLINK | G_FILE_TEST_IS_DIR))
      continue;

    // skip files other than shared librares
    if (!g_str_has_suffix (entry_name, "." G_MODULE_SUFFIX))
      continue;
    GST_INFO ("    found file '%s'", plugin_name);

    // 2.) try to open each as g_module
    //if((plugin=g_module_open(plugin_name,G_MODULE_BIND_LAZY))!=NULL) {
    GModule *const plugin =
        g_module_open (plugin_name, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);
    if (plugin != NULL) {
      gpointer bt_song_io_module_info;
      // 3.) gets the address of GType bt_song_io_detect(const gchar *);
      if (g_module_symbol (plugin, "bt_song_io_module_info",
              &bt_song_io_module_info)) {
        if (!g_list_find (plugins, bt_song_io_module_info)) {
          BtSongIOModuleInfo *info =
              (BtSongIOModuleInfo *) bt_song_io_module_info;
          // 4.) store the g_module handle and the function pointer in a list
          //     (uhm, global (static) variable)
          if (info->init && info->init ()) {
            plugins = g_list_append (plugins, bt_song_io_module_info);
            found_new_plugins = TRUE;
            GST_INFO ("    added plugin '%s'", plugin_name);
          }
        } else {
          GST_WARNING ("%s skipped as duplicate", plugin_name);
          g_module_close (plugin);
        }
      } else {
        GST_WARNING ("%s is not a songio plugin", plugin_name);
        g_module_close (plugin);
      }
    } else {
      GST_WARNING ("%s is not a shared object", plugin_name);
    }
  }
  g_dir_close (dir);
  return found_new_plugins;
}

/*
 * bt_song_io_register_plugins:
 *
 * Registers all song-io plugins for later use by bt_song_io_from_file().
 */
static void
bt_song_io_register_plugins (void)
{
  /* IDEA(ensonic): the plugin list now has structures
   * so that we could keep the modules handle.
   * we need this to close the plugins at sometime ... (do we?)
   */
  GST_INFO ("register song-io plugins...");
  // register internal song-io plugin
  if (bt_song_io_native_module_info.init
      && bt_song_io_native_module_info.init ()) {
    plugins =
        g_list_append (plugins, (gpointer) & bt_song_io_native_module_info);
  }
  // registering external song-io plugins
  // first try loading from $srcdir (when uninstalled)
  if (!bt_song_io_scan_dir (".libs")) {
    bt_song_io_scan_dir (LIBDIR G_DIR_SEPARATOR_S PACKAGE "-songio");
  }
}


/* TODO(ensonic): add proper mime-type detection (gio) */
#if 0
GFile *file;
GFileInfo *info;

file = g_file_new_for_path (file_name);

if ((info =
        g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
            G_FILE_QUERY_INFO_NONE, NULL, NULL))) {
  const gchar *mime_type = g_file_info_get_content_type (info);

  g_object_unref (info);
}
g_object_unref (file);
#endif

/*
 * bt_song_io_detect:
 * @file_name: the full filename of the song or %NULL
 * @media_type: the media-type for the song or %NULL
 *
 * Factory method that returns the GType of the class that is able to handle
 * the supplied file or media-type.
 *
 * Returns: the type of the #BtSongIO sub-class that can handle the supplied file
 * and %NULL otherwise
 */
static GType
bt_song_io_detect (const gchar * file_name, const gchar * media_type)
{
  GType type = 0;
  const GList *node;
  BtSongIOModuleInfo *info;
  guint i;
  gchar *lc_file_name = NULL, *ext = NULL;

  GST_INFO ("detecting loader for file '%s', type '%s'",
      safe_string (file_name), safe_string (media_type));

  if (!plugins)
    bt_song_io_register_plugins ();

  // try all registered plugins
  for (node = plugins; node; node = g_list_next (node)) {
    info = (BtSongIOModuleInfo *) node->data;
    GST_INFO ("  trying...");

    i = 0;
    while (info->formats[i].type) {
      if (media_type && info->formats[i].mime_type) {
        if (!strcmp (media_type, info->formats[i].mime_type)) {
          type = info->formats[i].type;
          break;
        }
      } else if (file_name) {
        if (!lc_file_name) {
          lc_file_name = g_ascii_strdown (file_name, -1);
          ext = strrchr (lc_file_name, '.');
        }
        if (ext && info->formats[i].extension) {
          if (!strcmp (&ext[1], info->formats[i].extension)) {
            type = info->formats[i].type;
            break;
          }
        }
      }
      i++;
    }
    if (type) {
      GST_INFO ("  found one: %s!", info->formats[i].name);
      break;
    }
  }

  g_free (lc_file_name);
  return type;
}

/*
 * bt_song_io_update_filename:
 *
 * Grab the file-name from the path in song-io and store it in song-info. Does
 * nothing if we load from memory.
 */
static void
bt_song_io_update_filename (const BtSongIO * const self,
    const BtSong * const song)
{
  if (self->priv->file_name) {
    gchar *file_path = self->priv->file_name;
    gboolean need_free = FALSE;

    if (!g_path_is_absolute (file_path)) {
      gchar *cur_path = g_get_current_dir ();
      file_path = g_build_filename (cur_path, file_path, NULL);
      g_free (cur_path);
      need_free = TRUE;
    }
    GST_INFO ("file path is : %s", file_path);
    bt_child_proxy_set ((gpointer) song, "song-info::file-name", file_path,
        NULL);
    if (need_free) {
      g_free (file_path);
    }
  }
}

//-- constructor methods

/**
 * bt_song_io_from_file:
 * @file_name: the file name of the song
 * @err: where to store the error message in case of an error, or %NULL
 *
 * Create a new instance from the given @file_name. Each installed plugin will
 * test if it can handle the file type.
 *
 * Returns: (transfer full): the new instance or %NULL in case of an error
 */
BtSongIO *
bt_song_io_from_file (const gchar * const file_name, GError ** err)
{
  BtSongIO *self = NULL;
  GType type = 0;

  if (!BT_IS_STRING (file_name)) {
    GST_WARNING ("filename must not be empty");
    g_set_error (err, BT_SONG_IO_ERROR, BT_SONG_IO_ERROR_UNKNOWN_FORMAT,
        _("Filename must not be empty."));
    return NULL;
  }
  type = bt_song_io_detect (file_name, NULL);
  if (type) {
    self = BT_SONG_IO (g_object_new (type, NULL));
    self->priv->file_name = g_strdup (file_name);
  } else {
    GST_WARNING ("no io-module for filename '%s'", file_name);
    g_set_error (err, BT_SONG_IO_ERROR, BT_SONG_IO_ERROR_UNKNOWN_FORMAT,
        _("No io-module for filename '%s'."), file_name);
  }
  return self;
}

/**
 * bt_song_io_from_data:
 * @data: in memory data of the song
 * @len: the siye of the @data block
 * @media_type: the media-type of the song, if available
 * @err: where to store the error message in case of an error, or %NULL
 *
 * Create a new instance from the given parameters. Each installed plugin will
 * test if it can handle the file type.
 *
 * Returns: (transfer full): the new instance or %NULL in case of an error
 */
BtSongIO *
bt_song_io_from_data (gpointer data, guint len, const gchar * media_type,
    GError ** err)
{
  BtSongIO *self = NULL;
  GType type = 0;

  if (!BT_IS_STRING (media_type)) {
    GST_WARNING ("media-type must not be empty");
    g_set_error (err, BT_SONG_IO_ERROR, BT_SONG_IO_ERROR_UNKNOWN_FORMAT,
        _("Media-type must not be empty."));
    return NULL;
  }
  type = bt_song_io_detect (NULL, media_type);
  if (type) {
    self = BT_SONG_IO (g_object_new (type, NULL));
    self->priv->data = data;
    self->priv->len = len;
  } else {
    GST_WARNING ("no io-module for media-type '%s'", media_type);
    g_set_error (err, BT_SONG_IO_ERROR, BT_SONG_IO_ERROR_UNKNOWN_FORMAT,
        _("No io-module for media-type '%s'."), media_type);
  }
  return self;
}

//-- methods

/**
 * bt_song_io_get_module_info_list:
 *
 * Get read only access to list of #BtSongIOModuleInfo entries.
 *
 * Returns: (element-type BuzztraxCore.SongIOModuleInfo) (transfer none): the
 * #GList.
 */
const GList *
bt_song_io_get_module_info_list (void)
{

  if (!plugins)
    bt_song_io_register_plugins ();

  return plugins;
}

//-- wrapper

/**
 * bt_song_io_load:
 * @self: the #BtSongIO instance to use
 * @song: the #BtSong instance that should initialized
 * @err: where to store the error message in case of an error, or %NULL
 *
 * load the song from a file.  The file is set in the constructor
 *
 * Returns: %TRUE for success
 */
gboolean
bt_song_io_load (BtSongIO const *self, const BtSong * const song, GError ** err)
{
  gboolean result;
  gchar *status;
  bt_song_io_virtual_load load;

  g_return_val_if_fail (BT_IS_SONG_IO (self), FALSE);
  g_return_val_if_fail (BT_IS_SONG (song), FALSE);

  load = BT_SONG_IO_GET_CLASS (self)->load;
  if (!load) {
    GST_ERROR
        ("virtual method bt_song_io_load(self=%p,song=%p) called without implementation",
        self, song);
    return FALSE;
  }

  const gchar *const msg = _("Loading file '%s'");
  if (self->priv->file_name) {
    status = g_alloca (1 + strlen (msg) + strlen (self->priv->file_name));
    g_sprintf (status, msg, self->priv->file_name);
    GST_INFO ("loading song [%s]", self->priv->file_name);
  } else {
    gint len = 1 + strlen (msg) + 4;
    status = g_alloca (len);
    g_snprintf (status, len, msg, "data");
    GST_INFO ("loading song [<data>]");
  }
  g_object_set ((gpointer) self, "status", status, NULL);

  g_object_set ((gpointer) song, "song-io", self, NULL);
  if ((result = load (self, song, err))) {
    bt_song_io_update_filename (BT_SONG_IO (self), song);
    GST_INFO ("loading done");
    //DEBUG
    /*
       {
       BtSetup * const setup;
       GList * const list;
       const GList *node;

       g_object_get((gpointer)song,"setup",&setup,NULL);
       g_object_get(setup,"machines",&list,NULL);
       for(node=list;node;node=g_list_next(node)) {
       const BtMachine * const machine=BT_MACHINE(node->data);
       if(BT_IS_SOURCE_MACHINE(machine)) {
       bt_machine_dbg_dump_global_controller_queue(machine);
       }
       }
       g_list_free(list);
       g_object_unref(setup);
       }
     */
    //DEBUG
  }
  g_object_set ((gpointer) song, "song-io", NULL, NULL);

  g_object_set ((gpointer) self, "status", NULL, NULL);
  GST_INFO ("loaded song [%s] = %d",
      self->priv->file_name ? self->priv->file_name : "data", result);
  return result;
}

/**
 * bt_song_io_save:
 * @self: the #BtSongIO instance to use
 * @song: the #BtSong instance that should stored
 * @err: where to store the error message in case of an error, or %NULL
 *
 * save the song to a file.  The file is set in the constructor
 *
 * Returns: %TRUE for success
 */
gboolean
bt_song_io_save (BtSongIO const *self, const BtSong * const song, GError ** err)
{
  gboolean result;
  gchar *status;
  bt_song_io_virtual_save save;

  g_return_val_if_fail (BT_IS_SONG_IO (self), FALSE);
  g_return_val_if_fail (BT_IS_SONG (song), FALSE);

  save = BT_SONG_IO_GET_CLASS (self)->save;
  if (!save) {
    GST_ERROR
        ("virtual method bt_song_io_save(self=%p,song=%p) called without implementation",
        self, song);
    return FALSE;
  }

  const gchar *const msg = _("Saving file '%s'");
  if (self->priv->file_name) {
    status = g_alloca (1 + strlen (msg) + strlen (self->priv->file_name));
    g_sprintf (status, msg, self->priv->file_name);
    GST_INFO ("saving song [%s]", self->priv->file_name);
  } else {
    gint len = 1 + strlen (msg) + 4;
    status = g_alloca (len);
    g_snprintf (status, len, msg, "data");
    GST_INFO ("saving song [<data>]");
  }
  g_object_set ((gpointer) self, "status", status, NULL);

  g_object_set ((gpointer) song, "song-io", self, NULL);
  if ((result = save (self, song, err))) {
    bt_song_io_update_filename (BT_SONG_IO (self), song);
  }
  g_object_set ((gpointer) song, "song-io", NULL, NULL);

  g_object_set ((gpointer) self, "status", NULL, NULL);
  GST_INFO ("saved song [%s] = %d",
      self->priv->file_name ? self->priv->file_name : "data", result);
  return result;
}

//-- class internals

static void
bt_song_io_get_property (GObject * const object, const guint property_id,
    GValue * const value, GParamSpec * const pspec)
{
  const BtSongIO *const self = BT_SONG_IO (object);
  return_if_disposed ();
  switch (property_id) {
    case SONG_IO_FILE_NAME:
      g_value_set_string (value, self->priv->file_name);
      break;
    case SONG_IO_DATA:
      g_value_set_pointer (value, self->priv->data);
      break;
    case SONG_IO_DATA_LEN:
      g_value_set_uint (value, self->priv->len);
      break;
    case SONG_IO_STATUS:
      g_value_set_string (value, self->priv->status);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_song_io_set_property (GObject * const object, const guint property_id,
    const GValue * const value, GParamSpec * const pspec)
{
  const BtSongIO *const self = BT_SONG_IO (object);
  return_if_disposed ();
  switch (property_id) {
    case SONG_IO_DATA:
      self->priv->data = g_value_get_pointer (value);
      break;
    case SONG_IO_DATA_LEN:
      self->priv->len = g_value_get_uint (value);
      break;
    case SONG_IO_STATUS:
      g_free (self->priv->status);
      self->priv->status = g_value_dup_string (value);
      GST_DEBUG ("set the status for song_io: %s", self->priv->status);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_song_io_dispose (GObject * const object)
{
  const BtSongIO *const self = BT_SONG_IO (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  G_OBJECT_CLASS (bt_song_io_parent_class)->dispose (object);
}

static void
bt_song_io_finalize (GObject * const object)
{
  const BtSongIO *const self = BT_SONG_IO (object);

  GST_DEBUG ("!!!! self=%p", self);

  g_free (self->priv->file_name);

  G_OBJECT_CLASS (bt_song_io_parent_class)->finalize (object);
}

static void
bt_song_io_init (BtSongIO * self)
{
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, BT_TYPE_SONG_IO, BtSongIOPrivate);
}

static void
bt_song_io_class_init (BtSongIOClass * const klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BtSongIOPrivate));

  gobject_class->set_property = bt_song_io_set_property;
  gobject_class->get_property = bt_song_io_get_property;
  gobject_class->dispose = bt_song_io_dispose;
  gobject_class->finalize = bt_song_io_finalize;

  g_object_class_install_property (gobject_class, SONG_IO_FILE_NAME,
      g_param_spec_string ("file-name", "filename prop",
          "full filename for load/save operations", NULL,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SONG_IO_DATA,
      g_param_spec_pointer ("data",
          "data prop",
          "in memory block pointer for load/save operations",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SONG_IO_DATA_LEN,
      g_param_spec_uint ("data-len",
          "data-len prop",
          "in memory block length for load/save operations",
          0, G_MAXUINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, SONG_IO_STATUS,
      g_param_spec_string ("status", "status prop",
          "status of load/save operations", NULL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}
