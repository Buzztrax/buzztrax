/* $Id: main-menu.c,v 1.39 2005-05-18 11:37:32 ensonic Exp $
 * class for the editor main menu
 */

#define BT_EDIT
#define BT_MAIN_MENU_C

#include "bt-edit.h"

enum {
  MAIN_MENU_APP=1,
};


struct _BtMainMenuPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;
	
	/* MenuItems */
	GtkWidget *save_item;	
};

static GtkMenuBarClass *parent_class=NULL;

//-- event handler

static void on_menu_quit_activate(GtkMenuItem *menuitem,gpointer user_data) {
  gboolean cont;
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtMainWindow *main_window;

  g_assert(user_data);

  GST_INFO("menu quit event occurred");
  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
	g_signal_emit_by_name(G_OBJECT(main_window),"delete_event",(gpointer)main_window,&cont);
  g_object_unref(main_window);
	GST_DEBUG("  result = %d",cont);
  if(!cont) {
		gtk_widget_destroy(GTK_WIDGET(main_window));
	}
}

static void on_menu_new_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtMainWindow *main_window;

  g_assert(user_data);

  GST_INFO("menu new event occurred");
  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
  bt_main_window_new_song(main_window);
  g_object_try_unref(main_window);
}

static void on_menu_open_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtMainWindow *main_window;

  g_assert(user_data);

  GST_INFO("menu open event occurred");
  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
  bt_main_window_open_song(main_window);
  g_object_try_unref(main_window);
}

static void on_menu_save_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtMainWindow *main_window;

  g_assert(user_data);

  GST_INFO("menu save event occurred");
  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
  bt_main_window_save_song(main_window);
  g_object_try_unref(main_window);
}

static void on_menu_saveas_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtMainWindow *main_window;

  g_assert(user_data);

  GST_INFO("menu saveas event occurred");
  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
  bt_main_window_save_song_as(main_window);
  g_object_try_unref(main_window);
}

static void on_menu_settings_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  GtkWidget *dialog;
  
  g_assert(user_data);

  GST_INFO("menu settings event occurred");  
  dialog=GTK_WIDGET(bt_settings_dialog_new(self->priv->app));
  
  gtk_widget_show_all(dialog);
                                                  
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

static void on_menu_view_toolbar_toggled(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
	BtMainWindow *main_window;
	BtMainToolbar *toolbar;
	BtSettings *settings;

  g_assert(user_data);

  GST_INFO("menu 'view toolbar' event occurred");
	g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,"settings",&settings,NULL);
	g_object_get(G_OBJECT(main_window),"toolbar",&toolbar,NULL);
	
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem))) {
		gtk_widget_show(GTK_WIDGET(toolbar));
		g_object_set(G_OBJECT(settings),"toolbar-hide",FALSE,NULL);
}
	else {
		gtk_widget_hide(GTK_WIDGET(toolbar));
		g_object_set(G_OBJECT(settings),"toolbar-hide",TRUE,NULL);
	}
	
	g_object_try_unref(toolbar);
	g_object_try_unref(settings);
	g_object_try_unref(main_window);
}

static void on_menu_view_tabs_toggled(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
	BtMainWindow *main_window;
	BtMainPages *pages;
	BtSettings *settings;

  g_assert(user_data);

  GST_INFO("menu 'view tabs' event occurred");
	g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,"settings",&settings,NULL);
	g_object_get(G_OBJECT(main_window),"pages",&pages,NULL);

	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem))) {
		gtk_notebook_set_show_tabs(GTK_NOTEBOOK(pages),TRUE);
		g_object_set(G_OBJECT(settings),"tabs-hide",FALSE,NULL);
}
	else {
		gtk_notebook_set_show_tabs(GTK_NOTEBOOK(pages),FALSE);
		g_object_set(G_OBJECT(settings),"tabs-hide",TRUE,NULL);
	}
	
	g_object_try_unref(pages);
	g_object_try_unref(settings);
	g_object_try_unref(main_window);
}

static void on_menu_goto_machine_view_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtMainWindow *main_window;
	BtMainPages *pages;
  
  g_assert(user_data);

  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
	g_object_get(G_OBJECT(main_window),"pages",&pages,NULL);
	
	gtk_notebook_set_current_page(GTK_NOTEBOOK(pages),BT_MAIN_PAGES_MACHINES_PAGE);

	g_object_try_unref(pages);
	g_object_try_unref(main_window);
}

