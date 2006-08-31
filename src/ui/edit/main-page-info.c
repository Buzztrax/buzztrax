/* $Id: main-page-info.c,v 1.42 2006-08-31 19:57:57 ensonic Exp $
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
 * SECTION:btmainpageinfo
 * @short_description: the editor main info page
 */ 

/* @todo
 * - use this tab as the first one?
 * - add time-stamps
 *   song created, song last changed
 * - add choice for metre (in german -> takt): beats (beats = bars / tpb)
 */
 
#define BT_EDIT
#define BT_MAIN_PAGE_INFO_C

#include "bt-edit.h"

enum {
  MAIN_PAGE_INFO_APP=1,
};


struct _BtMainPageInfoPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the application */
  union {
    BtEditApplication *app;
    gpointer app_ptr;
  };
  
  /* name, genre, author of the song */
  GtkEntry *name,*genre,*author;

  /* bpm,tpb,beats of the song */
  GtkSpinButton *bpm,*tpb,*beats;

  /* freeform info about the song */
  GtkTextView *info;
};

static GtkVBoxClass *parent_class=NULL;

//-- event handler

static void on_name_changed(GtkEditable *editable,gpointer user_data) {
  BtMainPageInfo *self=BT_MAIN_PAGE_INFO(user_data);
  BtSong *song;
  BtSongInfo *song_info;

  g_assert(user_data);

  GST_INFO("name changed : self=%p -> \"%s\"",self,gtk_entry_get_text(GTK_ENTRY(editable)));
  // get song from app
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"song-info",&song_info,NULL);
  // update info fields
  g_object_set(G_OBJECT(song_info),"name",g_strdup(gtk_entry_get_text(GTK_ENTRY(editable))),NULL);
  // release the references
  g_object_try_unref(song_info);
  g_object_try_unref(song);
}

static void on_genre_changed(GtkEditable *editable,gpointer user_data) {
  BtMainPageInfo *self=BT_MAIN_PAGE_INFO(user_data);
  BtSong *song;
  BtSongInfo *song_info;

  g_assert(user_data);

  GST_INFO("genre changed : self=%p -> \"%s\"",self,gtk_entry_get_text(GTK_ENTRY(editable)));
  // get song from app
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"song-info",&song_info,NULL);
  // update info fields
  g_object_set(G_OBJECT(song_info),"genre",g_strdup(gtk_entry_get_text(GTK_ENTRY(editable))),NULL);
  // release the references
  g_object_try_unref(song_info);
  g_object_try_unref(song);
}

static void on_author_changed(GtkEditable *editable,gpointer user_data) {
  BtMainPageInfo *self=BT_MAIN_PAGE_INFO(user_data);
  BtSong *song;
  BtSongInfo *song_info;

  g_assert(user_data);

  GST_INFO("author changed : self=%p -> \"%s\"",self,gtk_entry_get_text(GTK_ENTRY(editable)));
  // get song from app
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"song-info",&song_info,NULL);
  // update info fields
  g_object_set(G_OBJECT(song_info),"author",g_strdup(gtk_entry_get_text(GTK_ENTRY(editable))),NULL);
  // release the references
  g_object_try_unref(song_info);
  g_object_try_unref(song);
}

static void on_bpm_changed(GtkSpinButton *spinbutton,gpointer user_data) {
  BtMainPageInfo *self=BT_MAIN_PAGE_INFO(user_data);
  BtSong *song;
  BtSongInfo *song_info;

  g_assert(user_data);

  GST_INFO("bpm changed : self=%p -> %d",self,gtk_spin_button_get_value_as_int(spinbutton));
  // get song from app
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"song-info",&song_info,NULL);
  // update info fields
  g_object_set(G_OBJECT(song_info),"bpm",gtk_spin_button_get_value_as_int(spinbutton),NULL);
  // release the references
  g_object_try_unref(song_info);
  g_object_try_unref(song);
}

