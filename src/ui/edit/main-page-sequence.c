/* $Id: main-page-sequence.c,v 1.2 2004-08-20 16:35:52 ensonic Exp $
 * class for the editor main machines page
 */

#define BT_EDIT
#define BT_MAIN_PAGE_SEQUENCE_C

#include "bt-edit.h"

enum {
  MAIN_PAGE_SEQUENCE_APP=1,
};


struct _BtMainPageSequencePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the application */
  BtEditApplication *app;
  
  /* bar selection menu */
  GtkOptionMenu *bars_menu;
};

//-- event handler

static void on_song_changed(const BtEditApplication *app, gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  BtSong *song;
  glong index,bars;

  GST_INFO("song has changed : app=%p, window=%p\n",song,user_data);
  // get song from app
  song=BT_SONG(bt_g_object_get_object_property(G_OBJECT(self->private->app),"song"));
  // update page
  bars=bt_g_object_get_long_property(G_OBJECT(bt_song_get_song_info(song)),"bars");
  // find out to which entry it belongs and set the index
  // 1 -> 0, 2 -> 1, 4 -> 2 , 8 -> 3
  if(bars<4) {
    index=bars-1;
  }
  else {
    index=1+(bars>>2);
  }
  gtk_option_menu_set_history(GTK_OPTION_MENU(self->private->bars_menu),index);
}

//-- helper methods

static gboolean bt_main_page_sequence_init_ui(const BtMainPageSequence *self, const BtEditApplication *app) {
  GtkWidget *toolbar;
  GtkWidget *menu;
  GtkWidget *button,*label;
  BtSong *song;
  glong i;
  gchar str[4];

  // add toolbar
  toolbar=gtk_toolbar_new();
  gtk_widget_set_name(toolbar,_("machine view tool bar"));
  gtk_box_pack_start(GTK_BOX(self),toolbar,FALSE,FALSE,0);
  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar),GTK_TOOLBAR_BOTH);
  //gtk_container_set_border_width(GTK_CONTAINER(toolbar),2);
  // add toolbar widgets
  menu=gtk_menu_new();
  //-- build list of values
  sprintf(str,"1");gtk_menu_shell_append(GTK_MENU_SHELL(menu),gtk_menu_item_new_with_label(str));
  sprintf(str,"2");gtk_menu_shell_append(GTK_MENU_SHELL(menu),gtk_menu_item_new_with_label(str));
  for(i=4;i<=64;i+=4) {
    sprintf(str,"%d",i);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu),gtk_menu_item_new_with_label(str));
  }
  self->private->bars_menu=gtk_option_menu_new();
  gtk_option_menu_set_menu(GTK_OPTION_MENU(self->private->bars_menu),menu);
  // @todo how can we add some padding around the buttons in the toolbar?
  // @todo do we really have to add the label by our self
  label=gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
                                GTK_TOOLBAR_CHILD_WIDGET,
                                gtk_label_new(_("Steps")),
                                NULL,
                                NULL,NULL,
                                NULL,NULL,NULL);
  button=gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
                                GTK_TOOLBAR_CHILD_WIDGET,
                                GTK_WIDGET(self->private->bars_menu),
                                NULL,
                                NULL,NULL,
                                NULL,NULL,NULL);
  //gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("Steps"));

  
  // @todo add list-view

  gtk_container_add(GTK_CONTAINER(self),gtk_label_new("no sequence view yet"));

  // register event handlers
  g_signal_connect(G_OBJECT(app), "song-changed", (GCallback)on_song_changed, (gpointer)self);
  return(TRUE);
}

//-- constructor methods

/**
 * bt_main_page_sequence_new:
 * @app: the application the window belongs to
 *
 * Create a new instance
 *
 * Return: the new instance or NULL in case of an error
 */
BtMainPageSequence *bt_main_page_sequence_new(const BtEditApplication *app) {
  BtMainPageSequence *self;

  if(!(self=BT_MAIN_PAGE_SEQUENCE(g_object_new(BT_TYPE_MAIN_PAGE_SEQUENCE,"app",app,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_main_page_sequence_init_ui(self,app)) {
    goto Error;
  }
  return(self);
Error:
  if(self) g_object_unref(self);
  return(NULL);
}

//-- methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_main_page_sequence_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_PAGE_SEQUENCE_APP: {
      g_value_set_object(value, G_OBJECT(self->private->app));
    } break;
    default: {
 			g_assert(FALSE);
      break;
    }
  }
}

/* sets the given properties for this object */
static void bt_main_page_sequence_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_PAGE_SEQUENCE_APP: {
      self->private->app = g_object_ref(G_OBJECT(g_value_get_object(value)));
      //GST_DEBUG("set the app for MAIN_PAGE_SEQUENCE: %p",self->private->app);
    } break;
    default: {
			g_assert(FALSE);
      break;
    }
  }
}

static void bt_main_page_sequence_dispose(GObject *object) {
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE(object);
	return_if_disposed();
  self->private->dispose_has_run = TRUE;
}

static void bt_main_page_sequence_finalize(GObject *object) {
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE(object);
  
  g_object_unref(G_OBJECT(self->private->app));
  g_free(self->private);
}

static void bt_main_page_sequence_init(GTypeInstance *instance, gpointer g_class) {
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE(instance);
  self->private = g_new0(BtMainPageSequencePrivate,1);
  self->private->dispose_has_run = FALSE;
}

static void bt_main_page_sequence_class_init(BtMainPageSequenceClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;
  
  gobject_class->set_property = bt_main_page_sequence_set_property;
  gobject_class->get_property = bt_main_page_sequence_get_property;
  gobject_class->dispose      = bt_main_page_sequence_dispose;
  gobject_class->finalize     = bt_main_page_sequence_finalize;

  g_object_class_install_property(gobject_class,MAIN_PAGE_SEQUENCE_APP,
                                  g_param_spec_object("app",
                                     "app contruct prop",
                                     "Set application object, the window belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));
}

GType bt_main_page_sequence_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtMainPageSequenceClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_main_page_sequence_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtMainPageSequence),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_main_page_sequence_init, // instance_init
    };
		type = g_type_register_static(GTK_TYPE_VBOX,"BtMainPageSequence",&info,0);
  }
  return type;
}

