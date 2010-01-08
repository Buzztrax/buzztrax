/* $Id$
 *
 * Buzztard
 * Copyright (C) 2007 Buzztard team <buzztard-devel@lists.sf.net>
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
 * SECTION:btrenderprogress
 * @short_description: class for the editor render progress
 *
 * Song recording status dialog.
 */
/* @todo: 
 * turn off-level meters (and ev. open wire-analysis windows) to speed up recording
 */
#define BT_EDIT
#define BT_RENDER_PROGRESS_C

#include "bt-edit.h"

enum {
  RENDER_PROGRESS_APP=1,
  RENDER_PROGRESS_SETTINGS
};

struct _BtRenderProgressPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  G_POINTER_ALIAS(BtEditApplication *,app);

  /* dialog that has the settings */
  G_POINTER_ALIAS(BtRenderDialog *,settings);

  /* dialog widgets */
  GtkProgressBar *track_progress;
  GtkLabel *info;
};

static GtkProgressClass *parent_class=NULL;

//-- event handler

static void on_song_play_pos_notify(const BtSong *song,GParamSpec *arg,gpointer user_data) {
  BtRenderProgress *self=BT_RENDER_PROGRESS(user_data);
  BtSequence *sequence;
  gulong pos,length;
  gdouble progress;
  // the +4 is not really needed, but I get a stack smashing error on ubuntu without
  gchar str[3+2*(2+2+3+3) + 4];
  gulong msec1,sec1,min1,msec2,sec2,min2;
  GstClockTime bar_time;

  g_object_get(G_OBJECT(song),"sequence",&sequence,"play-pos",&pos,NULL);
  g_object_get(G_OBJECT(sequence),"length",&length,NULL);
  bar_time=bt_sequence_get_bar_time(sequence);

  progress=(gdouble)pos/(gdouble)length;
  if(progress>=1.0) {
    progress=1.0;
    bt_song_stop(song);
    gtk_dialog_response(GTK_DIALOG(self),GTK_RESPONSE_REJECT);
  }
  GST_INFO("progress %ld/%ld=%lf",pos,length,progress);

  msec1=(gulong)((pos*bar_time)/G_USEC_PER_SEC);
  min1=(gulong)(msec1/60000);msec1-=(min1*60000);
  sec1=(gulong)(msec1/ 1000);msec1-=(sec1* 1000);
  msec2=(gulong)((length*bar_time)/G_USEC_PER_SEC);
  min2=(gulong)(msec2/60000);msec2-=(min2*60000);
  sec2=(gulong)(msec2/ 1000);msec2-=(sec2* 1000);
  // format
  g_sprintf(str,"%02lu:%02lu.%03lu / %02lu:%02lu.%03lu",min1,sec1,msec1 ,min2,sec2,msec2);

  g_object_set(self->priv->track_progress,"fraction",progress,"text",str,NULL);

  g_object_unref(sequence);
}

//-- helper methods

static gboolean bt_render_progress_record(const BtRenderProgress *self, BtSong *song, BtSinkBin *sink_bin, gchar *file_name) {
  gchar *info;
  gboolean is_playing;

  g_object_set(sink_bin,"record-file-name",file_name,NULL);

  GST_INFO("recording to '%s'",file_name);
  info=g_strdup_printf(_("Recording to: %s"),file_name);
  gtk_label_set_text(self->priv->info,info);
  g_free(info);

  bt_song_play(song);
  gtk_dialog_run(GTK_DIALOG(self));
  g_object_get(G_OBJECT(song),"is-playing",&is_playing,NULL);
  if(!is_playing) {
    return(TRUE);
  }
  else {
    bt_song_stop(song);
    return FALSE;
  }
}


static gboolean bt_render_progress_init_ui(const BtRenderProgress *self) {
  GtkWidget *box;

  gtk_widget_set_name(GTK_WIDGET(self),"song render progress");

  gtk_window_set_title(GTK_WINDOW(self), _("song render progress"));

  // add progress commision widgets (okay, cancel)
  gtk_dialog_add_buttons(GTK_DIALOG(self),
                          GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT,
                          NULL);

  gtk_dialog_set_default_response(GTK_DIALOG(self),GTK_RESPONSE_REJECT);

  // add widgets to the progress content area
  box=GTK_DIALOG(self)->vbox;  //gtk_vbox_new(FALSE,12);
  gtk_box_set_spacing(GTK_BOX(box),12);
  gtk_container_set_border_width(GTK_CONTAINER(box),6);


  self->priv->info=GTK_LABEL(gtk_label_new(""));
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->priv->info),FALSE,FALSE,0);

  self->priv->track_progress=GTK_PROGRESS_BAR(gtk_progress_bar_new());
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->priv->track_progress),FALSE,FALSE,0);

  return(TRUE);
}

//-- constructor methods

/**
 * bt_render_progress_new:
 * @app: the application the progress-dialog belongs to
 * @settings: the settings for the rendering
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtRenderProgress *bt_render_progress_new(const BtEditApplication *app,BtRenderDialog *settings) {
  BtRenderProgress *self;

  if(!(self=BT_RENDER_PROGRESS(g_object_new(BT_TYPE_RENDER_PROGRESS,"app",app,"settings",settings,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_render_progress_init_ui(self)) {
    goto Error;
  }
  return(self);
Error:
  if(self) gtk_object_destroy(GTK_OBJECT(self));
  return(NULL);
}

//-- methods

/**
 * bt_render_progress_run:
 * @self: the progress-dialog
 *
 * Run the rendering and show the progress.
 */
