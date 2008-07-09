/* $Id$
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
 * SECTION:btmainpagewaves
 * @short_description: the editor wavetable page
 *
 * Manage a list of audio clips files. Provides an embeded file browser to load
 * files. A waveform viewer can show the selected clip.
 */
/* @todo: in or below wavetable list:
 *  - add loop-mode combo = {off, forward, ping-pong}
 *  - add volume property (how is it applied?)
 * @todo: need envelop editor and everything for it
 * @todo: add segmented playback for loops
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
  G_POINTER_ALIAS(BtEditApplication *,app);
  /* the wavetable we are showing*/
  BtWavetable *wavetable;
  /* the waveform graph widget */
  GtkWidget *waveform_viewer;

  /* the toolbar widgets */
  GtkWidget *list_toolbar,*browser_toolbar,*editor_toolbar;
  GtkWidget *browser_stop,*wavetable_stop;
  GtkWidget *browser_play,*wavetable_play,*wavetable_clear;

  /* the list of wavetable entries */
  GtkTreeView *waves_list;

  /* the list of wavelevels */
  GtkTreeView *wavelevels_list;

  /* the sample chooser */
  GtkWidget *file_chooser;

  /* playbin for filechooser preview */
  GstElement *playbin;
  
  /* elements for wavetable preview */
  GstElement *preview, *fmt;
  gulong play_length,play_rate;
  guint play_channels;
  gint16 *play_data;
  BtWave *play_wave;
};

static GtkVBoxClass *parent_class=NULL;

enum {
  WAVE_TABLE_ID=0,
  WAVE_TABLE_NAME,
  WAVE_TABLE_CT
};
  
enum {
  WAVELEVEL_TABLE_ID=0,
  WAVELEVEL_TABLE_ROOT_NOTE,
  WAVELEVEL_TABLE_LENGTH,
  WAVELEVEL_TABLE_RATE,
  WAVELEVEL_TABLE_CHANNELS,
  WAVELEVEL_TABLE_LOOP_START,
  WAVELEVEL_TABLE_LOOP_END,
  WAVELEVEL_TABLE_CT
};

//-- event handler helper

static void on_waves_list_cursor_changed(GtkTreeView *treeview,gpointer user_data);
static void on_wavelevels_list_cursor_changed(GtkTreeView *treeview,gpointer user_data);

static BtWave *waves_list_get_current(const BtMainPageWaves *self) {
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  BtWave *wave=NULL;

  selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(self->priv->waves_list));
  if(gtk_tree_selection_get_selected(selection, &model, &iter)) {
    gulong id;

    gtk_tree_model_get(model,&iter,WAVE_TABLE_ID,&id,-1);
    wave=bt_wavetable_get_wave_by_index(self->priv->wavetable,id);
  }
  return(wave);
}

static BtWavelevel *wavelevels_list_get_current(const BtMainPageWaves *self,BtWave *wave) {
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  BtWavelevel *wavelevel=NULL;

  selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(self->priv->wavelevels_list));
  if(gtk_tree_selection_get_selected(selection, &model, &iter)) {
    gulong id;

    gtk_tree_model_get(model,&iter,WAVELEVEL_TABLE_ID,&id,-1);
    wavelevel=bt_wave_get_level_by_index(wave,id);
  }
  return(wavelevel);
}

/*
 * waves_list_refresh:
 * @self: the waves page
 *
 * Build the list of waves from the songs wavetable
 */
static void waves_list_refresh(const BtMainPageWaves *self) {
  BtWave *wave;
  GtkListStore *store;
  GtkTreeIter tree_iter;
  GtkTreeSelection *selection;
  GtkTreePath *path=NULL;
  GtkTreeModel *old_store;
  gchar *str;
  gint i;
  gboolean have_selection=FALSE;

  GST_INFO("refresh waves list: self=%p, wavetable=%p",self,self->priv->wavetable);

  store=gtk_list_store_new(WAVE_TABLE_CT,G_TYPE_ULONG,G_TYPE_STRING);

  //-- append waves rows (buzz numbers them from 0x01 to 0xC8=200)
  for(i=0;i<200;i++) {
    gtk_list_store_append(store, &tree_iter);
    gtk_list_store_set(store,&tree_iter,WAVE_TABLE_ID,i,-1);
    if((wave=bt_wavetable_get_wave_by_index(self->priv->wavetable,i))) {
      g_object_get(G_OBJECT(wave),"name",&str,NULL);
      GST_INFO("  adding [%3d] \"%s\"",i,str);
      gtk_list_store_set(store,&tree_iter,WAVE_TABLE_NAME,str,-1);
      g_free(str);
      g_object_unref(wave);
    }
  }

  // get old selection or get first item
  selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(self->priv->waves_list));
  if(gtk_tree_selection_get_selected(selection, &old_store, &tree_iter)) {
    path=gtk_tree_model_get_path(old_store,&tree_iter);
  }

  gtk_tree_view_set_model(self->priv->waves_list,GTK_TREE_MODEL(store));
  if(path) {
    have_selection=gtk_tree_model_get_iter(GTK_TREE_MODEL(store),&tree_iter,path);
    gtk_tree_path_free(path);
  }
  else {
    have_selection=gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store),&tree_iter);
  }
  if(have_selection) {
    gtk_tree_selection_select_iter(selection,&tree_iter);
  }
  on_waves_list_cursor_changed(GTK_TREE_VIEW(self->priv->waves_list), (gpointer)self);

  g_object_unref(store); // drop with treeview
}

