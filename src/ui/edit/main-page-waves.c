/* $Id: main-page-waves.c,v 1.1 2004-12-02 10:45:33 ensonic Exp $
 * class for the editor main waves page
 */

#define BT_EDIT
#define BT_MAIN_PAGE_WAVES_C

#include "bt-edit.h"

enum {
  MAIN_PAGE_WAVES_APP=1,
};


struct _BtMainPageWavesPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the application */
  BtEditApplication *app;  
};

static GtkVBoxClass *parent_class=NULL;

//-- event handler helper

//-- event handler

static void on_song_changed(const BtEditApplication *app, gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);
  BtSong *song;
  BtSetup *setup;
  BtMachine *machine;

  g_assert(user_data);

  GST_INFO("song has changed : app=%p, self=%p",app,self);
  // get song from app and then setup from song
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  // update page
  // release the reference
  g_object_try_unref(song);
}

//-- helper methods

static gboolean bt_main_page_waves_init_ui(const BtMainPageWaves *self, const BtEditApplication *app) {
  //GtkWidget *toolbar;
  //GtkWidget *box,*menu,*button;
 
  // @todo add ui
	// vpane
	//   hpane
	//     vbox (loaded sample list)
	//       toolbar
	//       listview
	//     vbox (file browser)
	//       toolbar
	//       listview
	//   vbox (sample view)
	//     toolbar
	//     hbox
	//       properties (root key, sample rate, ...)
	//       sampleview
	//
  gtk_container_add(GTK_CONTAINER(self),gtk_label_new("no waves view yet"));

  // register event handlers
  g_signal_connect(G_OBJECT(app), "song-changed", (GCallback)on_song_changed, (gpointer)self);
  return(TRUE);
}

//-- constructor methods

/**
 * bt_main_page_waves_new:
 * @app: the application the window belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtMainPageWaves *bt_main_page_waves_new(const BtEditApplication *app) {
  BtMainPageWaves *self;

  if(!(self=BT_MAIN_PAGE_WAVES(g_object_new(BT_TYPE_MAIN_PAGE_WAVES,"app",app,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_main_page_waves_init_ui(self,app)) {
    goto Error;
  }
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_main_page_waves_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_PAGE_WAVES_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    default: {
 			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_main_page_waves_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_PAGE_WAVES_APP: {
      g_object_try_unref(self->priv->app);
      self->priv->app = g_object_try_ref(g_value_get_object(value));
      //GST_DEBUG("set the app for MAIN_PAGE_WAVES: %p",self->priv->app);
    } break;
    default: {
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_main_page_waves_dispose(GObject *object) {
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES(object);
	return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  g_object_try_unref(self->priv->app);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_main_page_waves_finalize(GObject *object) {
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES(object);
  
  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_main_page_waves_init(GTypeInstance *instance, gpointer g_class) {
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES(instance);
  self->priv = g_new0(BtMainPageWavesPrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_main_page_waves_class_init(BtMainPageWavesClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(GTK_TYPE_VBOX);
    
  gobject_class->set_property = bt_main_page_waves_set_property;
  gobject_class->get_property = bt_main_page_waves_get_property;
  gobject_class->dispose      = bt_main_page_waves_dispose;
  gobject_class->finalize     = bt_main_page_waves_finalize;

  g_object_class_install_property(gobject_class,MAIN_PAGE_WAVES_APP,
                                  g_param_spec_object("app",
                                     "app contruct prop",
                                     "Set application object, the window belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));
}

GType bt_main_page_waves_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtMainPageWavesClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_main_page_waves_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtMainPageWaves),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_main_page_waves_init, // instance_init
			NULL // value_table
    };
		type = g_type_register_static(GTK_TYPE_VBOX,"BtMainPageWaves",&info,0);
  }
  return type;
}
