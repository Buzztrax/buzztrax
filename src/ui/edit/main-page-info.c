/* $Id: main-page-info.c,v 1.7 2004-09-20 16:44:29 ensonic Exp $
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
  /* freeform info anout the song */
  GtkTextView *info;
};

static GtkVBoxClass *parent_class=NULL;

//-- event handler

static void on_song_changed(const BtEditApplication *app, gpointer user_data) {
  BtMainPageInfo *self=BT_MAIN_PAGE_INFO(user_data);
  BtSong *song;
  gchar *str;

  GST_INFO("song has changed : app=%p, self=%p",app,self);
  // get song from app
  song=BT_SONG(bt_g_object_get_object_property(G_OBJECT(self->private->app),"song"));
  // update info fields
  if(!(str=(gchar *)bt_g_object_get_string_property(G_OBJECT(bt_song_get_song_info(song)),"name"))) str="";
  gtk_entry_set_text(self->private->name,str);
  if(!(str=(gchar *)bt_g_object_get_string_property(G_OBJECT(bt_song_get_song_info(song)),"genre"))) str="";
  gtk_entry_set_text(self->private->genre,str);
  if(!(str=(gchar *)bt_g_object_get_string_property(G_OBJECT(bt_song_get_song_info(song)),"info"))) str="";
  gtk_text_buffer_set_text(gtk_text_view_get_buffer(self->private->info),str,-1);
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
  self->private->name=GTK_ENTRY(gtk_entry_new());
  gtk_table_attach(GTK_TABLE(table),GTK_WIDGET(self->private->name), 1, 2, 0, 1, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);

  label=gtk_label_new(_("genre"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 1, 2, 0,0, 2,1);
  self->private->genre=GTK_ENTRY(gtk_entry_new());
  gtk_table_attach(GTK_TABLE(table),GTK_WIDGET(self->private->genre), 1, 2, 1, 2, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);

  // second row of hbox
  frame=gtk_frame_new(_("free text info"));
  gtk_widget_set_name(frame,_("free text info"));
  gtk_box_pack_start(GTK_BOX(self),frame,TRUE,TRUE,0);

  scrolledwindow=gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_set_name(scrolledwindow,"scrolledwindow");
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledwindow),GTK_SHADOW_IN);
  gtk_container_add(GTK_CONTAINER(frame),scrolledwindow);

  self->private->info=GTK_TEXT_VIEW(gtk_text_view_new());
  gtk_widget_set_name(GTK_WIDGET(self->private->info),_("free text info"));
  //gtk_container_set_border_width(GTK_CONTAINER(self->private->info),1);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(self->private->info),GTK_WRAP_WORD);
  gtk_container_add(GTK_CONTAINER(scrolledwindow),GTK_WIDGET(self->private->info));

  // register event handlers
  g_signal_connect(G_OBJECT(app), "song-changed", (GCallback)on_song_changed, (gpointer)self);
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
      g_value_set_object(value, self->private->app);
    } break;
    default: {
 			g_assert(FALSE);
      break;
    }
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
      g_object_try_unref(self->private->app);
      self->private->app = g_object_ref(G_OBJECT(g_value_get_object(value)));
      //GST_DEBUG("set the app for main_page_info: %p",self->private->app);
    } break;
    default: {
			g_assert(FALSE);
      break;
    }
  }
}

static void bt_main_page_info_dispose(GObject *object) {
  BtMainPageInfo *self = BT_MAIN_PAGE_INFO(object);
	return_if_disposed();
  self->private->dispose_has_run = TRUE;

  g_object_try_unref(G_OBJECT(self->private->app));

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_main_page_info_finalize(GObject *object) {
  BtMainPageInfo *self = BT_MAIN_PAGE_INFO(object);
  
  g_free(self->private);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_main_page_info_init(GTypeInstance *instance, gpointer g_class) {
  BtMainPageInfo *self = BT_MAIN_PAGE_INFO(instance);
  self->private = g_new0(BtMainPageInfoPrivate,1);
  self->private->dispose_has_run = FALSE;
}

static void bt_main_page_info_class_init(BtMainPageInfoClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;

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
    };
		type = g_type_register_static(GTK_TYPE_VBOX,"BtMainPageInfo",&info,0);
  }
  return type;
}

