/* $Id: main-page-patterns.c,v 1.33 2005-01-11 12:31:59 ensonic Exp $
 * class for the editor main pattern page
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
  GtkComboBox *machine_menu;
  /* pattern selection menu */
  GtkComboBox *pattern_menu;

	/* the pattern table */
#ifdef USE_GTKGRID
  GtkGrid *pattern_table;
#endif
	
	/* icons */
	GdkPixbuf *source_icon,*procesor_icon,*sink_icon;
};

static GtkVBoxClass *parent_class=NULL;

enum {
  MACHINE_MENU_ICON=0,
  MACHINE_MENU_LABEL,
	MACHINE_MENU_MACHINE
};

enum {
  PATTERN_TABLE_POS,
  PATTERN_TABLE_PRE_CT
};

//-- tree model helper

static void machine_model_get_iter_by_machine(GtkTreeModel *store,GtkTreeIter *iter,BtMachine *that_machine) {
	BtMachine *this_machine;

	gtk_tree_model_get_iter_first(store,iter);
	do {
		gtk_tree_model_get(store,iter,MACHINE_MENU_MACHINE,&this_machine,-1);
		if(this_machine==that_machine) break;
	} while(gtk_tree_model_iter_next(store,iter));
}

//-- event handlers

static void on_machine_id_changed(BtMachine *machine,GParamSpec *arg,gpointer user_data) {
	BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
	GtkTreeModel *store;
	GtkTreeIter iter;
	gchar *str;
	
	g_assert(user_data);

	g_object_get(G_OBJECT(machine),"id",&str,NULL);
  GST_INFO("machine id changed to \"%s\"",str);
	
	store=gtk_combo_box_get_model(self->priv->machine_menu);
	// get the row where row.machine==machine
	machine_model_get_iter_by_machine(store,&iter,machine);
	gtk_list_store_set(GTK_LIST_STORE(store),&iter,MACHINE_MENU_LABEL,str,-1);

	g_free(str);
}

//-- event handler helper

static void machine_menu_add(const BtMainPagePatterns *self,BtMachine *machine,GtkListStore *store) {
	gchar *str;
	GtkTreeIter menu_iter;

  g_object_get(G_OBJECT(machine),"id",&str,NULL);
  GST_INFO("  adding \"%s\"",str);

	gtk_list_store_append(store,&menu_iter);
  if(BT_IS_SOURCE_MACHINE(machine)) {
		gtk_list_store_set(store,&menu_iter,MACHINE_MENU_ICON,self->priv->source_icon,-1);
  }
  else if(BT_IS_PROCESSOR_MACHINE(machine)) {
		gtk_list_store_set(store,&menu_iter,MACHINE_MENU_ICON,self->priv->procesor_icon,-1);
  }
  else if(BT_IS_SINK_MACHINE(machine)) {
		gtk_list_store_set(store,&menu_iter,MACHINE_MENU_ICON,self->priv->sink_icon,-1);
  }
	gtk_list_store_set(store,&menu_iter,MACHINE_MENU_LABEL,str,MACHINE_MENU_MACHINE,machine,-1);
	g_signal_connect(G_OBJECT(machine),"notify::id",(GCallback)on_machine_id_changed,(gpointer)self);
	
  g_free(str);
}

static void machine_menu_refresh(const BtMainPagePatterns *self,const BtSetup *setup) {
  BtMachine *machine=NULL;
	GtkListStore *store;
	GList *node,*list;

  // update machine menu
  store=gtk_list_store_new(3,GDK_TYPE_PIXBUF,G_TYPE_STRING,BT_TYPE_MACHINE);
  g_object_get(G_OBJECT(setup),"machines",&list,NULL);
	for(node=list;node;node=g_list_next(node)) {
    machine=BT_MACHINE(node->data);
		machine_menu_add(self,machine,store);
  }
	g_list_free(list);
	gtk_widget_set_sensitive(GTK_WIDGET(self->priv->machine_menu),(machine!=NULL));
	gtk_combo_box_set_model(self->priv->machine_menu,GTK_TREE_MODEL(store));
  gtk_combo_box_set_active(self->priv->machine_menu,((machine!=NULL)?0:-1));
	g_object_unref(store); // drop with comboxbox
}

