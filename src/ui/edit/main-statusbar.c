/* $Id: main-statusbar.c,v 1.7 2004-08-24 14:10:04 ensonic Exp $
 * class for the editor main tollbar
 */

#define BT_EDIT
#define BT_MAIN_STATUSBAR_C

#include "bt-edit.h"

enum {
  MAIN_STATUSBAR_APP=1,
};


struct _BtMainStatusbarPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;
  
  /* main status bar */
  GtkStatusbar *status;
  /* identifier of the status message group */
  gint status_context_id;

  /* time-elapsed status bar */
  GtkStatusbar *elapsed;
  /* identifier of the elapsed message group */
  gint elapsed_context_id;

  /* time-current status bar */
  GtkStatusbar *current;
  /* identifier of the current message group */
  gint current_context_id;

  /* time-loop status bar */
  GtkStatusbar *loop;
  /* identifier of the loop message group */
  gint loop_context_id;
};

//-- event handler

static void on_song_changed(const BtEditApplication *app, gpointer user_data) {
  BtMainStatusbar *self=BT_MAIN_STATUSBAR(user_data);
  BtSong *song;
  gchar *str;
  gulong msec,sec,min;

  GST_INFO("song has changed : app=%p, window=%p",song,user_data);
  // get song from app
  song=BT_SONG(bt_g_object_get_object_property(G_OBJECT(self->private->app),"song"));
  // get new song length
  msec=bt_sequence_get_loop_time(bt_song_get_sequence(song));
  GST_INFO("  new msec : %ld",msec);
  min=(gulong)(msec/60000);msec-=(min*60000);
  sec=(gulong)(msec/ 1000);msec-=(sec* 1000);
	str=g_strdup_printf("%02d:%02d.%03d",min,sec,msec);
  // update statusbar fields
  gtk_statusbar_pop(self->private->loop,self->private->loop_context_id); 
	gtk_statusbar_push(self->private->loop,self->private->loop_context_id,str);
 	g_free(str);
}

//-- helper methods

static gboolean bt_main_statusbar_init_ui(const BtMainStatusbar *self, const BtEditApplication *app) {
  gchar *msg="00:00.000";
  
  gtk_widget_set_name(GTK_WIDGET(self),_("status bar"));
  //gtk_box_set_spacing(GTK_BOX(self),1);

  self->private->status=GTK_STATUSBAR(gtk_statusbar_new());
  self->private->status_context_id=gtk_statusbar_get_context_id(GTK_STATUSBAR(self->private->status),_("default"));
  gtk_statusbar_set_has_resize_grip(self->private->status,FALSE);
  gtk_statusbar_push(GTK_STATUSBAR(self->private->status),self->private->status_context_id,_("Ready to rock!"));
  gtk_box_pack_start(GTK_BOX(self),GTK_WIDGET(self->private->status),TRUE,TRUE,1);

  self->private->elapsed=GTK_STATUSBAR(gtk_statusbar_new());
  self->private->elapsed_context_id=gtk_statusbar_get_context_id(GTK_STATUSBAR(self->private->elapsed),_("default"));
  gtk_statusbar_set_has_resize_grip(self->private->elapsed,FALSE);
  gtk_widget_set_size_request(GTK_WIDGET(self->private->elapsed),75,-1);
  gtk_statusbar_push(GTK_STATUSBAR(self->private->elapsed),self->private->elapsed_context_id,msg);
  gtk_box_pack_start(GTK_BOX(self),GTK_WIDGET(self->private->elapsed),FALSE,FALSE,1);

  self->private->current=GTK_STATUSBAR(gtk_statusbar_new());
  self->private->current_context_id=gtk_statusbar_get_context_id(GTK_STATUSBAR(self->private->current),_("default"));
  gtk_statusbar_set_has_resize_grip(self->private->current,FALSE);
  gtk_widget_set_size_request(GTK_WIDGET(self->private->current),75,-1);
  gtk_statusbar_push(GTK_STATUSBAR(self->private->current),self->private->current_context_id,msg);
  gtk_box_pack_start(GTK_BOX(self),GTK_WIDGET(self->private->current),FALSE,FALSE,1);

  self->private->loop=GTK_STATUSBAR(gtk_statusbar_new());
  self->private->loop_context_id=gtk_statusbar_get_context_id(GTK_STATUSBAR(self->private->loop),_("default"));
  gtk_widget_set_size_request(GTK_WIDGET(self->private->loop),75,-1);
  gtk_statusbar_push(GTK_STATUSBAR(self->private->loop),self->private->loop_context_id,msg);
  gtk_box_pack_start(GTK_BOX(self),GTK_WIDGET(self->private->loop),FALSE,FALSE,1);

  // register event handlers
  g_signal_connect(G_OBJECT(app), "song-changed", (GCallback)on_song_changed, (gpointer)self);
  return(TRUE);
}

