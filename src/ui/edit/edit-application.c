/* $Id: edit-application.c,v 1.41 2004-12-16 13:41:32 ensonic Exp $
 * class for a gtk based buzztard editor application
 */
 
#define BT_EDIT
#define BT_EDIT_APPLICATION_C

#include "bt-edit.h"
#include <libbtcore/application-private.h>

//-- signal ids

//-- property ids

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

static BtApplicationClass *parent_class=NULL;

//-- event handler

static gboolean on_songio_status_changed(BtSongIO *songio,GParamSpec *arg,gpointer user_data) {
  BtEditApplication *self=BT_EDIT_APPLICATION(user_data);
  BtMainStatusbar *statusbar;
  gchar *str;

  g_assert(BT_IS_SONG_IO(songio));
  g_assert(user_data);

  g_object_get(self->priv->main_window,"statusbar",&statusbar,NULL);
  
  /* @todo push loader status changes into the statusbar
   * - how to handle to push and pop stuff, first_acces=push_only, last_access=pop_only
   *   - str!=NULL old.pop & new.push
   *   - str==NULL old.pop & default.push
   */
  g_object_get(songio,"status",&str,NULL);
  GST_INFO("songio_status has changed : \"%s\"",safe_string(str));
  g_object_set(statusbar,"status",str,NULL);
	g_object_try_unref(statusbar);
  g_free(str);
}

//-- helper methods

static gboolean bt_edit_application_run_ui(const BtEditApplication *self) {
	gboolean res;
	
  g_assert(self);
  g_assert(self->priv->main_window);
  
	res=bt_main_window_run(self->priv->main_window);	
  return(res);
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
  
	if(!(self=BT_EDIT_APPLICATION(g_object_new(BT_TYPE_EDIT_APPLICATION,NULL)))) {
		goto Error;
	}
	if(!bt_application_new(BT_APPLICATION(self))) {
		goto Error;
	}
  GST_INFO("new edit app created");
  if(!(self->priv->main_window=bt_main_window_new(self))) {
		goto Error;
	}
	GST_INFO("new edit app window created");
  return(self);
Error:
	g_object_try_unref(self);
	return(NULL);
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
  BtSong *song;
	
  g_assert(BT_IS_EDIT_APPLICATION(self));

	song=bt_song_new(BT_APPLICATION(self));
	g_object_set(G_OBJECT(self),"song",song,NULL);
	res=TRUE;

	g_object_unref(song);
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
  BtSongIO *loader;
	BtSong *song;

  g_assert(BT_IS_EDIT_APPLICATION(self));

  GST_INFO("song name = %s",file_name);

  if((loader=bt_song_io_new(file_name))) {
    GdkCursor *cursor=gdk_cursor_new(GDK_WATCH);
    GdkWindow *window=GTK_WIDGET(self->priv->main_window)->window;
      
    cursor=gdk_cursor_new(GDK_WATCH);
    gdk_window_set_cursor(window,cursor);
    gdk_cursor_unref(cursor);
    gtk_widget_set_sensitive(GTK_WIDGET(self->priv->main_window),FALSE);
      
    g_signal_connect(G_OBJECT(loader),"notify::status",(GCallback)on_songio_status_changed,(gpointer)self);
    while(gtk_events_pending()) gtk_main_iteration();
		song=bt_song_new(BT_APPLICATION(self));
    if(bt_song_io_load(loader,song)) {
			g_object_set(G_OBJECT(self),"song",song,NULL);
      res=TRUE;
    }
    else {
      GST_ERROR("could not load song \"%s\"",file_name);
    }
      
    gtk_widget_set_sensitive(GTK_WIDGET(self->priv->main_window),TRUE);
    gdk_window_set_cursor(window,NULL);
		g_object_unref(song);
    g_object_unref(loader);
  }
  return(res);
}

/**
 * bt_edit_application_save_song:
 * @self: the application instance to save a song from
  *@file_name: the song filename to save
 *
 * Saves a song.
 *
 * Returns: true for success
 */
