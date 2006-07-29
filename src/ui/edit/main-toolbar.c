// $Id: main-toolbar.c,v 1.85 2006-07-29 19:55:06 ensonic Exp $
/**
 * SECTION:btmaintoolbar
 * @short_description: class for the editor main toolbar
 */ 

/* @todo should we separate the toolbars?
 * - common - load, save, ...
 * - volume - gain, levels
 * - load - cpu load
 */
#define BT_EDIT
#define BT_MAIN_TOOLBAR_C

#include "bt-edit.h"
#include "gtkvumeter.h"

enum {
  MAIN_TOOLBAR_APP=1,
};

#define MAX_VUMETER 4

struct _BtMainToolbarPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  union {
    BtEditApplication *app;
    gpointer app_ptr;
  };
  
  /* the toolbar widget */
  GtkWidget *toolbar;
  
  /* the level meters */
  GtkVUMeter *vumeter[MAX_VUMETER];
  
  /* the volume gain */
  GtkScale *volume;
  GstElement *gain;

  /* action buttons */
  GtkWidget *save_button;
  GtkWidget *play_button;
  GtkWidget *stop_button;
  GtkWidget *loop_button;
  
  /* update handler id */
  guint playback_update_id;
};

static GtkHandleBoxClass *parent_class=NULL;

//-- helper

static gint gst_caps_get_channels(GstCaps *caps) {
  GstStructure *structure;
  gint channels=0,size,i;

  g_assert(caps);
  
  if(GST_CAPS_IS_SIMPLE(caps)) {
    if((structure=gst_caps_get_structure(caps,0))) {
      gst_structure_get_int(structure,"channels",&channels);
      GST_DEBUG("---    simple caps with channels=%d",channels);
    }
  }
  else {
    size=gst_caps_get_size(caps);
    for(i=0;i<size;i++) {
      if((structure=gst_caps_get_structure(caps,i))) {
        gst_structure_get_int(structure,"channels",&channels);
        GST_DEBUG("---    caps %d with channels=%d",i,channels);
      }
    }
  }
  return(channels);
}

//-- event handler

static gboolean on_song_playback_update(gpointer user_data) {
  BtSong *self=BT_SONG(user_data);

  return(bt_song_update_playback_position(self));
}

static void on_song_is_playing_notify(const BtSong *song,GParamSpec *arg,gpointer user_data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);
  gboolean is_playing;

  g_assert(user_data);
  
  g_object_get(G_OBJECT(song),"is-playing",&is_playing,NULL);
  if(!is_playing) {
    gint i;
  
    GST_INFO("song stop event occured: %p",g_thread_self());
    g_source_remove(self->priv->playback_update_id);
    // disable stop button
    gtk_widget_set_sensitive(GTK_WIDGET(self->priv->stop_button),FALSE);
    // switch off play button
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(self->priv->play_button),FALSE);
    // enable play button
    gtk_widget_set_sensitive(GTK_WIDGET(self->priv->play_button),TRUE);
    // reset level meters
    for(i=0;i<MAX_VUMETER;i++) {
      gtk_vumeter_set_levels(self->priv->vumeter[i], -900, -900);
    }

    GST_INFO("song stop event handled");
  }
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

  if(gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(button))) {
    BtSong *song;

    GST_INFO("toolbar play event occurred");

    // disable play button
    gtk_widget_set_sensitive(GTK_WIDGET(self->priv->play_button),FALSE);
    
    // get song from app and start playback
    g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
    bt_song_play(song);
    self->priv->playback_update_id=g_timeout_add(500,on_song_playback_update,song);

    // enable stop button
    gtk_widget_set_sensitive(GTK_WIDGET(self->priv->stop_button),TRUE);
    
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
  GST_INFO("  song stopped");
  // release the reference
  g_object_try_unref(song);
}

static void on_toolbar_loop_toggled(GtkButton *button, gpointer user_data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);
  BtSong *song;
  BtSequence *sequence;
  gboolean loop;

  g_assert(user_data);

  loop=gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(button));
  GST_INFO("toolbar loop toggle event occurred, new-state=%d",loop);
  // get song from app
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"sequence",&sequence,NULL);
  g_object_set(G_OBJECT(sequence),"loop",loop,NULL);
  // release the references
  g_object_try_unref(sequence);
  g_object_try_unref(song);
}