//-- constructor methods

/**
 * bt_main_statusbar_new:
 * @app: the application the statusbar belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtMainStatusbar *bt_main_statusbar_new(const BtEditApplication *app) {
  BtMainStatusbar *self;

  if(!(self=BT_MAIN_STATUSBAR(g_object_new(BT_TYPE_MAIN_STATUSBAR,"app",app,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_main_statusbar_init_ui(self,app)) {
    goto Error;
  }
  return(self);
Error:
  if(self) g_object_unref(self);
  return(NULL);
}

//-- methods


//-- class internals

/* returns a property for the given property_id for this object */
static void bt_main_statusbar_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtMainStatusbar *self = BT_MAIN_STATUSBAR(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_STATUSBAR_APP: {
      g_value_set_object(value, G_OBJECT(self->private->app));
    } break;
    default: {
 			g_assert(FALSE);
      break;
    }
  }
}

/* sets the given properties for this object */
static void bt_main_statusbar_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtMainStatusbar *self = BT_MAIN_STATUSBAR(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_STATUSBAR_APP: {
      self->private->app = g_object_ref(G_OBJECT(g_value_get_object(value)));
      //GST_DEBUG("set the app for main_statusbar: %p",self->private->app);
    } break;
    default: {
			g_assert(FALSE);
      break;
    }
  }
}

static void bt_main_statusbar_dispose(GObject *object) {
  BtMainStatusbar *self = BT_MAIN_STATUSBAR(object);
	return_if_disposed();
  self->private->dispose_has_run = TRUE;
}

static void bt_main_statusbar_finalize(GObject *object) {
  BtMainStatusbar *self = BT_MAIN_STATUSBAR(object);
  
  g_object_unref(G_OBJECT(self->private->app));
  g_free(self->private);
}

static void bt_main_statusbar_init(GTypeInstance *instance, gpointer g_class) {
  BtMainStatusbar *self = BT_MAIN_STATUSBAR(instance);
  self->private = g_new0(BtMainStatusbarPrivate,1);
  self->private->dispose_has_run = FALSE;
}

static void bt_main_statusbar_class_init(BtMainStatusbarClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;
  
  gobject_class->set_property = bt_main_statusbar_set_property;
  gobject_class->get_property = bt_main_statusbar_get_property;
  gobject_class->dispose      = bt_main_statusbar_dispose;
  gobject_class->finalize     = bt_main_statusbar_finalize;

  g_object_class_install_property(gobject_class,MAIN_STATUSBAR_APP,
                                  g_param_spec_object("app",
                                     "app contruct prop",
                                     "Set application object, the menu belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));
}

GType bt_main_statusbar_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtMainStatusbarClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_main_statusbar_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtMainStatusbar),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_main_statusbar_init, // instance_init
    };
		type = g_type_register_static(GTK_TYPE_HBOX,"BtMainStatusbar",&info,0);
  }
  return type;
}

