/* $Id: main-statusbar.c,v 1.15 2004-09-29 16:56:47 ensonic Exp $
 * class for the editor main tollbar
 */

#define BT_EDIT
#define BT_MAIN_STATUSBAR_C

#include "bt-edit.h"

enum {
  MAIN_STATUSBAR_APP=1,
  MAIN_STATUSBAR_STATUS
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

static GtkHBoxClass *parent_class=NULL;

//-- event handler

static void on_song_stop(const BtSong *song, gpointer user_data) {
  BtMainStatusbar *self=BT_MAIN_STATUSBAR(user_data);
  gchar *str="00:00.000";

  // update statusbar fields
  gtk_statusbar_pop(self->priv->elapsed,self->priv->elapsed_context_id); 
	gtk_statusbar_push(self->priv->elapsed,self->priv->elapsed_context_id,str);
}

static void on_sequence_tick(const BtSequence *sequence, glong pos, gpointer user_data) {
  BtMainStatusbar *self=BT_MAIN_STATUSBAR(user_data);
  BtSong *song;
  gchar *str;
  gulong msec,sec,min;
  
  GST_INFO("sequence tick received : %d",pos);
  // update elapsed statusbar
  msec=pos*bt_sequence_get_bar_time(sequence);
  min=(gulong)(msec/60000);msec-=(min*60000);
  sec=(gulong)(msec/ 1000);msec-=(sec* 1000);
	str=g_strdup_printf("%02d:%02d.%03d",min,sec,msec);
  // update statusbar fields
  gtk_statusbar_pop(self->priv->elapsed,self->priv->elapsed_context_id); 
	gtk_statusbar_push(self->priv->elapsed,self->priv->elapsed_context_id,str);
 	g_free(str);
}

static void on_song_changed(const BtEditApplication *app, gpointer user_data) {
  BtMainStatusbar *self=BT_MAIN_STATUSBAR(user_data);
  BtSong *song;
  BtSequence *sequence;
  gchar *str;
  gulong msec,sec,min;

  GST_INFO("song has changed : app=%p, self=%p",app,self);
  // get song from app
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"sequence",&sequence,NULL);
  // get new song length
  msec=bt_sequence_get_loop_time(sequence);
  GST_INFO("  new msec : %ld",msec);
  min=(gulong)(msec/60000);msec-=(min*60000);
  sec=(gulong)(msec/ 1000);msec-=(sec* 1000);
	str=g_strdup_printf("%02d:%02d.%03d",min,sec,msec);
  // update statusbar fields
  gtk_statusbar_pop(self->priv->loop,self->priv->loop_context_id); 
	gtk_statusbar_push(self->priv->loop,self->priv->loop_context_id,str);
 	g_free(str);
  // subscribe to tick signal of song->sequence
  g_signal_connect(G_OBJECT(sequence), "tick", (GCallback)on_sequence_tick, (gpointer)self);
  g_signal_connect(G_OBJECT(song), "stop", (GCallback)on_song_stop, (gpointer)self);
  // @todo shouldn't we disconnet these signals somewhere
  // release the references
  g_object_try_unref(sequence);
  g_object_try_unref(song);
}

//-- helper methods