static gboolean on_song_level_change(GstBus *bus, GstMessage *message, gpointer user_data) {
  gboolean res=FALSE;
  g_assert(user_data);
  
  switch(GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ELEMENT: {
      BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);
      const GstStructure *structure=gst_message_get_structure(message);
      const gchar *name = gst_structure_get_name(structure);
      
      if(!strcmp(name,"level")) {
        const GValue *l_rms,*l_peak;
        gdouble rms, peak;
        guint i;
  
        l_rms=(GValue *)gst_structure_get_value(structure, "rms");
        l_peak=(GValue *)gst_structure_get_value(structure, "peak");
        //l_decay=(GValue *)gst_structure_get_value(structure, "decay");
        for(i=0;i<gst_value_list_get_size(l_rms);i++) {
          rms=g_value_get_double(gst_value_list_get_value(l_rms,i));
          peak=g_value_get_double(gst_value_list_get_value(l_peak,i));
          //GST_INFO("level.%d  %.3f %.3f", i, rms,peak);
          gtk_vumeter_set_levels(self->priv->vumeter[i], (gint)(rms*10.0), (gint)(peak*10.0));
        }
        res=TRUE;
      }
    } break;
    case GST_MESSAGE_APPLICATION: {
      BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);
      const GstStructure *structure=gst_message_get_structure(message);
      const gchar *name = gst_structure_get_name(structure);
      
      if(!strcmp(name,"level-caps-changed")) {
        gint i,channels;
        
        gst_structure_get_int(structure,"channels",&channels);
        GST_INFO("received application bus message: channel=%d",channels);
        
        for(i=0;i<channels;i++) {
          gtk_widget_show(GTK_WIDGET(self->priv->vumeter[i]));
        }
        for(i=channels;i<MAX_VUMETER;i++) {
          gtk_widget_hide(GTK_WIDGET(self->priv->vumeter[i]));
        }        
        res=TRUE;
      }      
    } break;
    default:
      //GST_INFO("received bus message: type=%s",gst_message_type_get_name(GST_MESSAGE_TYPE(message)));
      break;
  }
  return(res);
}

/*
static void on_song_level_change (GstBus * bus, GstMessage * message, gpointer user_data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);
  const GstStructure *structure = gst_message_get_structure(message);
  const GValue *l_rms,*l_peak;
  gdouble rms, peak;
  guint i;

  g_assert(user_data);
  GST_INFO("received bus message: application");

  l_rms=(GValue *)gst_structure_get_value(structure, "rms");
  l_peak=(GValue *)gst_structure_get_value(structure, "peak");
  //l_decay=(GValue *)gst_structure_get_value(structure, "decay");
  for(i=0;i<gst_value_list_get_size(l_rms);i++) {
    rms=g_value_get_double(gst_value_list_get_value(l_rms,i));
    peak=g_value_get_double(gst_value_list_get_value(l_peak,i));
    GST_INFO("level.%d  %.3f %.3f", i, rms,peak);
    gtk_vumeter_set_levels(self->priv->vumeter[i], (gint)(rms*10.0), (gint)(peak*10.0));
  }
}
*/

static void on_song_volume_change(GtkRange *range,gpointer user_data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);
  
  g_assert(user_data);
  g_assert(self->priv->gain);
  // get value from HScale and change volume
  GST_INFO("volume has changed : %f",gtk_range_get_value(GTK_RANGE(self->priv->volume)));
  g_object_set(self->priv->gain,"volume",gtk_range_get_value(GTK_RANGE(self->priv->volume)),NULL);
}

static void on_channels_negotiated(GstPad *pad,GParamSpec *arg,gpointer user_data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);
  GstCaps *caps;
  
  g_assert(user_data);
  if((caps=(GstCaps *)gst_pad_get_negotiated_caps(pad))) {
    gint channels;
    GstStructure *structure;
    GstMessage *message;
    GstElement *bin;
    GstBus *bus;
    
    channels=gst_caps_get_channels(caps);
    GST_INFO("!!!  input level src has %d output channels",channels);
    
    // post a message to the bus
    structure = gst_structure_new ("level-caps-changed",
        "channels", G_TYPE_INT, channels, NULL);
    message = gst_message_new_application (NULL, structure);
    
    g_object_get(G_OBJECT(self->priv->app),"bin",&bin,NULL);
    bus=gst_element_get_bus(bin);
    
    gst_bus_post (bus, message);

    gst_object_unref(bus);
    gst_object_unref(bin);
  }
}

static void on_song_unsaved_changed(const BtSong *song,GParamSpec *arg,gpointer user_data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);
  gboolean unsaved;

  g_assert(user_data);
  
  GST_INFO("song.unsaved has changed : song=%p, toolbar=%p",song,user_data);
  
  g_object_get(G_OBJECT(song),"unsaved",&unsaved,NULL);
  gtk_widget_set_sensitive(self->priv->save_button,unsaved);
}


