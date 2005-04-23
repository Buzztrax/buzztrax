/* $Id: main-page-waves.c,v 1.19 2005-04-23 10:33:09 ensonic Exp $
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

	/* the toolbar widgets */
	GtkWidget *list_toolbar,*browser_toolbar,*editor_toolbar;
	
  /* the list of wavetable entries */
  GtkTreeView *waves_list;
	
	/* the list of wavelevels */
	GtkTreeView *wavelevels_list;
	
	/* the sample chooser */
	GtkWidget *file_chooser;
};

static GtkVBoxClass *parent_class=NULL;

//-- event handler helper

/*
 * waves_list_refresh:
 * @self: the waves page
 * @wavetable: the wavetable that is the source for the list 
 * 
 * Build the list of waves from the songs wavetable
 */
static void waves_list_refresh(const BtMainPageWaves *self,const BtWavetable *wavetable) {
	BtWave *wave;
  GtkListStore *store;
  GtkTreeIter tree_iter;
  GList *node,*list;
	gulong index=0;
	gchar *str;

  GST_INFO("refresh waves list: self=%p, wavetable=%p",self,wavetable);
		
  store=gtk_list_store_new(2,G_TYPE_ULONG,G_TYPE_STRING);

  //-- append waves rows (buzz numbers them from 0x01 to 0xC8)
	g_object_get(G_OBJECT(wavetable),"waves",&list,NULL);
	for(node=list;node;node=g_list_next(node)) {
		wave=BT_WAVE(node->data);
		g_object_get(G_OBJECT(wave),"name",&str,NULL);
		GST_INFO("  adding \"%s\"",str);
		gtk_list_store_append(store, &tree_iter);
		gtk_list_store_set(store,&tree_iter,0,index,1,str,-1);
		g_free(str);
		index++;
	}
	g_list_free(list);

  gtk_tree_view_set_model(self->priv->waves_list,GTK_TREE_MODEL(store));
  g_object_unref(store); // drop with treeview
}

/*
 * wavelevels_list_refresh:
 * @self: the waves page
 * @wavetable: the wavetable that is the source for the list 
 * 
 * Build the list of waves from the songs wavetable
 */
static void wavelevels_list_refresh(const BtMainPageWaves *self,const BtWave *wave) {
	BtWavelevel *wavelevel;
  GtkListStore *store;
  GtkTreeIter tree_iter;
  GList *node,*list;
	guchar tmp;
	guint root_note;
	gulong length,rate;
	glong loop_start,loop_end;
	
  GST_INFO("refresh wavelevels list: self=%p, wave=%p",self,wave);
		
  store=gtk_list_store_new(5,G_TYPE_UINT,G_TYPE_ULONG,G_TYPE_LONG,G_TYPE_LONG,G_TYPE_ULONG);

  //-- append wavelevels rows
	g_object_get(G_OBJECT(wave),"wavelevels",&list,NULL);
	for(node=list;node;node=g_list_next(node)) {
		wavelevel=BT_WAVELEVEL(node->data);
		g_object_get(G_OBJECT(wavelevel),"root-note",&tmp,"length",&length,"loop-start",&loop_start,"loop-end",&loop_end,"rate",&rate,NULL);
		root_note=(guint)tmp;
		gtk_list_store_append(store, &tree_iter);
		gtk_list_store_set(store,&tree_iter,0,root_note,1,length,2,loop_start,3,loop_end,4,rate,-1);
	}
	g_list_free(list);

  gtk_tree_view_set_model(self->priv->wavelevels_list,GTK_TREE_MODEL(store));
  g_object_unref(store); // drop with treeview
}
//-- event handler

static void on_waves_list_cursor_changed(GtkTreeView *treeview,gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);
  GtkTreeSelection *selection;
  GtkTreeModel     *model;
  GtkTreeIter       iter;

  g_assert(user_data);
  
  GST_INFO("waves list cursor changed");
  selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(self->priv->waves_list));
  if(gtk_tree_selection_get_selected(selection, &model, &iter)) {
		BtSong *song;
		BtWavetable *wavetable;
		BtWave *wave;
		GList *waves;
    gulong id;

    gtk_tree_model_get(model,&iter,0,&id,-1);
    GST_INFO("selected entry id %d",id);
		
		g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
		g_return_if_fail(song);
		g_object_get(song,"wavetable",&wavetable,NULL);
		g_object_get(wavetable,"waves",&waves,NULL);
    wave=BT_WAVE(g_list_nth_data(waves,id));
		g_list_free(waves);
		wavelevels_list_refresh(self,wave);
		g_object_try_unref(wavetable);
  	g_object_try_unref(song);
  }
}

