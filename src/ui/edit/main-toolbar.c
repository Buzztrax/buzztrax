/* $Id: main-toolbar.c,v 1.1 2004-08-12 14:02:59 ensonic Exp $
 * class for the editor main tollbar
 */

#define BT_EDIT
#define BT_MAIN_TOOLBAR_C

#include "bt-edit.h"

enum {
  MAIN_TOOLBAR_APP=1,
};


struct _BtMainToolbarPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;
};

//-- event handler

static void on_toolbar_new_clicked(GtkButton *button, gpointer data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(data);
  GST_INFO("toolbar new event occurred\n");
  bt_main_window_new_song(BT_MAIN_WINDOW(bt_g_object_get_object_property(G_OBJECT(self->private->app),"main window")));
}

static void on_toolbar_open_clicked(GtkButton *button, gpointer data) {
  BtMainToolbar *self=BT_MAIN_TOOLBAR(data);
  GST_INFO("toolbar open event occurred\n");
  bt_main_window_open_song(BT_MAIN_WINDOW(bt_g_object_get_object_property(G_OBJECT(self->private->app),"main window")));
}

//-- helper methods

static gboolean bt_main_toolbar_init_ui(const BtMainToolbar *self) {
  GtkWidget *toolbar;
  GtkWidget *icon;
  GtkWidget *button;

  gtk_widget_set_name(GTK_WIDGET(self),_("handlebox for toolbar"));

  toolbar=gtk_toolbar_new();
  gtk_widget_set_name(toolbar,_("tool bar"));
  gtk_container_add(GTK_CONTAINER(self),toolbar);
  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar),GTK_TOOLBAR_BOTH);

  icon=gtk_image_new_from_stock(GTK_STOCK_NEW, gtk_toolbar_get_icon_size(GTK_TOOLBAR(toolbar)));
  button=gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                _("New"),
                                NULL,NULL,
                                icon,NULL,NULL);
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("New"));
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(on_toolbar_new_clicked),(gpointer)self);

  icon=gtk_image_new_from_stock(GTK_STOCK_OPEN, gtk_toolbar_get_icon_size(GTK_TOOLBAR(toolbar)));
  button=gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                _("Open"),
                                NULL,NULL,
                                icon,NULL,NULL);
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("Open"));
  g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(on_toolbar_open_clicked),(gpointer)self);

  icon=gtk_image_new_from_stock(GTK_STOCK_SAVE, gtk_toolbar_get_icon_size(GTK_TOOLBAR(toolbar)));
  button=gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                _("Save"),
                                NULL,NULL,
                                icon,NULL,NULL);
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("Save"));

  return(TRUE);
}

//-- constructor methods

/**
 * bt_main_toolbar_new:
 * @app: the application the toolbar belongs to
 *
 * Create a new instance
 *
 * Return: the new instance or NULL in case of an error
 */
BtMainToolbar *bt_main_toolbar_new(const BtEditApplication *app) {
  BtMainToolbar *self;

  if(!(self=BT_MAIN_TOOLBAR(g_object_new(BT_TYPE_MAIN_TOOLBAR,"app",app,NULL)))) {
    goto Error;
  }
  bt_main_toolbar_init_ui(self);
  return(self);
Error:
  return(NULL);
}

//-- methods


//-- class internals

/* returns a property for the given property_id for this object */
static void bt_main_toolbar_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtMainToolbar *self = BT_MAIN_TOOLBAR(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_TOOLBAR_APP: {
      g_value_set_object(value, G_OBJECT(self->private->app));
    } break;
    default: {
 			g_assert(FALSE);
      break;
    }
  }
}

/* sets the given properties for this object */
static void bt_main_toolbar_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtMainToolbar *self = BT_MAIN_TOOLBAR(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_TOOLBAR_APP: {
      self->private->app = g_object_ref(G_OBJECT(g_value_get_object(value)));
      //GST_DEBUG("set the app for main_toolbar: %p",self->private->app);
    } break;
    default: {
			g_assert(FALSE);
      break;
    }
  }
}

static void bt_main_toolbar_dispose(GObject *object) {
  BtMainToolbar *self = BT_MAIN_TOOLBAR(object);
	return_if_disposed();
  self->private->dispose_has_run = TRUE;
}

static void bt_main_toolbar_finalize(GObject *object) {
  BtMainToolbar *self = BT_MAIN_TOOLBAR(object);
  
  g_object_unref(G_OBJECT(self->private->app));
  g_free(self->private);
}

static void bt_main_toolbar_init(GTypeInstance *instance, gpointer g_class) {
  BtMainToolbar *self = BT_MAIN_TOOLBAR(instance);
  self->private = g_new0(BtMainToolbarPrivate,1);
  self->private->dispose_has_run = FALSE;
}

static void bt_main_toolbar_class_init(BtMainToolbarClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;
  
  gobject_class->set_property = bt_main_toolbar_set_property;
  gobject_class->get_property = bt_main_toolbar_get_property;
  gobject_class->dispose      = bt_main_toolbar_dispose;
  gobject_class->finalize     = bt_main_toolbar_finalize;

  g_object_class_install_property(gobject_class,MAIN_TOOLBAR_APP,
                                  g_param_spec_object("app",
                                     "app contruct prop",
                                     "Set application object, the menu belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));
}

GType bt_main_toolbar_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtMainToolbarClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_main_toolbar_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtMainToolbar),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_main_toolbar_init, // instance_init
    };
		type = g_type_register_static(GTK_TYPE_HANDLE_BOX,"BtMainToolbar",&info,0);
  }
  return type;
}