static void on_menu_goto_pattern_view_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtMainWindow *main_window;
	BtMainPages *pages;
  
  g_assert(user_data);

  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
	g_object_get(G_OBJECT(main_window),"pages",&pages,NULL);
	
	gtk_notebook_set_current_page(GTK_NOTEBOOK(pages),BT_MAIN_PAGES_PATTERNS_PAGE);

	g_object_try_unref(pages);
	g_object_try_unref(main_window);
}

static void on_menu_goto_sequence_view_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtMainWindow *main_window;
	BtMainPages *pages;
  
  g_assert(user_data);

  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
	g_object_get(G_OBJECT(main_window),"pages",&pages,NULL);
	
	gtk_notebook_set_current_page(GTK_NOTEBOOK(pages),BT_MAIN_PAGES_SEQUENCE_PAGE);

	g_object_try_unref(pages);
	g_object_try_unref(main_window);
}

static void on_menu_goto_waves_view_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtMainWindow *main_window;
	BtMainPages *pages;
  
  g_assert(user_data);

  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
	g_object_get(G_OBJECT(main_window),"pages",&pages,NULL);
	
	gtk_notebook_set_current_page(GTK_NOTEBOOK(pages),BT_MAIN_PAGES_WAVES_PAGE);

	g_object_try_unref(pages);
	g_object_try_unref(main_window);
}

static void on_menu_goto_info_view_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtMainWindow *main_window;
	BtMainPages *pages;
  
  g_assert(user_data);

  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
	g_object_get(G_OBJECT(main_window),"pages",&pages,NULL);
	
	gtk_notebook_set_current_page(GTK_NOTEBOOK(pages),BT_MAIN_PAGES_INFO_PAGE);

	g_object_try_unref(pages);
	g_object_try_unref(main_window);
}

static void on_menu_about_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtMainWindow *main_window;
  gchar *str;
  
  g_assert(user_data);

  GST_INFO("menu about event occurred");
  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
	
	str=g_strdup_printf(
		"%s\n\nhttp://www.buzztard.org",
    _("brought to you by\n\nStefan 'ensonic' Kost\nThomas 'waffel' Wabner")
  );
	bt_dialog_message(main_window,_("About ..."),PACKAGE_STRING,str);

  g_free(str);
  g_object_try_unref(main_window);
}

static void on_song_unsaved_changed(const BtSong *song,GParamSpec *arg,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
	gboolean unsaved;

	g_assert(user_data);
	
	GST_INFO("song.unsaved has changed : song=%p, menu=%p",song,user_data);
	
  g_object_get(G_OBJECT(song),"unsaved",&unsaved,NULL);
	gtk_widget_set_sensitive(self->priv->save_item,unsaved);
}	

static void on_song_changed(const BtEditApplication *app,GParamSpec *arg,gpointer user_data) {
  BtMainMenu *self=BT_MAIN_MENU(user_data);
  BtSong *song;

  g_assert(user_data);

  GST_INFO("song has changed : app=%p, toolbar=%p",app,user_data);
  
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
	g_return_if_fail(song);

	g_signal_connect(G_OBJECT(song), "notify::unsaved", G_CALLBACK(on_song_unsaved_changed), (gpointer)self);
  g_object_try_unref(song);

}


//-- helper methods