/*
 * wavelevels_list_refresh:
 * @self: the waves page
 * @wave: the wave that
 *
 * Build the list of wavelevels for the given @wave
 */
static void wavelevels_list_refresh(const BtMainPageWaves *self,const BtWave *wave) {
  GtkListStore *store;
  GtkTreeSelection *selection;
    GtkTreeIter tree_iter;

  GST_INFO("refresh wavelevels list: self=%p, wave=%p",self,wave);

  store=gtk_list_store_new(WAVELEVEL_TABLE_CT,G_TYPE_ULONG,G_TYPE_STRING,G_TYPE_ULONG,G_TYPE_ULONG,G_TYPE_UINT,G_TYPE_LONG,G_TYPE_LONG);

  if(wave) {
    BtWavelevel *wavelevel;
    GList *node,*list;
    guchar root_note;
    guint channels;
    gulong length,rate;
    glong loop_start,loop_end;
    gulong i=0;

    //-- append wavelevels rows
    g_object_get(G_OBJECT(wave),"wavelevels",&list,NULL);
    for(node=list;node;node=g_list_next(node),i++) {
      wavelevel=BT_WAVELEVEL(node->data);
      g_object_get(G_OBJECT(wavelevel),
        "root-note",&root_note,
        "length",&length,
        "loop-start",&loop_start,
        "loop-end",&loop_end,
        "channels",&channels,
        "rate",&rate,
        NULL);
      gtk_list_store_append(store, &tree_iter);
      gtk_list_store_set(store,&tree_iter,
        WAVELEVEL_TABLE_ID,i,
        WAVELEVEL_TABLE_ROOT_NOTE,gst_note_2_frequency_note_number_2_string(root_note),
        WAVELEVEL_TABLE_LENGTH,length,
        WAVELEVEL_TABLE_RATE,rate,
        WAVELEVEL_TABLE_CHANNELS,channels,
        WAVELEVEL_TABLE_LOOP_START,loop_start,
        WAVELEVEL_TABLE_LOOP_END,loop_end,
        -1);
    }
    g_list_free(list);
  }

  gtk_tree_view_set_model(self->priv->wavelevels_list,GTK_TREE_MODEL(store));

  selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(self->priv->wavelevels_list));
  if(!gtk_tree_selection_get_selected(selection, NULL, NULL)) {
    if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store),&tree_iter))
      gtk_tree_selection_select_iter(selection,&tree_iter);
  }

  g_object_unref(store); // drop with treeview 
}

static BtWavelevel *wavelevels_get_wavelevel_and_set_iter(BtMainPageWaves *self,GtkTreeIter *iter,GtkTreeModel **store,gchar *path_string) {
  BtWavelevel *wavelevel=NULL;
  BtWave *wave;
  
  g_assert(iter);
  g_assert(store);
 
  if((wave=waves_list_get_current(self))) {
    if((*store=gtk_tree_view_get_model(self->priv->wavelevels_list))) {
      if(gtk_tree_model_get_iter_from_string(*store,iter,path_string)) {
        gulong id;
  
        gtk_tree_model_get(*store,iter,WAVELEVEL_TABLE_ID,&id,-1);
        wavelevel=bt_wave_get_level_by_index(wave,id);
      }
    }
    g_object_unref(wave);
  }
  return(wavelevel);
}

//-- event handler

static void on_playbin_state_changed(GstBus * bus, GstMessage * message, gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);

  if(GST_MESSAGE_SRC(message) == GST_OBJECT(self->priv->playbin)) {
    GstState oldstate,newstate,pending;

    gst_message_parse_state_changed(message,&oldstate,&newstate,&pending);
    GST_INFO("state change on the bin: %s -> %s",gst_element_state_get_name(oldstate),gst_element_state_get_name(newstate));
    switch(GST_STATE_TRANSITION(oldstate,newstate)) {
      case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
        gtk_widget_set_sensitive(self->priv->browser_stop,TRUE);
        break;
      case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
        gtk_widget_set_sensitive(self->priv->browser_stop,FALSE);
        gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(self->priv->browser_play),FALSE);
        break;
      default:
        break;
    }
  }
}

static void on_preview_state_changed(GstBus * bus, GstMessage * message, gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);

  if(GST_MESSAGE_SRC(message) == GST_OBJECT(self->priv->preview)) {
    GstState oldstate,newstate,pending;

    gst_message_parse_state_changed(message,&oldstate,&newstate,&pending);
    GST_INFO("state change on the bin: %s -> %s",gst_element_state_get_name(oldstate),gst_element_state_get_name(newstate));
    switch(GST_STATE_TRANSITION(oldstate,newstate)) {
      case GST_STATE_CHANGE_READY_TO_PAUSED:
        /* if loop
        if(!(gst_element_send_event(GST_ELEMENT(self->priv->preview),gst_event_ref(self->priv->play_seek_event)))) {
          GST_WARNING("bin failed to handle seek event");
        }
        */
        break;
      case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
        gtk_widget_set_sensitive(self->priv->wavetable_stop,TRUE);
        break;
      case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
        gtk_widget_set_sensitive(self->priv->wavetable_stop,FALSE);
        gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(self->priv->wavetable_play),FALSE);
        break;
      default:
        break;
    }
  }
}

