/* $Id: main-page-patterns.c,v 1.13 2004-09-29 16:56:47 ensonic Exp $
 * class for the editor main machines page
 */

#define BT_EDIT
#define BT_MAIN_PAGE_PATTERNS_C

#include "bt-edit.h"

enum {
  MAIN_PAGE_PATTERNS_APP=1,
};


struct _BtMainPagePatternsPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the application */
  BtEditApplication *app;
  
  /* machine selection menu */
  GtkOptionMenu *machine_menu;
  /* pattern selection menu */
  GtkOptionMenu *pattern_menu;
};

static GtkVBoxClass *parent_class=NULL;

//-- event handler helper

static void machine_menu_refresh(const BtMainPagePatterns *self,const BtSetup *setup) {
  BtMachine *machine;
  GtkWidget *menu,*menu_item;
  gpointer *iter;
  gchar *str;

  // update machine menu
  menu=gtk_menu_new();
  iter=bt_setup_machine_iterator_new(setup);
  while(iter) {
    machine=bt_setup_machine_iterator_get_machine(iter);
    g_object_get(G_OBJECT(machine),"id",&str,NULL);
    GST_INFO("  adding \"%s\"",str);
    menu_item=gtk_menu_item_new_with_label(str);g_free(str);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
    gtk_widget_show(menu_item);
    iter=bt_setup_machine_iterator_next(iter);
  }
  gtk_option_menu_set_menu(self->priv->machine_menu,menu);
  gtk_option_menu_set_history(self->priv->machine_menu,0);
}

static void pattern_menu_refresh(const BtMainPagePatterns *self,const BtMachine *machine) {
  BtPattern *pattern=NULL;
  GtkWidget *menu,*menu_item;
  gpointer *iter;
  gchar *str;

  // update pattern menu
  menu=gtk_menu_new();
  if(machine) {
    iter=bt_machine_pattern_iterator_new(machine);
    while(iter) {
      pattern=bt_machine_pattern_iterator_get_pattern(iter);
      g_object_get(G_OBJECT(pattern),"name",&str,NULL);
      GST_INFO("  adding \"%s\"",str);
      menu_item=gtk_menu_item_new_with_label(str);g_free(str);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
      gtk_widget_show(menu_item);
      iter=bt_machine_pattern_iterator_next(iter);
    }
  }
  if(!pattern) { // generate an empty item
    menu_item=gtk_menu_item_new_with_label("---");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
    gtk_widget_show(menu_item);
  }
  gtk_option_menu_set_menu(self->priv->pattern_menu,menu);
  gtk_option_menu_set_history(self->priv->pattern_menu,0);
}

//-- event handler

static void on_machine_menu_changed(GtkOptionMenu *optionmenu, gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);

  pattern_menu_refresh(self,bt_main_page_patterns_get_current_machine(self));
}

static void on_song_changed(const BtEditApplication *app, gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  BtSong *song;
  BtSetup *setup;
  BtMachine *machine;

  GST_INFO("song has changed : app=%p, self=%p",app,self);
  // get song from app and then setup from song
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"setup",&setup,NULL);
  // update page
  machine_menu_refresh(self,setup);
  pattern_menu_refresh(self,bt_main_page_patterns_get_current_machine(self));
  // release the reference
  g_object_try_unref(setup);
  g_object_try_unref(song);
}

//-- helper methods