static void pattern_menu_refresh(const BtMainPagePatterns *self,const BtMachine *machine) {
  BtPattern *pattern=NULL;
  GList *node,*list;
  gchar *str;
	GtkListStore *store;
	GtkTreeIter menu_iter;

  // update pattern menu
  store=gtk_list_store_new(1,G_TYPE_STRING);
  if(machine) {
		g_object_get(G_OBJECT(machine),"patterns",&list,NULL);
		for(node=list;node;node=g_list_next(node)) {
      pattern=BT_PATTERN(node->data);
      g_object_get(G_OBJECT(pattern),"name",&str,NULL);
      GST_INFO("  adding \"%s\"",str);
			gtk_list_store_append(store,&menu_iter);
			gtk_list_store_set(store,&menu_iter,0,str,-1);
			g_free(str);
    }
		g_list_free(list);
  }
	gtk_widget_set_sensitive(GTK_WIDGET(self->priv->pattern_menu),(pattern!=NULL));
	gtk_combo_box_set_model(self->priv->pattern_menu,GTK_TREE_MODEL(store));
  gtk_combo_box_set_active(self->priv->pattern_menu,((pattern!=NULL)?0:-1));
	g_object_unref(store); // drop with comboxbox
}

/**
 * pattern_table_clear:
 * @self: the pattern page
 *
 * removes old columns
 */
static void pattern_table_clear(const BtMainPagePatterns *self) {
  GList *columns,*node;
		
  // remove columns
#ifdef USE_GTKGRID
	g_assert(self->priv->pattern_table);
	GST_INFO("clearing pattern table");
  if((columns=gtk_grid_get_columns(self->priv->pattern_table))) {
		GST_INFO("is not empty");
		for(node=g_list_first(columns);node;node=g_list_next(node)) {
      gtk_grid_remove_column(self->priv->pattern_table,GTK_GRID_COLUMN(node->data));
    }
    g_list_free(columns);
  }
#endif
	GST_INFO("done");
}

/**
 * pattern_table_refresh:
 * @self:  the pattern page
 * @song: the new pattern to display
 *
 * rebuild the pattern table after a new pattern has been chosen
 */
static void pattern_table_refresh(const BtMainPagePatterns *self,const BtPattern *pattern) {
	BtMachine *machine;
	gulong i,number_of_ticks,pos=0;
  GtkCellRenderer *renderer;
  GtkListStore *store;
  GType *store_types;
	GtkTreeIter tree_iter;
#ifdef USE_GTKGRID
  GtkGridColumn *grid_col;
#endif

	GST_INFO("refresh pattern table");
	g_assert(BT_IS_PATTERN(pattern));
#ifdef USE_GTKGRID
	g_assert(GTK_IS_GRID(self->priv->pattern_table));
#endif
	
	g_object_get(G_OBJECT(pattern),"length",&number_of_ticks,"machine",&machine,NULL);
  GST_INFO("  size is %2d,?",number_of_ticks);

  // reset columns
	pattern_table_clear(self);

	// build model
	GST_DEBUG("  build model");
	store_types=(GType *)g_new(GType *,1);
	store_types[0]=G_TYPE_LONG;
	store=gtk_list_store_newv(1,store_types);
  g_free(store_types);
	// add events
  for(i=0;i<number_of_ticks;i++) {
    //timeline=bt_sequence_get_timeline_by_time(sequence,i);
    gtk_list_store_append(store, &tree_iter);
    // set position, highlight-color
    gtk_list_store_set(store,&tree_iter,
			PATTERN_TABLE_POS,pos,
			-1);
		pos++;
	}
#ifdef USE_GTKGRID
	GST_DEBUG("    activating store: %p",store);
	gtk_grid_set_model(self->priv->pattern_table,GTK_TREE_MODEL(store));
#endif

  // build view
	GST_DEBUG("  build view");
	// add initial columns
#ifdef USE_GTKGRID
  renderer=gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer),"xalign",1.0,NULL);
	GST_DEBUG("    created cell renderer");
  grid_col=gtk_grid_column_new_with_attributes(_("Pos."),renderer,
    "text",PATTERN_TABLE_POS,
    NULL);
	GST_DEBUG("    created column");
	gtk_grid_append_column(self->priv->pattern_table,grid_col);
