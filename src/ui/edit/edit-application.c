/* $Id: edit-application.c,v 1.13 2004-08-24 17:07:51 ensonic Exp $
 * class for a gtk based buzztard editor application
 */
 
#define BT_EDIT
#define BT_EDIT_APPLICATION_C

#include "bt-edit.h"

enum {
  EDIT_APPLICATION_SONG=1,
  EDIT_APPLICATION_MAIN_WINDOW
};

// this needs to be here because of gtk-doc and unit-tests
GST_DEBUG_CATEGORY(GST_CAT_DEFAULT);

struct _BtEditApplicationPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the currently loaded song */
  BtSong *song;
  /* the top-level window of our app */
  BtMainWindow *main_window;
};

//-- helper methods

gboolean bt_edit_application_prepare_song(const BtEditApplication *self) {
  gboolean res=FALSE;
  
  if(self->private->song) {
    g_object_unref(G_OBJECT(self->private->song));
  }
  if((self->private->song=bt_song_new(GST_BIN(bt_g_object_get_object_property(G_OBJECT(self),"bin"))))) {
    res=TRUE;
  }
  return(res);
}

static gboolean bt_edit_application_run_ui(const BtEditApplication *self) {
  g_assert(self->private->main_window);
  
  return(bt_main_window_run(self->private->main_window));
}

//-- constructor methods

/**
 * bt_edit_application_new:
 *
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtEditApplication *bt_edit_application_new(void) {
  BtEditApplication *self;
  self=BT_EDIT_APPLICATION(g_object_new(BT_TYPE_EDIT_APPLICATION,NULL));
  
  self->private->main_window=bt_main_window_new(self);
  return(self);
}

//-- methods

/**
 * bt_edit_application_new_song:
 * @self: the application instance to create a new song in
 *
 * Creates a new blank song instance. If there is a previous song instance it
 * will be freed.
 *
 * Returns: true for success
 */
gboolean bt_edit_application_new_song(const BtEditApplication *self) {
  gboolean res=FALSE;
  
  if(bt_edit_application_prepare_song(self)) {
    // emit signal that song has been changed
    g_signal_emit(G_OBJECT(self),BT_EDIT_APPLICATION_GET_CLASS(self)->song_changed_signal_id,0);
    res=TRUE;
  }
  return(res);
}

/**
 * bt_edit_application_load_song:
 * @self: the application instance to load a new song in
  *@file_name: the song filename to load
 *
 * Loads a new song. If there is a previous song instance it will be freed.
 *
 * Returns: true for success
 */
gboolean bt_edit_application_load_song(const BtEditApplication *self,const char *file_name) {
  gboolean res=FALSE;

  GST_INFO("new song name = %s\n",file_name);

  if(bt_edit_application_prepare_song(self)) {
    BtSongIO *loader=bt_song_io_new(file_name);

    if(loader) {
      // @todo does not work -> (gdk_window_set_cursor): assertion `window != NULL' failed
      //GdkWindow *window=gtk_window_get_transient_for(GTK_WINDOW(self->private->main_window));
      //gdk_window_set_cursor(window,gdk_cursor_new(GDK_WATCH));
      if(bt_song_io_load(loader,self->private->song)) {
        // emit signal that song has been changed
        g_signal_emit(G_OBJECT(self),BT_EDIT_APPLICATION_GET_CLASS(self)->song_changed_signal_id,0);
         res=TRUE;
      }
      else {
        GST_ERROR("could not load song \"%s\"",file_name);
      }
      //gdk_window_set_cursor(window,NULL);
      g_object_unref(loader);
    }
  }
  return(res);
}


/**
 * bt_edit_application_run:
 * @self: the application instance to run
 *
 * start the gtk based editor application
 *
 * Returns: true for success
 */
gboolean bt_edit_application_run(const BtEditApplication *self) {
	gboolean res=FALSE;

	GST_INFO("application.play launched");

  if(bt_edit_application_new_song(self)) {
    res=bt_edit_application_run_ui(self);
  }
	return(res);
}

