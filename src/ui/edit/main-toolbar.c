/* $Id: main-toolbar.c,v 1.29 2004-12-13 10:31:42 ensonic Exp $
 * class for the editor main toolbar
 */

#define BT_EDIT
#define BT_MAIN_TOOLBAR_C

#include "bt-edit.h"
#include "gtkvumeter.h"

enum {
  MAIN_TOOLBAR_APP=1,
};


struct _BtMainToolbarPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;
  
  /* the level meters */
  GtkVUMeter *vumeter[4];
};

static GtkHandleBoxClass *parent_class=NULL;

//-- event handler

// @todo globale variables are not nice
gulong on_song_stop_handler_id;
GThread* player_thread=NULL;

static void on_song_stop(const BtSong *song, gpointer user_data) {
  GtkToggleButton *button=GTK_TOGGLE_BUTTON(user_data);

  g_assert(user_data);
  
  GST_INFO("song stop event occured, button=%p",button);
  g_signal_handler_disconnect(G_OBJECT(song),on_song_stop_handler_id);
  gtk_toggle_button_set_active(button,FALSE);
}

static void on_toolbar_new_clicked(GtkButton *button, gpointer user_data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);
  BtMainWindow *main_window;

  g_assert(user_data);
  
  GST_INFO("toolbar new event occurred");
  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
  bt_main_window_new_song(main_window);
  g_object_try_unref(main_window);
}

static void on_toolbar_open_clicked(GtkButton *button, gpointer user_data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);
  BtMainWindow *main_window;

  g_assert(user_data);
  
  GST_INFO("toolbar open event occurred");
  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
  bt_main_window_open_song(main_window);
  g_object_try_unref(main_window);
}

static void on_toolbar_save_clicked(GtkButton *button, gpointer user_data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);
  BtMainWindow *main_window;

  g_assert(user_data);
  
  GST_INFO("toolbar open event occurred");
  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
  bt_main_window_save_song(main_window);
  g_object_try_unref(main_window);
}

static void on_toolbar_play_clicked(GtkButton *button, gpointer user_data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);

  g_assert(user_data);

  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))) {
    BtSong *song;
    GError *error;

    GST_INFO("toolbar play event occurred");
    // get song from app
    g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);

    on_song_stop_handler_id=g_signal_connect(G_OBJECT(song),"stop",(GCallback)on_song_stop,(gpointer)button);
    //-- start playing in a thread
    if(!(player_thread=g_thread_create((GThreadFunc)&bt_song_play, (gpointer)song, FALSE, &error))) {
      GST_ERROR("error creating player thread : \"%s\"", error->message);
      g_error_free(error);
    }
    // release the reference
    g_object_try_unref(song);
  }
}

static void on_toolbar_stop_clicked(GtkButton *button, gpointer user_data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);
  BtSong *song;

  g_assert(user_data);

  GST_INFO("toolbar stop event occurred");
  // get song from app
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  bt_song_stop(song);
  // release the reference
  g_object_try_unref(song);
}

static void on_toolbar_loop_toggled(GtkButton *button, gpointer user_data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);
  BtSong *song;
  BtSequence *sequence;
  gboolean loop;

  g_assert(user_data);

  //loop=gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(button));
  loop=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
  GST_INFO("toolbar loop toggle event occurred, new-state=%d",loop);
  // get song from app
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"sequence",&sequence,NULL);
  g_object_set(G_OBJECT(sequence),"loop",loop,NULL);
  // release the references
  g_object_try_unref(sequence);
  g_object_try_unref(song);
}

static void on_song_level_change(GstElement * element, gdouble time, gint channel, gdouble rms, gdouble peak, gdouble decay, gpointer user_data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);
  
  g_assert(user_data);

  //GST_INFO("%d  %.3f  %.3f %.3f %.3f", channel,time,rms,peak,decay);
	gtk_vumeter_set_levels(self->priv->vumeter[channel], (gint)(rms*10.0), (gint)(peak*10.0));
}

