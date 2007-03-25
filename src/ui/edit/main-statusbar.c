/* $Id: main-statusbar.c,v 1.56 2007-03-25 14:18:32 ensonic Exp $
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
 * SECTION:btmainstatusbar
 * @short_description: class for the editor main statusbar
 *
 * The statusbar shows some contextual help, as well as things like playback
 * status.
 */
/* buzz uses 3 time counters 
 * file:///windows/C/Programme/Jeskola/Buzz%20(work)/Help/Files/Time%20Window.htm
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
  G_POINTER_ALIAS(BtEditApplication *,app);
  
  /* main status bar */
  GtkStatusbar *status;
  /* identifier of the status message group */
  gint status_context_id;
  
  /* time-elapsed (total) status bar */
  GtkStatusbar *elapsed;
  /* identifier of the elapsed message group */
  gint elapsed_context_id;

  /* time-current (current play pos) status bar */
  GtkStatusbar *current;
  /* identifier of the current message group */
  gint current_context_id;

  /* time-loop status bar */
  GtkStatusbar *loop;
  /* identifier of the loop message group */
  gint loop_context_id;
  
  /* cpu load */
  GtkProgressBar *cpu_load;
  guint cpu_load_handler_id;
  
  /* total playtime */
  GstClockTime total_time;
  gulong last_pos, play_start;
};

static GtkHBoxClass *parent_class=NULL;

//-- helper

static void bt_main_statusbar_update_length(const BtMainStatusbar *self, const BtSong *song,const BtSequence *sequence) {
  gchar str[2+2+3+3];
  gulong msec,sec,min;

  // get new song length
  msec=(gulong)(bt_sequence_get_loop_time(sequence)/G_USEC_PER_SEC);
  GST_INFO("  new msec : %lu",msec);
  min=(gulong)(msec/60000);msec-=(min*60000);
  sec=(gulong)(msec/ 1000);msec-=(sec* 1000);
  g_sprintf(str,"%02lu:%02lu.%03lu",min,sec,msec);
  // update statusbar fields
  gtk_statusbar_pop(self->priv->loop,self->priv->loop_context_id); 
  gtk_statusbar_push(self->priv->loop,self->priv->loop_context_id,str);
}

//-- event handler

static void on_song_play_pos_notify(const BtSong *song,GParamSpec *arg,gpointer user_data) {
  BtMainStatusbar *self=BT_MAIN_STATUSBAR(user_data);
  BtSequence *sequence;
  // the +4 is not really needed, but I get a stack smashing error on ubuntu without
  gchar str[2+2+3+3 + 4];
  gulong pos,msec,sec,min;
  GstClockTime bar_time;
  
  g_assert(user_data);
  GST_DEBUG("tick update");

  g_object_get(G_OBJECT(song),"sequence",&sequence,"play-pos",&pos,NULL);
  //GST_INFO("sequence tick received : %d",pos);
  bar_time=bt_sequence_get_bar_time(sequence);
  
  // update current statusbar
  msec=(gulong)((pos*bar_time)/G_USEC_PER_SEC);
  min=(gulong)(msec/60000);msec-=(min*60000);
  sec=(gulong)(msec/ 1000);msec-=(sec* 1000);
  // format
  g_sprintf(str,"%02lu:%02lu.%03lu",min,sec,msec);
  // update statusbar fields
  gtk_statusbar_pop(self->priv->current,self->priv->current_context_id);
  gtk_statusbar_push(self->priv->current,self->priv->current_context_id,str);

  // update elapsed statusbar
  if(pos<self->priv->last_pos) {
    self->priv->total_time+=bt_sequence_get_loop_time(sequence);
    GST_INFO("wrapped around total_time=%"G_GUINT64_FORMAT,self->priv->total_time);
  }
  pos-=self->priv->play_start;
  msec=(gulong)(((pos*bar_time)+self->priv->total_time)/G_USEC_PER_SEC);
  min=(gulong)(msec/60000);msec-=(min*60000);
  sec=(gulong)(msec/ 1000);msec-=(sec* 1000);
  // format
  g_sprintf(str,"%02lu:%02lu.%03lu",min,sec,msec);
  // update statusbar fields
  gtk_statusbar_pop(self->priv->elapsed,self->priv->elapsed_context_id);
  gtk_statusbar_push(self->priv->elapsed,self->priv->elapsed_context_id,str);

  self->priv->last_pos=pos;
  g_object_unref(sequence);
}

static void on_song_is_playing_notify(const BtSong *song,GParamSpec *arg,gpointer user_data) {
  BtMainStatusbar *self=BT_MAIN_STATUSBAR(user_data);
  gboolean is_playing;
  gulong play_start;

  g_assert(user_data);

  g_object_get(G_OBJECT(song),"is-playing",&is_playing,"play-pos",&play_start,NULL);
  self->priv->total_time=G_GINT64_CONSTANT(0);
  if(!is_playing) {
    GST_INFO("play_start=%d",play_start);
    // update statusbar fields
    self->priv->last_pos=play_start;
    self->priv->play_start=play_start;
    on_song_play_pos_notify(song,NULL,user_data);
  }
  else {
    self->priv->last_pos=0;
    self->priv->play_start=0;
  }
}