static gboolean bt_main_statusbar_init_ui(const BtMainStatusbar *self, const BtEditApplication *app) {
  gchar *str="00:00.000";
  
  gtk_widget_set_name(GTK_WIDGET(self),_("status bar"));
  //gtk_box_set_spacing(GTK_BOX(self),1);

  self->priv->status=GTK_STATUSBAR(gtk_statusbar_new());
  self->priv->status_context_id=gtk_statusbar_get_context_id(GTK_STATUSBAR(self->priv->status),_("default"));
  gtk_statusbar_set_has_resize_grip(self->priv->status,FALSE);
  gtk_statusbar_push(GTK_STATUSBAR(self->priv->status),self->priv->status_context_id,_("Ready to rock!"));
  gtk_box_pack_start(GTK_BOX(self),GTK_WIDGET(self->priv->status),TRUE,TRUE,1);

  self->priv->elapsed=GTK_STATUSBAR(gtk_statusbar_new());
  self->priv->elapsed_context_id=gtk_statusbar_get_context_id(GTK_STATUSBAR(self->priv->elapsed),_("default"));
  gtk_statusbar_set_has_resize_grip(self->priv->elapsed,FALSE);
  gtk_widget_set_size_request(GTK_WIDGET(self->priv->elapsed),100,-1);
  gtk_statusbar_push(GTK_STATUSBAR(self->priv->elapsed),self->priv->elapsed_context_id,str);
  gtk_box_pack_start(GTK_BOX(self),GTK_WIDGET(self->priv->elapsed),FALSE,FALSE,1);

  self->priv->current=GTK_STATUSBAR(gtk_statusbar_new());
  self->priv->current_context_id=gtk_statusbar_get_context_id(GTK_STATUSBAR(self->priv->current),_("default"));
  gtk_statusbar_set_has_resize_grip(self->priv->current,FALSE);
  gtk_widget_set_size_request(GTK_WIDGET(self->priv->current),100,-1);
  gtk_statusbar_push(GTK_STATUSBAR(self->priv->current),self->priv->current_context_id,str);
  gtk_box_pack_start(GTK_BOX(self),GTK_WIDGET(self->priv->current),FALSE,FALSE,1);

  self->priv->loop=GTK_STATUSBAR(gtk_statusbar_new());
  self->priv->loop_context_id=gtk_statusbar_get_context_id(GTK_STATUSBAR(self->priv->loop),_("default"));
  gtk_widget_set_size_request(GTK_WIDGET(self->priv->loop),100,-1);
  gtk_statusbar_push(GTK_STATUSBAR(self->priv->loop),self->priv->loop_context_id,str);
  gtk_box_pack_start(GTK_BOX(self),GTK_WIDGET(self->priv->loop),FALSE,FALSE,1);

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
  g_object_try_unref(self);
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
      g_value_set_object(value, self->priv->app);
    } break;
    default: {
 			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
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
      g_object_try_unref(self->priv->app);
      self->priv->app = g_object_try_ref(g_value_get_object(value));
      //GST_DEBUG("set the app for main_statusbar: %p",self->priv->app);
    } break;
    case MAIN_STATUSBAR_STATUS: {
      char *str=g_value_dup_string(value);
      gtk_statusbar_pop(GTK_STATUSBAR(self->priv->status),self->priv->status_context_id);
      if(str) {
        gtk_statusbar_push(GTK_STATUSBAR(self->priv->status),self->priv->status_context_id,str);
        g_free(str);
      }
      else gtk_statusbar_push(GTK_STATUSBAR(self->priv->status),self->priv->status_context_id,_("Ready to rock!"));
      while(gtk_events_pending()) gtk_main_iteration();
      //GST_DEBUG("set the status-text for main_statusbar");
    } break;
    default: {
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_main_statusbar_dispose(GObject *object) {
  BtMainStatusbar *self = BT_MAIN_STATUSBAR(object);
	return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  g_object_try_unref(self->priv->app);
  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_main_statusbar_finalize(GObject *object) {
  BtMainStatusbar *self = BT_MAIN_STATUSBAR(object);
  
  GST_DEBUG("!!!! self=%p",self);
  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_main_statusbar_init(GTypeInstance *instance, gpointer g_class) {
  BtMainStatusbar *self = BT_MAIN_STATUSBAR(instance);
  self->priv = g_new0(BtMainStatusbarPrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_main_statusbar_class_init(BtMainStatusbarClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;

  parent_class=g_type_class_ref(GTK_TYPE_HBOX);

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

  g_object_class_install_property(gobject_class,MAIN_STATUSBAR_STATUS,
                                  g_param_spec_string("status",
                                     "status prop",
                                     "main status text",
                                     _("Ready to rock!"), /* default value */
                                     G_PARAM_WRITABLE));
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

