/* $Id: main-window.c,v 1.27 2004-10-05 15:46:09 ensonic Exp $
 * class for the editor main window
 */

#define BT_EDIT
#define BT_MAIN_WINDOW_C

#include "bt-edit.h"

enum {
  MAIN_WINDOW_APP=1,
  MAIN_WINDOW_STATUSBAR
};


struct _BtMainWindowPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the application */
  BtEditApplication *app;

  /* the menu of the window */
  BtMainMenu *menu;
  /* the toolbar of the window */
  BtMainToolbar *toolbar;
  /* the content pages of the window */
  BtMainPages *pages;
  /* the statusbar of the window */
  BtMainStatusbar *statusbar;
  
  /* the keyboard shortcut table for the window */
  GtkAccelGroup *accel_group;
};

static GtkWindowClass *parent_class=NULL;

//-- event handler

static gboolean on_window_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
  BtMainWindow *self=BT_MAIN_WINDOW(user_data);
  gboolean res=TRUE;

  g_assert(user_data);

  GST_INFO("delete event occurred");
  // returning TRUE means, we don't want the window to be destroyed
  if(bt_main_window_check_quit(self)) {
    res=FALSE;
  }
  return(res);
}

static void on_window_destroy(GtkWidget *widget, gpointer user_data) {
  GST_INFO("destroy occurred");
  gtk_main_quit();
}

static void on_song_changed(const BtEditApplication *app, gpointer user_data) {
  BtMainWindow *self=BT_MAIN_WINDOW(user_data);
  gchar *title,*name;
  BtSong *song;
  BtSongInfo *song_info;

  g_assert(user_data);

  GST_INFO("song has changed : app=%p, window=%p",app,user_data);

  // get song from app
  g_object_get(G_OBJECT(app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"song-info",&song_info,NULL);
  // compose title
  g_object_get(G_OBJECT(song_info),"name",&name,NULL);
  title=g_strdup_printf("%s - "PACKAGE_NAME,name);g_free(name);
  gtk_window_set_title(GTK_WINDOW(self), title);g_free(title);
  //-- release the references
  g_object_try_unref(song_info);
  g_object_try_unref(song);
}

//-- helper methods

static gboolean bt_main_window_init_ui(const BtMainWindow *self) {
  GtkWidget *box;
  GdkPixbuf *main_icon;
  
  self->priv->accel_group=gtk_accel_group_new();
  
  // create and set window icon
  if((main_icon=create_pixbuf("buzztard.png"))) {
    gtk_window_set_icon(GTK_WINDOW(self),main_icon);
  }
  gtk_widget_set_size_request(GTK_WIDGET(self),800,600);
  
  // create main layout container
  box=gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(self),box);
 
  // add the menu-bar
  self->priv->menu=bt_main_menu_new(self->priv->app,self->priv->accel_group);
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->priv->menu),FALSE,FALSE,0);
  // add the tool-bar
  self->priv->toolbar=bt_main_toolbar_new(self->priv->app);
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->priv->toolbar),FALSE,FALSE,0);
  // add the window content pages
  self->priv->pages=bt_main_pages_new(self->priv->app);
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->priv->pages),TRUE,TRUE,0);
  // add the status bar
  self->priv->statusbar=bt_main_statusbar_new(self->priv->app);
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->priv->statusbar),FALSE,FALSE,0);

  g_signal_connect(G_OBJECT(self->priv->app), "song-changed", (GCallback)on_song_changed, (gpointer)self);

  gtk_window_add_accel_group(GTK_WINDOW(self),self->priv->accel_group);

  g_signal_connect(G_OBJECT(self),"delete-event", G_CALLBACK(on_window_delete_event),(gpointer)self);
  g_signal_connect(G_OBJECT(self),"destroy",      G_CALLBACK(on_window_destroy),(gpointer)self);

  return(TRUE);
}

//-- constructor methods

/**
 * bt_main_window_new:
 * @app: the application the window belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtMainWindow *bt_main_window_new(const BtEditApplication *app) {
  BtMainWindow *self;

  if(!(self=BT_MAIN_WINDOW(g_object_new(BT_TYPE_MAIN_WINDOW,"app",app,"type",GTK_WINDOW_TOPLEVEL,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_main_window_init_ui(self)) {
    goto Error;
  }
  gtk_widget_show_all(GTK_WIDGET(self));
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods

/**
 * bt_main_window_run:
 * @self: the window instance to setup and run
 *
 * build, show and run the main window
 *
 * Returns: true for success
 */
gboolean bt_main_window_run(const BtMainWindow *self) {
  gboolean res=TRUE;
  GST_INFO("before running the UI");
  gtk_main();
  GST_INFO("after running the UI");
  return(res);
}

/**
 * bt_main_window_check_quit:
 * @self: the main window instance
 *
 * Displays a dialog box, that asks the user to confirm exiting the application.
 *
 * Returns: TRUE if the user has confirmed to exit
 */