static void on_preview_eos(GstBus * bus, GstMessage * message, gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);

  if(GST_MESSAGE_SRC(message) == GST_OBJECT(self->priv->preview)) {
    GstStateChangeReturn ret;

    ret=gst_element_set_state(self->priv->preview,GST_STATE_READY);
    GST_INFO("received eos on the bin, going to NULL, ret=%d",ret);
    self->priv->play_wave=NULL;
  }
}

static void on_preview_segment_done(GstBus * bus, GstMessage * message, gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);

  if(GST_MESSAGE_SRC(message) == GST_OBJECT(self->priv->preview)) {
    GST_INFO("received segment_done on the bin, looping");
    /*
    if(!(gst_element_send_event(GST_ELEMENT(self->priv->preview),gst_event_ref(self->priv->loop_seek_event)))) {
      GST_WARNING("element failed to handle continuing play seek event");
    }
    */
  }
}

static void on_wave_name_edited(GtkCellRendererText *cellrenderertext,gchar *path_string,gchar *new_text,gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);
  GtkTreeModel *store;
 
  if((store=gtk_tree_view_get_model(self->priv->waves_list))) {
    GtkTreeIter iter;

    if(gtk_tree_model_get_iter_from_string(store,&iter,path_string)) {
      BtWave *wave;
      gulong id;

      gtk_tree_model_get(store,&iter,WAVE_TABLE_ID,&id,-1);
      if((wave=bt_wavetable_get_wave_by_index(self->priv->wavetable,id))) {
        g_object_set(wave,"name",new_text,NULL); 
        gtk_list_store_set(GTK_LIST_STORE(store),&iter,WAVE_TABLE_NAME,new_text,-1);
        g_object_unref(wave);
      }
    }
  }
}

static void on_wavelevel_root_note_edited(GtkCellRendererText *cellrenderertext,gchar *path_string,gchar *new_text,gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);
  GtkTreeIter iter;
  GtkTreeModel *store=NULL;
  BtWavelevel *wavelevel;
  
  if((wavelevel=wavelevels_get_wavelevel_and_set_iter(self,&iter,&store,path_string))) {
    guchar root_note=gst_note_2_frequency_note_string_2_number(new_text);
    
    if(root_note) {
      g_object_set(wavelevel,"root-note",root_note,NULL); 
      gtk_list_store_set(GTK_LIST_STORE(store),&iter,WAVELEVEL_TABLE_ROOT_NOTE,gst_note_2_frequency_note_number_2_string(root_note),-1);
    }
    g_object_unref(wavelevel);
  }
}

static void on_wavelevel_rate_edited(GtkCellRendererText *cellrenderertext,gchar *path_string,gchar *new_text,gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);
  GtkTreeIter iter;
  GtkTreeModel *store=NULL;
  BtWavelevel *wavelevel;
  
  if((wavelevel=wavelevels_get_wavelevel_and_set_iter(self,&iter,&store,path_string))) {
    gulong rate=atol(new_text);

    g_object_set(wavelevel,"rate",rate,NULL); 
    gtk_list_store_set(GTK_LIST_STORE(store),&iter,WAVELEVEL_TABLE_RATE,rate,-1);
    g_object_unref(wavelevel);
  }
}

static void on_wavelevel_loop_start_edited(GtkCellRendererText *cellrenderertext,gchar *path_string,gchar *new_text,gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);
  GtkTreeIter iter;
  GtkTreeModel *store=NULL;
  BtWavelevel *wavelevel;
  
  if((wavelevel=wavelevels_get_wavelevel_and_set_iter(self,&iter,&store,path_string))) {
    glong loop_start=atol(new_text);
    
    g_object_set(wavelevel,"loop-start",loop_start,NULL);
    g_object_get(wavelevel,"loop-start",&loop_start,NULL);
    gtk_list_store_set(GTK_LIST_STORE(store),&iter,WAVELEVEL_TABLE_LOOP_START,loop_start,-1);
    g_object_unref(wavelevel);
  }
}

static void on_wavelevel_loop_end_edited(GtkCellRendererText *cellrenderertext,gchar *path_string,gchar *new_text,gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);
  GtkTreeIter iter;
  GtkTreeModel *store=NULL;
  BtWavelevel *wavelevel;
  
  if((wavelevel=wavelevels_get_wavelevel_and_set_iter(self,&iter,&store,path_string))) {
    glong loop_end=atol(new_text);

    g_object_set(wavelevel,"loop-end",loop_end,NULL);
    g_object_get(wavelevel,"loop-end",&loop_end,NULL);
    gtk_list_store_set(GTK_LIST_STORE(store),&iter,WAVELEVEL_TABLE_LOOP_END,loop_end,-1);
    g_object_unref(wavelevel);
  }
}

static void on_wave_loading_done(BtWave *wave,gboolean success,gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);
  
  if(success) {
    waves_list_refresh(self);
    GST_DEBUG("loading done, refreshing");
  }
  else {
    // @todo: error message
    GST_WARNING("loading the wave failed");
  }
}