void bt_render_progress_run(const BtRenderProgress *self) {
  BtSong *song;
  BtSetup *setup;
  BtSongInfo *song_info;
  BtMachine *machine;
  gboolean unsaved;

  g_object_get(self->priv->app,"song",&song,NULL);
  g_object_get(G_OBJECT(song),"setup",&setup,"song-info",&song_info,"unsaved",&unsaved,NULL);

  // lookup the audio-sink machine and change mode
  if((machine=bt_setup_get_machine_by_type(setup,BT_TYPE_SINK_MACHINE))) {
    gchar *file_name;
    BtSinkBinRecordFormat format;
    BtRenderMode mode;
    BtSinkBin *sink_bin;

    g_object_get(G_OBJECT(machine),"machine",&sink_bin,NULL);
    g_object_get(G_OBJECT(self->priv->settings),"format",&format,"mode",&mode,"file-name",&file_name,NULL);

    g_signal_connect(G_OBJECT(song), "notify::play-pos", G_CALLBACK(on_song_play_pos_notify), (gpointer)self);

    g_object_set(sink_bin,
      "mode",BT_SINK_BIN_MODE_RECORD,
      "record-format",format,
      NULL);

    if(mode==BT_RENDER_MODE_MIXDOWN) {
      bt_render_progress_record(self,song,sink_bin,file_name);
    }
    else {
      BtMachine *machine;
      GList *list,*node;
      gchar *track_file_name;
      gchar *song_name,*track_name,*id;
      guint track=0;
      gboolean res;

      g_object_get(G_OBJECT(song_info),"name",&song_name,NULL);

      list=bt_setup_get_machines_by_type(setup,BT_TYPE_SOURCE_MACHINE);
      for(node=list;node;node=g_list_next(node)) {
        machine=BT_MACHINE(node->data);
        g_object_set(G_OBJECT(machine),"state",BT_MACHINE_STATE_SOLO,NULL);

        g_object_get(G_OBJECT(machine),"id",&id,NULL);
        track_name=g_strdup_printf("%s : %s",song_name,id);
        g_object_set(G_OBJECT(song_info),"name",track_name,NULL);
        g_free(track_name);
        g_free(id);

        track_file_name=g_strdup_printf(file_name,track);
        res=bt_render_progress_record(self,song,sink_bin,track_file_name);
        g_free(track_file_name);

        g_object_set(G_OBJECT(machine),"state",BT_MACHINE_STATE_NORMAL,NULL);
        if(!res) break;
        track++;
      }
      g_object_set(G_OBJECT(song_info),"name",song_name,NULL);
      g_list_free(list);
      g_free(song_name);
      bt_song_set_unsaved(song,unsaved);
    }

    g_signal_handlers_disconnect_matched(song,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_song_play_pos_notify,NULL);

    g_object_set(sink_bin,
      "mode",BT_SINK_BIN_MODE_PLAY,
      NULL);

    g_free(file_name);
    gst_object_unref(sink_bin);
    g_object_unref(machine);
  }
  g_object_unref(song_info);
  g_object_unref(setup);
  g_object_unref(song);
}

//-- wrapper

//-- class internals

static void bt_render_progress_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtRenderProgress *self = BT_RENDER_PROGRESS(object);
  return_if_disposed();
  switch (property_id) {
    case RENDER_PROGRESS_APP: {
      g_object_try_weak_unref(self->priv->app);
      self->priv->app = BT_EDIT_APPLICATION(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->app);
      //GST_DEBUG("set the app for render_progress: %p",self->priv->app);
    } break;
    case RENDER_PROGRESS_SETTINGS: {
      g_object_try_weak_unref(self->priv->settings);
      self->priv->settings = BT_RENDER_DIALOG(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->settings);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_render_progress_dispose(GObject *object) {
  BtRenderProgress *self = BT_RENDER_PROGRESS(object);
  BtSong *song;
 
  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);

  g_object_get(self->priv->app,"song",&song,NULL);
  if(song) {
    g_signal_handlers_disconnect_matched(G_OBJECT(song),G_SIGNAL_MATCH_DATA,0,0,NULL,NULL,(gpointer)self);
    g_object_unref(song);
  }
  
  g_object_try_weak_unref(self->priv->app);
  g_object_try_weak_unref(self->priv->settings);

  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_render_progress_finalize(GObject *object) {
  //BtRenderProgress *self = BT_RENDER_PROGRESS(object);

  //GST_DEBUG("!!!! self=%p",self);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_render_progress_init(GTypeInstance *instance, gpointer g_class) {
  BtRenderProgress *self = BT_RENDER_PROGRESS(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_RENDER_PROGRESS, BtRenderProgressPrivate);
}

static void bt_render_progress_class_init(BtRenderProgressClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtRenderProgressPrivate));

  gobject_class->set_property = bt_render_progress_set_property;
  gobject_class->dispose      = bt_render_progress_dispose;
  gobject_class->finalize     = bt_render_progress_finalize;

  g_object_class_install_property(gobject_class,RENDER_PROGRESS_APP,
                                  g_param_spec_object("app",
                                     "app construct prop",
                                     "Set application object, the progress belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_WRITABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,RENDER_PROGRESS_SETTINGS,
                                  g_param_spec_object("settings",
                                     "settings construct prop",
                                     "Set settings object, the progress dialog handles",
                                     BT_TYPE_RENDER_DIALOG, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_WRITABLE|G_PARAM_STATIC_STRINGS));

}

GType bt_render_progress_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof(BtRenderProgressClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_render_progress_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtRenderProgress),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_render_progress_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(GTK_TYPE_DIALOG,"BtRenderProgress",&info,0);
  }
  return type;
}
