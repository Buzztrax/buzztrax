/* $Id: main-page-waves.c,v 1.9 2005-01-07 17:50:53 ensonic Exp $
 * class for the editor main waves page
 */

#define BT_EDIT
#define BT_MAIN_PAGE_WAVES_C

#include "bt-edit.h"

enum {
  MAIN_PAGE_WAVES_APP=1,
};


struct _BtMainPageWavesPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the application */
  BtEditApplication *app;
	
  /* the waves list */
  GtkTreeView *waves_list;
};

static GtkVBoxClass *parent_class=NULL;

//-- event handler helper

/**
 * waves_list_refresh:
 * @self: the waves page
 * 
 * Build the list of waves from the songs wavetable
 */
static void waves_list_refresh(const BtMainPageWaves *self) {
  GtkListStore *store;
  GtkTreeIter tree_iter;
  gpointer *iter;
	gulong index=0;

  GST_INFO("refresh waves list");
  store=gtk_list_store_new(2,G_TYPE_STRING,G_TYPE_STRING);

  //-- append waves rows (buzz numbers them from 0x01 to 0xC8)
	/*
  iter=bt_wavetable_waves_iterator_new();
  while(iter) {
    wave=bt_wavetable_waves_iterator_get_wave(iter);
    g_object_get(G_OBJECT(pattern),"name",&str,NULL);
		GST_INFO("  adding \"%s\"");
    gtk_list_store_append(store, &tree_iter);
    gtk_list_store_set(store,&tree_iter,0,key,1,str,-1);
    g_free(str);
    iter=bt_wavetable_waves_iterator_next(iter);
		index++;
  }
	*/
  gtk_tree_view_set_model(self->priv->waves_list,GTK_TREE_MODEL(store));
  g_object_unref(store); // drop with treeview
}

//-- event handler

static void on_song_changed(const BtEditApplication *app,GParamSpec *arg,gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);
  BtSong *song;
  BtSetup *setup;
  BtMachine *machine;

  g_assert(user_data);

  GST_INFO("song has changed : app=%p, self=%p",app,self);
  // get song from app and then setup from song
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  // update page
	waves_list_refresh(self);
  // release the reference
  g_object_try_unref(song);
}

//-- helper methods