static void on_song_changed(const BtEditApplication *app,GParamSpec *arg,gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);
  BtSong *song;
  BtWavetable *wavetable;

  g_assert(user_data);

  GST_INFO("song has changed : app=%p, self=%p",app,self);
  // get song from app and then setup from song
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
	g_return_if_fail(song);

	g_object_get(song,"wavetable",&wavetable,NULL);
  // update page
	waves_list_refresh(self,wavetable);
  // release the references
	g_object_try_unref(wavetable);
  g_object_try_unref(song);
}

static void on_toolbar_style_changed(const BtSettings *settings,GParamSpec *arg,gpointer user_data) {
  BtMainPageWaves *self=BT_MAIN_PAGE_WAVES(user_data);
	GtkToolbarStyle style;
	gchar *toolbar_style;
	
	g_object_get(G_OBJECT(settings),"toolbar-style",&toolbar_style,NULL);
	
	GST_INFO("!!!  toolbar style has changed '%s'",toolbar_style);
	style=gtk_toolbar_get_style_from_string(toolbar_style);
	gtk_toolbar_set_style(GTK_TOOLBAR(self->priv->list_toolbar),style);
	gtk_toolbar_set_style(GTK_TOOLBAR(self->priv->browser_toolbar),style);
	gtk_toolbar_set_style(GTK_TOOLBAR(self->priv->editor_toolbar),style);
	g_free(toolbar_style);
}

//-- helper methods

