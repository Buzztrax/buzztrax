/* $Id: main-page-info.c,v 1.18 2004-12-18 14:44:27 ensonic Exp $
 * class for the editor main info page
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
  BtEditApplication *app;
  
  /* name of the song */
  GtkEntry *name;
  /* genre of the song  */
  GtkEntry *genre;
  /* author of the song */
  GtkEntry *author;
  /* freeform info anout the song */
  GtkTextView *info;
};

static GtkVBoxClass *parent_class=NULL;

//-- event handler

static void on_song_changed(const BtEditApplication *app,GParamSpec *arg,gpointer user_data) {
  BtMainPageInfo *self=BT_MAIN_PAGE_INFO(user_data);
  BtSong *song;
  BtSongInfo *song_info;
  gchar *name,*genre,*author,*info;

  g_assert(user_data);

  GST_INFO("song has changed : app=%p, self=%p",app,self);
  // get song from app
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"song-info",&song_info,NULL);
  // update info fields
  g_object_get(G_OBJECT(song_info),"name",&name,"genre",&genre,"author",&author,"info",&info,NULL);
  gtk_entry_set_text(self->priv->name,safe_string(name));g_free(name);
  gtk_entry_set_text(self->priv->genre,safe_string(genre));g_free(genre);
  gtk_entry_set_text(self->priv->author,safe_string(author));g_free(author);
  gtk_text_buffer_set_text(gtk_text_view_get_buffer(self->priv->info),safe_string(info),-1);g_free(info);
  // release the references
  g_object_try_unref(song_info);
  g_object_try_unref(song);
}

void on_name_changed(GtkEditable *editable,gpointer user_data) {
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

void on_genre_changed(GtkEditable *editable,gpointer user_data) {
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

void on_author_changed(GtkEditable *editable,gpointer user_data) {
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

void on_info_changed(GtkTextBuffer *textbuffer,gpointer user_data) {
  BtMainPageInfo *self=BT_MAIN_PAGE_INFO(user_data);
  BtSong *song;
  BtSongInfo *song_info;
	gchar *str;
	GtkTextIter beg_iter,end_iter;

  g_assert(user_data);

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

//-- helper methods

static gboolean bt_main_page_info_init_ui(const BtMainPageInfo *self, const BtEditApplication *app) {
  GtkWidget *label,*frame;
  GtkWidget *table,*entry;
  GtkWidget *scrolledwindow;

  // @todo display all properties from BTSongInfo
  
  // first row of hbox
  frame=gtk_frame_new(_("song meta data"));
  gtk_widget_set_name(frame,_("song meta data"));
  gtk_box_pack_start(GTK_BOX(self),frame,FALSE,TRUE,0);

  table=gtk_table_new(2,2,FALSE);
  gtk_container_add(GTK_CONTAINER(frame),table);

  label=gtk_label_new(_("name"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 0, 1, GTK_SHRINK,GTK_SHRINK, 2,1);
  self->priv->name=GTK_ENTRY(gtk_entry_new());
  gtk_table_attach(GTK_TABLE(table),GTK_WIDGET(self->priv->name), 1, 2, 0, 1, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);
	g_signal_connect(G_OBJECT(self->priv->name), "changed", (GCallback)on_name_changed, (gpointer)self);

  label=gtk_label_new(_("genre"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 1, 2, GTK_SHRINK,GTK_SHRINK, 2,1);
  self->priv->genre=GTK_ENTRY(gtk_entry_new());
  gtk_table_attach(GTK_TABLE(table),GTK_WIDGET(self->priv->genre), 1, 2, 1, 2, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);
	g_signal_connect(G_OBJECT(self->priv->genre), "changed", (GCallback)on_genre_changed, (gpointer)self);

  label=gtk_label_new(_("author"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 2, 3, GTK_SHRINK,GTK_SHRINK, 2,1);
  self->priv->author=GTK_ENTRY(gtk_entry_new());
  gtk_table_attach(GTK_TABLE(table),GTK_WIDGET(self->priv->author), 1, 2, 2, 3, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);
	g_signal_connect(G_OBJECT(self->priv->genre), "changed", (GCallback)on_author_changed, (gpointer)self);

  // second row of hbox
  frame=gtk_frame_new(_("free text info"));
  gtk_widget_set_name(frame,_("free text info"));
  gtk_box_pack_start(GTK_BOX(self),frame,TRUE,TRUE,0);

  scrolledwindow=gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_set_name(scrolledwindow,"scrolledwindow");
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledwindow),GTK_SHADOW_IN);
  gtk_container_add(GTK_CONTAINER(frame),scrolledwindow);

  self->priv->info=GTK_TEXT_VIEW(gtk_text_view_new());
  gtk_widget_set_name(GTK_WIDGET(self->priv->info),_("free text info"));
  //gtk_container_set_border_width(GTK_CONTAINER(self->priv->info),1);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(self->priv->info),GTK_WRAP_WORD);
  gtk_container_add(GTK_CONTAINER(scrolledwindow),GTK_WIDGET(self->priv->info));
	g_signal_connect(G_OBJECT(gtk_text_view_get_buffer(self->priv->info)), "changed", (GCallback)on_info_changed, (gpointer)self);

  // register event handlers
  g_signal_connect(G_OBJECT(app), "notify::song", (GCallback)on_song_changed, (gpointer)self);
  return(TRUE);
}

//-- constructor methods

/**
 * bt_main_page_info_new:
 * @app: the application the window belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtMainPageInfo *bt_main_page_info_new(const BtEditApplication *app) {
  BtMainPageInfo *self;

  if(!(self=BT_MAIN_PAGE_INFO(g_object_new(BT_TYPE_MAIN_PAGE_INFO,"app",app,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_main_page_info_init_ui(self,app)) {
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
      g_object_try_unref(self->priv->app);
      self->priv->app = g_object_try_ref(g_value_get_object(value));
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

  g_object_try_unref(self->priv->app);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_main_page_info_finalize(GObject *object) {
  BtMainPageInfo *self = BT_MAIN_PAGE_INFO(object);
  
  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_main_page_info_init(GTypeInstance *instance, gpointer g_class) {
  BtMainPageInfo *self = BT_MAIN_PAGE_INFO(instance);
  self->priv = g_new0(BtMainPageInfoPrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_main_page_info_class_init(BtMainPageInfoClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(GTK_TYPE_VBOX);
  
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
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtMainPageInfoClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_main_page_info_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtMainPageInfo),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_main_page_info_init, // instance_init
			NULL // value_table
    };
		type = g_type_register_static(GTK_TYPE_VBOX,"BtMainPageInfo",&info,0);
  }
  return type;
}
