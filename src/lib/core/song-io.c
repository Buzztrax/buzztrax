/* $Id$
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
/**
 * SECTION:btsongio
 * @short_description: base class for song input and output
 *
 * A base class for #BtSong loader and saver implementations. A #SongIO module
 * needs to be installed as a shared library into LIBDIR/songio. It is
 * recognized, if it exports method named bt_song_io_detect(). At runtime the
 * detect method of each module is called with the choosen file-name. The module
 * should return its #GType if it can handle the format or %NULL else.
 *
 * Such a module should overwrite bt_song_io_load() and/or bt_song_io_save().
 *
 * There is an internal subclass of this called #BtSongIONative.
 *
 * <note><para>
 * This API is not yet fully stable. Please discuss with the deverloper team. if
 * you intend to write a io plugin
 * </para></note>
 */

#define BT_CORE
#define BT_SONG_IO_C

#include "core_private.h"

//-- property ids

enum {
  SONG_IO_FILE_NAME=1,
  SONG_IO_STATUS
};

struct _BtSongIOPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* used to load or save the song file */
  gchar *file_name;

  /* informs about the progress of the loader */
  gchar *status;
};

static GObjectClass *parent_class=NULL;

/* list of registered io-classes, each entry points to a detect function
 * @todo: we should gather a BtSongIOModuleInfo structure for each plugin
 * This would help with code in main-window.c
 */
static GList *plugins=NULL;


//-- helper methods

/*
 * bt_song_io_register_plugins:
 *
 * Registers all song-io plugins for later use by bt_song_io_detect().
 */
static void bt_song_io_register_plugins(void) {
  DIR * const dirp=opendir(LIBDIR G_DIR_SEPARATOR_S PACKAGE "-songio");

  /* @todo the plugin list now has structures
   * so that apart from the detect ptr, we could keep the modules handle.
   * we need this to close the plugins at sometime ... (do we?)
   */
  GST_INFO("register song-io plugins...");
  // register internal song-io plugin
  plugins=g_list_append(plugins,(gpointer)&bt_song_io_native_module_info);
  // registering external song-io plugins
  GST_INFO("  scanning external song-io plugins in "LIBDIR G_DIR_SEPARATOR_S PACKAGE "-songio");
  if(dirp) {
    const struct dirent *dire;
    gpointer bt_song_io_module_info=NULL;
    gchar link_target[FILENAME_MAX],plugin_name[FILENAME_MAX];

    // 1.) scan plugin-folder (LIBDIR/songio)
    while((dire=readdir(dirp))!=NULL) {
      // skip names starting with a dot
      if((!dire->d_name) || (*dire->d_name=='.')) continue;
      g_sprintf(plugin_name,LIBDIR G_DIR_SEPARATOR_S PACKAGE "-songio"G_DIR_SEPARATOR_S"%s",dire->d_name);
      // skip symlinks
      if(readlink((const char *)plugin_name,link_target,FILENAME_MAX-1)!=-1) continue;
      // skip files other then shared librares
      if(!g_str_has_suffix(plugin_name,"."G_MODULE_SUFFIX)) continue;
      GST_INFO("    found file '%s'",plugin_name);
      

      // 2.) try to open each as g_module
      //if((plugin=g_module_open(plugin_name,G_MODULE_BIND_LAZY))!=NULL) {
      GModule * const plugin=g_module_open(plugin_name,G_MODULE_BIND_LOCAL);
      if(plugin!=NULL) {
        // 3.) gets the address of GType bt_song_io_detect(const gchar *);
        if(g_module_symbol(plugin,"bt_song_io_module_info",&bt_song_io_module_info)) {
          if(!g_list_find(plugins,bt_song_io_module_info)) {
            // 4.) store the g_module handle and the function pointer in a list (uhm, global (static) variable)
            plugins=g_list_append(plugins,bt_song_io_module_info);
          }
          else {
            GST_WARNING("%s skipped as duplicate",plugin_name);
            g_module_close(plugin);
          }
        }
        else {
          GST_WARNING("%s is not a songio plugin",plugin_name);
          g_module_close(plugin);
        }
      }
      else {
        GST_WARNING("%s is not a shared object",plugin_name); 
      }
    }
    closedir(dirp);
  }
}