static gboolean bt_main_menu_init_ui(const BtMainMenu *self,GtkAccelGroup *accel_group) {
  GtkWidget *item,*menu,*subitem,*image;
	BtSettings *settings;
	gboolean toolbar_hide,tabs_hide;
	
  gtk_widget_set_name(GTK_WIDGET(self),_("main menu"));
	g_object_get(G_OBJECT(self->priv->app),"settings",&settings,NULL);
	g_object_get(G_OBJECT(settings),"toolbar-hide",&toolbar_hide,"tabs-hide",&tabs_hide,NULL);
	g_object_unref(settings);

  //-- file menu
  item=gtk_menu_item_new_with_mnemonic(_("_File"));
  gtk_widget_set_name(item,_("file menu"));
  gtk_container_add(GTK_CONTAINER(self),item);

  menu=gtk_menu_new();
  gtk_widget_set_name(menu,_("file menu"));
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item),menu);

  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_NEW,accel_group);
  gtk_widget_set_name(subitem,_("New"));
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_new_activate),(gpointer)self);

  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN,accel_group);
  gtk_widget_set_name(subitem,_("Open"));
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_open_activate),(gpointer)self);

  subitem=gtk_separator_menu_item_new();
  gtk_widget_set_name(subitem,_("separator"));
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  gtk_widget_set_sensitive(subitem,FALSE);

  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_SAVE,accel_group);
  gtk_widget_set_name(subitem,_("Save"));
  gtk_container_add(GTK_CONTAINER(menu),subitem);
	g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_save_activate),(gpointer)self);
	self->priv->save_item=subitem;

  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_SAVE_AS,accel_group);
  gtk_widget_set_name(subitem,_("Save as"));
  gtk_container_add(GTK_CONTAINER(menu),subitem);
	g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_saveas_activate),(gpointer)self);

  subitem=gtk_separator_menu_item_new();
  gtk_widget_set_name(subitem,_("separator"));
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  gtk_widget_set_sensitive(subitem,FALSE);

  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT,accel_group);
  gtk_widget_set_name(subitem,_("Quit"));
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_quit_activate),(gpointer)self);

  // edit menu
  item=gtk_menu_item_new_with_mnemonic(_("_Edit"));
  gtk_widget_set_name(item,_("edit menu"));
  gtk_container_add(GTK_CONTAINER(self),item);

  menu=gtk_menu_new();
  gtk_widget_set_name(menu,_("edit menu"));
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item),menu);

  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_CUT,accel_group);
  gtk_widget_set_name(subitem,_("cut"));
  gtk_container_add(GTK_CONTAINER(menu),subitem);

  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_COPY,accel_group);
  gtk_widget_set_name(subitem,_("copy"));
  gtk_container_add(GTK_CONTAINER(menu),subitem);

  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_PASTE,accel_group);
  gtk_widget_set_name(subitem,_("paste"));
  gtk_container_add(GTK_CONTAINER(menu),subitem);

  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE,accel_group);
  gtk_widget_set_name(subitem,_("delete"));
  gtk_container_add(GTK_CONTAINER(menu),subitem);

  subitem=gtk_separator_menu_item_new();
  gtk_widget_set_name(subitem,_("separator"));
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  gtk_widget_set_sensitive(subitem,FALSE);

  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES,accel_group);
  gtk_widget_set_name(subitem,_("settings"));
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_settings_activate),(gpointer)self);
  
  // view menu
  item=gtk_menu_item_new_with_mnemonic(_("_View"));
  gtk_widget_set_name(item,_("view menu"));
  gtk_container_add(GTK_CONTAINER(self),item);

  menu=gtk_menu_new();
  gtk_widget_set_name(menu,_("view menu"));
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item),menu);

  subitem=gtk_check_menu_item_new_with_mnemonic(_("Toolbar"));
  gtk_widget_set_name(subitem,_("Toolbar"));
	// from here we can't hide the toolbar as it is not yet created and shown
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(subitem),!toolbar_hide);
  gtk_container_add(GTK_CONTAINER(menu),subitem);
	g_signal_connect(G_OBJECT(subitem),"toggled",G_CALLBACK(on_menu_view_toolbar_toggled),(gpointer)self);

  subitem=gtk_check_menu_item_new_with_mnemonic(_("Tabs"));
  gtk_widget_set_name(subitem,_("Tabs"));
	// from here we can't hide the tabs as they are not yet created and shown
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(subitem),!tabs_hide);
  gtk_container_add(GTK_CONTAINER(menu),subitem);
	g_signal_connect(G_OBJECT(subitem),"toggled",G_CALLBACK(on_menu_view_tabs_toggled),(gpointer)self);

  subitem=gtk_separator_menu_item_new();
  gtk_widget_set_name(subitem,_("separator"));
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  gtk_widget_set_sensitive(subitem,FALSE);
	
  subitem=gtk_image_menu_item_new_with_mnemonic(_("Go to machine view"));
  gtk_widget_set_name(subitem,_("Go to machine view"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(subitem),gtk_image_new_from_filename("tab_machines.png"));
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_goto_machine_view_activate),(gpointer)self);
	
  subitem=gtk_image_menu_item_new_with_mnemonic(_("Go to pattern view"));
  gtk_widget_set_name(subitem,_("Go to pattern view"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(subitem),gtk_image_new_from_filename("tab_patterns.png"));
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_goto_pattern_view_activate),(gpointer)self);

  subitem=gtk_image_menu_item_new_with_mnemonic(_("Go to sequence view"));
  gtk_widget_set_name(subitem,_("Go to sequence view"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(subitem),gtk_image_new_from_filename("tab_sequence.png"));
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_goto_sequence_view_activate),(gpointer)self);

  subitem=gtk_image_menu_item_new_with_mnemonic(_("Go to wave table view"));
  gtk_widget_set_name(subitem,_("Go to wave table view"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(subitem),gtk_image_new_from_filename("tab_waves.png"));
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_goto_waves_view_activate),(gpointer)self);

  subitem=gtk_image_menu_item_new_with_mnemonic(_("Go to song information"));
  gtk_widget_set_name(subitem,_("Go to song information"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(subitem),gtk_image_new_from_filename("tab_info.png"));
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_goto_info_view_activate),(gpointer)self);

	// help menu
  item=gtk_menu_item_new_with_mnemonic(_("_Help"));
  gtk_widget_set_name(item,_("help menu"));
  gtk_menu_item_right_justify(GTK_MENU_ITEM(item));
  gtk_container_add(GTK_CONTAINER(self),item);

  menu=gtk_menu_new();
  gtk_widget_set_name(menu,_("help menu"));
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item),menu);

  subitem=gtk_image_menu_item_new_from_stock(GTK_STOCK_HELP,accel_group);
  gtk_widget_set_name(subitem,_("Content"));
  gtk_container_add(GTK_CONTAINER(menu),subitem);

  subitem=gtk_image_menu_item_new_with_mnemonic(_("About"));
  gtk_widget_set_name(subitem,_("About"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(subitem),gtk_image_new_from_filename("stock_about.png"));
  gtk_container_add(GTK_CONTAINER(menu),subitem);
  g_signal_connect(G_OBJECT(subitem),"activate",G_CALLBACK(on_menu_about_activate),(gpointer)self);

  // register event handlers
  g_signal_connect(G_OBJECT(self->priv->app), "notify::song", G_CALLBACK(on_song_changed), (gpointer)self);

  return(TRUE);
}