static void on_song_info_rhythm_notify(const BtSongInfo *song_info,GParamSpec *arg,gpointer user_data) {
  BtMainStatusbar *self=BT_MAIN_STATUSBAR(user_data);
  BtSong *song;
  BtSequence *sequence;

  // get song from app
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_return_if_fail(song);
  g_object_get(G_OBJECT(song),"sequence",&sequence,NULL);

  bt_main_statusbar_update_length(self,song,sequence);

  // release the references
  g_object_unref(sequence);
  g_object_unref(song);
}

static void on_sequence_loop_time_notify(const BtSequence *sequence,GParamSpec *arg,gpointer user_data) {
  BtMainStatusbar *self=BT_MAIN_STATUSBAR(user_data);
  BtSong *song;

  // get song from app
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_return_if_fail(song);

  bt_main_statusbar_update_length(self,song,sequence);

  // release the references
  g_object_unref(song);
}

static void on_song_changed(const BtEditApplication *app,GParamSpec *arg,gpointer user_data) {
  BtMainStatusbar *self=BT_MAIN_STATUSBAR(user_data);
  BtSong *song;
  BtSongInfo *song_info;
  BtSequence *sequence;

  g_assert(user_data);

  GST_INFO("song has changed : app=%p, self=%p",app,self);
  // get song from app
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  if(!song) return;

  g_object_get(G_OBJECT(song),"sequence",&sequence,"song-info",&song_info,NULL);
  bt_main_statusbar_update_length(self,song,sequence);
  // subscribe to property changes in song
  g_signal_connect(G_OBJECT(song), "notify::play-pos", G_CALLBACK(on_song_play_pos_notify), (gpointer)self);
  g_signal_connect(G_OBJECT(song), "notify::is-playing", G_CALLBACK(on_song_is_playing_notify), (gpointer)self);
  // subscribe to property changes in song_info
  g_signal_connect(G_OBJECT(song_info), "notify::bpm", G_CALLBACK(on_song_info_rhythm_notify), (gpointer)self);
  g_signal_connect(G_OBJECT(song_info), "notify::tpb", G_CALLBACK(on_song_info_rhythm_notify), (gpointer)self);
  // subscribe to property changes in sequence
  g_signal_connect(G_OBJECT(sequence), "notify::length", G_CALLBACK(on_sequence_loop_time_notify), (gpointer)self);
  g_signal_connect(G_OBJECT(sequence), "notify::loop", G_CALLBACK(on_sequence_loop_time_notify), (gpointer)self);
  g_signal_connect(G_OBJECT(sequence), "notify::loop_start", G_CALLBACK(on_sequence_loop_time_notify), (gpointer)self);
  g_signal_connect(G_OBJECT(sequence), "notify::loop_end", G_CALLBACK(on_sequence_loop_time_notify), (gpointer)self);
  // release the references
  g_object_unref(song_info);
  g_object_unref(sequence);
  g_object_unref(song);
}

static gboolean on_cpu_load_update(gpointer user_data) {
  BtMainStatusbar *self=BT_MAIN_STATUSBAR(user_data);
  guint cpu_load=bt_cpu_load_get_current();
  gchar str[strlen("CPU: 000 %") + 3];
  
  g_assert(user_data);
  g_sprintf(str,"CPU: %d %%",cpu_load);
  gtk_progress_bar_set_fraction(self->priv->cpu_load,(gdouble)cpu_load/100.0);
  gtk_progress_bar_set_text(self->priv->cpu_load,str);
  return(TRUE);
}

//-- helper methods