static void on_tpb_changed(GtkSpinButton *spinbutton,gpointer user_data) {
  BtMainPageInfo *self=BT_MAIN_PAGE_INFO(user_data);
  BtSong *song;
  BtSongInfo *song_info;
  gulong tpb,beats,bars;

  g_assert(user_data);

  GST_INFO("tpb changed : self=%p -> %d",self,gtk_spin_button_get_value_as_int(spinbutton));
  // get song from app
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"song-info",&song_info,NULL);
  // update info fields
  g_object_get(G_OBJECT(song_info),"bars",&bars,"tpb",&tpb,NULL);
  beats = bars/tpb;
  tpb = gtk_spin_button_get_value_as_int(spinbutton);
  g_object_set(G_OBJECT(song_info),"tpb",tpb,"bars",beats*tpb,NULL);
  // release the references
  g_object_try_unref(song_info);
  g_object_try_unref(song);
}

static void on_beats_changed(GtkSpinButton *spinbutton,gpointer user_data) {
  BtMainPageInfo *self=BT_MAIN_PAGE_INFO(user_data);
  BtSong *song;
  BtSongInfo *song_info;
  gulong tpb,beats;

  g_assert(user_data);

  GST_INFO("beats changed : self=%p -> %d",self,gtk_spin_button_get_value_as_int(spinbutton));
  // get song from app
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"song-info",&song_info,NULL);
  // update info fields
  g_object_get(G_OBJECT(song_info),"tpb",&tpb,NULL);
  beats = gtk_spin_button_get_value_as_int(spinbutton);
  g_object_set(G_OBJECT(song_info),"bars",beats*tpb,NULL);
  // release the references
  g_object_try_unref(song_info);
  g_object_try_unref(song);
}

static void on_info_changed(GtkTextBuffer *textbuffer,gpointer user_data) {
  BtMainPageInfo *self=BT_MAIN_PAGE_INFO(user_data);
  BtSong *song;
  BtSongInfo *song_info;
  gchar *str;
  GtkTextIter beg_iter,end_iter;

  GST_INFO("info changed : self=%p",self);
  // get song from app
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"song-info",&song_info,NULL);
  // update info fields
  // get begin and end textiters :(, then get text
  gtk_text_buffer_get_iter_at_offset(textbuffer,&beg_iter,0);
  gtk_text_buffer_get_iter_at_offset(textbuffer,&end_iter,-1);
  str=gtk_text_buffer_get_text(textbuffer,&beg_iter,&end_iter,FALSE);
  g_object_set(G_OBJECT(song_info),"info",str,NULL);
  g_free(str);
  // release the references
  g_object_try_unref(song_info);
  g_object_try_unref(song);
}

