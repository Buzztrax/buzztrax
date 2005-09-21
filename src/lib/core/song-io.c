// $Id: song-io.c,v 1.47 2005-09-21 19:46:03 ensonic Exp $
/**
 * SECTION:btsongio
 * @short_description: base class for song input and output
 *
 * A base class for #BtSong loader and saver implementations. A #SongIO module
 * needs to be installed as a shared library into LIBDIR/songio. It is
 * recognized, if it exports method named bt_song_io_detect(). At runtime the
 * detect method of each module is called with the choosen file-name. The module
 * should return its type if it can handle the format or %NULL else.
 *
 * Such a module should overwrite bt_song_io_load() and/or bt_song_io_save().
 *
 * There is an internal subclass of this called #BtSongIONative.
 */ 
 
#define BT_CORE
#define BT_SONG_IO_C

#include <libbtcore/core.h>

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

/* list of registered io-classes */
static GList *plugins=NULL;


//-- helper methods

/**
 * bt_song_io_register_plugins:
 *
 * Registers all song-io plugins for later use by bt_song_io_detect().
 */
static void bt_song_io_register_plugins(void) {
  DIR *dirp=opendir(LIBDIR"/songio");
  
  /* @todo shoudn't the contents of the plugin list be structures 
   * (so that apart from the detect ptr, we could keep the modules handle. we need this to close the plugins at sometime ... )
   */
  GST_INFO("register song-io plugins ...");
  // register internal song-io plugin
  plugins=g_list_append(plugins,(gpointer)bt_song_io_native_detect);
  // registering external song-io plugins
  GST_INFO("  scanning external song-io plugins in "LIBDIR"/songio/ ...");
  if(dirp) {
    struct dirent *dire;
    GModule *plugin;
    gpointer bt_song_io_plugin_detect=NULL;
    gchar link_target[FILENAME_MAX],plugin_name[FILENAME_MAX];
    
    //   1.) scan plugin-folder (LIBDIR/songio)
    while((dire=readdir(dirp))!=NULL) {
      // skip names starting with a dot
      if((!dire->d_name) || (*dire->d_name=='.')) continue;
      g_sprintf(plugin_name,LIBDIR"/songio/%s",dire->d_name);
      // skip symlinks
      if(readlink((const char *)plugin_name,link_target,FILENAME_MAX-1)!=-1) continue;
      GST_INFO("    found file \"%s\"",dire->d_name);
      //   2.) try to open each as g_module
       if((plugin=g_module_open(plugin_name,G_MODULE_BIND_LAZY))!=NULL) {
        GST_INFO("    that is a shared object");
        //   3.) gets the address of GType bt_song_io_detect(const gchar *);
        if(g_module_symbol(plugin,"bt_song_io_detect",&bt_song_io_plugin_detect)) {
          GST_INFO("    and implements a songio subclass");
          //   4.) store the g_module handle and the function pointer in a list (uhm, global (static) variable)
          plugins=g_list_append(plugins,bt_song_io_plugin_detect);
        }
        else g_module_close(plugin);
      }
    }
    closedir(dirp);
  }
}

/**
 * bt_song_io_detect:
 * @filename: the full filename of the song
 * 
 * factory method that returns the GType of the class that is able to handle
 * the supplied file
 *
 * Returns: the type of the #SongIO sub-class that can handle the supplied file
 */
static GType bt_song_io_detect(const gchar *file_name) {
  GType type=0;
  GList *node;
  BtSongIODetect detect;
  
  GST_INFO("detecting loader for file \"%s\"",file_name);
  
  if(!plugins) bt_song_io_register_plugins();
  
  // try all registered plugins
  for(node=plugins;node;node=g_list_next(node)) {
    detect=(BtSongIODetect)node->data;
    GST_INFO("  trying ...");
    // the detect function return a GType if the file matches to the plugin or NULL otheriwse
    if((type=detect(file_name))) {
      GST_INFO("  found one!");
      break;
    }
  }
  //return(BT_TYPE_SONG_IO_NATIVE);
  return(type);
}

