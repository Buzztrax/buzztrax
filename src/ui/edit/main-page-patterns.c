/* $Id: main-page-patterns.c,v 1.3 2004-08-23 15:45:38 ensonic Exp $
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

//-- event handler

static void on_song_changed(const BtEditApplication *app, gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  BtSong *song;
  BtSetup *setup;
  BtMachine *machine;
  GtkWidget *menu,*menu_item,*old_menu;
  gpointer *iter;
  gchar *str;

  GST_INFO("song has changed : app=%p, window=%p\n",song,user_data);
  // get song from app and then setup
  song=BT_SONG(bt_g_object_get_object_property(G_OBJECT(self->private->app),"song"));
  setup=bt_song_get_setup(song);
  // update page
  // update machine menu
  old_menu=gtk_option_menu_get_menu(self->private->machine_menu);
  menu=gtk_menu_new();
  iter=bt_setup_machine_iterator_new(setup);
  while(iter) {
    machine=bt_setup_machine_iterator_get_machine(iter);
    str=bt_g_object_get_string_property(G_OBJECT(machine),"id");
    GST_INFO("  adding \"%s\"\n",str);
    menu_item=gtk_menu_item_new_with_label(str);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
    gtk_widget_show(menu_item);
    iter=bt_setup_machine_iterator_next(iter);
  }
  gtk_option_menu_set_menu(self->private->machine_menu,menu);
  gtk_option_menu_set_history(self->private->machine_menu,0);
  if(old_menu) gtk_widget_destroy(old_menu);
  // update pattern menu (for widget id=0)
  old_menu=gtk_option_menu_get_menu(self->private->pattern_menu);
  menu=gtk_menu_new();
  // @todo get patterns forcurrently slected machine
  // foreach(pattnern) {
    menu_item=gtk_menu_item_new_with_label("patern");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
    gtk_widget_show(menu_item);
  //}
  gtk_option_menu_set_menu(self->private->pattern_menu,menu);
  gtk_option_menu_set_history(self->private->pattern_menu,0);
  if(old_menu) gtk_widget_destroy(old_menu);
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
  self->private->machine_menu=gtk_option_menu_new();
  // @todo do we really have to add the label by our self
  gtk_box_pack_start(GTK_BOX(box),gtk_label_new(_("Machine")),FALSE,FALSE,2);
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->private->machine_menu),TRUE,TRUE,2);

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
  self->private->pattern_menu=gtk_option_menu_new();
  // @todo do we really have to add the label by our self
  gtk_box_pack_start(GTK_BOX(box),gtk_label_new(_("Pattern")),FALSE,FALSE,2);
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->private->pattern_menu),TRUE,TRUE,2);

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
  return(TRUE);
}

//-- constructor methods

/**
 * bt_main_page_patterns_new:
 * @app: the application the window belongs to
 *
 * Create a new instance
 *
 * Return: the new instance or NULL in case of an error
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
  if(self) g_object_unref(self);
  return(NULL);
}

//-- methods

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
      g_value_set_object(value, G_OBJECT(self->private->app));
    } break;
    default: {
 			g_assert(FALSE);
      break;
    }
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
      self->private->app = g_object_ref(G_OBJECT(g_value_get_object(value)));
      //GST_DEBUG("set the app for MAIN_PAGE_PATTERNS: %p",self->private->app);
    } break;
    default: {
			g_assert(FALSE);
      break;
    }
  }
}

static void bt_main_page_patterns_dispose(GObject *object) {
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS(object);
	return_if_disposed();
  self->private->dispose_has_run = TRUE;
}

static void bt_main_page_patterns_finalize(GObject *object) {
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS(object);
  
  g_object_unref(G_OBJECT(self->private->app));
  g_free(self->private);
}

static void bt_main_page_patterns_init(GTypeInstance *instance, gpointer g_class) {
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS(instance);
  self->private = g_new0(BtMainPagePatternsPrivate,1);
  self->private->dispose_has_run = FALSE;
}

static void bt_main_page_patterns_class_init(BtMainPagePatternsClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;
  
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