static void on_waves_list_cursor_changed(GtkTreeView *treeview,gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);
  BtWave *wave;

  g_assert(user_data);

  GST_DEBUG("waves list cursor changed");
  wave=waves_list_get_current(self);
  wavelevels_list_refresh(self,wave);
  if(wave) {
    // enable toolbar buttons
    gtk_widget_set_sensitive(self->priv->wavetable_play,TRUE);
    gtk_widget_set_sensitive(self->priv->wavetable_clear,TRUE);
  }
  else {
    // disable toolbar buttons
    gtk_widget_set_sensitive(self->priv->wavetable_play,FALSE);
    gtk_widget_set_sensitive(self->priv->wavetable_clear,FALSE);
  }
  on_wavelevels_list_cursor_changed(self->priv->wavelevels_list, self);
}

static void on_wavelevels_list_cursor_changed(GtkTreeView *treeview,gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);
  BtWave *wave;
  gboolean drawn=FALSE;

  GST_DEBUG("wavelevels list cursor changed");
  if((wave=waves_list_get_current(self))) {
    BtWavelevel *wavelevel;
  
    if((wavelevel=wavelevels_list_get_current(self,wave))) {
      int16_t *data;
      guint channels;
      gulong length;
  
      g_object_get(G_OBJECT(wavelevel),"length",&length,"channels",&channels,"data",&data,NULL);  
      bt_waveform_viewer_set_wave(BT_WAVEFORM_VIEWER(self->priv->waveform_viewer),data,channels,length);
      g_object_unref(wavelevel);
      drawn=TRUE;
    }
    else {
      GST_WARNING("no current wavelevel");
    }
    g_object_unref(wave);
  }
  else {
    GST_INFO("no current wave");
  }
  if(!drawn) {
    bt_waveform_viewer_set_wave(BT_WAVEFORM_VIEWER(self->priv->waveform_viewer), NULL, 0, 0);
  }
}

static void on_song_changed(const BtEditApplication *app,GParamSpec *arg,gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);
  BtSong *song;
  BtWave *wave;

  g_assert(user_data);

  GST_INFO("song has changed : app=%p, self=%p",app,self);
  // get song from app and then setup from song
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  if(!song) return;
  GST_INFO("song->ref_ct=%d",G_OBJECT(song)->ref_count);

  g_object_try_unref(self->priv->wavetable);
  g_object_get(song,"wavetable",&self->priv->wavetable,NULL);
  // update page
  if((wave=waves_list_get_current(self))) {
    g_signal_connect(G_OBJECT(wave),"loading-done",G_CALLBACK(on_wave_loading_done),(gpointer)self);
    g_object_unref(wave);
  }
  waves_list_refresh(self);
  on_wavelevels_list_cursor_changed(self->priv->waves_list,self);
  // release the references
  g_object_unref(song);
  GST_INFO("song has changed done");
}

static void on_toolbar_style_changed(const BtSettings *settings,GParamSpec *arg,gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);
  GtkToolbarStyle style;
  gchar *toolbar_style;

  g_object_get(G_OBJECT(settings),"toolbar-style",&toolbar_style,NULL);
  if(!BT_IS_STRING(toolbar_style)) return;

  GST_INFO("!!!  toolbar style has changed '%s'",toolbar_style);
  style=gtk_toolbar_get_style_from_string(toolbar_style);
  gtk_toolbar_set_style(GTK_TOOLBAR(self->priv->list_toolbar),style);
  gtk_toolbar_set_style(GTK_TOOLBAR(self->priv->browser_toolbar),style);
  gtk_toolbar_set_style(GTK_TOOLBAR(self->priv->editor_toolbar),style);
  g_free(toolbar_style);
}

static void on_default_sample_folder_changed(const BtSettings *settings,GParamSpec *arg,gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);
  gchar *sample_folder;

  g_object_get(G_OBJECT(settings),"sample-folder",&sample_folder,NULL);
  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (self->priv->file_chooser), sample_folder);
  g_free(sample_folder);
}

static void on_browser_toolbar_play_clicked(GtkToolButton *button, gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);
  gchar *uri;

  if(!gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(button)))
    return;

  // get current entry and play
  uri=gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(self->priv->file_chooser));
  GST_INFO("current uri : %s",uri);
  if(uri) {
    // stop previous play
    gst_element_set_state(self->priv->playbin,GST_STATE_READY);

    // ... and play
    g_object_set(self->priv->playbin,"uri",uri,NULL);
    gst_element_set_state(self->priv->playbin,GST_STATE_PLAYING);

    g_free(uri);
  }
}

static void on_browser_toolbar_stop_clicked(GtkToolButton *button, gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);

  gst_element_set_state(self->priv->playbin,GST_STATE_READY);
}

static void on_fakesrc_handoff (GstElement* object, GstBuffer* buf, GstPad* pad, gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);
  
  if(GST_BUFFER_OFFSET(buf)==0) {
    GST_BUFFER_DATA(buf)=(gpointer)self->priv->play_data;
    GST_BUFFER_SIZE(buf)=self->priv->play_length*sizeof(gint16)*self->priv->play_channels;
    GST_BUFFER_TIMESTAMP(buf)=G_GUINT64_CONSTANT(0);
    GST_BUFFER_DURATION(buf)=gst_util_uint64_scale_int(GST_SECOND,self->priv->play_length,self->priv->play_rate);
    
    GST_INFO("play seg %3"G_GUINT64_FORMAT" from ts %"GST_TIME_FORMAT
      " with duration %"GST_TIME_FORMAT" and size of %u",
      GST_BUFFER_OFFSET(buf),GST_TIME_ARGS(GST_BUFFER_TIMESTAMP(buf)),
      GST_TIME_ARGS(GST_BUFFER_DURATION(buf)),GST_BUFFER_SIZE(buf));
  }
}