static void on_song_changed(const BtEditApplication *app,GParamSpec *arg,gpointer user_data) {
  BtMainPageInfo *self=BT_MAIN_PAGE_INFO(user_data);
  BtSong *song;
  BtSongInfo *song_info;
  GtkTextBuffer *buffer;
  gchar *name,*genre,*author;
  gulong bpm,tpb,bars;
  gchar *info;

  g_assert(user_data);

  GST_INFO("song has changed : app=%p, self=%p",app,self);
  // get song from app
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_return_if_fail(song);

  g_object_get(G_OBJECT(song),"song-info",&song_info,NULL);
  // update info fields
  g_object_get(G_OBJECT(song_info),
    "name",&name,"genre",&genre,"author",&author,"info",&info,
    "bpm",&bpm,"tpb",&tpb,"bars",&bars,
    NULL);
  g_signal_handlers_block_matched(self->priv->name,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_name_changed,(gpointer)self);
  gtk_entry_set_text(self->priv->name,safe_string(name));g_free(name);
  g_signal_handlers_unblock_matched(self->priv->name,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_name_changed,(gpointer)self);

  g_signal_handlers_block_matched(self->priv->genre,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_genre_changed,(gpointer)self);
  gtk_entry_set_text(self->priv->genre,safe_string(genre));g_free(genre);
  g_signal_handlers_unblock_matched(self->priv->genre,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_genre_changed,(gpointer)self);

  g_signal_handlers_block_matched(self->priv->author,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_author_changed,(gpointer)self);
  gtk_entry_set_text(self->priv->author,safe_string(author));g_free(author);
  g_signal_handlers_unblock_matched(self->priv->author,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_author_changed,(gpointer)self);

  g_signal_handlers_block_matched(self->priv->bpm,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_bpm_changed,(gpointer)self);
  gtk_spin_button_set_value(self->priv->bpm,(gdouble)bpm);
  g_signal_handlers_unblock_matched(self->priv->bpm,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_bpm_changed,(gpointer)self);

  g_signal_handlers_block_matched(self->priv->tpb,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_tpb_changed,(gpointer)self);
  gtk_spin_button_set_value(self->priv->tpb,(gdouble)tpb);
  g_signal_handlers_unblock_matched(self->priv->tpb,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_tpb_changed,(gpointer)self);

  g_signal_handlers_block_matched(self->priv->beats,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_beats_changed,(gpointer)self);
  gtk_spin_button_set_value(self->priv->beats,(gdouble)(bars/tpb));
  g_signal_handlers_unblock_matched(self->priv->beats,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_beats_changed,(gpointer)self);

  buffer=gtk_text_view_get_buffer(self->priv->info);
  g_signal_handlers_block_matched(G_OBJECT(buffer),G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_name_changed,(gpointer)self);
  gtk_text_buffer_set_text(buffer,safe_string(info),-1);g_free(info);
  g_signal_handlers_unblock_matched(G_OBJECT(buffer),G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,on_name_changed,(gpointer)self);

// release the references
  g_object_try_unref(song_info);
  g_object_try_unref(song);
}

//-- helper methods

