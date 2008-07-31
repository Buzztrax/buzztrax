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
/* @todo: need envelop editor and everything for it
 * @todo: update loops
 * - if on_loop_mode_changed(), on_wavelevel_loop_start/end_edited()
 *   for current playing -> update seeks
 * @todo: use rate for playing waves, when base_note has changed
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
  /* and their parameters */
  GtkHScale *volume;
  GtkComboBox *loop_mode;

  /* the list of wavelevels */
  GtkTreeView *wavelevels_list;

  /* the sample chooser */
  GtkWidget *file_chooser;

  /* playbin for filechooser preview */
  GstElement *playbin;
  /* seek events */
  GstEvent *play_seek_event;
  GstEvent *loop_seek_event[2];
  gint loop_seek_dir;
  
  /* elements for wavetable preview */
  GstElement *preview, *preview_src;
  BtWave *play_wave;

  /* the query is used in update_playback_position */
  GstQuery *position_query;
  /* update handler id */
  guint preview_update_id;
};

static GtkVBoxClass *parent_class=NULL;

enum {
  WAVE_TABLE_ID=0,
  WAVE_TABLE_HEX_ID,
  WAVE_TABLE_NAME,
  WAVE_TABLE_CT
};
  
enum {
  WAVELEVEL_TABLE_ID=0,
  WAVELEVEL_TABLE_ROOT_NOTE,
  WAVELEVEL_TABLE_LENGTH,
  WAVELEVEL_TABLE_RATE,
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
  gchar *str,hstr[3];
  gint i;
  gboolean have_selection=FALSE;

  GST_INFO("refresh waves list: self=%p, wavetable=%p",self,self->priv->wavetable);

  store=gtk_list_store_new(WAVE_TABLE_CT,G_TYPE_ULONG,G_TYPE_STRING,G_TYPE_STRING);

  // append waves rows (buzz numbers them from 0x01 to 0xC8=200)
  for(i=1;i<=200;i++) {
    gtk_list_store_append(store, &tree_iter);
    // buzz shows index as hex, because trackers needs it this way
    sprintf(hstr,"%02x",i);
    gtk_list_store_set(store,&tree_iter,WAVE_TABLE_ID,i,WAVE_TABLE_HEX_ID,hstr,-1);
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
  }
  else {
    have_selection=gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store),&tree_iter);
    path=gtk_tree_path_new_from_indices(0,-1);
  }
  if(have_selection) {
    gtk_tree_selection_select_iter(selection,&tree_iter);
    gtk_tree_view_scroll_to_cell(self->priv->waves_list,path,NULL,TRUE,0.5,0.5);
    
  }
  if(path) {
    gtk_tree_path_free(path);
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

  store=gtk_list_store_new(WAVELEVEL_TABLE_CT,G_TYPE_ULONG,G_TYPE_STRING,G_TYPE_ULONG,G_TYPE_ULONG,G_TYPE_LONG,G_TYPE_LONG);

  if(wave) {
    BtWavelevel *wavelevel;
    GList *node,*list;
    guchar root_note;
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
        "rate",&rate,
        NULL);
      gtk_list_store_append(store, &tree_iter);
      gtk_list_store_set(store,&tree_iter,
        WAVELEVEL_TABLE_ID,i,
        WAVELEVEL_TABLE_ROOT_NOTE,gst_note_2_frequency_note_number_2_string(root_note),
        WAVELEVEL_TABLE_LENGTH,length,
        WAVELEVEL_TABLE_RATE,rate,
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

static void preview_stop(const BtMainPageWaves *self) {
  if(self->priv->preview_update_id) {
    g_source_remove(self->priv->preview_update_id);
    self->priv->preview_update_id=0;
    g_object_set(self->priv->waveform_viewer,"playback-cursor",G_GINT64_CONSTANT(-1),NULL);
  }
  /* if I set state directly to NULL, I don't get inbetween state-change messages */
  gst_element_set_state(self->priv->preview,GST_STATE_READY);
  self->priv->play_wave=NULL;
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
        gst_element_set_state(self->priv->playbin,GST_STATE_NULL);
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
      case GST_STATE_CHANGE_NULL_TO_READY:
        if(!(gst_element_send_event(GST_ELEMENT(self->priv->preview),gst_event_ref(self->priv->play_seek_event)))) {
          GST_WARNING("bin failed to handle seek event");
        }
        //gst_element_set_state(self->priv->preview,GST_STATE_PLAYING);
        break;
      case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
        gtk_widget_set_sensitive(self->priv->wavetable_stop,TRUE);
        break;
      case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
        gtk_widget_set_sensitive(self->priv->wavetable_stop,FALSE);
        gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(self->priv->wavetable_play),FALSE);
        gst_element_set_state(self->priv->preview,GST_STATE_NULL);
        break;
      default:
        break;
    }
  }
}