/**
 * bt_edit_application_load_and_run:
 * @self: the application instance to run
 * @input_file_name: the file to load initially
 *
 * load the file of the supplied name and start the gtk based editor application
 *
 * Returns: true for success
 */
gboolean bt_edit_application_load_and_run(const BtEditApplication *self, const gchar *input_file_name) {
	gboolean res=FALSE;

	GST_INFO("application.info launched");

  if(bt_edit_application_load_song(self,input_file_name)) {
    res=bt_edit_application_run_ui(self);
  }
	return(res);
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_edit_application_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtEditApplication *self = BT_EDIT_APPLICATION(object);
  return_if_disposed();
  switch (property_id) {
    case EDIT_APPLICATION_SONG: {
      g_value_set_object(value, G_OBJECT(self->private->song));
    } break;
    case EDIT_APPLICATION_MAIN_WINDOW: {
      g_value_set_object(value, G_OBJECT(self->private->main_window));
    } break;
    default: {
 			g_assert(FALSE);
      break;
    }
  }
}

/* sets the given properties for this object */
static void bt_edit_application_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtEditApplication *self = BT_EDIT_APPLICATION(object);
  return_if_disposed();
  switch (property_id) {
    case EDIT_APPLICATION_SONG: {
      self->private->song = g_object_ref(G_OBJECT(g_value_get_object(value)));
      //GST_DEBUG("set the song for edit_application: %p",self->private->song);
    } break;
    default: {
			g_assert(FALSE);
      break;
    }
  }
}

static void bt_edit_application_dispose(GObject *object) {
  BtEditApplication *self = BT_EDIT_APPLICATION(object);
	return_if_disposed();
  self->private->dispose_has_run = TRUE;
}

static void bt_edit_application_finalize(GObject *object) {
  BtEditApplication *self = BT_EDIT_APPLICATION(object);
  
  bt_song_stop(self->private->song);
  g_object_unref(G_OBJECT(self->private->song));
  g_free(self->private);
}

static void bt_edit_application_init(GTypeInstance *instance, gpointer g_class) {
  BtEditApplication *self = BT_EDIT_APPLICATION(instance);
  self->private = g_new0(BtEditApplicationPrivate,1);
  self->private->dispose_has_run = FALSE;
}

static void bt_edit_application_class_init(BtEditApplicationClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;
  
  gobject_class->set_property = bt_edit_application_set_property;
  gobject_class->get_property = bt_edit_application_get_property;
  gobject_class->dispose      = bt_edit_application_dispose;
  gobject_class->finalize     = bt_edit_application_finalize;

  /** 
	 * BtEditApplication::song-changed:
   * @self: the application object that emitted the signal
	 *
	 * the song of the application has changed.
   * This happens after a load or new action
	 */
  klass->song_changed_signal_id = g_signal_newv("song-changed",
                                        G_TYPE_FROM_CLASS(klass),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                        NULL, // class closure
                                        NULL, // accumulator
                                        NULL, // acc data
                                        g_cclosure_marshal_VOID__VOID,
                                        G_TYPE_NONE, // return type
                                        0, // n_params
                                        NULL /* param data */ );

  g_object_class_install_property(gobject_class,EDIT_APPLICATION_SONG,
																	g_param_spec_object("song",
                                     "song construct prop",
                                     "the song object, the wire belongs to",
                                     BT_TYPE_SONG, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,EDIT_APPLICATION_MAIN_WINDOW,
																	g_param_spec_object("main-window",
                                     "main window prop",
                                     "the main window of this application",
                                     BT_TYPE_MAIN_WINDOW, /* object type */
                                     G_PARAM_READABLE));
}

GType bt_edit_application_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtEditApplicationClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_edit_application_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtEditApplication),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_edit_application_init, // instance_init
    };
		type = g_type_register_static(BT_TYPE_APPLICATION,"BtEditApplication",&info,0);
  }
  return type;
}