static gboolean bt_main_page_info_init_ui(const BtMainPageInfo *self) {
  GtkWidget *label,*frame,*box;
  GtkWidget *table,*spacer;
  GtkWidget *scrolledwindow;
  GtkAdjustment *spin_adjustment;
  gchar *str;
  
  GST_DEBUG("!!!! self=%p",self);
  
  gtk_widget_set_name(GTK_WIDGET(self),_("song information"));
  
  // first row of vbox
  frame=gtk_frame_new(NULL);
  label=gtk_label_new(NULL);
  str=g_strdup_printf("<big><b>%s</b></big>",_("song meta data"));
  gtk_label_set_markup(GTK_LABEL(label),str);
  g_free(str);
  gtk_frame_set_label_widget(GTK_FRAME(frame),label);
  gtk_frame_set_shadow_type(GTK_FRAME(frame),GTK_SHADOW_NONE);
  gtk_container_set_border_width(GTK_CONTAINER(frame),6);
  gtk_widget_set_name(frame,_("song meta data"));
  gtk_box_pack_start(GTK_BOX(self),frame,FALSE,TRUE,0);
  
  box=gtk_hbox_new(FALSE,6);
  gtk_container_add(GTK_CONTAINER(frame),box);

  spacer=gtk_label_new("    ");
  gtk_box_pack_start(GTK_BOX(box),spacer,FALSE,TRUE,0);

  // first column
  table=gtk_table_new(/*rows=*/3,/*columns=*/2,/*homogenous=*/FALSE);
  gtk_box_pack_start(GTK_BOX(box),table,TRUE,TRUE,0);

  label=gtk_label_new(_("name"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 0, 1, GTK_FILL,GTK_SHRINK, 2,1);
  self->priv->name=GTK_ENTRY(gtk_entry_new());
  gtk_table_attach(GTK_TABLE(table),GTK_WIDGET(self->priv->name), 1, 2, 0, 1, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);
  g_signal_connect(G_OBJECT(self->priv->name), "changed", G_CALLBACK(on_name_changed), (gpointer)self);

  label=gtk_label_new(_("genre"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 1, 2, GTK_FILL,GTK_SHRINK, 2,1);
  self->priv->genre=GTK_ENTRY(gtk_entry_new());
  gtk_table_attach(GTK_TABLE(table),GTK_WIDGET(self->priv->genre), 1, 2, 1, 2, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);
  g_signal_connect(G_OBJECT(self->priv->genre), "changed", G_CALLBACK(on_genre_changed), (gpointer)self);

  label=gtk_label_new(_("author"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 2, 3, GTK_FILL,GTK_SHRINK, 2,1);
  self->priv->author=GTK_ENTRY(gtk_entry_new());
  gtk_table_attach(GTK_TABLE(table),GTK_WIDGET(self->priv->author), 1, 2, 2, 3, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);
  g_signal_connect(G_OBJECT(self->priv->genre), "changed", G_CALLBACK(on_author_changed), (gpointer)self);

  // second column
  table=gtk_table_new(/*rows=*/3,/*columns=*/2,/*homogenous=*/FALSE);
  gtk_box_pack_start(GTK_BOX(box),table,TRUE,TRUE,0);

  label=gtk_label_new(_("beats per minute"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 0, 1, GTK_FILL,GTK_SHRINK, 2,1);
  spin_adjustment=GTK_ADJUSTMENT(gtk_adjustment_new(130.0, 20.0, 300.0, 1.0, 5.0, 5.0));
  self->priv->bpm=GTK_SPIN_BUTTON(gtk_spin_button_new(spin_adjustment,1.0,0));
  gtk_table_attach(GTK_TABLE(table),GTK_WIDGET(self->priv->bpm), 1, 2, 0, 1, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);
  g_signal_connect(G_OBJECT(self->priv->bpm), "value-changed", G_CALLBACK(on_bpm_changed), (gpointer)self);

  label=gtk_label_new(_("beats"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 1, 2, GTK_FILL,GTK_SHRINK, 2,1);
  spin_adjustment=GTK_ADJUSTMENT(gtk_adjustment_new(8.0, 1.0, 32.0, 1.0, 4.0, 4.0));
  self->priv->beats=GTK_SPIN_BUTTON(gtk_spin_button_new(spin_adjustment,1.0,0));
  gtk_table_attach(GTK_TABLE(table),GTK_WIDGET(self->priv->beats), 1, 2, 1, 2, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);
  g_signal_connect(G_OBJECT(self->priv->beats), "value-changed", G_CALLBACK(on_beats_changed), (gpointer)self);

  label=gtk_label_new(_("ticks per beat"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 2, 3, GTK_FILL,GTK_SHRINK, 2,1);
  spin_adjustment=GTK_ADJUSTMENT(gtk_adjustment_new(8.0, 1.0, 64.0, 1.0, 4.0, 4.0));
  self->priv->tpb=GTK_SPIN_BUTTON(gtk_spin_button_new(spin_adjustment,1.0,0));
  gtk_table_attach(GTK_TABLE(table),GTK_WIDGET(self->priv->tpb), 1, 2, 2, 3, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);
  g_signal_connect(G_OBJECT(self->priv->tpb), "value-changed", G_CALLBACK(on_tpb_changed), (gpointer)self);

  // @idea have another field with subticks (GstController parameter smoothing)
  // @idea show tick and subtick interval as time (s:ms)
  
  gtk_table_attach(GTK_TABLE(table),gtk_label_new(" "), 0, 2, 2, 3, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);

  // second row of hbox
  frame=gtk_frame_new(NULL);
  label=gtk_label_new(NULL);
  str=g_strdup_printf("<big><b>%s</b></big>",_("free text info"));
  gtk_label_set_markup(GTK_LABEL(label),str);
  g_free(str);
  gtk_frame_set_label_widget(GTK_FRAME(frame),label);
  gtk_frame_set_shadow_type(GTK_FRAME(frame),GTK_SHADOW_NONE);
  gtk_container_set_border_width(GTK_CONTAINER(frame),6);
  gtk_widget_set_name(frame,_("free text info"));
  gtk_box_pack_start(GTK_BOX(self),frame,TRUE,TRUE,0);

  box=gtk_hbox_new(FALSE,6);
  gtk_container_add(GTK_CONTAINER(frame),box);

  spacer=gtk_label_new("    ");
  gtk_box_pack_start(GTK_BOX(box),spacer,FALSE,TRUE,0);

  scrolledwindow=gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_set_name(scrolledwindow,"scrolledwindow");
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledwindow),GTK_SHADOW_IN);
  gtk_container_add(GTK_CONTAINER(box),scrolledwindow);
  GST_DEBUG("  scrolled window: %p",scrolledwindow);

  self->priv->info=GTK_TEXT_VIEW(gtk_text_view_new());
  gtk_widget_set_name(GTK_WIDGET(self->priv->info),_("free text info"));
  //gtk_container_set_border_width(GTK_CONTAINER(self->priv->info),1);
  GST_DEBUG("  text view: %p",self->priv->info);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(self->priv->info),GTK_WRAP_WORD);
  gtk_container_add(GTK_CONTAINER(scrolledwindow),GTK_WIDGET(self->priv->info));
  g_signal_connect(G_OBJECT(gtk_text_view_get_buffer(self->priv->info)), "changed", G_CALLBACK(on_info_changed), (gpointer)self);

  // register event handlers
  g_signal_connect(G_OBJECT(self->priv->app), "notify::song", G_CALLBACK(on_song_changed), (gpointer)self);

  GST_DEBUG("  done");
  return(TRUE);
}

//-- constructor methods

/**
 * bt_main_page_info_new:
 * @app: the application the window belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtMainPageInfo *bt_main_page_info_new(const BtEditApplication *app) {
  BtMainPageInfo *self;

  if(!(self=BT_MAIN_PAGE_INFO(g_object_new(BT_TYPE_MAIN_PAGE_INFO,"app",app,"spacing",6,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_main_page_info_init_ui(self)) {
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
static void bt_main_page_info_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtMainPageInfo *self = BT_MAIN_PAGE_INFO(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_PAGE_INFO_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    default: {
       G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_main_page_info_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtMainPageInfo *self = BT_MAIN_PAGE_INFO(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_PAGE_INFO_APP: {
      g_object_try_weak_unref(self->priv->app);
      self->priv->app = BT_EDIT_APPLICATION(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->app);
      //GST_DEBUG("set the app for main_page_info: %p",self->priv->app);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_main_page_info_dispose(GObject *object) {
  BtMainPageInfo *self = BT_MAIN_PAGE_INFO(object);
  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  g_object_try_weak_unref(self->priv->app);

  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_main_page_info_finalize(GObject *object) {
  //BtMainPageInfo *self = BT_MAIN_PAGE_INFO(object);
  
  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_main_page_info_init(GTypeInstance *instance, gpointer g_class) {
  BtMainPageInfo *self = BT_MAIN_PAGE_INFO(instance);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_MAIN_PAGE_INFO, BtMainPageInfoPrivate);
}

static void bt_main_page_info_class_init(BtMainPageInfoClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtMainPageInfoPrivate));
  
  gobject_class->set_property = bt_main_page_info_set_property;
  gobject_class->get_property = bt_main_page_info_get_property;
  gobject_class->dispose      = bt_main_page_info_dispose;
  gobject_class->finalize     = bt_main_page_info_finalize;

  g_object_class_install_property(gobject_class,MAIN_PAGE_INFO_APP,
                                  g_param_spec_object("app",
                                     "app contruct prop",
                                     "Set application object, the window belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));
}

GType bt_main_page_info_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtMainPageInfoClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_main_page_info_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtMainPageInfo),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_main_page_info_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(GTK_TYPE_VBOX,"BtMainPageInfo",&info,0);
  }
  return type;
}