static gboolean bt_main_statusbar_init_ui(const BtMainStatusbar *self, const BtEditApplication *app) {
  GtkTooltips *tips;
  GtkWidget *ev_box;
  gchar str[]="00:00.000";
  
  gtk_widget_set_name(GTK_WIDGET(self),_("status bar"));
  //gtk_box_set_spacing(GTK_BOX(self),1);
  
  tips=gtk_tooltips_new();

  self->priv->status=GTK_STATUSBAR(gtk_statusbar_new());
  self->priv->status_context_id=gtk_statusbar_get_context_id(GTK_STATUSBAR(self->priv->status),_("default"));
  gtk_statusbar_set_has_resize_grip(self->priv->status,FALSE);
  gtk_statusbar_push(GTK_STATUSBAR(self->priv->status),self->priv->status_context_id,_("Ready to rock!"));
  gtk_box_pack_start(GTK_BOX(self),GTK_WIDGET(self->priv->status),TRUE,TRUE,1);

  ev_box=gtk_event_box_new();
  gtk_tooltips_set_tip(tips,ev_box,_("Playback time"),NULL);
  self->priv->elapsed=GTK_STATUSBAR(gtk_statusbar_new());
  self->priv->elapsed_context_id=gtk_statusbar_get_context_id(GTK_STATUSBAR(self->priv->elapsed),_("default"));
  gtk_statusbar_set_has_resize_grip(self->priv->elapsed,FALSE);
  gtk_widget_set_size_request(GTK_WIDGET(self->priv->elapsed),100,-1);
  gtk_statusbar_push(GTK_STATUSBAR(self->priv->elapsed),self->priv->elapsed_context_id,str);
  gtk_container_add(GTK_CONTAINER(ev_box),GTK_WIDGET(self->priv->elapsed));
  gtk_box_pack_start(GTK_BOX(self),ev_box,FALSE,FALSE,1);

  ev_box=gtk_event_box_new();
  gtk_tooltips_set_tip(tips,ev_box,_("Playback position"),NULL);
  self->priv->current=GTK_STATUSBAR(gtk_statusbar_new());
  self->priv->current_context_id=gtk_statusbar_get_context_id(GTK_STATUSBAR(self->priv->current),_("default"));
  gtk_statusbar_set_has_resize_grip(self->priv->current,FALSE);
  gtk_widget_set_size_request(GTK_WIDGET(self->priv->current),100,-1);
  gtk_statusbar_push(GTK_STATUSBAR(self->priv->current),self->priv->current_context_id,str);
  gtk_container_add(GTK_CONTAINER(ev_box),GTK_WIDGET(self->priv->current));
  gtk_box_pack_start(GTK_BOX(self),ev_box,FALSE,FALSE,1);

  ev_box=gtk_event_box_new();
  gtk_tooltips_set_tip(tips,ev_box,_("Playback length"),NULL);
  self->priv->loop=GTK_STATUSBAR(gtk_statusbar_new());
  self->priv->loop_context_id=gtk_statusbar_get_context_id(GTK_STATUSBAR(self->priv->loop),_("default"));
  gtk_widget_set_size_request(GTK_WIDGET(self->priv->loop),100,-1);
  gtk_statusbar_push(GTK_STATUSBAR(self->priv->loop),self->priv->loop_context_id,str);
  gtk_container_add(GTK_CONTAINER(ev_box),GTK_WIDGET(self->priv->loop));
  gtk_box_pack_start(GTK_BOX(self),ev_box,FALSE,FALSE,1);
  
  // @todo: make this dependend on settings (view menu?)
  self->priv->cpu_load=GTK_PROGRESS_BAR(gtk_progress_bar_new());
  gtk_tooltips_set_tip(tips,GTK_WIDGET(self->priv->cpu_load),_("CPU load"),NULL);
  gtk_box_pack_start(GTK_BOX(self),GTK_WIDGET(self->priv->cpu_load),FALSE,FALSE,1);
  self->priv->cpu_load_handler_id=g_timeout_add(1000, on_cpu_load_update, (gpointer)self);

  // register event handlers
  g_signal_connect(G_OBJECT(app), "notify::song", G_CALLBACK(on_song_changed), (gpointer)self);

  return(TRUE);
}

//-- constructor methods

/**
 * bt_main_statusbar_new:
 * @app: the application the statusbar belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
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
      g_object_try_weak_unref(self->priv->app);
      self->priv->app = BT_EDIT_APPLICATION(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->app);
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
  BtSong *song;

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);

  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  if(song) {
    BtSongInfo *song_info;
    BtSequence *sequence;

    GST_DEBUG("disconnect handlers from song=%p",song);
    g_object_get(G_OBJECT(song),"sequence",&sequence,"song-info",&song_info,NULL);
    
    g_signal_handlers_disconnect_matched(song,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_song_is_playing_notify,NULL);
    g_signal_handlers_disconnect_matched(song,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_song_play_pos_notify,NULL);
    g_signal_handlers_disconnect_matched(song_info,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_song_info_rhythm_notify,NULL);
    g_signal_handlers_disconnect_matched(sequence,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_sequence_loop_time_notify,NULL);
    g_object_unref(song_info);
    g_object_unref(sequence);
    g_object_unref(song);
  }
  g_source_remove(self->priv->cpu_load_handler_id);
  
  g_object_try_weak_unref(self->priv->app);
  
  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_main_statusbar_finalize(GObject *object) {
  //BtMainStatusbar *self = BT_MAIN_STATUSBAR(object);
  
  //GST_DEBUG("!!!! self=%p",self);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_main_statusbar_init(GTypeInstance *instance, gpointer g_class) {
  BtMainStatusbar *self = BT_MAIN_STATUSBAR(instance);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_MAIN_STATUSBAR, BtMainStatusbarPrivate);
}

static void bt_main_statusbar_class_init(BtMainStatusbarClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtMainStatusbarPrivate));

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
                                     BT_MAIN_STATUSBAR_DEFAULT, /* default value */
                                     G_PARAM_WRITABLE));
}

GType bt_main_statusbar_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      G_STRUCT_SIZE(BtMainStatusbarClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_main_statusbar_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtMainStatusbar),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_main_statusbar_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(GTK_TYPE_HBOX,"BtMainStatusbar",&info,0);
  }
  return type;
}