static void on_song_changed(const BtEditApplication *app,GParamSpec *arg,gpointer user_data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);
  BtSong *song;
  BtSequence *sequence;
  BtSinkMachine *master;
  GstElement *level;
  gboolean loop;

  g_assert(user_data);

  GST_INFO("song has changed : app=%p, toolbar=%p",app,user_data);
  
  // get the audio_sink (song->master is a bt_sink_machine) if there is one already
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_return_if_fail(song);

  g_object_get(G_OBJECT(song),"master",&master,"sequence",&sequence,NULL);
  g_object_get(sequence,"loop",&loop,NULL);
  gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(self->priv->loop_button),loop);
  
  if(master) {
    GstPad *pad;
    gdouble volume;
    
    GST_INFO("connect to input-level : song=%p,  master=%p",song,master);

    // get the input_level property from audio_sink
    g_object_get(G_OBJECT(master),"input-level",&level,"input-gain",&self->priv->gain,NULL);

    g_assert(GST_IS_ELEMENT(level));
    bt_application_add_bus_watch(BT_APPLICATION(self->priv->app),GST_DEBUG_FUNCPTR(on_song_level_change),(gpointer)self);

    // get the pad from the input-level and listen there for channel negotiation
    if((pad=gst_element_get_pad(level,"src"))) {
      g_signal_connect(pad,"notify::caps",G_CALLBACK(on_channels_negotiated),(gpointer)self);
    }
    // release the references
    gst_object_unref(level);
    
    g_assert(GST_IS_ELEMENT(self->priv->gain));
    // get the current input_gain and adjust volume widget
    g_object_get(self->priv->gain,"volume",&volume,NULL);
    gtk_range_set_value(GTK_RANGE(self->priv->volume),volume);
    // connect volume event
    g_signal_connect(G_OBJECT(self->priv->volume),"value_changed",G_CALLBACK(on_song_volume_change),self);

    g_object_unref(master);
  }
  else {
    GST_WARNING("failed to get the master element of the song");
  }
  g_signal_connect(G_OBJECT(song),"notify::is-playing",G_CALLBACK(on_song_is_playing_notify),(gpointer)self);
  on_song_unsaved_changed(song,NULL,self);
  g_signal_connect(G_OBJECT(song), "notify::unsaved", G_CALLBACK(on_song_unsaved_changed), (gpointer)self);
  g_object_unref(sequence);
  g_object_unref(song);
}

static void on_toolbar_style_changed(const BtSettings *settings,GParamSpec *arg,gpointer user_data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(user_data);
  gchar *toolbar_style;
  
  g_object_get(G_OBJECT(settings),"toolbar-style",&toolbar_style,NULL);
  
  GST_INFO("!!!  toolbar style has changed '%s'",toolbar_style);
  gtk_toolbar_set_style(GTK_TOOLBAR(self->priv->toolbar),gtk_toolbar_get_style_from_string(toolbar_style));
  g_free(toolbar_style);
}

//-- helper methods

