/* $Id: main-window.c,v 1.1 2004-08-06 19:42:45 ensonic Exp $
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

  /* the top-level window of our app */
  GtkWidget *window;
};

//-- event handler

static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
  /* If you return FALSE in the "delete_event" signal handler, GTK will emit the
   * "destroy" signal. Returning TRUE means you don't want the window to be
   * destroyed.
   * This is useful for popping up 'are you sure to quit?' type dialogs.
   */

  GST_INFO("delete event occurred\n");
 return(FALSE);
}

static void destroy(GtkWidget *widget, gpointer data) {
  /* destroy callback */
  gtk_main_quit();
}

//-- helper methods

GtkWidget *bt_main_window_init_menubar(const BtMainWindow *self, GtkWidget *box) {
  GtkWidget *menubar,*item;
  
  // @todo make this a sub-class of menu-bar
  menubar=gtk_menu_bar_new();
  gtk_widget_set_name(menubar,"main menu");
  gtk_box_pack_start(GTK_BOX(box),menubar,FALSE,FALSE,0);

  item=gtk_menu_item_new_with_mnemonic("_File");
  gtk_widget_set_name(item, "file menu");
  gtk_container_add(GTK_CONTAINER(menubar),item);

  return(menubar);
}

GtkWidget *bt_main_window_init_toolbar(const BtMainWindow *self, GtkWidget *box) {
  GtkWidget *handlebox,*toolbar;
  GtkWidget *tmp_toolbar_icon;
  GtkWidget *button;

  // @todo make this a sub-class of  tool-bar
  handlebox=gtk_handle_box_new();
  gtk_widget_set_name(handlebox,"handlebox for toolbar");
  gtk_box_pack_start(GTK_BOX(box),handlebox,FALSE,FALSE,0);

  toolbar=gtk_toolbar_new();
  gtk_widget_set_name(toolbar,"toolbar");
  gtk_container_add(GTK_CONTAINER(handlebox),toolbar);
  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar),GTK_TOOLBAR_BOTH);

  tmp_toolbar_icon=gtk_image_new_from_stock("gtk-new", gtk_toolbar_get_icon_size(GTK_TOOLBAR(toolbar)));
  button=gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                "New",
                                NULL,NULL,
                                tmp_toolbar_icon,NULL,NULL);
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,"new");
  
  return(handlebox);
}

GtkWidget *bt_main_window_init_notebook(const BtMainWindow *self, GtkWidget *box) {
  GtkWidget *notebook;
  GtkWidget *empty_notebook_page,*label;

  // @todo make this a sub-class of notebook widget
  notebook=gtk_notebook_new();
  gtk_widget_set_name(notebook, "song views");
  gtk_box_pack_start(GTK_BOX(box),notebook,TRUE,TRUE,0);

  empty_notebook_page=gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(notebook),empty_notebook_page);

  label=gtk_label_new("Machine View");
  gtk_widget_set_name(label,"Machine View");
  gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook),gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),0),label);

  return(notebook);
}

GtkWidget *bt_main_window_init_statusbar(const BtMainWindow *self, GtkWidget *box) {
  GtkWidget *statusbar;
  
  // @todo make this a sub-class of status bar
  statusbar=gtk_statusbar_new();
  gtk_widget_set_name(statusbar, "statusbar");
  gtk_box_pack_start(GTK_BOX(box),statusbar,FALSE,FALSE,0);

  return(statusbar);
}

static gboolean bt_main_window_init_ui(const BtMainWindow *self) {
  GtkWidget *box;
  GtkWidget *menubar;
  GtkWidget *handlebox;
  GtkWidget *notebook;
  GtkWidget *statusbar;
  
  // create the window
  self->private->window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  // @todo make the title: PACKAGE_NAME": %s",song->name 
  gtk_window_set_title(GTK_WINDOW(self->private->window), PACKAGE_NAME);
  g_signal_connect(G_OBJECT(self->private->window), "delete_event", G_CALLBACK(delete_event), NULL);
  g_signal_connect(G_OBJECT(self->private->window), "destroy",      G_CALLBACK(destroy), NULL);
  
  // create main layout container
  box=gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(self->private->window),box);

  // add the menu-bar
  menubar=bt_main_window_init_menubar(self, box);
  // add the tool-bar
  handlebox=bt_main_window_init_toolbar(self, box);
  // add the notebook widget
  notebook=bt_main_window_init_notebook(self, box);
  // add the status bar
  statusbar=bt_main_window_init_statusbar(self, box);

  return(TRUE);
}

static void bt_main_window_run_ui(const BtMainWindow *self) {
  GST_INFO("show UI and start main-loop\n");
  gtk_widget_show_all(self->private->window);
  gtk_main();
}

static void bt_main_window_run_done(const BtMainWindow *self) {
  self->private->window=NULL;
}

//-- constructor methods

/**
 * bt_main_window_new:
 * @app: the application the winodw belongs to
 *
 * Create a new instance
 *
 * Return: the new instance or NULL in case of an error
 */
BtMainWindow *bt_main_window_new(const BtEditApplication *app) {
  BtMainWindow *self;
  self=BT_MAIN_WINDOW(g_object_new(BT_TYPE_MAIN_WINDOW,"app",app));
  
  return(self);
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
  gboolean res=FALSE;
  GST_INFO("before running the UI\n");
  // generate UI
  if(bt_main_window_init_ui(self)) {
    // run UI loop
    bt_main_window_run_ui(self);
    // shut down UI
    bt_main_window_run_done(self);
    res=TRUE;
  }
  GST_INFO("after running the UI\n");
  return(res);
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
		type = g_type_register_static(G_TYPE_OBJECT,"BtMainWindow",&info,0);
  }
  return type;
}