//-- constructor methods

/**
 * bt_main_menu_new:
 * @app: the application the menu belongs to
 * @accel_group: the main-windows accelerator group
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtMainMenu *bt_main_menu_new(const BtEditApplication *app,GtkAccelGroup *accel_group) {
  BtMainMenu *self;

  if(!(self=BT_MAIN_MENU(g_object_new(BT_TYPE_MAIN_MENU,"app",app,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_main_menu_init_ui(self,accel_group)) {
    goto Error;
  }
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods


//-- class internals

/* returns a property for the given property_id for this object */
static void bt_main_menu_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtMainMenu *self = BT_MAIN_MENU(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_MENU_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    default: {
 			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_main_menu_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtMainMenu *self = BT_MAIN_MENU(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_MENU_APP: {
      g_object_try_weak_unref(self->priv->app);
      self->priv->app = BT_EDIT_APPLICATION(g_value_get_object(value));
			g_object_try_weak_ref(self->priv->app);
      //GST_DEBUG("set the app for main_menu: %p",self->priv->app);
    } break;
    default: {
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_main_menu_dispose(GObject *object) {
  BtMainMenu *self = BT_MAIN_MENU(object);
	return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);	
  g_object_try_weak_unref(self->priv->app);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_main_menu_finalize(GObject *object) {
  BtMainMenu *self = BT_MAIN_MENU(object);
  
  GST_DEBUG("!!!! self=%p",self);
  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_main_menu_init(GTypeInstance *instance, gpointer g_class) {
  BtMainMenu *self = BT_MAIN_MENU(instance);
  self->priv = g_new0(BtMainMenuPrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_main_menu_class_init(BtMainMenuClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(GTK_TYPE_MENU_BAR);
  
  gobject_class->set_property = bt_main_menu_set_property;
  gobject_class->get_property = bt_main_menu_get_property;
  gobject_class->dispose      = bt_main_menu_dispose;
  gobject_class->finalize     = bt_main_menu_finalize;

  g_object_class_install_property(gobject_class,MAIN_MENU_APP,
                                  g_param_spec_object("app",
                                     "app contruct prop",
                                     "Set application object, the menu belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));
}

GType bt_main_menu_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtMainMenuClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_main_menu_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtMainMenu),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_main_menu_init, // instance_init
			NULL // value_table
    };
		type = g_type_register_static(GTK_TYPE_MENU_BAR,"BtMainMenu",&info,0);
  }
  return type;
}