#endif

	GST_DEBUG("  done");
	// release the references
	g_object_unref(store); // drop with gridview
}

//-- event handler

static void on_pattern_menu_changed(GtkComboBox *menu, gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
	BtPattern *pattern;

  g_assert(user_data);
  // refresh pattern view
	if((pattern=bt_main_page_patterns_get_current_pattern(self))) {
		pattern_table_refresh(self,pattern);
	}
}

static void on_machine_added(BtSetup *setup,BtMachine *machine,gpointer user_data) {
	BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
	GtkTreeModel *store;
	
  g_assert(user_data);
	
	GST_INFO("new machine has been added");
	store=gtk_combo_box_get_model(self->priv->machine_menu);
	machine_menu_add(self,machine,GTK_LIST_STORE(store));

	if(gtk_tree_model_iter_n_children(store,NULL)==1) {
		gtk_widget_set_sensitive(GTK_WIDGET(self->priv->machine_menu),TRUE);
		gtk_combo_box_set_active(self->priv->machine_menu,0);
	}
}

static void on_machine_removed(BtSetup *setup,BtMachine *machine,gpointer user_data) {
	BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
	GtkTreeModel *store;
	GtkTreeIter iter;
	
  g_assert(user_data);
	
	GST_INFO("machine has been removed");
	store=gtk_combo_box_get_model(self->priv->machine_menu);
	// get the row where row.machine==machine
	machine_model_get_iter_by_machine(store,&iter,machine);
	gtk_list_store_remove(GTK_LIST_STORE(store),&iter);
	if(gtk_tree_model_iter_n_children(store,NULL)==0) {
		gtk_widget_set_sensitive(GTK_WIDGET(self->priv->machine_menu),FALSE);
	}
}

static void on_machine_menu_changed(GtkComboBox *menu, gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);

  g_assert(user_data);
	// show new list of pattern in pattern menu
  pattern_menu_refresh(self,bt_main_page_patterns_get_current_machine(self));
}

static void on_song_changed(const BtEditApplication *app,GParamSpec *arg,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  BtSong *song;
  BtSetup *setup;
  BtMachine *machine;

  g_assert(user_data);

  GST_INFO("song has changed : app=%p, self=%p",app,self);
  // get song from app and then setup from song
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"setup",&setup,NULL);
  // update page
  machine_menu_refresh(self,setup);
  pattern_menu_refresh(self,bt_main_page_patterns_get_current_machine(self));
	g_signal_connect(G_OBJECT(setup),"machine-added",(GCallback)on_machine_added,(gpointer)self);
	g_signal_connect(G_OBJECT(setup),"machine-removed",(GCallback)on_machine_removed,(gpointer)self);
  // release the references
  g_object_try_unref(setup);
  g_object_try_unref(song);
}

//-- helper methods