/*
 * bt_song_io_detect:
 * @filename: the full filename of the song
 *
 * Factory method that returns the GType of the class that is able to handle
 * the supplied file
 *
 * Returns: the type of the #SongIO sub-class that can handle the supplied file
 * and %NULL otherwise
 */
static GType bt_song_io_detect(const gchar * const file_name) {
  GType type=0;
  const GList *node;
  BtSongIOModuleInfo *info;

  GST_INFO("detecting loader for file '%s'",file_name);

  if(!plugins) bt_song_io_register_plugins();

  // try all registered plugins
  for(node=plugins;node;node=g_list_next(node)) {
    info=(BtSongIOModuleInfo *)node->data;
    GST_INFO("  trying...");
    // the detect function return a GType if the file matches to the plugin or
    // NULL otheriwse
    if((type=info->detect(file_name))) {
      /* @idea: would be good if the detect method could also return some extra
       * data which the plugin can use for loading/saving (e.g. mime-type)
       * would it make sense to add the GType to BtSongIOFormatInfo
       */
       GST_INFO("  found one: %s!", info->formats[0].name);
      break;
    }
  }
  return(type);
}

/*
 * bt_song_io_update_filename:
 *
 * Grab the file-name from the path and store it in song-info.
 */
static void bt_song_io_update_filename(const BtSongIO * const self, const BtSong * const song) {
  BtSongInfo *song_info;
  gchar *file_path;

  g_object_get(G_OBJECT(self),"file-name",&file_path,NULL);
  if(!g_path_is_absolute(file_path)) {
    gchar *cur_path,*rel_path;
    
    cur_path=g_get_current_dir();
    rel_path=file_path;
    file_path=g_build_filename(cur_path,rel_path,NULL);
    g_free(cur_path);
    g_free(rel_path);
  }
  GST_INFO("file path is : %s",file_path);
  g_object_get(G_OBJECT(song),"song-info",&song_info,NULL);
  g_object_set(song_info,"file-name",file_path,NULL);
  g_free(file_path);
  g_object_unref(song_info);
}

//-- constructor methods

/**
 * bt_song_io_new:
 * @file_name: the file name of the new song
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtSongIO *bt_song_io_new(const gchar * const file_name) {
  BtSongIO *self=NULL;
  GType type = 0;

  if(!BT_IS_STRING(file_name)) {
    GST_WARNING("filename should not be empty");
    return NULL;
  }
  type=bt_song_io_detect(file_name);
  if(type) {
    self=BT_SONG_IO(g_object_new(type,NULL));
    if(self) {
      self->priv->file_name=(gchar *)file_name;
    }
  }
  else {
    GST_WARNING("failed to detect type for filename %s",file_name);
  }
  return(self);
}


//-- methods

/**
 * bt_song_io_get_module_info_list:
 *
 * Get read only access to list of #BtSongIOModuleInfo entries.
 *
 * Returns: the #GList.
 */
const GList *bt_song_io_get_module_info_list(void) {

  if(!plugins) bt_song_io_register_plugins();

  return (plugins);
}

//-- virtual methods

static gboolean bt_song_io_default_load(gconstpointer const self, const BtSong * const song) {
  GST_ERROR("virtual method bt_song_io_load(self=%p,song=%p) called",self,song);
  return(FALSE);  // this is a base class that can't load anything
}

static gboolean bt_song_io_default_save(gconstpointer const self, const BtSong * const song) {
  GST_ERROR("virtual method bt_song_io_save(self=%p,song=%p) called",self,song);
  return(FALSE);  // this is a base class that can't save anything
}

//-- wrapper

/**
 * bt_song_io_load:
 * @self: the #SongIO instance to use
 * @song: the #Song instance that should initialized
 *
 * load the song from a file.  The file is set in the constructor
 *
 * Returns: %TRUE for success
 */
