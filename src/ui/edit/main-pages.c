// $Id: main-pages.c,v 1.30 2006-04-08 16:18:26 ensonic Exp $
/**
 * SECTION:btmainpages
 * @short_description: class for the editor main pages
 *
 * The user interface of the buzztard editor is divided into several pages.
 * This class implements the notebook widgets to manage the sub-pages:
 * #tMainPageMachines, #BtMainPagePatterns, #BtMainPageSequence,
 * #BtMainPageWaves and #BtMainPageInfo.
 */ 

#define BT_EDIT
#define BT_MAIN_PAGES_C

#include "bt-edit.h"

enum {
  MAIN_PAGES_APP=1,
  MAIN_PAGES_MACHINES_PAGE,
  MAIN_PAGES_PATTERNS_PAGE,
  MAIN_PAGES_SEQUENCE_PAGE,
  MAIN_PAGES_WAVES_PAGE,
  MAIN_PAGES_INFO_PAGE
};


struct _BtMainPagesPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the application */
  BtEditApplication *app;
  
  /* the machines tab */
  BtMainPageMachines *machines_page;
  /* the patterns tab */
  BtMainPagePatterns *patterns_page;
  /* the sequence tab */
  BtMainPageSequence *sequence_page;
  /* the waves tab */
  BtMainPageWaves *waves_page;
  /* the information tab */
  BtMainPageInfo *info_page;
};

static GtkNotebookClass *parent_class=NULL;

//-- event handler

//-- helper methods

static void bt_main_pages_init_tab(const BtMainPages *self,GtkTooltips *tips,guint index,gchar *str,gchar *icon,gchar *tip) {
  GtkWidget *label,*event_box,*box,*image;

  label=gtk_label_new(str);
  gtk_widget_set_name(label,str);
  gtk_widget_show(label);

  image=gtk_image_new_from_filename(icon);
  gtk_widget_show(image);
  
  box=gtk_hbox_new(FALSE,6);
  gtk_widget_show(box);
  gtk_container_add(GTK_CONTAINER(box),image);
  gtk_container_add(GTK_CONTAINER(box),label);
  
  event_box=gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(event_box),box);
  
  gtk_notebook_set_tab_label(GTK_NOTEBOOK(self),gtk_notebook_get_nth_page(GTK_NOTEBOOK(self),index),event_box);
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),event_box,tip,NULL);
}

static gboolean bt_main_pages_init_ui(const BtMainPages *self) {
  GtkTooltips *tips;
  
  tips=gtk_tooltips_new();
  
  gtk_widget_set_name(GTK_WIDGET(self),_("song views"));

  GST_INFO("before creating pages, app->ref_ct=%d",G_OBJECT(self->priv->app)->ref_count);
  
  // add wigets for machine view
  self->priv->machines_page=bt_main_page_machines_new(self->priv->app);
  gtk_container_add(GTK_CONTAINER(self),GTK_WIDGET(self->priv->machines_page));
  bt_main_pages_init_tab(self,tips,BT_MAIN_PAGES_MACHINES_PAGE,_("machine view"),"tab_machines.png",_("machines used in the song and their wires"));
  
  // add wigets for pattern view
  self->priv->patterns_page=bt_main_page_patterns_new(self->priv->app);
  gtk_container_add(GTK_CONTAINER(self),GTK_WIDGET(self->priv->patterns_page));
  bt_main_pages_init_tab(self,tips,BT_MAIN_PAGES_PATTERNS_PAGE,_("pattern view"),"tab_patterns.png",_("event pattern editor"));
  
  // add wigets for sequence view
  self->priv->sequence_page=bt_main_page_sequence_new(self->priv->app);
  gtk_container_add(GTK_CONTAINER(self),GTK_WIDGET(self->priv->sequence_page));
  bt_main_pages_init_tab(self,tips,BT_MAIN_PAGES_SEQUENCE_PAGE,_("sequence view"),"tab_sequence.png",_("song sequence editor"));

  // add wigets for waves view
  self->priv->waves_page=bt_main_page_waves_new(self->priv->app);
  gtk_container_add(GTK_CONTAINER(self),GTK_WIDGET(self->priv->waves_page));
  bt_main_pages_init_tab(self,tips,BT_MAIN_PAGES_WAVES_PAGE,_("wave table view"),"tab_waves.png",_("sample wave table editor"));

  // add widgets for song info view
  self->priv->info_page=bt_main_page_info_new(self->priv->app);
  gtk_container_add(GTK_CONTAINER(self),GTK_WIDGET(self->priv->info_page));
  bt_main_pages_init_tab(self,tips,BT_MAIN_PAGES_INFO_PAGE,_("song information"),"tab_info.png",_("song meta data editor"));

  // @idea add widgets for machine help view
  // GTK_STOCK_HELP icon
  // embed mozilla/gtk-html

  GST_DEBUG("  done");
  return(TRUE);
}

