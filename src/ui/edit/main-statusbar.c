/* $Id: main-statusbar.c,v 1.3 2004-08-13 20:44:07 ensonic Exp $
 * class for the editor main tollbar
 */

#define BT_EDIT
#define BT_MAIN_STATUSBAR_C

#include "bt-edit.h"

enum {
  MAIN_STATUSBAR_APP=1,
};


struct _BtMainStatusbarPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;
  /* identifier of the status message group */
  gint status_context_id;
};

//-- event handler


//-- helper methods

static gboolean bt_main_statusbar_init_ui(const BtMainStatusbar *self) {
  gtk_widget_set_name(GTK_WIDGET(self),_("status bar"));

  self->private->status_context_id=gtk_statusbar_get_context_id(GTK_STATUSBAR(self),_("default"));

  gtk_statusbar_push(GTK_STATUSBAR(self),self->private->status_context_id,_("Ready to rock!"));
 
  return(TRUE);
}

//-- constructor methods

/**
 * bt_main_statusbar_new:
 * @app: the application the statusbar belongs to
 *
 * Create a new instance
 *
 * Return: the new instance or NULL in case of an error
 */
BtMainStatusbar *bt_main_statusbar_new(const BtEditApplication *app) {
  BtMainStatusbar *self;

  if(!(self=BT_MAIN_STATUSBAR(g_object_new(BT_TYPE_MAIN_STATUSBAR,"app",app,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_main_statusbar_init_ui(self)) {
    goto Error;
  }
  return(self);
Error:
  if(self) g_object_unref(self);
  return(NULL);
}

//-- methods


//-- class internals

/* returns a property for the given property_id for this object */
static void bt_main_statusbar_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtMainStatusbar *self = BT_MAIN_STATUSBAR(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_STATUSBAR_APP: {
      g_value_set_object(value, G_OBJECT(self->private->app));
    } break;
    default: {
 			g_assert(FALSE);
      break;
    }
  }
}

/* sets the given properties for this object */
static void bt_main_statusbar_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtMainStatusbar *self = BT_MAIN_STATUSBAR(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_STATUSBAR_APP: {
      self->private->app = g_object_ref(G_OBJECT(g_value_get_object(value)));
      //GST_DEBUG("set the app for main_statusbar: %p",self->private->app);
    } break;
    default: {
			g_assert(FALSE);
      break;
    }
  }
}

static void bt_main_statusbar_dispose(GObject *object) {
  BtMainStatusbar *self = BT_MAIN_STATUSBAR(object);
	return_if_disposed();
  self->private->dispose_has_run = TRUE;
}

static void bt_main_statusbar_finalize(GObject *object) {
  BtMainStatusbar *self = BT_MAIN_STATUSBAR(object);
  
  g_object_unref(G_OBJECT(self->private->app));
  g_free(self->private);
}

static void bt_main_statusbar_init(GTypeInstance *instance, gpointer g_class) {
  BtMainStatusbar *self = BT_MAIN_STATUSBAR(instance);
  self->private = g_new0(BtMainStatusbarPrivate,1);
  self->private->dispose_has_run = FALSE;
}

static void bt_main_statusbar_class_init(BtMainStatusbarClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;
  
  gobject_class->set_property = bt_main_statusbar_set_property;
  gobject_class->get_property = bt_main_statusbar_get_property;
  gobject_class->dispose      = bt_main_statusbar_dispose;
  gobject_class->finalize     = bt_main_statusbar_finalize;

  g_object_class_install_property(gobject_class,MAIN_STATUSBAR_APP,
                                  g_param_spec_object("app",
                                     "app contruct prop",
                                     "Set application object, the menu belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));
}

GType bt_main_statusbar_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtMainStatusbarClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_main_statusbar_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtMainStatusbar),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_main_statusbar_init, // instance_init
    };
		type = g_type_register_static(GTK_TYPE_STATUSBAR,"BtMainStatusbar",&info,0);
  }
  return type;
}