static gboolean bt_main_toolbar_init_ui(const BtMainToolbar *self) {
  BtSettings *settings;
  GtkWidget *tool_item;
  GtkTooltips *tips;
  GtkWidget *box;
  gulong i;
   
  tips=gtk_tooltips_new();

  gtk_widget_set_name(GTK_WIDGET(self),_("handlebox for toolbar"));
  
  self->priv->toolbar=gtk_toolbar_new();
  gtk_widget_set_name(self->priv->toolbar,_("tool bar"));
  
  //-- file controls

  tool_item=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_NEW));
  gtk_widget_set_name(tool_item,_("New"));
  gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(tool_item),GTK_TOOLTIPS(tips),_("Prepare a new empty song"),NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_toolbar_new_clicked),(gpointer)self);

  tool_item=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_OPEN));
  gtk_widget_set_name(tool_item,_("Open"));
  gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(tool_item),GTK_TOOLTIPS(tips),_("Load a new song"),NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_toolbar_open_clicked),(gpointer)self);

  tool_item=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_SAVE));
  gtk_widget_set_name(tool_item,_("Save"));
  gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(tool_item),GTK_TOOLTIPS(tips),_("Save this song"),NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_toolbar_save_clicked),(gpointer)self);
  self->priv->save_button=tool_item;

  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),gtk_separator_tool_item_new(),-1);

  //-- media controls
  
  tool_item=GTK_WIDGET(gtk_toggle_tool_button_new_from_stock(GTK_STOCK_MEDIA_PLAY));
  gtk_widget_set_name(tool_item,_("Play"));
  gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(tool_item),GTK_TOOLTIPS(tips),_("Play this song"),NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_toolbar_play_clicked),(gpointer)self);
  self->priv->play_button=tool_item;

  tool_item=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_STOP));
  gtk_widget_set_name(tool_item,_("Stop"));
  gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(tool_item),GTK_TOOLTIPS(tips),_("Stop playback of this song"),NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_toolbar_stop_clicked),(gpointer)self);
  gtk_widget_set_sensitive(tool_item,FALSE);
  self->priv->stop_button=tool_item;

  tool_item=GTK_WIDGET(gtk_toggle_tool_button_new());
  gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(tool_item),gtk_image_new_from_filename("stock_repeat.png"));
  gtk_tool_button_set_label(GTK_TOOL_BUTTON(tool_item),_("Loop"));
  gtk_widget_set_name(tool_item,_("Loop"));
  gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(tool_item),GTK_TOOLTIPS(tips),_("Toggle looping of playback"),NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(G_OBJECT(tool_item),"toggled",G_CALLBACK(on_toolbar_loop_toggled),(gpointer)self);
  self->priv->loop_button=tool_item;

  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),gtk_separator_tool_item_new(),-1);
  
  //-- volume level and control

  box=gtk_vbox_new(FALSE,0);
  gtk_container_set_border_width(GTK_CONTAINER(box),2);
  gtk_widget_set_size_request(GTK_WIDGET(box),150,-1);
  // add gtk_vumeter widgets and update from level_callback
  for(i=0;i<MAX_VUMETER;i++) {
    self->priv->vumeter[i]=GTK_VUMETER(gtk_vumeter_new(FALSE));
    // @idea have distinct tooltips
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),GTK_WIDGET(self->priv->vumeter[i]),_("playback volume"),NULL);
    gtk_vumeter_set_min_max(self->priv->vumeter[i], -900, 0);
    gtk_vumeter_set_scale(self->priv->vumeter[i], GTK_VUMETER_SCALE_LOG);
    gtk_vumeter_set_levels(self->priv->vumeter[i], -900, -900);
    gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->priv->vumeter[i]),TRUE,TRUE,0);
  }

  // add gain-control
  self->priv->volume=GTK_SCALE(gtk_hscale_new_with_range(/*min=*/0.0,/*max=*/1.0,/*step=*/0.01));
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),GTK_WIDGET(self->priv->volume),_("Change playback volume"),NULL);
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->priv->volume),TRUE,TRUE,0);
  gtk_scale_set_draw_value(self->priv->volume,FALSE);
  gtk_range_set_update_policy(GTK_RANGE(self->priv->volume),GTK_UPDATE_DELAYED);

  tool_item=GTK_WIDGET(gtk_tool_item_new());
  gtk_widget_set_name(tool_item,_("Volume"));
  gtk_container_add(GTK_CONTAINER(tool_item),box);
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),GTK_TOOL_ITEM(tool_item),-1);

  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->toolbar),gtk_separator_tool_item_new(),-1);

  gtk_container_add(GTK_CONTAINER(self),self->priv->toolbar);

  // register event handlers
  g_signal_connect(G_OBJECT(self->priv->app), "notify::song", G_CALLBACK(on_song_changed), (gpointer)self);

  // let settings control toolbar style
  g_object_get(G_OBJECT(self->priv->app),"settings",&settings,NULL);
  on_toolbar_style_changed(settings,NULL,(gpointer)self);
  g_signal_connect(G_OBJECT(settings), "notify::toolbar-style", G_CALLBACK(on_toolbar_style_changed), (gpointer)self);
  g_object_unref(settings);

  return(TRUE);
}

//-- constructor methods

/**
 * bt_main_toolbar_new:
 * @app: the application the toolbar belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
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
      g_object_try_weak_unref(self->priv->app);
      self->priv->app = BT_EDIT_APPLICATION(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->app);
      //GST_DEBUG("set the app for main_toolbar: %p",self->priv->app);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_main_toolbar_dispose(GObject *object) {
  BtMainToolbar *self = BT_MAIN_TOOLBAR(object);
	BtSong *song;
	
  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  
  bt_application_remove_bus_watch(BT_APPLICATION(self->priv->app),on_song_level_change);
	
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  if(song) {
    g_signal_handlers_disconnect_matched(song,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_song_is_playing_notify,NULL);
    g_object_unref(song);
  }

  if(self->priv->gain) gst_object_unref(self->priv->gain);
  
  g_object_try_weak_unref(self->priv->app);

  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_main_toolbar_finalize(GObject *object) {
  //BtMainToolbar *self = BT_MAIN_TOOLBAR(object);
  
  //GST_DEBUG("!!!! self=%p",self);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_main_toolbar_init(GTypeInstance *instance, gpointer g_class) {
  BtMainToolbar *self = BT_MAIN_TOOLBAR(instance);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_MAIN_TOOLBAR, BtMainToolbarPrivate);
}

static void bt_main_toolbar_class_init(BtMainToolbarClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtMainToolbarPrivate));
  
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
  if (G_UNLIKELY(type == 0)) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtMainToolbarClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_main_toolbar_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtMainToolbar),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_main_toolbar_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(GTK_TYPE_HANDLE_BOX,"BtMainToolbar",&info,0);
  }
  return type;
}