//-- constructor methods

/**
 * bt_main_pages_new:
 * @app: the application the window belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtMainPages *bt_main_pages_new(const BtEditApplication *app) {
  BtMainPages *self;

  if(!(self=BT_MAIN_PAGES(g_object_new(BT_TYPE_MAIN_PAGES,"app",app,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_main_pages_init_ui(self)) {
    goto Error;
  }
  return(self);
Error:
  g_object_try_unref(self);
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
      g_value_set_object(value, self->priv->app);
    } break;
    case MAIN_PAGES_MACHINES_PAGE: {
      g_value_set_object(value, self->priv->machines_page);
    } break;
    case MAIN_PAGES_PATTERNS_PAGE: {
      g_value_set_object(value, self->priv->patterns_page);
    } break;
    case MAIN_PAGES_SEQUENCE_PAGE: {
      g_value_set_object(value, self->priv->sequence_page);
    } break;
    case MAIN_PAGES_WAVES_PAGE: {
      g_value_set_object(value, self->priv->waves_page);
    } break;
    case MAIN_PAGES_INFO_PAGE: {
      g_value_set_object(value, self->priv->info_page);
    } break;
    default: {
       G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
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
      g_object_try_weak_unref(self->priv->app);
      self->priv->app = BT_EDIT_APPLICATION(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->app);
      //GST_DEBUG("set the app for main_pages: %p",self->priv->app);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_main_pages_dispose(GObject *object) {
  BtMainPages *self = BT_MAIN_PAGES(object);
  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  g_object_try_weak_unref(self->priv->app);
  // this disposes the pages for us
  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_main_pages_finalize(GObject *object) {
  //BtMainPages *self = BT_MAIN_PAGES(object);
  
  //GST_DEBUG("!!!! self=%p",self);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_main_pages_init(GTypeInstance *instance, gpointer g_class) {
  BtMainPages *self = BT_MAIN_PAGES(instance);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_MAIN_PAGES, BtMainPagesPrivate);
}

static void bt_main_pages_class_init(BtMainPagesClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(GTK_TYPE_NOTEBOOK);
  g_type_class_add_private(klass,sizeof(BtMainPagesPrivate));

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

  g_object_class_install_property(gobject_class,MAIN_PAGES_MACHINES_PAGE,
                                  g_param_spec_object("machines-page",
                                     "machines page prop",
                                     "the machines view page",
                                     BT_TYPE_MAIN_PAGE_MACHINES, /* object type */
                                     G_PARAM_READABLE));

  g_object_class_install_property(gobject_class,MAIN_PAGES_PATTERNS_PAGE,
                                  g_param_spec_object("patterns-page",
                                     "patterns page prop",
                                     "the patterns view page",
                                     BT_TYPE_MAIN_PAGE_PATTERNS, /* object type */
                                     G_PARAM_READABLE));

  g_object_class_install_property(gobject_class,MAIN_PAGES_SEQUENCE_PAGE,
                                  g_param_spec_object("sequence-page",
                                     "sequence page prop",
                                     "the sequence view page",
                                     BT_TYPE_MAIN_PAGE_SEQUENCE, /* object type */
                                     G_PARAM_READABLE));

  g_object_class_install_property(gobject_class,MAIN_PAGES_WAVES_PAGE,
                                  g_param_spec_object("waves-page",
                                     "waves page prop",
                                     "the waves view page",
                                     BT_TYPE_MAIN_PAGE_WAVES, /* object type */
                                     G_PARAM_READABLE));

  g_object_class_install_property(gobject_class,MAIN_PAGES_INFO_PAGE,
                                  g_param_spec_object("info-page",
                                     "info page prop",
                                     "the info view page",
                                     BT_TYPE_MAIN_PAGE_INFO, /* object type */
                                     G_PARAM_READABLE));
}

GType bt_main_pages_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtMainPagesClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_main_pages_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtMainPages),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_main_pages_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(GTK_TYPE_NOTEBOOK,"BtMainPages",&info,0);
  }
  return type;
}