static void on_song_changed(const BtEditApplication *app,GParamSpec *arg,gpointer user_data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);
  BtSong *song;
  BtSinkMachine *master;
  GstElement *level;

  g_assert(user_data);

  GST_INFO("song has changed : app=%p, toolbar=%p",app,user_data);
  
  // get the audio_sink (song->master is a bt_sink_machine) if there is one already
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"master",&master,NULL);
	if(master) {
		// get the input_level property from audio_sink
		g_object_get(G_OBJECT(master),"input-level",&level,NULL);
		// connect to the level signal
		g_signal_connect(level, "level", G_CALLBACK(on_song_level_change), self);
		// DEBUG
		{
			GstPad *pad;
			GstCaps *caps;
			GstStructure *structure;
			gint channels_i=0,channels_o=0;
    
			pad=gst_element_get_pad(level,"sink");
			caps=gst_pad_get_caps(pad);
			// check if it is simple caps (only one element)
			if(GST_CAPS_IS_SIMPLE(caps)) {
				structure = gst_caps_get_structure(caps, 0);
				gst_structure_get_int (structure, "channels", &channels_i);
			}
			pad=gst_element_get_pad(level,"src");
			caps=gst_pad_get_caps(pad);
			// check if it is simple caps (only one element)
			if(GST_CAPS_IS_SIMPLE(caps)) {
				structure = gst_caps_get_structure(caps, 0);
				gst_structure_get_int (structure, "channels", &channels_o);
			}
			GST_INFO("  input level analyser will process %d,%d channels",channels_i,channels_o);
		}
		// DEBUG  // release the reference
		g_object_try_unref(level);
	}
  g_object_try_unref(master);
  g_object_try_unref(song);

}

//-- helper methods

static gboolean bt_main_toolbar_init_ui(const BtMainToolbar *self) {
  GtkWidget *toolbar;
  GtkWidget *icon,*button,*image;
  GtkTooltips *tips;
  GtkWidget *box;
  gulong i;
   
  tips=gtk_tooltips_new();

  gtk_widget_set_name(GTK_WIDGET(self),_("handlebox for toolbar"));

  toolbar=gtk_toolbar_new();
  gtk_widget_set_name(toolbar,_("tool bar"));
  gtk_container_add(GTK_CONTAINER(self),toolbar);
  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar),GTK_TOOLBAR_BOTH);

  //-- file controls

  icon=gtk_image_new_from_stock(GTK_STOCK_NEW, gtk_toolbar_get_icon_size(GTK_TOOLBAR(toolbar)));
  button=gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                _("New"),
                                NULL,NULL,
                                icon,NULL,NULL);
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("New"));
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),button,_("Prepare a new empty song"),NULL);
	g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(on_toolbar_new_clicked),(gpointer)self);

  icon=gtk_image_new_from_stock(GTK_STOCK_OPEN, gtk_toolbar_get_icon_size(GTK_TOOLBAR(toolbar)));
  button=gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                _("Open"),
                                NULL,NULL,
                                icon,NULL,NULL);
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("Open"));
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),button,_("Load a new song"),NULL);
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(on_toolbar_open_clicked),(gpointer)self);

  icon=gtk_image_new_from_stock(GTK_STOCK_SAVE, gtk_toolbar_get_icon_size(GTK_TOOLBAR(toolbar)));
  button=gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                _("Save"),
                                NULL,NULL,
                                icon,NULL,NULL);
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("Save"));
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),button,_("Save this song"),NULL);
	g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(on_toolbar_save_clicked),(gpointer)self);

  gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

  //-- media controls
  
  image=gtk_image_new_from_filename("stock_media-play.png");
  button=gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
                                GTK_TOOLBAR_CHILD_TOGGLEBUTTON,
                                NULL,
                                _("Play"),
                                NULL, NULL,
                                image, NULL, NULL);
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("Play"));
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),button,_("Play this song"),NULL);
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(on_toolbar_play_clicked),(gpointer)self);

  image=gtk_image_new_from_filename("stock_media-stop.png");
  button=gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                _("Stop"),
                                NULL, NULL,
                                image, NULL, NULL);
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("Stop"));
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),button,_("Stop playback of this song"),NULL);
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(on_toolbar_stop_clicked),(gpointer)self);

  image=gtk_image_new_from_filename("stock_repeat.png");
  button=gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
                                GTK_TOOLBAR_CHILD_TOGGLEBUTTON,
                                NULL,
                                _("Loop"),
                                NULL, NULL,
                                image, NULL, NULL);
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("Loop"));
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),button,_("Toggle looping of playback"),NULL);
  g_signal_connect(G_OBJECT(button),"toggled",G_CALLBACK(on_toolbar_loop_toggled),(gpointer)self);

  gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
  
  // volume level
  box=gtk_vbox_new(FALSE,2);
  gtk_container_set_border_width(GTK_CONTAINER(box),4);
  // add gtk_vumeter widgets and update from level_callback
  //   how do we determine the number of channels?
  //   irks, this can change when changing the audio-sink to use!
  //   -> what about adding 4 VUMeter and making enough of them visible after a song_change (for now)
  for(i=0;i<2;i++) {
    self->priv->vumeter[i]=GTK_VUMETER(gtk_vumeter_new(FALSE));
    gtk_vumeter_set_min_max(self->priv->vumeter[i], -900, 0);
    gtk_vumeter_set_scale(self->priv->vumeter[i], GTK_VUMETER_SCALE_LOG);
    gtk_vumeter_set_levels(self->priv->vumeter[i], -900, -900);
    gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->priv->vumeter[i]),TRUE,TRUE,2);
  }
  gtk_widget_set_size_request(GTK_WIDGET(box),150,-1);
  // @todo add gain-control

  button=gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
                                GTK_TOOLBAR_CHILD_WIDGET,
                                box,
                                NULL,
                                NULL,NULL,
                                NULL,NULL,NULL);
  //gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("Volume"));

  gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
  
  g_signal_connect(G_OBJECT(self->priv->app), "notify::song", (GCallback)on_song_changed, (gpointer)self);

  return(TRUE);
}