static gboolean bt_main_page_waves_init_ui(const BtMainPageWaves *self) {
	BtSettings *settings;
  GtkWidget *vpaned,*hpaned,*box,*box2;
	GtkWidget *button,*image,*icon;
	GtkWidget *scrolled_window;
	GtkCellRenderer *renderer;
  GtkTooltips *tips;
	
	GST_DEBUG("!!!! self=%p",self);
	
	tips=gtk_tooltips_new();
	
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
	self->priv->list_toolbar=gtk_toolbar_new();
  gtk_widget_set_name(self->priv->list_toolbar,_("sample list tool bar"));
	
	// add buttons (play,stop,clear)
  image=gtk_image_new_from_filename("stock_media-play.png");
  button=gtk_toolbar_append_element(GTK_TOOLBAR(self->priv->list_toolbar),
                                GTK_TOOLBAR_CHILD_TOGGLEBUTTON,
                                NULL,
                                _("Play"),
                                NULL, NULL,
                                image, NULL, NULL);
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(self->priv->list_toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("Play"));
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),button,_("Play current wave table entry"),NULL);
  //g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(on_toolbar_play_clicked),(gpointer)self);
  image=gtk_image_new_from_filename("stock_media-stop.png");
  button=gtk_toolbar_append_element(GTK_TOOLBAR(self->priv->list_toolbar),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                _("Stop"),
                                NULL, NULL,
                                image, NULL, NULL);
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(self->priv->list_toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("Stop"));
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),button,_("Stop playback of current wave table entry"),NULL);
  //g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(on_toolbar_stop_clicked),(gpointer)self);
  icon=gtk_image_new_from_stock(GTK_STOCK_CLEAR, gtk_toolbar_get_icon_size(GTK_TOOLBAR(self->priv->list_toolbar)));
  button=gtk_toolbar_append_element(GTK_TOOLBAR(self->priv->list_toolbar),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                _("Clear"),
                                NULL,NULL,
                                icon,NULL,NULL);
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(self->priv->list_toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("Clear"));
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),button,_("Clear current wave table entry"),NULL);
	
  gtk_box_pack_start(GTK_BOX(box),self->priv->list_toolbar,FALSE,FALSE,0);

	//       listview
  scrolled_window=gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_ETCHED_IN);
  self->priv->waves_list=GTK_TREE_VIEW(gtk_tree_view_new());
	g_object_set(self->priv->waves_list,"enable-search",FALSE,"rules-hint",TRUE,NULL);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(self->priv->waves_list),GTK_SELECTION_BROWSE);
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
	self->priv->browser_toolbar=gtk_toolbar_new();
  gtk_widget_set_name(self->priv->browser_toolbar,_("sample browser tool bar"));
    
	// add buttons (play,stop,load)
  image=gtk_image_new_from_filename("stock_media-play.png");
  button=gtk_toolbar_append_element(GTK_TOOLBAR(self->priv->browser_toolbar),
                                GTK_TOOLBAR_CHILD_TOGGLEBUTTON,
                                NULL,
                                _("Play"),
                                NULL, NULL,
                                image, NULL, NULL);
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(self->priv->browser_toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("Play"));
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),button,_("Play current sample"),NULL);
  //g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(on_toolbar_play_clicked),(gpointer)self);
  image=gtk_image_new_from_filename("stock_media-stop.png");
  button=gtk_toolbar_append_element(GTK_TOOLBAR(self->priv->browser_toolbar),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                _("Stop"),
                                NULL, NULL,
                                image, NULL, NULL);
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(self->priv->browser_toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("Stop"));
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),button,_("Stop playback of current sample"),NULL);
  //g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(on_toolbar_stop_clicked),(gpointer)self);
  icon=gtk_image_new_from_stock(GTK_STOCK_OPEN, gtk_toolbar_get_icon_size(GTK_TOOLBAR(self->priv->browser_toolbar)));
  button=gtk_toolbar_append_element(GTK_TOOLBAR(self->priv->browser_toolbar),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                _("Open"),
                                NULL,NULL,
                                icon,NULL,NULL);
  gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(self->priv->browser_toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("Open"));
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tips),button,_("Load current sample into wave table"),NULL);
  //g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(on_toolbar_open_clicked),(gpointer)self);
	
	gtk_box_pack_start(GTK_BOX(box),self->priv->browser_toolbar,FALSE,FALSE,0);

	//       file-chooser
	// this causes warning on gtk 2.4
	self->priv->file_chooser=gtk_file_chooser_widget_new(GTK_FILE_CHOOSER_ACTION_OPEN);
	gtk_box_pack_start(GTK_BOX(box),self->priv->file_chooser,TRUE,TRUE,6);

	//   vbox (sample view)
	box=gtk_vbox_new(FALSE,0);
	gtk_paned_pack2(GTK_PANED(vpaned),GTK_WIDGET(box),FALSE,FALSE);
	//     toolbar
	self->priv->editor_toolbar=gtk_toolbar_new();
  gtk_widget_set_name(self->priv->editor_toolbar,_("sample edit tool bar"));
	
  gtk_box_pack_start(GTK_BOX(box),self->priv->editor_toolbar,FALSE,FALSE,0);

	//     hbox
	box2=gtk_hbox_new(FALSE,0);
	gtk_container_add(GTK_CONTAINER(box),box2);
	//       zone entries (multiple waves per sample (xm?)) -> (per entry: root key, length, rate, loope start, loop end
  scrolled_window=gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_ETCHED_IN);
  self->priv->wavelevels_list=GTK_TREE_VIEW(gtk_tree_view_new());
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(self->priv->wavelevels_list),GTK_SELECTION_BROWSE);
	g_object_set(self->priv->wavelevels_list,"enable-search",FALSE,"rules-hint",TRUE,NULL);
  renderer=gtk_cell_renderer_text_new();
	//g_object_set(G_OBJECT(renderer),"xalign",1.0,NULL);
  gtk_tree_view_insert_column_with_attributes(self->priv->wavelevels_list,-1,_("Root"),renderer,"text",0,NULL);
  renderer=gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(self->priv->wavelevels_list,-1,_("Length"),renderer,"text",1,NULL);
  renderer=gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(self->priv->wavelevels_list,-1,_("Rate"),renderer,"text",2,NULL);
  renderer=gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(self->priv->wavelevels_list,-1,_("Loop begin"),renderer,"text",3,NULL);
  renderer=gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(self->priv->wavelevels_list,-1,_("Loop end"),renderer,"text",4,NULL);
  gtk_container_add(GTK_CONTAINER(scrolled_window),GTK_WIDGET(self->priv->wavelevels_list));
	gtk_container_add(GTK_CONTAINER(box2),scrolled_window);
	//gtk_container_add(GTK_CONTAINER(box2),gtk_label_new("no sample zone entries yet"));

	//       sampleview (which widget do we need?)
	//       properties (loop, envelope, ...)
	gtk_container_add(GTK_CONTAINER(box2),gtk_label_new("no sample waveform view yet"));

  // register event handlers
	g_signal_connect(G_OBJECT(self->priv->waves_list),"cursor-changed",G_CALLBACK(on_waves_list_cursor_changed),(gpointer)self);
  g_signal_connect(G_OBJECT(self->priv->app), "notify::song", (GCallback)on_song_changed, (gpointer)self);

	// let settings control toolbar style
	g_object_get(G_OBJECT(self->priv->app),"settings",&settings,NULL);
	on_toolbar_style_changed(settings,NULL,(gpointer)self);
	g_signal_connect(G_OBJECT(settings), "notify::toolbar-style", G_CALLBACK(on_toolbar_style_changed), (gpointer)self);
	g_object_unref(settings);

	GST_DEBUG("  done");
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
  if(!bt_main_page_waves_init_ui(self)) {
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
      g_object_try_weak_unref(self->priv->app);
      self->priv->app = BT_EDIT_APPLICATION(g_value_get_object(value));
			g_object_try_weak_ref(self->priv->app);
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

  g_object_try_weak_unref(self->priv->app);

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