static void on_preview_eos(GstBus * bus, GstMessage * message, gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);

  if(GST_MESSAGE_SRC(message) == GST_OBJECT(self->priv->preview)) {
    GST_INFO("received eos on the bin");
    preview_stop(self);
  }
}

static void on_preview_segment_done(GstBus * bus, GstMessage * message, gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);

  if(GST_MESSAGE_SRC(message) == GST_OBJECT(self->priv->preview)) {
    GST_INFO("received segment_done on the bin, looping %d",self->priv->loop_seek_dir);
    if(!(gst_element_send_event(GST_ELEMENT(self->priv->preview),gst_event_ref(self->priv->loop_seek_event[self->priv->loop_seek_dir])))) {
      GST_WARNING("element failed to handle continuing play seek event");
      /* if I set state directly to NULL, I don't get inbetween state-change messages */
      gst_element_set_state(self->priv->preview,GST_STATE_READY);
    }
    self->priv->loop_seek_dir=1-self->priv->loop_seek_dir;
  }
}

static void on_preview_error(const GstBus * const bus, GstMessage *message, gconstpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);
  GError *err = NULL;
  gchar *dbg = NULL;
  
  gst_message_parse_error(message, &err, &dbg);
  GST_WARNING ("ERROR: %s (%s)", err->message, (dbg) ? dbg : "no details");
  g_error_free (err);
  g_free (dbg);
  preview_stop(self);
}

static void on_preview_warning(const GstBus * const bus, GstMessage * message, gconstpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);
  GError *err = NULL;
  gchar *dbg = NULL;
  
  gst_message_parse_warning(message, &err, &dbg);
  GST_WARNING ("WARNING: %s (%s)", err->message, (dbg) ? dbg : "no details");
  g_error_free (err);
  g_free (dbg);
  preview_stop(self);
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
    glong loop_start=atol(new_text), loop_end = -1;
    
    g_object_set(wavelevel,"loop-start",loop_start,NULL);
    g_object_get(wavelevel,"loop-start",&loop_start,"loop-end", &loop_end, NULL);
    if (loop_start == -1 && loop_end != -1)
    {
      loop_end = -1;
      g_object_set(wavelevel,"loop-end",loop_end,NULL);
    }
    if (loop_start != -1 && loop_end == -1)
    {
      g_object_get(wavelevel,"length", &loop_end,NULL);
      if (loop_end > 0)
        loop_end--;
      g_object_set(wavelevel,"loop-end",loop_end,NULL);
    }
    gtk_list_store_set(GTK_LIST_STORE(store),&iter,WAVELEVEL_TABLE_LOOP_START,loop_start,WAVELEVEL_TABLE_LOOP_END,loop_end,-1);
    if (loop_end != -1)
      loop_end++;
    g_object_set(G_OBJECT(self->priv->waveform_viewer), "loop-begin", (int64_t)loop_start, "loop-end", (int64_t)loop_end, NULL);
    g_object_unref(wavelevel);
  }
}

