/* $Id: main-window.c,v 1.9 2004-08-12 14:02:59 ensonic Exp $
 * class for the editor main window
 */

#define BT_EDIT
#define BT_MAIN_WINDOW_C

#include "bt-edit.h"

enum {
  MAIN_WINDOW_APP=1,
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
  /* the statusbar of the window */
  BtMainStatusbar *statusbar;
  
  /* the keyboard shortcut table for the window */
  GtkAccelGroup *accel_group;
};

//-- helper

static void bt_main_window_refresh_ui(const BtMainWindow *self) {
  static gchar *title;
  BtSong *song;

  // get song from app
  song=BT_SONG(bt_g_object_get_object_property(G_OBJECT(self->private->app),"song"));
  // compose title
  title=g_strdup_printf(PACKAGE_NAME": %s",bt_g_object_get_string_property(G_OBJECT(bt_song_get_song_info(song)),"name"));
  gtk_window_set_title(GTK_WINDOW(self), title);
}

//-- event handler

static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
  GST_INFO("delete event occurred\n");
  return(!bt_main_window_check_quit(BT_MAIN_WINDOW(data)));
}

static void destroy(GtkWidget *widget, gpointer data) {
  GST_INFO("destroy event occurred\n");
  gtk_main_quit();
}

//-- helper methods

static GtkWidget *bt_main_window_init_notebook(const BtMainWindow *self, GtkWidget *box) {
  GtkWidget *notebook;
  GtkWidget *page,*label,*frame;
  GtkWidget *table,*entry;
  GtkWidget *scrolledwindow,*textfield;

  // @todo make this a sub-class of notebook widget
  notebook=gtk_notebook_new();
  gtk_widget_set_name(notebook,_("song views"));
  gtk_box_pack_start(GTK_BOX(box),notebook,TRUE,TRUE,0);

  // @todo add real wigets for machine view
  //(canvas)
  page=gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(notebook),page);

  label=gtk_label_new(_("machine view"));
  gtk_widget_set_name(label,_("machine view"));
  gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook),gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),0),label);
  gtk_container_add(GTK_CONTAINER(page),gtk_label_new("nothing here"));

  // @todo add real wigets for pattern view
  // table ?
  page=gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(notebook),page);

  label=gtk_label_new(_("pattern view"));
  gtk_widget_set_name(label,_("pattern view"));
  gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook),gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),1),label);

  // @todo add real wigets for sequence view
  // table ?
  page=gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(notebook),page);

  label=gtk_label_new(_("sequence view"));
  gtk_widget_set_name(label,_("sequence view"));
  gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook),gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),2),label);
  
  // @todo add real wigets for song info view
  // display all properties from BTSongInfo
  page=gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(notebook),page);

  label=gtk_label_new(_("song information"));
  gtk_widget_set_name(label,_("song information"));
  gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook),gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),3),label);

  // first row of hbox
  frame=gtk_frame_new(_("song meta data"));
  gtk_widget_set_name(label,_("song meta data"));
  gtk_box_pack_start(GTK_BOX(page),frame,FALSE,TRUE,0);

  table=gtk_table_new(2,2,FALSE);
  gtk_container_add(GTK_CONTAINER(frame),table);

  label=gtk_label_new(_("name"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 0, 1, GTK_SHRINK,GTK_SHRINK, 2,1);
  entry=gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table),entry, 1, 2, 0, 1, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);

  label=gtk_label_new(_("genre"));
  gtk_misc_set_alignment(GTK_MISC(label),1.0,0.5);
  gtk_table_attach(GTK_TABLE(table),label, 0, 1, 1, 2, 0,0, 2,1);
  entry=gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table),entry, 1, 2, 1, 2, GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND, 2,1);

  // second row of hbox
  frame=gtk_frame_new(_("free text info"));
  gtk_widget_set_name(label,_("free text info"));
  gtk_box_pack_start(GTK_BOX(page),frame,TRUE,TRUE,0);

  scrolledwindow=gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_set_name(scrolledwindow,"scrolledwindow");
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledwindow),GTK_SHADOW_IN);
  gtk_container_add(GTK_CONTAINER(frame),scrolledwindow);

  textfield=gtk_text_view_new();
  gtk_widget_set_name(textfield,_("free text info"));
  //gtk_container_set_border_width(GTK_CONTAINER(textfield),1);
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textfield),GTK_WRAP_WORD);
  gtk_container_add(GTK_CONTAINER(scrolledwindow),textfield);

  return(notebook);
}

static gboolean bt_main_window_init_ui(const BtMainWindow *self) {
  GtkWidget *box;
  GtkWidget *notebook;
  GtkWidget *statusbar;
  
  self->private->accel_group=gtk_accel_group_new();
  
  g_signal_connect(G_OBJECT(self),"delete_event",G_CALLBACK(delete_event),(gpointer)self);
  g_signal_connect(G_OBJECT(self),"destroy",     G_CALLBACK(destroy),(gpointer)self);
  
  // create main layout container
  box=gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(self),box);

  // add the menu-bar
  self->private->menu=bt_main_menu_new(self->private->app,self->private->accel_group);
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->private->menu),FALSE,FALSE,0);
  // add the tool-bar
  self->private->toolbar=bt_main_toolbar_new(self->private->app);
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->private->toolbar),FALSE,FALSE,0);
  
  // add the notebook widget
  notebook=bt_main_window_init_notebook(self, box);
  
  // add the status bar
  self->private->statusbar=bt_main_statusbar_new(self->private->app);
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->private->statusbar),FALSE,FALSE,0);

  bt_main_window_refresh_ui(self);

  return(TRUE);
}