static void on_wavetable_toolbar_play_clicked(GtkToolButton *button, gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);
  BtWave *wave;
  
  if(!gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(button)))
    return;

  if((wave=waves_list_get_current(self))) {
    BtWavelevel *wavelevel;

    self->priv->play_wave=wave;

    // get current wavelevel
    if((wavelevel=wavelevels_list_get_current(self,wave))) {
      GstCaps *caps;

      // create playback pipeline on demand
      if(!self->priv->preview) {
        GstElement *fakesrc, *ares, *aconv, *asink;
        GstBus *bus;
        
        self->priv->preview=gst_element_factory_make("pipeline",NULL);
        fakesrc=gst_element_factory_make("fakesrc",NULL);
        self->priv->fmt=gst_element_factory_make("capsfilter",NULL);
        ares=gst_element_factory_make("audioresample",NULL);
        aconv=gst_element_factory_make("audioconvert",NULL);
        asink=gst_element_factory_make("autoaudiosink",NULL);
        gst_bin_add_many(GST_BIN(self->priv->preview),fakesrc,self->priv->fmt,ares,aconv,asink,NULL);
        gst_element_link_many(fakesrc,self->priv->fmt,ares,aconv,asink,NULL);
        
        bus=gst_element_get_bus(self->priv->preview);
        gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);
        g_signal_connect(bus, "message::state-changed", G_CALLBACK(on_preview_state_changed), (gpointer)self);
        g_signal_connect(bus, "message::eos", G_CALLBACK(on_preview_eos), (gpointer)self);
        g_signal_connect(bus, "message::segment-done", G_CALLBACK(on_preview_segment_done), (gpointer)self);
        gst_object_unref(bus);

        // register callback
        g_object_set(G_OBJECT(fakesrc),
          "signal-handoffs",TRUE,
          "sizetype", 1,
          "filltype", 1,
          "num-buffers", 1,
          "silent",TRUE,
          "sync",TRUE,
          "format",GST_FORMAT_TIME,
          NULL);
        g_signal_connect(G_OBJECT(fakesrc),"handoff",G_CALLBACK(on_fakesrc_handoff),(gpointer)self);
      }
      
      // get parameters and build caps
      g_object_get(G_OBJECT(wavelevel),
        "length",&self->priv->play_length,
        "channels",&self->priv->play_channels,
        "rate",&self->priv->play_rate,
        "data",&self->priv->play_data,
        NULL);
      caps=gst_caps_new_simple("audio/x-raw-int",
        "rate", G_TYPE_INT,self->priv->play_rate,
        "channels",G_TYPE_INT,self->priv->play_channels,
        "width",G_TYPE_INT,16,
        "endianness",G_TYPE_INT,G_BYTE_ORDER,
        "signedness",G_TYPE_INT,TRUE,
        NULL);
      g_object_set(self->priv->fmt,"caps",caps,NULL);
      gst_caps_unref(caps);
          
      // set playing
      gst_element_set_state(self->priv->preview,GST_STATE_PLAYING);
      g_object_unref(wavelevel);
    }
    else {
      GST_WARNING("no current wavelevel");
    }
    g_object_unref(wave);
  }
  else {
    GST_WARNING("no current wave");
  }
}

static void on_wavetable_toolbar_stop_clicked(GtkToolButton *button, gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);

  gst_element_set_state(self->priv->preview,GST_STATE_READY);
}

static void on_wavetable_toolbar_clear_clicked(GtkToolButton *button, gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);
  GtkTreeSelection *selection;
  GtkTreeModel     *model;
  GtkTreeIter       iter;

  selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(self->priv->waves_list));
  if(gtk_tree_selection_get_selected(selection, &model, &iter)) {
    BtWave *wave;
    gulong id;

    gtk_tree_model_get(model,&iter,WAVE_TABLE_ID,&id,-1);
    GST_INFO("selected entry id %d",id);

    wave=bt_wavetable_get_wave_by_index(self->priv->wavetable,id);
    bt_wavetable_remove_wave(self->priv->wavetable,wave);
    // update page
    waves_list_refresh(self);
    // release the references
    g_object_unref(wave);
  }
}

static void on_file_chooser_load_sample(GtkFileChooser *chooser, gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);
  GtkTreeSelection *selection;
  GtkTreeModel     *model;
  GtkTreeIter       iter;

  selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(self->priv->waves_list));
  if(gtk_tree_selection_get_selected(selection, &model, &iter)) {
    BtSong *song;
    BtWave *wave;
    gchar *uri,*name,*tmp_name,*ext;
    gulong id;

    gtk_tree_model_get(model,&iter,WAVE_TABLE_ID,&id,-1);
    GST_INFO("selected entry id %d",id);
    
    if((wave=waves_list_get_current(self))) {
      if(wave==self->priv->play_wave) {
        gst_element_set_state(self->priv->preview,GST_STATE_READY);
      }
      g_object_unref(wave);
      wave=NULL;
    }

    // get current entry and load
    uri=gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(self->priv->file_chooser));

    g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);

    // trim off protocol, path and extension
    tmp_name=g_filename_from_uri(uri,NULL,NULL);
    name=g_path_get_basename(tmp_name);
    if((ext=strrchr(name,'.'))) *ext='\0';
    g_free(tmp_name);
    wave=bt_wave_new(song,name,uri,id);
    // @idea: listen to status property on wave for loader updates
    
    // listen to wave::loaded signal and refresh the wave list on success
    g_signal_connect(G_OBJECT(wave),"loading-done",G_CALLBACK(on_wave_loading_done),(gpointer)self);

    // release the references
    g_object_unref(wave);
    g_object_unref(song);
  }
}