static void on_wavelevel_loop_end_edited(GtkCellRendererText *cellrenderertext,gchar *path_string,gchar *new_text,gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);
  GtkTreeIter iter;
  GtkTreeModel *store=NULL;
  BtWavelevel *wavelevel;
  
  if((wavelevel=wavelevels_get_wavelevel_and_set_iter(self,&iter,&store,path_string))) {
    glong loop_start = -1, loop_end=atol(new_text);

    g_object_set(wavelevel,"loop-end",loop_end,NULL);
    g_object_get(wavelevel,"loop-start",&loop_start,"loop-end", &loop_end, NULL);
    if (loop_start != -1 && loop_end == -1)
    {
      loop_start = -1;
      g_object_set(wavelevel,"loop-end",loop_end,NULL);
    }
    if (loop_start == -1 && loop_end != -1)
    {
      loop_start = 0;
      g_object_set(wavelevel,"loop-start",loop_start,NULL);
    }
    gtk_list_store_set(GTK_LIST_STORE(store),&iter,WAVELEVEL_TABLE_LOOP_START,loop_start,WAVELEVEL_TABLE_LOOP_END,loop_end,-1);
    if (loop_end != -1)
      loop_end++;
    g_object_set(G_OBJECT(self->priv->waveform_viewer), "loop-begin", (int64_t)loop_start, "loop-end", (int64_t)loop_end, NULL);
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
    gdouble volume;
    BtWaveLoopMode loop_mode;

    // enable toolbar buttons
    gtk_widget_set_sensitive(self->priv->wavetable_play,TRUE);
    gtk_widget_set_sensitive(self->priv->wavetable_clear,TRUE);
    // enable and update properties
    gtk_widget_set_sensitive(GTK_WIDGET(self->priv->volume),TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(self->priv->loop_mode),TRUE);
    g_object_get(wave,"volume",&volume,"loop-mode",&loop_mode,NULL);
    gtk_range_set_value(GTK_RANGE(self->priv->volume),volume);
    gtk_combo_box_set_active(self->priv->loop_mode,loop_mode);
  }
  else {
    // disable toolbar buttons
    gtk_widget_set_sensitive(self->priv->wavetable_play,FALSE);
    gtk_widget_set_sensitive(self->priv->wavetable_clear,FALSE);
    // disable properties
    gtk_widget_set_sensitive(GTK_WIDGET(self->priv->volume),FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(self->priv->loop_mode),FALSE);
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
      glong loop_start, loop_end;
  
      g_object_get(G_OBJECT(wave),"channels",&channels,NULL);
      g_object_get(G_OBJECT(wavelevel),"length",&length,"data",&data,"loop-start", &loop_start,"loop-end",&loop_end,NULL);  
      bt_waveform_viewer_set_wave(BT_WAVEFORM_VIEWER(self->priv->waveform_viewer),data,channels,length);
      
      g_object_set(G_OBJECT(self->priv->waveform_viewer),"loop-begin",(int64_t)loop_start,"loop-end",(int64_t)loop_end,NULL);
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

static void on_volume_changed(GtkRange *range,gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);
  BtWave *wave;

  g_assert(user_data);

  if((wave=waves_list_get_current(self))) {
    gdouble volume=gtk_range_get_value(range);
    g_object_set(wave,"volume",volume,NULL);
    g_object_unref(wave);
  }  
}

static void on_loop_mode_changed(GtkComboBox *menu, gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);
  BtWave *wave;

  g_assert(user_data);

  if((wave=waves_list_get_current(self))) {
    BtWaveLoopMode loop_mode=gtk_combo_box_get_active(self->priv->loop_mode);
    g_object_set(wave,"loop-mode",loop_mode,NULL);
    g_object_unref(wave);
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

  /* if I set state directly to NULL, I don't get inbetween state-change messages */
  gst_element_set_state(self->priv->playbin,GST_STATE_READY);
}

static gboolean on_preview_playback_update(gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);
  gint64 pos_cur;

  // query playback position and update playcursor;
  gst_element_query(GST_ELEMENT(self->priv->preview),self->priv->position_query);
  gst_query_parse_position(self->priv->position_query,NULL,&pos_cur);
  // update play-cursor in samples
  g_object_set(self->priv->waveform_viewer,"playback-cursor",pos_cur,NULL);
  
  return(TRUE);
}