static gboolean bt_main_page_waves_init_ui(const BtMainPageWaves *self, const BtEditApplication *app) {
  GtkWidget *toolbar;
  GtkWidget *vpaned,*hpaned,*box,*box2;
	GtkWidget *button,*image,*icon;
	GtkWidget *scrolled_window;
	GtkCellRenderer *renderer;
  GtkTooltips *tips;
	
	tips=gtk_tooltips_new();
	
  // @todo add ui
	// vpane
	vpaned=gtk_vpaned_new();
  gtk_container_add(GTK_CONTAINER(self),vpaned);
	
	//   hpane
	hpaned=gtk_hpaned_new();
	gtk_paned_pack1(GTK_PANED(vpaned),GTK_WIDGET(hpaned),FALSE,FALSE);

	//     vbox (loaded sample list)
	box=gtk_vbox_new(FALSE,0);
	gtk_paned_pack1(GTK_PANED(hpaned),GTK_WIDGET(box),FALSE,FALSE);
	//       toolbar
	toolbar=gtk_toolbar_new();
  gtk_widget_set_name(toolbar,_("sample list tool bar"));
  gtk_box_pack_start(GTK_BOX(box),toolbar,FALSE,FALSE,0);
  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar),GTK_TOOLBAR_BOTH);
	// @todo add buttons (play,stop,clear)
  image=gtk_image_new_from_filename("stock_media-play.png");
  button=gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
                                GTK_TOOLBAR_CHILD_TOGGLEBUTTON,
                                NULL,
                                _("Play"),
                                NULL, NULL,
                                image, NULL, NULL);
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("Play"));
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),button,_("Play current wave table entry"),NULL);
  //g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(on_toolbar_play_clicked),(gpointer)self);
  image=gtk_image_new_from_filename("stock_media-stop.png");
  button=gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                _("Stop"),
                                NULL, NULL,
                                image, NULL, NULL);
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("Stop"));
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),button,_("Stop playback of current wave table entry"),NULL);
  //g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(on_toolbar_stop_clicked),(gpointer)self);
  icon=gtk_image_new_from_stock(GTK_STOCK_CLEAR, gtk_toolbar_get_icon_size(GTK_TOOLBAR(toolbar)));
  button=gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                _("Open"),
                                NULL,NULL,
                                icon,NULL,NULL);
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("Clear"));
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),button,_("Clear current wave table entry"),NULL);
	//       listview
  scrolled_window=gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_ETCHED_IN);
  self->priv->waves_list=GTK_TREE_VIEW(gtk_tree_view_new());
  renderer=gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer),"xalign",1.0,NULL);
  gtk_tree_view_insert_column_with_attributes(self->priv->waves_list,-1,_("Ix"),renderer,"text",0,NULL);
  renderer=gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(self->priv->waves_list,-1,_("Wave"),renderer,"text",1,NULL);
  gtk_container_add(GTK_CONTAINER(scrolled_window),GTK_WIDGET(self->priv->waves_list));
	gtk_container_add(GTK_CONTAINER(box),scrolled_window);

	//     vbox (file browser)
	box=gtk_vbox_new(FALSE,0);
	gtk_paned_pack2(GTK_PANED(hpaned),GTK_WIDGET(box),FALSE,FALSE);
	//       toolbar
	toolbar=gtk_toolbar_new();
  gtk_widget_set_name(toolbar,_("sample browser tool bar"));
  gtk_box_pack_start(GTK_BOX(box),toolbar,FALSE,FALSE,0);
  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar),GTK_TOOLBAR_BOTH);
	// @todo add buttons (play,stop,load)
  image=gtk_image_new_from_filename("stock_media-play.png");
  button=gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
                                GTK_TOOLBAR_CHILD_TOGGLEBUTTON,
                                NULL,
                                _("Play"),
                                NULL, NULL,
                                image, NULL, NULL);
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("Play"));
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),button,_("Play current sample"),NULL);
  //g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(on_toolbar_play_clicked),(gpointer)self);
  image=gtk_image_new_from_filename("stock_media-stop.png");
  button=gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                _("Stop"),
                                NULL, NULL,
                                image, NULL, NULL);
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("Stop"));
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),button,_("Stop playback of current sample"),NULL);
  //g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(on_toolbar_stop_clicked),(gpointer)self);
  icon=gtk_image_new_from_stock(GTK_STOCK_OPEN, gtk_toolbar_get_icon_size(GTK_TOOLBAR(toolbar)));
  button=gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                _("Open"),
                                NULL,NULL,
                                icon,NULL,NULL);
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("Open"));
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),button,_("Load current sample into wave table"),NULL);
  //g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(on_toolbar_open_clicked),(gpointer)self);
	//       listview
	gtk_container_add(GTK_CONTAINER(box),gtk_label_new("no sample browser yet"));


	//   vbox (sample view)
	box=gtk_vbox_new(FALSE,0);
	gtk_paned_pack2(GTK_PANED(vpaned),GTK_WIDGET(box),FALSE,FALSE);
	//     toolbar
	toolbar=gtk_toolbar_new();
  gtk_widget_set_name(toolbar,_("sample edit tool bar"));
  gtk_box_pack_start(GTK_BOX(box),toolbar,FALSE,FALSE,0);
  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar),GTK_TOOLBAR_BOTH);
	// @todo add buttons (...)
	//     hbox
	box2=gtk_hbox_new(FALSE,0);
	gtk_container_add(GTK_CONTAINER(box),box2);
	//       zone entries (multiple waves per sample (xm?)) -> (per entry: root key, length, rate, loope start, loop end
	gtk_container_add(GTK_CONTAINER(box2),gtk_label_new("no sample zone entries yet"));
	//       sampleview (which widget do we need?)
	//       properties (loop, envelope, ...)
	gtk_container_add(GTK_CONTAINER(box2),gtk_label_new("no sample waveform view yet"));

  // register event handlers
  g_signal_connect(G_OBJECT(app), "notify::song", (GCallback)on_song_changed, (gpointer)self);
  return(TRUE);
}

//-- constructor methods

/**
 * bt_main_page_waves_new:
 * @app: the application the window belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtMainPageWaves *bt_main_page_waves_new(const BtEditApplication *app) {
  BtMainPageWaves *self;

  if(!(self=BT_MAIN_PAGE_WAVES(g_object_new(BT_TYPE_MAIN_PAGE_WAVES,"app",app,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_main_page_waves_init_ui(self,app)) {
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
static void bt_main_page_waves_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_PAGE_WAVES_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    default: {
 			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_main_page_waves_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_PAGE_WAVES_APP: {
      g_object_try_unref(self->priv->app);
      self->priv->app = g_object_try_ref(g_value_get_object(value));
      //GST_DEBUG("set the app for MAIN_PAGE_WAVES: %p",self->priv->app);
    } break;
    default: {
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_main_page_waves_dispose(GObject *object) {
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES(object);
	return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  g_object_try_unref(self->priv->app);

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_main_page_waves_finalize(GObject *object) {
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES(object);
  
  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_main_page_waves_init(GTypeInstance *instance, gpointer g_class) {
  BtMainPageWaves *self = BT_MAIN_PAGE_WAVES(instance);
  self->priv = g_new0(BtMainPageWavesPrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_main_page_waves_class_init(BtMainPageWavesClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(GTK_TYPE_VBOX);
    
  gobject_class->set_property = bt_main_page_waves_set_property;
  gobject_class->get_property = bt_main_page_waves_get_property;
  gobject_class->dispose      = bt_main_page_waves_dispose;
  gobject_class->finalize     = bt_main_page_waves_finalize;

  g_object_class_install_property(gobject_class,MAIN_PAGE_WAVES_APP,
                                  g_param_spec_object("app",
                                     "app contruct prop",
                                     "Set application object, the window belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));
}

GType bt_main_page_waves_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtMainPageWavesClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_main_page_waves_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtMainPageWaves),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_main_page_waves_init, // instance_init
			NULL // value_table
    };
		type = g_type_register_static(GTK_TYPE_VBOX,"BtMainPageWaves",&info,0);
  }
  return type;
}