static void on_waves_list_row_activated(GtkTreeView *tree_view,GtkTreePath *path,GtkTreeViewColumn *column,gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);

  //on_wavetable_toolbar_play_clicked(GTK_TOOL_BUTTON(self->priv->wavetable_play),user_data);
  gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(self->priv->wavetable_play),TRUE);
}

static void on_wavelevels_list_row_activated(GtkTreeView *tree_view,GtkTreePath *path,GtkTreeViewColumn *column,gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);

  //on_wavetable_toolbar_play_clicked(GTK_TOOL_BUTTON(self->priv->wavetable_play),user_data);
  gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(self->priv->wavetable_play),TRUE);
}

//-- helper methods

static gboolean bt_main_page_waves_init_ui(const BtMainPageWaves *self,const BtMainPages *pages) {
  BtSettings *settings;
  GtkWidget *vpaned,*hpaned,*box,*box2;
  GtkWidget *tool_item;
  GtkWidget *scrolled_window;
  GtkCellRenderer *renderer;
#ifndef HAVE_GTK_2_12
  GtkTooltips *tips=gtk_tooltips_new();
#endif

  GST_DEBUG("!!!! self=%p",self);

  gtk_widget_set_name(GTK_WIDGET(self),_("wave table view"));

  g_object_get(G_OBJECT(self->priv->app),"settings",&settings,NULL);
  
  // vpane
  vpaned=gtk_vpaned_new();
  gtk_container_add(GTK_CONTAINER(self),vpaned);

  //   hpane
  hpaned=gtk_hpaned_new();
  gtk_paned_pack1(GTK_PANED(vpaned),GTK_WIDGET(hpaned),FALSE,FALSE);

  //     vbox (loaded sample list)
  box=gtk_vbox_new(FALSE,0);
  gtk_paned_pack1(GTK_PANED(hpaned),GTK_WIDGET(box),FALSE,FALSE);
  //       toolbar
  self->priv->list_toolbar=gtk_toolbar_new();
  gtk_widget_set_name(self->priv->list_toolbar,_("sample list tool bar"));

  // add buttons (play,stop,clear)
  self->priv->wavetable_play=tool_item=GTK_WIDGET(gtk_toggle_tool_button_new_from_stock(GTK_STOCK_MEDIA_PLAY));
  gtk_widget_set_name(tool_item,_("Play"));
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM(tool_item),_("Play current wave table entry"));
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->list_toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_wavetable_toolbar_play_clicked),(gpointer)self);
  gtk_widget_set_sensitive(tool_item,FALSE);

  self->priv->wavetable_stop=tool_item=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_STOP));
  gtk_widget_set_name(tool_item,_("Stop"));
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM(tool_item),_("Stop playback of current wave table entry"));
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->list_toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_wavetable_toolbar_stop_clicked),(gpointer)self);
  gtk_widget_set_sensitive(tool_item,FALSE);

  self->priv->wavetable_clear=tool_item=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_CLEAR));
  gtk_widget_set_name(tool_item,_("Clear"));
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM(tool_item),_("Clear current wave table entry"));
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->list_toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_wavetable_toolbar_clear_clicked),(gpointer)self);
  gtk_widget_set_sensitive(tool_item,FALSE);

  gtk_box_pack_start(GTK_BOX(box),self->priv->list_toolbar,FALSE,FALSE,0);

  //       wave listview
  scrolled_window=gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_ETCHED_IN);
  self->priv->waves_list=GTK_TREE_VIEW(gtk_tree_view_new());
  gtk_widget_set_name(GTK_WIDGET(self->priv->waves_list),_("wave list"));
  gtk_tree_selection_set_mode(gtk_tree_view_get_selection(self->priv->waves_list),GTK_SELECTION_BROWSE);
  g_object_set(self->priv->waves_list,"enable-search",FALSE,"rules-hint",TRUE,NULL);
  g_signal_connect(G_OBJECT(self->priv->waves_list),"cursor-changed",G_CALLBACK(on_waves_list_cursor_changed),(gpointer)self);
  g_signal_connect(G_OBJECT(self->priv->waves_list),"row-activated",G_CALLBACK(on_waves_list_row_activated),(gpointer)self);

  renderer=gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer),"xalign",1.0,NULL);
  gtk_tree_view_insert_column_with_attributes(self->priv->waves_list,-1,_("Ix"),renderer,"text",0,NULL);
  renderer=gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer),"mode",GTK_CELL_RENDERER_MODE_EDITABLE,"editable",TRUE,NULL);
  g_signal_connect(G_OBJECT(renderer),"edited",G_CALLBACK(on_wave_name_edited),(gpointer)self);
  gtk_tree_view_insert_column_with_attributes(self->priv->waves_list,-1,_("Wave"),renderer,"text",1,NULL);
  gtk_container_add(GTK_CONTAINER(scrolled_window),GTK_WIDGET(self->priv->waves_list));
  gtk_container_add(GTK_CONTAINER(box),scrolled_window);
  
  // @todo: add loop-mode combo and volume slider

  //     vbox (file browser)
  box=gtk_vbox_new(FALSE,0);
  gtk_paned_pack2(GTK_PANED(hpaned),GTK_WIDGET(box),FALSE,FALSE);
  //       toolbar
  self->priv->browser_toolbar=gtk_toolbar_new();
  gtk_widget_set_name(self->priv->browser_toolbar,_("sample browser tool bar"));

  // add buttons (play,stop,load)
  self->priv->browser_play=tool_item=GTK_WIDGET(gtk_toggle_tool_button_new_from_stock(GTK_STOCK_MEDIA_PLAY));
  gtk_widget_set_name(tool_item,_("Play"));
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM(tool_item),_("Play current sample"));
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->browser_toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_browser_toolbar_play_clicked),(gpointer)self);

  self->priv->browser_stop=tool_item=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_STOP));
  gtk_widget_set_name(tool_item,_("Stop"));
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM(tool_item),_("Stop playback of current sample"));
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->browser_toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_browser_toolbar_stop_clicked),(gpointer)self);
  gtk_widget_set_sensitive(tool_item,FALSE);

  tool_item=GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_OPEN));
  gtk_widget_set_name(tool_item,_("Open"));
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM(tool_item),_("Load current sample into selected wave table entry"));
  gtk_toolbar_insert(GTK_TOOLBAR(self->priv->browser_toolbar),GTK_TOOL_ITEM(tool_item),-1);
  //g_signal_connect(G_OBJECT(tool_item),"clicked",G_CALLBACK(on_browser_toolbar_open_clicked),(gpointer)self);

  gtk_box_pack_start(GTK_BOX(box),self->priv->browser_toolbar,FALSE,FALSE,0);

  //       file-chooser
  /* this causes warning on gtk 2.x
   * Gtk-CRITICAL **: gtk_file_system_path_is_local: assertion `path != NULL' failed
   * Gdk-CRITICAL **: gdk_drawable_get_size: assertion `GDK_IS_DRAWABLE (drawable)' failed
   */
  self->priv->file_chooser=gtk_file_chooser_widget_new(GTK_FILE_CHOOSER_ACTION_OPEN);
  g_signal_connect(G_OBJECT(self->priv->file_chooser),"file-activated",G_CALLBACK(on_file_chooser_load_sample),(gpointer)self);
  //g_signal_connect(G_OBJECT(self->priv->file_chooser),"update-preview",G_CALLBACK(on_file_chooser_info_sample),(gpointer)self);
  gtk_box_pack_start(GTK_BOX(box),self->priv->file_chooser,TRUE,TRUE,6);

  //   vbox (sample view)
  box=gtk_vbox_new(FALSE,0);
  gtk_paned_pack2(GTK_PANED(vpaned),GTK_WIDGET(box),FALSE,FALSE);
  //     toolbar
  self->priv->editor_toolbar=gtk_toolbar_new();
  gtk_widget_set_name(self->priv->editor_toolbar,_("sample edit tool bar"));

  gtk_box_pack_start(GTK_BOX(box),self->priv->editor_toolbar,FALSE,FALSE,0);

  //     hbox
  box2=gtk_hbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(box),box2);
  //       zone entries (multiple waves per sample (xm?)) -> (per entry: root key, length, rate, loope start, loop end
  scrolled_window=gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_ETCHED_IN);
  self->priv->wavelevels_list=GTK_TREE_VIEW(gtk_tree_view_new());
  gtk_widget_set_name(GTK_WIDGET(self->priv->wavelevels_list),_("wave-level list"));
  gtk_tree_selection_set_mode(gtk_tree_view_get_selection(self->priv->wavelevels_list),GTK_SELECTION_BROWSE);
  g_object_set(self->priv->wavelevels_list,"enable-search",FALSE,"rules-hint",TRUE,NULL);
  g_signal_connect(G_OBJECT(self->priv->wavelevels_list),"cursor-changed",G_CALLBACK(on_wavelevels_list_cursor_changed),(gpointer)self);
  g_signal_connect(G_OBJECT(self->priv->wavelevels_list),"row-activated",G_CALLBACK(on_wavelevels_list_row_activated),(gpointer)self);

  renderer=gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer),"mode",GTK_CELL_RENDERER_MODE_EDITABLE,"editable",TRUE,NULL);
  gtk_tree_view_insert_column_with_attributes(self->priv->wavelevels_list,-1,_("Root"),renderer,"text",WAVELEVEL_TABLE_ROOT_NOTE,NULL);
  g_signal_connect(G_OBJECT(renderer),"edited",G_CALLBACK(on_wavelevel_root_note_edited),(gpointer)self);
  renderer=gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(self->priv->wavelevels_list,-1,_("Length"),renderer,"text",WAVELEVEL_TABLE_LENGTH,NULL);
  renderer=gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer),"mode",GTK_CELL_RENDERER_MODE_EDITABLE,"editable",TRUE,NULL);
  gtk_tree_view_insert_column_with_attributes(self->priv->wavelevels_list,-1,_("Rate"),renderer,"text",WAVELEVEL_TABLE_RATE,NULL);
  g_signal_connect(G_OBJECT(renderer),"edited",G_CALLBACK(on_wavelevel_rate_edited),(gpointer)self);
  renderer=gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(self->priv->wavelevels_list,-1,_("Channels"),renderer,"text",WAVELEVEL_TABLE_CHANNELS,NULL);
  renderer=gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer),"mode",GTK_CELL_RENDERER_MODE_EDITABLE,"editable",TRUE,NULL);
  gtk_tree_view_insert_column_with_attributes(self->priv->wavelevels_list,-1,_("Loop start"),renderer,"text",WAVELEVEL_TABLE_LOOP_START,NULL);
  g_signal_connect(G_OBJECT(renderer),"edited",G_CALLBACK(on_wavelevel_loop_start_edited),(gpointer)self);
  renderer=gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer),"mode",GTK_CELL_RENDERER_MODE_EDITABLE,"editable",TRUE,NULL);
  gtk_tree_view_insert_column_with_attributes(self->priv->wavelevels_list,-1,_("Loop end"),renderer,"text",WAVELEVEL_TABLE_LOOP_END,NULL);
  g_signal_connect(G_OBJECT(renderer),"edited",G_CALLBACK(on_wavelevel_loop_end_edited),(gpointer)self);
  gtk_container_add(GTK_CONTAINER(scrolled_window),GTK_WIDGET(self->priv->wavelevels_list));
  gtk_box_pack_start(GTK_BOX(box2),scrolled_window,FALSE,FALSE,0);

  //       sampleview (which widget do we need?)
  //       properties (loop, envelope, ...)
  self->priv->waveform_viewer = bt_waveform_viewer_new();
  gtk_box_pack_start(GTK_BOX(box2),self->priv->waveform_viewer,TRUE,TRUE,0);

  // register event handlers
  g_signal_connect(G_OBJECT(self->priv->app), "notify::song", G_CALLBACK(on_song_changed), (gpointer)self);

  // let settings control toolbar style and listen to other settings changes
  on_toolbar_style_changed(settings,NULL,(gpointer)self);
  on_default_sample_folder_changed(settings,NULL,(gpointer)self);
  g_signal_connect(G_OBJECT(settings), "notify::toolbar-style", G_CALLBACK(on_toolbar_style_changed), (gpointer)self);
  g_signal_connect(G_OBJECT(settings), "notify::sample-folder", G_CALLBACK(on_default_sample_folder_changed), (gpointer)self);

  g_object_unref(settings);

  GST_DEBUG("  done");
  return(TRUE);
}