static void bt_main_window_run_ui(const BtMainWindow *self) {
  GST_INFO("show UI and start main-loop\n");
  gtk_widget_show_all(GTK_WIDGET(self));
  gtk_main();
}

static void bt_main_window_run_done(const BtMainWindow *self) {
}

//-- constructor methods

/**
 * bt_main_window_new:
 * @app: the application the window belongs to
 *
 * Create a new instance
 *
 * Return: the new instance or NULL in case of an error
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
  return(self);
Error:
  if(self) g_object_unref(self);
  return(NULL);
}

//-- methods

/**
 * bt_main_window_show_and_run:
 * @self: the window instance to setup and run
 *
 * build, show and run the main window
 *
 * Returns: true for success
 */
gboolean bt_main_window_show_and_run(const BtMainWindow *self) {
  gboolean res=TRUE;
  GST_INFO("before running the UI\n");
  // run UI loop
  bt_main_window_run_ui(self);
  // shut down UI
  bt_main_window_run_done(self);
  GST_INFO("after running the UI\n");
  return(res);
}

gboolean bt_main_window_check_quit(const BtMainWindow *self) {
  gboolean quit=FALSE;
  gint result;
  GtkWidget *label,*icon,*box;
  GtkWidget *dialog = gtk_dialog_new_with_buttons (_("Really quit ?"),
                                                  GTK_WINDOW(self),
                                                  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                  GTK_STOCK_OK,
                                                  GTK_RESPONSE_ACCEPT,
                                                  GTK_STOCK_CANCEL,
                                                  GTK_RESPONSE_REJECT,
                                                  NULL);

  box=gtk_hbox_new(FALSE,0);
  icon=gtk_image_new_from_stock(GTK_STOCK_DIALOG_QUESTION,GTK_ICON_SIZE_DIALOG);
  gtk_container_add(GTK_CONTAINER(box),icon);
  label=gtk_label_new(_("Really quit ?"));
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
      GST_WARNING("unhandled response code = %d\n",result);
  }
  gtk_widget_destroy(dialog);
  GST_INFO("menu quit result = %d\n",quit);
  return(quit);
}

void bt_main_window_new_song(const BtMainWindow *self) {
  // @todo if unsaved ask the use, if we should save the song
  if(bt_edit_application_new_song(self->private->app)) {
    bt_main_window_refresh_ui(self);
  }
}

void bt_main_window_open_song(const BtMainWindow *self) {
  GtkWidget *dialog=gtk_file_selection_new(_("Choose a song"));
  gint result;
    
  /* Lets set the filename, as if this were a save dialog, and we are giving
   a default filename */
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(dialog),DATADIR"/songs/");
  result=gtk_dialog_run(GTK_DIALOG(dialog));
  switch(result) {
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
      if(bt_edit_application_load_song(self->private->app,gtk_file_selection_get_filename(GTK_FILE_SELECTION(dialog)))) {
        bt_main_window_refresh_ui(self);
      }
      break;
    case GTK_RESPONSE_REJECT:
    case GTK_RESPONSE_CANCEL:
    case GTK_RESPONSE_CLOSE:
      break;
    default:
      GST_WARNING("unhandled response code = %d\n",result);
  } 
  gtk_widget_destroy(dialog);
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
      g_value_set_object(value, G_OBJECT(self->private->app));
    } break;
    default: {
 			g_assert(FALSE);
      break;
    }
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
      self->private->app = g_object_ref(G_OBJECT(g_value_get_object(value)));
      //GST_DEBUG("set the app for main_window: %p",self->private->app);
    } break;
    default: {
			g_assert(FALSE);
      break;
    }
  }
}

static void bt_main_window_dispose(GObject *object) {
  BtMainWindow *self = BT_MAIN_WINDOW(object);
	return_if_disposed();
  self->private->dispose_has_run = TRUE;
}

static void bt_main_window_finalize(GObject *object) {
  BtMainWindow *self = BT_MAIN_WINDOW(object);
  
  g_object_unref(G_OBJECT(self->private->app));
  g_free(self->private);
}

static void bt_main_window_init(GTypeInstance *instance, gpointer g_class) {
  BtMainWindow *self = BT_MAIN_WINDOW(instance);
  self->private = g_new0(BtMainWindowPrivate,1);
  self->private->dispose_has_run = FALSE;
}

static void bt_main_window_class_init(BtMainWindowClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;
  
  gobject_class->set_property = bt_main_window_set_property;
  gobject_class->get_property = bt_main_window_get_property;
  gobject_class->dispose      = bt_main_window_dispose;
  gobject_class->finalize     = bt_main_window_finalize;

  g_object_class_install_property(gobject_class,MAIN_WINDOW_APP,
                                  g_param_spec_object("app",
                                     "app contruct prop",
                                     "Set application object, the window belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));
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