static gboolean bt_main_page_patterns_init_ui(const BtMainPagePatterns *self, const BtEditApplication *app) {
  GtkWidget *toolbar;
  GtkWidget *box,*menu,*button;

  // add toolbar
  toolbar=gtk_toolbar_new();
  gtk_widget_set_name(toolbar,_("machine view tool bar"));
  gtk_box_pack_start(GTK_BOX(self),toolbar,FALSE,FALSE,0);
  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar),GTK_TOOLBAR_BOTH);
  
  // @todo add toolbar widgets
  // machine select
  box=gtk_hbox_new(FALSE,2);
  gtk_container_set_border_width(GTK_CONTAINER(box),4);
  self->priv->machine_menu=GTK_OPTION_MENU(gtk_option_menu_new());
  // @todo do we really have to add the label by our self
  gtk_box_pack_start(GTK_BOX(box),gtk_label_new(_("Machine")),FALSE,FALSE,2);
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->priv->machine_menu),TRUE,TRUE,2);

  button=gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
                                GTK_TOOLBAR_CHILD_WIDGET,
                                box,
                                NULL,
                                NULL,NULL,
                                NULL,NULL,NULL);
  //gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("Machine"));
  gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

  // pattern select
  box=gtk_hbox_new(FALSE,2);
  gtk_container_set_border_width(GTK_CONTAINER(box),4);
  self->priv->pattern_menu=GTK_OPTION_MENU(gtk_option_menu_new());
  // @todo do we really have to add the label by our self
  gtk_box_pack_start(GTK_BOX(box),gtk_label_new(_("Pattern")),FALSE,FALSE,2);
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->priv->pattern_menu),TRUE,TRUE,2);

  button=gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
                                GTK_TOOLBAR_CHILD_WIDGET,
                                box,
                                NULL,
                                NULL,NULL,
                                NULL,NULL,NULL);
  //gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("Pattern"));
  gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

  // wavetable entry select
  // base octave
  // play notes ?
  
  // @todo add list-view  
  gtk_container_add(GTK_CONTAINER(self),gtk_label_new("no pattern view yet"));

  // register event handlers
  g_signal_connect(G_OBJECT(app), "song-changed", (GCallback)on_song_changed, (gpointer)self);
  g_signal_connect(G_OBJECT(self->priv->machine_menu), "changed", (GCallback)on_machine_menu_changed, (gpointer)self);
  return(TRUE);
}

//-- constructor methods

/**
 * bt_main_page_patterns_new:
 * @app: the application the window belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtMainPagePatterns *bt_main_page_patterns_new(const BtEditApplication *app) {
  BtMainPagePatterns *self;

  if(!(self=BT_MAIN_PAGE_PATTERNS(g_object_new(BT_TYPE_MAIN_PAGE_PATTERNS,"app",app,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_main_page_patterns_init_ui(self,app)) {
    goto Error;
  }
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods

/**
 * bt_main_page_patterns_get_current_machine:
 * @self: the pattern subpage
 *
 * Get the currently active #BtMachine as determined by the machine option menu
 * in the toolbar.
 *
 * Returns: the #BtMachine instance or NULL in case of an error
 */
BtMachine *bt_main_page_patterns_get_current_machine(const BtMainPagePatterns *self) {
  glong index;
  BtSong *song;
  BtSetup *setup;
  BtMachine *machine;

  GST_INFO("get machine for pattern");
  
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"setup",&setup,NULL);

  index=gtk_option_menu_get_history(self->priv->machine_menu);
  machine=bt_setup_get_machine_by_index(setup,index);

  //-- release the reference
  g_object_try_unref(setup);
  g_object_try_unref(song);
  return(machine);
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_main_page_patterns_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_PAGE_PATTERNS_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    default: {
 			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_main_page_patterns_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_PAGE_PATTERNS_APP: {
      g_object_try_unref(self->priv->app);
      self->priv->app = g_object_try_ref(g_value_get_object(value));
      //GST_DEBUG("set the app for MAIN_PAGE_PATTERNS: %p",self->priv->app);
    } break;
    default: {
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_main_page_patterns_dispose(GObject *object) {
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS(object);
	return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  g_object_try_unref(self->priv->app);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_main_page_patterns_finalize(GObject *object) {
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS(object);
  
  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_main_page_patterns_init(GTypeInstance *instance, gpointer g_class) {
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS(instance);
  self->priv = g_new0(BtMainPagePatternsPrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_main_page_patterns_class_init(BtMainPagePatternsClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;

  parent_class=g_type_class_ref(GTK_TYPE_VBOX);
    
  gobject_class->set_property = bt_main_page_patterns_set_property;
  gobject_class->get_property = bt_main_page_patterns_get_property;
  gobject_class->dispose      = bt_main_page_patterns_dispose;
  gobject_class->finalize     = bt_main_page_patterns_finalize;

  g_object_class_install_property(gobject_class,MAIN_PAGE_PATTERNS_APP,
                                  g_param_spec_object("app",
                                     "app contruct prop",
                                     "Set application object, the window belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));
}

GType bt_main_page_patterns_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtMainPagePatternsClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_main_page_patterns_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtMainPagePatterns),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_main_page_patterns_init, // instance_init
    };
		type = g_type_register_static(GTK_TYPE_VBOX,"BtMainPagePatterns",&info,0);
  }
  return type;
}

