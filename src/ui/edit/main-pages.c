/* $Id: main-pages.c,v 1.2 2004-08-13 18:58:11 ensonic Exp $
 * class for the editor main pages
 */

#define BT_EDIT
#define BT_MAIN_PAGES_C

#include "bt-edit.h"

enum {
  MAIN_PAGES_APP=1,
};


struct _BtMainPagesPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the application */
  BtEditApplication *app;
  
  /* the information tab */
  BtMainPageInfo *info_page;
};

//-- event handler

//-- helper methods

static gboolean bt_main_pages_init_ui(const BtMainPages *self, const BtEditApplication *app) {
  GtkWidget *page,*label,*frame;
  GtkWidget *table,*entry;
  GtkWidget *scrolledwindow,*textfield;

  // @todo split pages into separate classes
  gtk_widget_set_name(GTK_WIDGET(self),_("song views"));

  // @todo add real wigets for machine view
  // (canvas)
  page=gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(self),page);

  label=gtk_label_new(_("machine view"));
  gtk_widget_set_name(label,_("machine view"));
  gtk_notebook_set_tab_label(GTK_NOTEBOOK(self),gtk_notebook_get_nth_page(GTK_NOTEBOOK(self),0),label);
  gtk_container_add(GTK_CONTAINER(page),gtk_label_new("nothing here"));

  // @todo add real wigets for pattern view
  // table ?
  page=gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(self),page);

  label=gtk_label_new(_("pattern view"));
  gtk_widget_set_name(label,_("pattern view"));
  gtk_notebook_set_tab_label(GTK_NOTEBOOK(self),gtk_notebook_get_nth_page(GTK_NOTEBOOK(self),1),label);

  // @todo add real wigets for sequence view
  // table ?
  page=gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(self),page);

  label=gtk_label_new(_("sequence view"));
  gtk_widget_set_name(label,_("sequence view"));
  gtk_notebook_set_tab_label(GTK_NOTEBOOK(self),gtk_notebook_get_nth_page(GTK_NOTEBOOK(self),2),label);
  
  // add widgets for song info view
  self->private->info_page=bt_main_page_info_new(app);
  gtk_container_add(GTK_CONTAINER(self),GTK_WIDGET(self->private->info_page));

  label=gtk_label_new(_("song information"));
  gtk_widget_set_name(label,_("song information"));
  gtk_notebook_set_tab_label(GTK_NOTEBOOK(self),gtk_notebook_get_nth_page(GTK_NOTEBOOK(self),3),label);

  return(TRUE);
}

//-- constructor methods

/**
 * bt_main_pages_new:
 * @app: the application the window belongs to
 *
 * Create a new instance
 *
 * Return: the new instance or NULL in case of an error
 */
BtMainPages *bt_main_pages_new(const BtEditApplication *app) {
  BtMainPages *self;

  if(!(self=BT_MAIN_PAGES(g_object_new(BT_TYPE_MAIN_PAGES,"app",app,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_main_pages_init_ui(self,app)) {
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
static void bt_main_pages_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtMainPages *self = BT_MAIN_PAGES(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_PAGES_APP: {
      g_value_set_object(value, G_OBJECT(self->private->app));
    } break;
    default: {
 			g_assert(FALSE);
      break;
    }
  }
}

/* sets the given properties for this object */
static void bt_main_pages_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtMainPages *self = BT_MAIN_PAGES(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_PAGES_APP: {
      self->private->app = g_object_ref(G_OBJECT(g_value_get_object(value)));
      //GST_DEBUG("set the app for main_pages: %p",self->private->app);
    } break;
    default: {
			g_assert(FALSE);
      break;
    }
  }
}

static void bt_main_pages_dispose(GObject *object) {
  BtMainPages *self = BT_MAIN_PAGES(object);
	return_if_disposed();
  self->private->dispose_has_run = TRUE;
}

static void bt_main_pages_finalize(GObject *object) {
  BtMainPages *self = BT_MAIN_PAGES(object);
  
  g_object_unref(G_OBJECT(self->private->app));
  g_free(self->private);
}

static void bt_main_pages_init(GTypeInstance *instance, gpointer g_class) {
  BtMainPages *self = BT_MAIN_PAGES(instance);
  self->private = g_new0(BtMainPagesPrivate,1);
  self->private->dispose_has_run = FALSE;
}

static void bt_main_pages_class_init(BtMainPagesClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;
  
  gobject_class->set_property = bt_main_pages_set_property;
  gobject_class->get_property = bt_main_pages_get_property;
  gobject_class->dispose      = bt_main_pages_dispose;
  gobject_class->finalize     = bt_main_pages_finalize;

  g_object_class_install_property(gobject_class,MAIN_PAGES_APP,
                                  g_param_spec_object("app",
                                     "app contruct prop",
                                     "Set application object, the window belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));
}

GType bt_main_pages_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtMainPagesClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_main_pages_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtMainPages),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_main_pages_init, // instance_init
    };
		type = g_type_register_static(GTK_TYPE_NOTEBOOK,"BtMainPages",&info,0);
  }
  return type;
}