//-- constructor methods

/**
 * bt_main_toolbar_new:
 * @app: the application the toolbar belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtMainToolbar *bt_main_toolbar_new(const BtEditApplication *app) {
  BtMainToolbar *self;

  if(!(self=BT_MAIN_TOOLBAR(g_object_new(BT_TYPE_MAIN_TOOLBAR,"app",app,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_main_toolbar_init_ui(self)) {
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
static void bt_main_toolbar_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtMainToolbar *self = BT_MAIN_TOOLBAR(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_TOOLBAR_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    default: {
 			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_main_toolbar_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtMainToolbar *self = BT_MAIN_TOOLBAR(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_TOOLBAR_APP: {
      g_object_try_unref(self->priv->app);
      self->priv->app = g_object_try_ref(g_value_get_object(value));
      //GST_DEBUG("set the app for main_toolbar: %p",self->priv->app);
    } break;
    default: {
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_main_toolbar_dispose(GObject *object) {
  BtMainToolbar *self = BT_MAIN_TOOLBAR(object);
	return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  g_object_try_unref(self->priv->app);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_main_toolbar_finalize(GObject *object) {
  BtMainToolbar *self = BT_MAIN_TOOLBAR(object);
  
  GST_DEBUG("!!!! self=%p",self);
  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_main_toolbar_init(GTypeInstance *instance, gpointer g_class) {
  BtMainToolbar *self = BT_MAIN_TOOLBAR(instance);
  self->priv = g_new0(BtMainToolbarPrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_main_toolbar_class_init(BtMainToolbarClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(GTK_TYPE_HANDLE_BOX);
  
  gobject_class->set_property = bt_main_toolbar_set_property;
  gobject_class->get_property = bt_main_toolbar_get_property;
  gobject_class->dispose      = bt_main_toolbar_dispose;
  gobject_class->finalize     = bt_main_toolbar_finalize;

  g_object_class_install_property(gobject_class,MAIN_TOOLBAR_APP,
                                  g_param_spec_object("app",
                                     "app contruct prop",
                                     "Set application object, the menu belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));
}

GType bt_main_toolbar_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtMainToolbarClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_main_toolbar_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtMainToolbar),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_main_toolbar_init, // instance_init
			NULL // value_table
    };
		type = g_type_register_static(GTK_TYPE_HANDLE_BOX,"BtMainToolbar",&info,0);
  }
  return type;
}