//-- constructor methods

/**
 * bt_main_page_waves_new:
 * @app: the application the window belongs to
 * @pages: the page collection
 *
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtMainPageWaves *bt_main_page_waves_new(const BtEditApplication *app,const BtMainPages *pages) {
  BtMainPageWaves *self;
  GstBus *bus;

  if(!(self=BT_MAIN_PAGE_WAVES(g_object_new(BT_TYPE_MAIN_PAGE_WAVES,"app",app,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_main_page_waves_init_ui(self,pages)) {
    goto Error;
  }
  // create playbin
  self->priv->playbin=gst_element_factory_make("playbin",NULL);
  bus=gst_element_get_bus(self->priv->playbin);
  gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);
  g_signal_connect(bus, "message::state-changed", G_CALLBACK(on_playbin_state_changed), (gpointer)self);
  gst_object_unref(bus);
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
      g_object_try_weak_unref(self->priv->app);
      self->priv->app = BT_EDIT_APPLICATION(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->app);
      //GST_DEBUG("set the app for MAIN_PAGE_WAVES: %p",self->priv->app);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_main_page_waves_dispose(GObject *object) {
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES(object);
  GstBus *bus;
  
  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  g_object_try_unref(self->priv->wavetable);
  
  g_object_try_weak_unref(self->priv->app);

  // shut down loader-preview playbin
  gst_element_set_state(self->priv->playbin,GST_STATE_NULL);
  bus=gst_element_get_bus(GST_ELEMENT(self->priv->playbin));
  g_signal_handlers_disconnect_matched(bus,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_playbin_state_changed,(gpointer)self);
  gst_bus_remove_signal_watch(bus);
  gst_object_unref(bus);
  gst_object_unref(self->priv->playbin);
  
  // shut down wavetable-preview playbin
  if(self->priv->preview) {
    gst_element_set_state(self->priv->preview,GST_STATE_NULL);
    bus=gst_element_get_bus(GST_ELEMENT(self->priv->preview));
    g_signal_handlers_disconnect_matched(bus,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_preview_state_changed,(gpointer)self);
    gst_bus_remove_signal_watch(bus);
    gst_object_unref(bus); 
    gst_object_unref(self->priv->preview);
  }

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_main_page_waves_finalize(GObject *object) {
  //BtMainPageWaves *self = BT_MAIN_PAGE_WAVES(object);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_main_page_waves_init(GTypeInstance *instance, gpointer g_class) {
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_MAIN_PAGE_WAVES, BtMainPageWavesPrivate);
}

static void bt_main_page_waves_class_init(BtMainPageWavesClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtMainPageWavesPrivate));

  gobject_class->set_property = bt_main_page_waves_set_property;
  gobject_class->get_property = bt_main_page_waves_get_property;
  gobject_class->dispose      = bt_main_page_waves_dispose;
  gobject_class->finalize     = bt_main_page_waves_finalize;

  g_object_class_install_property(gobject_class,MAIN_PAGE_WAVES_APP,
                                  g_param_spec_object("app",
                                     "app contruct prop",
                                     "Set application object, the window belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));
}

GType bt_main_page_waves_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    const GTypeInfo info = {
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