static void bt_song_io_update_filename(const BtSongIO *self, const BtSong *song) {
  BtSongInfo *song_info;
  gchar *file_path,*file_name;

  g_object_get(G_OBJECT(self),"file-name",&file_path,NULL);
  // file_name is last part from file_path
  file_name=g_strrstr(file_path,G_DIR_SEPARATOR_S);
  if(file_name) {
    file_name++;
  }
  else {
    file_name=file_path;
  }
  //GST_INFO("file name is : %s",file_name);
  g_object_get(G_OBJECT(song),"song-info",&song_info,NULL);
  g_object_set(song_info,"file-name",file_name,NULL);
  g_free(file_path);
  g_object_try_unref(song_info);
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
BtSongIO *bt_song_io_new(const gchar *file_name) {
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
  return(self);
}


//-- methods

static gboolean bt_song_io_real_load(const gpointer _self, const BtSong *song) {
  GST_ERROR("virtual method bt_song_io_real_load(self,song) called");
  return(FALSE);  // this is a base class that can't load anything
}

static gboolean bt_song_io_real_save(const gpointer _self, const BtSong *song) {
  GST_ERROR("virtual method bt_song_io_real_save(self,song) called");
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
gboolean bt_song_io_load(const gpointer self, const BtSong *song) {
  gboolean result;
  
  if((result=BT_SONG_IO_GET_CLASS(self)->load(self,song))) {
    bt_song_io_update_filename(BT_SONG_IO(self),song);
    g_object_set(G_OBJECT(song),"unsaved",FALSE,NULL);
    //DEBUG
    /*
		bt_song_write_to_xml_file(song);
    {
      BtSetup *setup;
      BtMachine *machine;
      GList *list,*node;
      
      g_object_get(G_OBJECT(song),"setup",&setup,NULL);
      g_object_get(G_OBJECT(setup),"machines",&list,NULL);
      for(node=list;node;node=g_list_next(node)) {
        machine=BT_MACHINE(node->data);
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
gboolean bt_song_io_save(const gpointer self, const BtSong *song) {
  gboolean result;
	BtSongInfo *song_info;
	
	// this updates the time-stamp
	g_object_get(G_OBJECT(song),"song-info",&song_info,NULL);
	g_object_set(G_OBJECT(song_info),"change-dts",NULL,NULL);
	g_object_try_unref(song_info);

  if((result=BT_SONG_IO_GET_CLASS(self)->save(self,song))) {
    bt_song_io_update_filename(BT_SONG_IO(self),song);
    g_object_set(G_OBJECT(song),"unsaved",FALSE,NULL);
  }
  return(result);
}

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_song_io_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtSongIO *self = BT_SONG_IO(object);
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
static void bt_song_io_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtSongIO *self = BT_SONG_IO(object);
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

static void bt_song_io_dispose(GObject *object) {
  BtSongIO *self = BT_SONG_IO(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_song_io_finalize(GObject *object) {
  BtSongIO *self = BT_SONG_IO(object);

  GST_DEBUG("!!!! self=%p",self);

  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_song_io_init(GTypeInstance *instance, gpointer g_class) {
  BtSongIO *self = BT_SONG_IO(instance);
  self->priv = g_new0(BtSongIOPrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_song_io_class_init(BtSongIOClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(G_TYPE_OBJECT);
  
  gobject_class->set_property = bt_song_io_set_property;
  gobject_class->get_property = bt_song_io_get_property;
  gobject_class->dispose      = bt_song_io_dispose;
  gobject_class->finalize     = bt_song_io_finalize;
  
  klass->load           = bt_song_io_real_load;
  klass->save           = bt_song_io_real_save;
  
  g_object_class_install_property(gobject_class,SONG_IO_FILE_NAME,
                                  g_param_spec_string("file-name",
                                     "filename prop",
                                     "filename for load save operations",
                                     NULL, /* default value */
                                     G_PARAM_READABLE));

  g_object_class_install_property(gobject_class,SONG_IO_STATUS,
                                  g_param_spec_string("status",
                                     "status prop",
                                     "status of load save operations",
                                     NULL, /* default value */
                                     G_PARAM_READWRITE));
}

GType bt_song_io_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtSongIOClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_song_io_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtSongIO),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_song_io_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(G_TYPE_OBJECT,"BtSongIO",&info,0);
  }
  return type;
}