gboolean bt_song_io_load(gconstpointer const self, const BtSong * const song) {
  gboolean result;

  g_assert(BT_IS_SONG_IO(self));

  bt_song_idle_stop(self);
  g_object_set(G_OBJECT(song),"song-io",self,NULL);
  if((result=BT_SONG_IO_GET_CLASS(self)->load(self,song))) {
    bt_song_io_update_filename(BT_SONG_IO(self),song);
    GST_INFO("loading done");
    bt_song_set_unsaved(song,FALSE);
    //DEBUG
    //bt_song_write_to_highlevel_dot_file(song);
    //bt_song_write_to_xml_file(song);
    /*
    {
      BtSetup * const setup;
      GList * const list;
      const GList *node;

      g_object_get(G_OBJECT(song),"setup",&setup,NULL);
      g_object_get(G_OBJECT(setup),"machines",&list,NULL);
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
  g_object_set(G_OBJECT(song),"song-io",NULL,NULL);
  bt_song_idle_start(self);
  return(result);
}

/**
 * bt_song_io_save:
 * @self: the #SongIO instance to use
 * @song: the #Song instance that should stored
 *
 * save the song to a file.  The file is set in the constructor
 *
 * Returns: %TRUE for success
 */
gboolean bt_song_io_save(gconstpointer const self, const BtSong * const song) {
  gboolean result;
  BtSongInfo * const song_info;

  g_assert(BT_IS_SONG_IO(self));

  // this updates the time-stamp
  g_object_get(G_OBJECT(song),"song-info",&song_info,NULL);
  g_object_set(G_OBJECT(song_info),"change-dts",NULL,NULL);
  g_object_unref(song_info);

  bt_song_idle_stop(self);
  g_object_set(G_OBJECT(song),"song-io",self,NULL);
  if((result=BT_SONG_IO_GET_CLASS(self)->save(self,song))) {
    bt_song_io_update_filename(BT_SONG_IO(self),song);
    bt_song_set_unsaved(song,FALSE);
  }
  g_object_set(G_OBJECT(song),"song-io",NULL,NULL);
  bt_song_idle_start(self);
  return(result);
}

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_song_io_get_property(GObject      * const object,
                               const guint         property_id,
                               GValue       * const value,
                               GParamSpec   * const pspec)
{
  const BtSongIO * const self = BT_SONG_IO(object);
  return_if_disposed();
  switch (property_id) {
    case SONG_IO_FILE_NAME: {
      g_value_set_string(value, self->priv->file_name);
    } break;
    case SONG_IO_STATUS: {
      g_value_set_string(value, self->priv->status);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_song_io_set_property(GObject      * const object,
                              const guint         property_id,
                              const GValue * const value,
                              GParamSpec   * const pspec)
{
  const BtSongIO * const self = BT_SONG_IO(object);
  return_if_disposed();
  switch (property_id) {
    case SONG_IO_STATUS: {
      g_free(self->priv->status);
      self->priv->status = g_value_dup_string(value);
      GST_DEBUG("set the status for song_io: %s",self->priv->status);
    } break;
     default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_song_io_dispose(GObject * const object) {
  const BtSongIO * const self = BT_SONG_IO(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);

  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_song_io_finalize(GObject * const object) {
  const BtSongIO * const self = BT_SONG_IO(object);

  GST_DEBUG("!!!! self=%p",self);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_song_io_init(GTypeInstance * const instance, gconstpointer g_class) {
  BtSongIO * const self = BT_SONG_IO(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_SONG_IO, BtSongIOPrivate);
}

static void bt_song_io_class_init(BtSongIOClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtSongIOPrivate));

  gobject_class->set_property = bt_song_io_set_property;
  gobject_class->get_property = bt_song_io_get_property;
  gobject_class->dispose      = bt_song_io_dispose;
  gobject_class->finalize     = bt_song_io_finalize;

  klass->load = bt_song_io_default_load;
  klass->save = bt_song_io_default_save;

  g_object_class_install_property(gobject_class,SONG_IO_FILE_NAME,
                                  g_param_spec_string("file-name",
                                     "filename prop",
                                     "full filename for load save operations",
                                     NULL, /* default value */
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SONG_IO_STATUS,
                                  g_param_spec_string("status",
                                     "status prop",
                                     "status of load save operations",
                                     NULL, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));
}

GType bt_song_io_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof(BtSongIOClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_song_io_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtSongIO),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_song_io_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(G_TYPE_OBJECT,"BtSongIO",&info,0);
  }
  return type;
}