gboolean bt_main_window_check_quit(const BtMainWindow *self) {
  gboolean quit=FALSE;
  gint result;
  GtkWidget *label,*icon,*box;
  GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Really quit ?"),
                                                  GTK_WINDOW(self),
                                                  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                  GTK_STOCK_OK,
                                                  GTK_RESPONSE_ACCEPT,
                                                  GTK_STOCK_CANCEL,
                                                  GTK_RESPONSE_REJECT,
                                                  NULL);

  box=gtk_hbox_new(FALSE,12);
  gtk_container_set_border_width(GTK_CONTAINER(box),6);

  icon=gtk_image_new_from_stock(GTK_STOCK_DIALOG_QUESTION,GTK_ICON_SIZE_DIALOG);
  gtk_container_add(GTK_CONTAINER(box),icon);
  
  label=gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(label), g_strdup_printf(
    "<big><b>%s</b></big>\n\n%s",_("Really quit ?"),_("All unsaved changes will be lost then.")
  ));
  //label=gtk_label_new(_("Really quit ?"));
  gtk_container_add(GTK_CONTAINER(box),label);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),box);
  gtk_widget_show_all(dialog);
                                                  
  result=gtk_dialog_run(GTK_DIALOG(dialog));
  switch(result) {
    case GTK_RESPONSE_ACCEPT:
      quit=TRUE;
      break;
    case GTK_RESPONSE_REJECT:
      quit=FALSE;
      break;
    default:
      GST_WARNING("unhandled response code = %d",result);
  }
  gtk_widget_destroy(dialog);
  GST_INFO("menu quit result = %d",quit);
  return(quit);
}

/**
 * bt_main_window_new_song:
 * @self: the main window instance
 *
 * Prepares a new song. Triggers cleaning up the old song and refreshes the ui.
 */
void bt_main_window_new_song(const BtMainWindow *self) {
  // @todo if unsaved ask the use, if we should save the song
  if(!bt_edit_application_new_song(self->priv->app)) {
    // @todo show error message
  }
}

/**
 * bt_main_window_open_song:
 * @self: the main window instance
 *
 * Opens a dialog box, where the user can choose a song to load.
 * If the dialog is not canceld, the old song will be freed, the new song will
 * be loaded and the ui will be refreshed upon success.
 */
void bt_main_window_open_song(const BtMainWindow *self) {
  GtkWidget *dialog=gtk_file_selection_new(_("Choose a song"));
  gint result;
  char *filename=NULL;
    
  /* lets set the filename, as if this were a save dialog, and we are giving
   a default filename */
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(dialog),DATADIR""G_DIR_SEPARATOR_S"songs"G_DIR_SEPARATOR_S);
  result=gtk_dialog_run(GTK_DIALOG(dialog));
  switch(result) {
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
      filename=g_strdup(gtk_file_selection_get_filename(GTK_FILE_SELECTION(dialog)));
      break;
    case GTK_RESPONSE_REJECT:
    case GTK_RESPONSE_CANCEL:
    case GTK_RESPONSE_CLOSE:
      break;
    default:
      GST_WARNING("unhandled response code = %d",result);
  } 
  gtk_widget_destroy(dialog);
  // load after destoying the dialog, otherwise it stays open all time
  if(filename) {
    if(!bt_edit_application_load_song(self->priv->app,filename)) {
      // @todo show error message
    }
    g_free(filename);
  }
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_main_window_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtMainWindow *self = BT_MAIN_WINDOW(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_WINDOW_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    case MAIN_WINDOW_STATUSBAR: {
      g_value_set_object(value, self->priv->statusbar);
    } break;
    default: {
 			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_main_window_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtMainWindow *self = BT_MAIN_WINDOW(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_WINDOW_APP: {
      g_object_try_unref(self->priv->app);
      self->priv->app = g_object_try_ref(g_value_get_object(value));
      //GST_DEBUG("set the app for main_window: %p",self->priv->app);
    } break;
    default: {
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_main_window_dispose(GObject *object) {
  BtMainWindow *self = BT_MAIN_WINDOW(object);
	return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  g_object_try_unref(self->priv->app);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_main_window_finalize(GObject *object) {
  BtMainWindow *self = BT_MAIN_WINDOW(object);

  GST_DEBUG("!!!! self=%p",self);
  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_main_window_init(GTypeInstance *instance, gpointer g_class) {
  BtMainWindow *self = BT_MAIN_WINDOW(instance);
  self->priv = g_new0(BtMainWindowPrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_main_window_class_init(BtMainWindowClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;

  parent_class=g_type_class_ref(GTK_TYPE_WINDOW);
  
  gobject_class->set_property = bt_main_window_set_property;
  gobject_class->get_property = bt_main_window_get_property;
  gobject_class->dispose      = bt_main_window_dispose;
  gobject_class->finalize     = bt_main_window_finalize;

  g_object_class_install_property(gobject_class,MAIN_WINDOW_APP,
                                  g_param_spec_object("app",
                                     "app construct prop",
                                     "Set application object, the window belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,MAIN_WINDOW_STATUSBAR,
                                  g_param_spec_object("statusbar",
                                     "statusbar prop",
                                     "Get the status bar",
                                     BT_TYPE_MAIN_STATUSBAR, /* object type */
                                     G_PARAM_READABLE));
}

GType bt_main_window_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtMainWindowClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_main_window_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtMainWindow),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_main_window_init, // instance_init
    };
		type = g_type_register_static(GTK_TYPE_WINDOW,"BtMainWindow",&info,0);
  }
  return type;
}