static void on_wavetable_toolbar_play_clicked(GtkToolButton *button, gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);
  BtWave *wave;
  
  if(!gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(button)))
    return;

  if((wave=waves_list_get_current(self))) {
    BtWavelevel *wavelevel;
    BtWaveLoopMode loop_mode;

    self->priv->play_wave=wave;

    // get current wavelevel
    if((wavelevel=wavelevels_list_get_current(self,wave))) {
      GstCaps *caps;
      gulong play_length, play_rate;
      guint play_channels;
      gint16 *play_data;
      glong loop_start,loop_end;
      gulong bytes_per_frame;

      // create playback pipeline on demand
      if(!self->priv->preview) {
        GstElement *ares, *aconv, *asink;
        GstBus *bus;
        
        self->priv->preview=gst_element_factory_make("pipeline",NULL);
        self->priv->preview_src=gst_element_factory_make("memoryaudiosrc",NULL);
        ares=gst_element_factory_make("audioresample",NULL);
        aconv=gst_element_factory_make("audioconvert",NULL);
        asink=gst_element_factory_make("autoaudiosink",NULL);
        gst_bin_add_many(GST_BIN(self->priv->preview),self->priv->preview_src,ares,aconv,asink,NULL);
        gst_element_link_many(self->priv->preview_src,ares,aconv,asink,NULL);
        
        bus=gst_element_get_bus(self->priv->preview);
        gst_bus_add_signal_watch_full (bus, G_PRIORITY_HIGH);
        g_signal_connect(bus, "message::state-changed", G_CALLBACK(on_preview_state_changed), (gpointer)self);
        g_signal_connect(bus, "message::eos", G_CALLBACK(on_preview_eos), (gpointer)self);
        g_signal_connect(bus, "message::segment-done", G_CALLBACK(on_preview_segment_done), (gpointer)self);
        g_signal_connect(bus, "message::error", G_CALLBACK(on_preview_error), (gpointer)self);
        g_signal_connect(bus, "message::warning", G_CALLBACK(on_preview_warning), (gpointer)self);
        gst_object_unref(bus);
        
        self->priv->position_query=gst_query_new_position(GST_FORMAT_DEFAULT);
      }
      
      // get parameters
      g_object_get(wave,
        "loop-mode",&loop_mode,
        "channels",&play_channels,
        NULL);
      g_object_get(wavelevel,
        "length",&play_length,
        "loop-start",&loop_start,
        "loop-end",&loop_end,
        "rate",&play_rate,
        "data",&play_data,
        NULL);
      // build caps
      caps=gst_caps_new_simple("audio/x-raw-int",
        "rate", G_TYPE_INT,play_rate,
        "channels",G_TYPE_INT,play_channels,
        "width",G_TYPE_INT,16,
        "endianness",G_TYPE_INT,G_BYTE_ORDER,
        "signedness",G_TYPE_INT,TRUE,
        NULL);
      g_object_set(self->priv->preview_src,
        "caps",caps,
        "data",play_data,
        "length",play_length,
        NULL);
      gst_caps_unref(caps);

      // build seek events for looping
      if(self->priv->play_seek_event) gst_event_unref(self->priv->play_seek_event);
      if(self->priv->loop_seek_event[0]) gst_event_unref(self->priv->loop_seek_event[0]);
      if(self->priv->loop_seek_event[1]) gst_event_unref(self->priv->loop_seek_event[1]);
      bytes_per_frame=sizeof(gint16)*play_channels;
      self->priv->loop_seek_dir=0;
      if (loop_mode!=BT_WAVE_LOOP_MODE_OFF) {
        GstClockTime play_beg=gst_util_uint64_scale_int(GST_SECOND,loop_start,play_rate);
        GstClockTime play_end=gst_util_uint64_scale_int(GST_SECOND,loop_end,play_rate);
        
        self->priv->play_seek_event = gst_event_new_seek(1.0, GST_FORMAT_TIME,
            GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT,
            GST_SEEK_TYPE_SET, G_GUINT64_CONSTANT(0),
            GST_SEEK_TYPE_SET, play_end);
        if(loop_mode==BT_WAVE_LOOP_MODE_FORWARD) {
          GST_WARNING("prepare for forward loop play: %"GST_TIME_FORMAT" ... %"GST_TIME_FORMAT,
            GST_TIME_ARGS(play_beg),GST_TIME_ARGS(play_end));
  
          self->priv->loop_seek_event[0] = gst_event_new_seek(1.0, GST_FORMAT_TIME,
              GST_SEEK_FLAG_SEGMENT,
              GST_SEEK_TYPE_SET, play_beg,
              GST_SEEK_TYPE_SET, play_end);
        }
        else {
          GST_WARNING("prepare for pingpong loop play: %"GST_TIME_FORMAT" ... %"GST_TIME_FORMAT,
            GST_TIME_ARGS(play_beg),GST_TIME_ARGS(play_end));

          self->priv->loop_seek_event[0] = gst_event_new_seek(-1.0, GST_FORMAT_TIME,
             GST_SEEK_FLAG_SEGMENT,
             GST_SEEK_TYPE_SET, play_beg,
             GST_SEEK_TYPE_SET, play_end);
        }
        self->priv->loop_seek_event[1] = gst_event_new_seek(1.0, GST_FORMAT_TIME,
            GST_SEEK_FLAG_SEGMENT,
            GST_SEEK_TYPE_SET, play_beg,
            GST_SEEK_TYPE_SET, play_end);
      }
      else {
        GstClockTime play_end=gst_util_uint64_scale_int(GST_SECOND,play_length,play_rate);
        
        GST_WARNING("prepare for no loop play: 0 ... %"GST_TIME_FORMAT,GST_TIME_ARGS(play_end));
        self->priv->play_seek_event = gst_event_new_seek(1.0, GST_FORMAT_TIME,
            GST_SEEK_FLAG_FLUSH,
            GST_SEEK_TYPE_SET, G_GUINT64_CONSTANT(0),
            GST_SEEK_TYPE_SET, play_end);
        self->priv->loop_seek_event[0] = gst_event_new_seek(1.0, GST_FORMAT_TIME,
            GST_SEEK_FLAG_NONE,
            GST_SEEK_TYPE_SET, G_GUINT64_CONSTANT(0),
            GST_SEEK_TYPE_SET, play_end);
        self->priv->loop_seek_event[1] = NULL;
      }

      // update playback position 20 times a second
      self->priv->preview_update_id=g_timeout_add(50,on_preview_playback_update,(gpointer)self);

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

  preview_stop(self);
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
        preview_stop(self);
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
    wave=bt_wave_new(song,name,uri,id,1.0,BT_WAVE_LOOP_MODE_OFF,0);
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
  GtkWidget *vpaned,*hpaned,*box,*box2,*table;
  GtkWidget *tool_item;
  GtkWidget *scrolled_window;
  GtkCellRenderer *renderer;
#ifndef HAVE_GTK_2_12
  GtkTooltips *tips=gtk_tooltips_new();
#endif
  GEnumClass *enum_class;
  GEnumValue *enum_value;
  guint i;

  GST_DEBUG("!!!! self=%p",self);

  gtk_widget_set_name(GTK_WIDGET(self),_("wave table view"));

  g_object_get(G_OBJECT(self->priv->app),"settings",&settings,NULL);
  
  // vpane
  vpaned=gtk_vpaned_new();
  gtk_container_add(GTK_CONTAINER(self),vpaned);

  //   hpane
  hpaned=gtk_hpaned_new();
  gtk_paned_pack1(GTK_PANED(vpaned),GTK_WIDGET(hpaned),TRUE,FALSE);

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
  gtk_tree_view_insert_column_with_attributes(self->priv->waves_list,-1,_("Ix"),renderer,"text",WAVE_TABLE_HEX_ID,NULL);
  renderer=gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer),"mode",GTK_CELL_RENDERER_MODE_EDITABLE,"editable",TRUE,NULL);
  g_signal_connect(G_OBJECT(renderer),"edited",G_CALLBACK(on_wave_name_edited),(gpointer)self);
  gtk_tree_view_insert_column_with_attributes(self->priv->waves_list,-1,_("Wave"),renderer,"text",WAVE_TABLE_NAME,NULL);
  gtk_container_add(GTK_CONTAINER(scrolled_window),GTK_WIDGET(self->priv->waves_list));
  gtk_container_add(GTK_CONTAINER(box),scrolled_window);
  
  // loop-mode combo and volume slider
  table=gtk_table_new(2,2,FALSE);
  gtk_table_attach(GTK_TABLE(table),gtk_label_new(_("Volume")), 0, 1, 0, 1, GTK_FILL,GTK_SHRINK, 2,1);
  self->priv->volume=GTK_HSCALE(gtk_hscale_new_with_range(0.0,1.0,0.01));
  gtk_scale_set_value_pos(GTK_SCALE(self->priv->volume),GTK_POS_RIGHT);
  g_signal_connect(G_OBJECT(self->priv->volume), "value-changed", G_CALLBACK(on_volume_changed), (gpointer)self);
  gtk_table_attach(GTK_TABLE(table),GTK_WIDGET(self->priv->volume), 1, 2, 0, 1, GTK_FILL|GTK_EXPAND,GTK_SHRINK, 2,1);
  gtk_table_attach(GTK_TABLE(table),gtk_label_new(_("Loop")), 0, 1, 1, 2, GTK_FILL,GTK_SHRINK, 2,1);
  self->priv->loop_mode=GTK_COMBO_BOX(gtk_combo_box_new_text());
  // g_type_class_peek_static() returns NULL :/
  enum_class=g_type_class_ref(BT_TYPE_WAVE_LOOP_MODE);
  for(i=enum_class->minimum;i<=enum_class->maximum;i++) {
    if((enum_value=g_enum_get_value(enum_class,i))) {
      gtk_combo_box_append_text(self->priv->loop_mode,enum_value->value_name);
    }
  }
  g_type_class_unref(enum_class);
  gtk_combo_box_set_active(self->priv->loop_mode,BT_WAVE_LOOP_MODE_OFF);
  g_signal_connect(G_OBJECT(self->priv->loop_mode), "changed", G_CALLBACK(on_loop_mode_changed), (gpointer)self);
  gtk_table_attach(GTK_TABLE(table),GTK_WIDGET(self->priv->loop_mode), 1, 2, 1, 2, GTK_FILL|GTK_EXPAND,GTK_SHRINK, 2,1);
  gtk_box_pack_start(GTK_BOX(box),table,FALSE,FALSE,0);

  //     vbox (file browser)
  box=gtk_vbox_new(FALSE,0);
  gtk_paned_pack2(GTK_PANED(hpaned),GTK_WIDGET(box),TRUE,FALSE);
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
  g_object_set(G_OBJECT(renderer),"mode",GTK_CELL_RENDERER_MODE_EDITABLE,"editable",TRUE,NULL);
  gtk_tree_view_insert_column_with_attributes(self->priv->wavelevels_list,-1,_("Loop start"),renderer,"text",WAVELEVEL_TABLE_LOOP_START,NULL);
  g_signal_connect(G_OBJECT(renderer),"edited",G_CALLBACK(on_wavelevel_loop_start_edited),(gpointer)self);
  renderer=gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer),"mode",GTK_CELL_RENDERER_MODE_EDITABLE,"editable",TRUE,NULL);
  gtk_tree_view_insert_column_with_attributes(self->priv->wavelevels_list,-1,_("Loop end"),renderer,"text",WAVELEVEL_TABLE_LOOP_END,NULL);
  g_signal_connect(G_OBJECT(renderer),"edited",G_CALLBACK(on_wavelevel_loop_end_edited),(gpointer)self);
  gtk_container_add(GTK_CONTAINER(scrolled_window),GTK_WIDGET(self->priv->wavelevels_list));
  gtk_box_pack_start(GTK_BOX(box2),scrolled_window,FALSE,FALSE,0);

  //       tabs for waveform/envelope? or envelope overlayed?
  //       sampleview
  self->priv->waveform_viewer = bt_waveform_viewer_new();
  gtk_widget_set_size_request (self->priv->waveform_viewer, -1, 96);
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
  g_signal_handlers_disconnect_matched(bus,G_SIGNAL_MATCH_DATA,0,0,NULL,NULL,(gpointer)self);
  gst_bus_remove_signal_watch(bus);
  gst_object_unref(bus);
  gst_object_unref(self->priv->playbin);
  
  // shut down wavetable-preview playbin
  if(self->priv->preview) {
    preview_stop(self);
    bus=gst_element_get_bus(GST_ELEMENT(self->priv->preview));
    g_signal_handlers_disconnect_matched(bus,G_SIGNAL_MATCH_DATA,0,0,NULL,NULL,(gpointer)self);
    gst_bus_remove_signal_watch(bus);
    gst_object_unref(bus); 
    gst_object_unref(self->priv->preview);

    if(self->priv->play_seek_event) gst_event_unref(self->priv->play_seek_event);
    if(self->priv->loop_seek_event[0]) gst_event_unref(self->priv->loop_seek_event[0]);
    if(self->priv->loop_seek_event[1]) gst_event_unref(self->priv->loop_seek_event[1]);
    gst_query_unref(self->priv->position_query);
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