static gboolean bt_main_page_patterns_init_ui(const BtMainPagePatterns *self, const BtEditApplication *app) {
  GtkWidget *toolbar,*scrolled_window;
  GtkWidget *box,*menu,*button;
	GtkCellRenderer *renderer;
	
  // add toolbar
  toolbar=gtk_toolbar_new();
  gtk_widget_set_name(toolbar,_("machine view tool bar"));
  gtk_box_pack_start(GTK_BOX(self),toolbar,FALSE,FALSE,0);
  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar),GTK_TOOLBAR_BOTH);
  
  // add toolbar widgets
  // machine select
  box=gtk_hbox_new(FALSE,2);
  gtk_container_set_border_width(GTK_CONTAINER(box),4);
  self->priv->machine_menu=GTK_COMBO_BOX(gtk_combo_box_new());
	renderer=gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(self->priv->machine_menu),renderer,FALSE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(self->priv->machine_menu),renderer,"pixbuf",MACHINE_MENU_ICON,NULL);
	renderer=gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(self->priv->machine_menu),renderer,TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(self->priv->machine_menu),renderer,"text",MACHINE_MENU_LABEL,NULL);

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
  self->priv->pattern_menu=GTK_COMBO_BOX(gtk_combo_box_new());
	renderer=gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(self->priv->pattern_menu),renderer,TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(self->priv->pattern_menu),renderer,"text", 0,NULL);
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

  // @todo add wavetable entry select
  // @todo add base octave (0-8)
  // @todo add play notes ?
  
  // @todo add list-view / grid-view
	scrolled_window=gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_ETCHED_IN);
#ifdef USE_GTKGRID
  self->priv->pattern_table=GTK_GRID(gtk_grid_new());
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window),GTK_WIDGET(self->priv->pattern_table));
#else
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window),gtk_label_new("pattern view use GtkGrid"));
#endif
  gtk_container_add(GTK_CONTAINER(self),scrolled_window);

  // register event handlers
  g_signal_connect(G_OBJECT(app), "notify::song", (GCallback)on_song_changed, (gpointer)self);
  g_signal_connect(G_OBJECT(self->priv->machine_menu), "changed", (GCallback)on_machine_menu_changed, (gpointer)self);
	g_signal_connect(G_OBJECT(self->priv->pattern_menu), "changed", (GCallback)on_pattern_menu_changed, (gpointer)self);

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
  gulong index;
  BtSong *song;
  BtSetup *setup;
  BtMachine *machine;

  GST_INFO("get machine for pattern");
  
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"setup",&setup,NULL);

  index=gtk_combo_box_get_active(self->priv->machine_menu);
  machine=bt_setup_get_machine_by_index(setup,index);

  //-- release the reference
  g_object_try_unref(setup);
  g_object_try_unref(song);
  return(machine);
}

/**
 * bt_main_page_patterns_get_current_pattern:
 * @self: the pattern subpage
 *
 * Get the currently active #BtPattern as determined by the pattern option menu
 * in the toolbar.
 *
 * Returns: the #BtPattern instance or NULL in case of an error
 */
BtPattern *bt_main_page_patterns_get_current_pattern(const BtMainPagePatterns *self) {
  gulong index;
  BtSong *song;
  BtSetup *setup;
  BtMachine *machine;
	BtPattern *pattern;

  GST_INFO("get machine for pattern");
  
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"setup",&setup,NULL);

  index=gtk_combo_box_get_active(self->priv->machine_menu);
  machine=bt_setup_get_machine_by_index(setup,index);
  index=gtk_combo_box_get_active(self->priv->pattern_menu);
	pattern=bt_machine_get_pattern_by_index(machine,index);

  //-- release the reference
  g_object_try_unref(setup);
  g_object_try_unref(song);
  return(pattern);
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

	g_object_try_unref(self->priv->source_icon);
	g_object_try_unref(self->priv->procesor_icon);
	g_object_try_unref(self->priv->sink_icon);
	
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

	self->priv->source_icon=gdk_pixbuf_new_from_filename("menu_source_machine.png");
	self->priv->procesor_icon=gdk_pixbuf_new_from_filename("menu_processor_machine.png");
	self->priv->sink_icon=gdk_pixbuf_new_from_filename("menu_sink_machine.png");
}

static void bt_main_page_patterns_class_init(BtMainPagePatternsClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

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
			NULL // value_table
    };
		type = g_type_register_static(GTK_TYPE_VBOX,"BtMainPagePatterns",&info,0);
  }
  return type;
}