gboolean bt_edit_application_save_song(const BtEditApplication *self,const char *file_name) {
  gboolean res=FALSE;
  BtSongIO *saver;

  g_assert(BT_IS_EDIT_APPLICATION(self));

  GST_INFO("song name = %s",file_name);


  if((saver=bt_song_io_new(file_name))) {
    GdkCursor *cursor=gdk_cursor_new(GDK_WATCH);
    GdkWindow *window=GTK_WIDGET(self->priv->main_window)->window;
      
    cursor=gdk_cursor_new(GDK_WATCH);
    gdk_window_set_cursor(window,cursor);
    gdk_cursor_unref(cursor);
    gtk_widget_set_sensitive(GTK_WIDGET(self->priv->main_window),FALSE);
      
    g_signal_connect(G_OBJECT(saver),"notify::status",(GCallback)on_songio_status_changed,(gpointer)self);
    while(gtk_events_pending()) gtk_main_iteration();
    if(bt_song_io_save(saver,self->priv->song)) {
      res=TRUE;
    }
    else {
      GST_ERROR("could not save song \"%s\"",file_name);
    }
    GST_INFO("saving done");

    gtk_widget_set_sensitive(GTK_WIDGET(self->priv->main_window),TRUE);
    gdk_window_set_cursor(window,NULL);
    g_object_unref(saver);
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

  g_assert(BT_IS_EDIT_APPLICATION(self));

	GST_INFO("application.run launched");

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

  g_assert(BT_IS_EDIT_APPLICATION(self));

	GST_INFO("application.load_and_run launched");

  if(bt_edit_application_load_song(self,input_file_name)) {
    res=bt_edit_application_run_ui(self);
  }
	return(res);
}

//-- wrapper

//-- default signal handler

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
      g_value_set_object(value, self->priv->song);
    } break;
    case EDIT_APPLICATION_MAIN_WINDOW: {
      g_value_set_object(value, self->priv->main_window);
    } break;
    default: {
 			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
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
		  // DEBUG
  		if(self->priv->song) GST_INFO("song->ref_ct=%d",G_OBJECT(self->priv->song)->ref_count);
  		// DEBUG
      g_object_try_unref(self->priv->song);
      self->priv->song=g_object_try_ref(g_value_get_object(value));
      GST_DEBUG("set the song for edit_application: %p",self->priv->song);
    } break;
    default: {
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_edit_application_dispose(GObject *object) {
  BtEditApplication *self = BT_EDIT_APPLICATION(object);

	return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);

  if(self->priv->song) {
    GST_INFO("1. song->ref_ct=%d",G_OBJECT(self->priv->song)->ref_count);
    bt_song_stop(self->priv->song);
  }
  g_object_try_unref(self->priv->song);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_edit_application_finalize(GObject *object) {
  BtEditApplication *self = BT_EDIT_APPLICATION(object);
  
  GST_DEBUG("!!!! self=%p",self);

  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_edit_application_init(GTypeInstance *instance, gpointer g_class) {
  BtEditApplication *self = BT_EDIT_APPLICATION(instance);
  self->priv = g_new0(BtEditApplicationPrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_edit_application_class_init(BtEditApplicationClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(BT_TYPE_APPLICATION);

  gobject_class->set_property = bt_edit_application_set_property;
  gobject_class->get_property = bt_edit_application_get_property;
  gobject_class->dispose      = bt_edit_application_dispose;
  gobject_class->finalize     = bt_edit_application_finalize;

  klass->song_changed = NULL;

  g_object_class_install_property(gobject_class,EDIT_APPLICATION_SONG,
																	g_param_spec_object("song",
                                     "song construct prop",
                                     "the song object, the wire belongs to",
                                     BT_TYPE_SONG, /* object type */
                                     G_PARAM_READWRITE));

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
			NULL // value_table
    };
		type = g_type_register_static(BT_TYPE_APPLICATION,"BtEditApplication",&info,0);
  }
  return type;
}
