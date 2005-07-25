/* $Id: main-page-patterns.c,v 1.69 2005-07-25 20:38:50 ensonic Exp $
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
  /* wavetable selection menu */
  GtkComboBox *wavetable_menu;
  /* base octave selection menu */
  GtkComboBox *base_octave_menu;

	/* the pattern table */
  GtkTreeView *pattern_table;

  /* pattern context_menu */
  GtkMenu *context_menu;
	GtkWidget *context_menu_track_add,*context_menu_track_remove;
	GtkWidget *context_menu_pattern_properties,*context_menu_pattern_remove,*context_menu_pattern_copy;
	
	/* the pattern that is currently shown */
	BtPattern *pattern;
	/* signal handler ids */
	gint pattern_length_changed,pattern_voices_changed;
	gint pattern_menu_changed;
};

static GtkVBoxClass *parent_class=NULL;

enum {
  MACHINE_MENU_ICON=0,
  MACHINE_MENU_LABEL,
	MACHINE_MENU_MACHINE
};

enum {
  PATTERN_MENU_LABEL=0,
	PATTERN_MENU_PATTERN
};

enum {
  PATTERN_TABLE_POS=0,
  PATTERN_TABLE_PRE_CT
};

#define PATTERN_CELL_WIDTH 80
#define PATTERN_CELL_HEIGHT 28

//-- tree model helper

static void machine_model_get_iter_by_machine(GtkTreeModel *store,GtkTreeIter *iter,BtMachine *that_machine) {
	BtMachine *this_machine;

	gtk_tree_model_get_iter_first(store,iter);
	do {
		gtk_tree_model_get(store,iter,MACHINE_MENU_MACHINE,&this_machine,-1);
		if(this_machine==that_machine) break;
	} while(gtk_tree_model_iter_next(store,iter));
}

static void pattern_model_get_iter_by_pattern(GtkTreeModel *store,GtkTreeIter *iter,BtPattern *that_pattern) {
	BtPattern *this_pattern;

	gtk_tree_model_get_iter_first(store,iter);
	do {
		gtk_tree_model_get(store,iter,PATTERN_MENU_PATTERN,&this_pattern,-1);
		if(this_pattern==that_pattern) break;
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

static void on_pattern_name_changed(BtPattern *pattern,GParamSpec *arg,gpointer user_data) {
	BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
	GtkTreeModel *store;
	GtkTreeIter iter;
	gchar *str;
	
	g_assert(user_data);

	g_object_get(G_OBJECT(pattern),"name",&str,NULL);
  GST_INFO("pattern name changed to \"%s\"",str);
	
	store=gtk_combo_box_get_model(self->priv->pattern_menu);
	// get the row where row.pattern==pattern
	pattern_model_get_iter_by_pattern(store,&iter,pattern);
	gtk_list_store_set(GTK_LIST_STORE(store),&iter,PATTERN_MENU_LABEL,str,-1);

	g_free(str);
}

static gboolean on_pattern_table_key_release_event(GtkWidget *widget,GdkEventKey *event,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
	gboolean res=FALSE;
	
  g_assert(user_data);
	
	GST_INFO("pattern_table key : state 0x%x, keyval 0x%x",event->state,event->keyval);
	if(event->keyval==GDK_Return) {	/* GDK_KP_Enter */
		BtMainWindow *main_window;
		BtMainPages *pages;
		//BtMainPageSequence *sequence_page;

		g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
		g_object_get(G_OBJECT(main_window),"pages",&pages,NULL);
		//g_object_get(G_OBJECT(pages),"sequence-page",&sequence_page,NULL);
	
		gtk_notebook_set_current_page(GTK_NOTEBOOK(pages),BT_MAIN_PAGES_SEQUENCE_PAGE);
		//bt_main_page_sequence_goto_???(sequence_page,pattern);

		//g_object_try_unref(sequence_page);
		g_object_try_unref(pages);
		g_object_try_unref(main_window);

		res=TRUE;
	}
	return(res);
}

static gboolean on_pattern_table_button_press_event(GtkWidget *widget,GdkEventButton *event,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
	gboolean res=FALSE;

  g_assert(user_data);
	
	GST_INFO("pattern_table button : button 0x%x, type 0x%d",event->button,event->type);
	if(event->button==1) {
		if(gtk_tree_view_get_bin_window(self->priv->pattern_table)==(event->window)) {
			GtkTreePath *path;
			GtkTreeViewColumn *column;
			// determine sequence position from mouse coordinates
			if(gtk_tree_view_get_path_at_pos(self->priv->pattern_table,event->x,event->y,&path,&column,NULL,NULL)) {
				gtk_tree_view_set_cursor_on_cell(self->priv->pattern_table,path,column,NULL,FALSE);
				gtk_widget_grab_focus(GTK_WIDGET(self->priv->pattern_table));
				res=TRUE;
			}
			if(path) gtk_tree_path_free(path);
		}
	}
	else if(event->button==3) {
		gtk_menu_popup(self->priv->context_menu,NULL,NULL,NULL,NULL,3,gtk_get_current_event_time());
		res=TRUE;
	}
	return(res);
}

//-- event handler helper

static void machine_menu_add(const BtMainPagePatterns *self,BtMachine *machine,GtkListStore *store) {
	gchar *str;
	GtkTreeIter menu_iter;

  g_object_get(G_OBJECT(machine),"id",&str,NULL);
  GST_INFO("  adding \"%s\"",str);

	gtk_list_store_append(store,&menu_iter);
	gtk_list_store_set(store,&menu_iter,
		MACHINE_MENU_ICON,bt_ui_ressources_get_pixbuf_by_machine(machine),
		MACHINE_MENU_LABEL,str,
		MACHINE_MENU_MACHINE,machine,
		-1);
	g_signal_connect(G_OBJECT(machine),"notify::id",G_CALLBACK(on_machine_id_changed),(gpointer)self);
	
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

static void pattern_menu_refresh(const BtMainPagePatterns *self,BtMachine *machine) {
  BtPattern *pattern=NULL;
  GList *node,*list;
  gchar *str;
	GtkListStore *store;
	GtkTreeIter menu_iter;
	gint index=-1;
	gboolean is_internal;

  // update pattern menu
  store=gtk_list_store_new(2,G_TYPE_STRING,BT_TYPE_PATTERN);
  if(machine) {
		g_object_get(G_OBJECT(machine),"patterns",&list,NULL);
		for(node=list;node;node=g_list_next(node)) {
      pattern=BT_PATTERN(node->data);
      g_object_get(G_OBJECT(pattern),"name",&str,"is-internal",&is_internal,NULL);
			if(!is_internal) {
				GST_INFO("  adding \"%s\"",str);
				gtk_list_store_append(store,&menu_iter);
				gtk_list_store_set(store,&menu_iter,
					PATTERN_MENU_LABEL,str,
					PATTERN_MENU_PATTERN,pattern,
					-1);
				g_signal_connect(G_OBJECT(pattern),"notify::name",G_CALLBACK(on_pattern_name_changed),(gpointer)self);
        index++;	// count so that we can activate the last one
			}
			g_free(str);
    }
		g_list_free(list);
  }
	gtk_widget_set_sensitive(GTK_WIDGET(self->priv->pattern_menu),(pattern!=NULL));
	gtk_combo_box_set_model(self->priv->pattern_menu,GTK_TREE_MODEL(store));
  gtk_combo_box_set_active(self->priv->pattern_menu,((pattern!=NULL)?index:-1));
	g_object_unref(store); // drop with comboxbox
}

static void wavetable_menu_refresh(const BtMainPagePatterns *self) {
	BtWave *wave=NULL;
  //GList *node,*list;
  //gchar *str;
	GtkListStore *store;
	//GtkTreeIter menu_iter;
	gint index=-1;

  // update pattern menu
  store=gtk_list_store_new(2,G_TYPE_STRING,BT_TYPE_WAVE);
	
	// @todo scan wavetable list for waves
	
	gtk_widget_set_sensitive(GTK_WIDGET(self->priv->wavetable_menu),(wave!=NULL));
	gtk_combo_box_set_model(self->priv->wavetable_menu,GTK_TREE_MODEL(store));
  gtk_combo_box_set_active(self->priv->wavetable_menu,((wave!=NULL)?index:-1));
	g_object_unref(store); // drop with comboxbox
}

/*
 * pattern_table_clear:
 * @self: the pattern page
 *
 * removes old columns
 */
static void pattern_table_clear(const BtMainPagePatterns *self) {
  GList *columns,*node;
		
  // remove columns
	g_assert(self->priv->pattern_table);
	GST_INFO("clearing pattern table");
  if((columns=gtk_tree_view_get_columns(self->priv->pattern_table))) {
		GST_DEBUG("is not empty");
		for(node=g_list_first(columns);node;node=g_list_next(node)) {
      gtk_tree_view_remove_column(self->priv->pattern_table,GTK_TREE_VIEW_COLUMN(node->data));
    }
    g_list_free(columns);
  }
	GST_INFO("done");
}

/*
 * pattern_table_refresh:
 * @self:  the pattern page
 * @pattern: the new pattern to display
 *
 * rebuild the pattern table after a new pattern has been chosen
 */
static void pattern_table_refresh(const BtMainPagePatterns *self,const BtPattern *pattern) {
	BtMachine *machine;
	gulong i,j,number_of_ticks,pos=0;
	gulong col_ct,number_of_global_params;
	gint col_index;
  GtkCellRenderer *renderer;
  GtkListStore *store;
  GType *store_types;
	GtkTreeIter tree_iter;
	gchar *str;
	//GParamSpec *pspec;
  GtkTreeViewColumn *tree_col;

	GST_INFO("refresh pattern table");
	g_assert(GTK_IS_TREE_VIEW	(self->priv->pattern_table));
	
  // reset columns
	pattern_table_clear(self);

	if(pattern) {
		g_object_get(G_OBJECT(pattern),"length",&number_of_ticks,"machine",&machine,NULL);
		g_object_get(G_OBJECT(machine),"global-params",&number_of_global_params,NULL);
	  GST_DEBUG("  size is %2d,%2d",number_of_ticks,number_of_global_params);

		// @todo include voice params
		
		// build model
		GST_DEBUG("  build model");
		col_ct=1+number_of_global_params;
		store_types=(GType *)g_new(GType *,col_ct);
		store_types[0]=G_TYPE_LONG;
		for(i=1;i<col_ct;i++) {
			store_types[i]=G_TYPE_STRING;
		}
		store=gtk_list_store_newv(col_ct,store_types);
  	g_free(store_types);
		// add events
  	for(i=0;i<number_of_ticks;i++) {
    	gtk_list_store_append(store, &tree_iter);
    	// set position, highlight-color
    	gtk_list_store_set(store,&tree_iter,
				PATTERN_TABLE_POS,pos,
				-1);
			for(j=0;j<number_of_global_params;j++) {
				if((str=bt_pattern_get_global_event(pattern,i,j))) {
					gtk_list_store_set(store,&tree_iter,
						PATTERN_TABLE_PRE_CT+j,str,
						-1);
					g_free(str);
				}
			}
			pos++;
		}
		GST_DEBUG("    activating store: %p",store);
		gtk_tree_view_set_model(self->priv->pattern_table,GTK_TREE_MODEL(store));

  	// build view
		GST_DEBUG("  build view");
		// add initial columns
  	renderer=gtk_cell_renderer_text_new();
  	g_object_set(G_OBJECT(renderer),
			"mode",GTK_CELL_RENDERER_MODE_INERT,
			"xalign",1.0,
			// workaround to make focus visible
			"height",PATTERN_CELL_HEIGHT-4,
			NULL);
  	if((tree_col=gtk_tree_view_column_new_with_attributes(_("Pos."),renderer,
    	"text",PATTERN_TABLE_POS,
    	NULL))
		) {
			g_object_set(tree_col,
				"sizing",GTK_TREE_VIEW_COLUMN_FIXED,
				"fixed-width",40,
					NULL);
			gtk_tree_view_append_column(self->priv->pattern_table,tree_col);
		}
		else GST_WARNING("can't create treeview column");
		for(j=0;j<number_of_global_params;j++) {
			//pspec=bt_machine_get_global_param_spec(machine,j);
	  	renderer=gtk_cell_renderer_text_new();
			g_object_set(G_OBJECT(renderer),
				"mode",GTK_CELL_RENDERER_MODE_ACTIVATABLE,
				"xalign",1.0,
				NULL);
	  	if((tree_col=gtk_tree_view_column_new_with_attributes(bt_machine_get_global_param_name(machine,j),renderer,
  	  	"text",PATTERN_TABLE_PRE_CT+j,
    		NULL))
			) {
				g_object_set(tree_col,
					"sizing",GTK_TREE_VIEW_COLUMN_FIXED,
					"fixed-width",PATTERN_CELL_WIDTH,
					NULL);
				gtk_tree_view_append_column(self->priv->pattern_table,tree_col);
			}
			else GST_WARNING("can't create treeview column");
		}
	}
	else {
		// create empty list model, so that the context menu works
		store=gtk_list_store_new(1,G_TYPE_STRING);
		// one empty field
		gtk_list_store_append(store, &tree_iter);
		gtk_list_store_set(store,&tree_iter, 0, "",-1);

		GST_DEBUG("    activating dummy store: %p",store);
		gtk_tree_view_set_model(self->priv->pattern_table,GTK_TREE_MODEL(store));
	}
	// add a final column that eats remaining space
	renderer=gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer),
		"mode",GTK_CELL_RENDERER_MODE_INERT,
		NULL);
	if((tree_col=gtk_tree_view_column_new_with_attributes(/*title=*/NULL,renderer,NULL))) {
		g_object_set(tree_col,
			"sizing",GTK_TREE_VIEW_COLUMN_FIXED,
			NULL);
		col_index=gtk_tree_view_append_column(self->priv->pattern_table,tree_col);
		GST_DEBUG("    number of columns : %d",col_index);
	}
	else GST_WARNING("can't create treeview column");

	//gtk_widget_set_sensitive(GTK_WIDGET(self->priv->pattern_table),TRUE);

	GST_DEBUG("  done");
	// release the references
	g_object_unref(store); // drop with treeview
}

/*
 * context_menu_refresh:
 * @self:  the pattern page
 * @machine: the currently selected machine
 *
 * enable/disable context menu items
 */
static void context_menu_refresh(const BtMainPagePatterns *self,BtMachine *machine) {
	GList *list;

	if(machine) {
		g_object_get(G_OBJECT(machine),"patterns",&list,NULL);
		
		//gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu),TRUE);
		if(bt_machine_is_polyphonic(machine)) {
			gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_track_add),TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_track_remove),TRUE);
		}
		else {
			gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_track_add),FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_track_remove),FALSE);
		}
		if(list) {
			gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_pattern_properties),TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_pattern_remove),TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_pattern_copy),TRUE);
			g_list_free(list);
		}
		else {
			GST_WARNING("machine has no patterns");
			gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_pattern_properties),FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_pattern_remove),FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_pattern_copy),FALSE);
		}
	}
	else {
		GST_WARNING("no machine, huh?");
		//gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu),FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_pattern_properties),FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_pattern_remove),FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_pattern_copy),FALSE);
	}
}

//-- event handler

static void on_pattern_size_changed(BtPattern *pattern,GParamSpec *arg,gpointer user_data) {
	BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);

	GST_INFO("pattern size changed : %p",self->priv->pattern);
	pattern_table_refresh(self,pattern);
}

static void on_pattern_menu_changed(GtkComboBox *menu, gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);

  g_assert(user_data);

	if(self->priv->pattern) {
		g_signal_handler_disconnect(self->priv->pattern,self->priv->pattern_length_changed);
		g_signal_handler_disconnect(self->priv->pattern,self->priv->pattern_voices_changed);
		g_object_unref(self->priv->pattern);
	}

  // refresh pattern view
	self->priv->pattern=bt_main_page_patterns_get_current_pattern(self);
	
	GST_INFO("new pattern selected : %p",self->priv->pattern);
	pattern_table_refresh(self,self->priv->pattern);
	if(self->priv->pattern) {
		// watch the pattern
		self->priv->pattern_length_changed=g_signal_connect(G_OBJECT(self->priv->pattern),"notify::length",G_CALLBACK(on_pattern_size_changed),(gpointer)self);
		self->priv->pattern_voices_changed=g_signal_connect(G_OBJECT(self->priv->pattern),"notify::voices",G_CALLBACK(on_pattern_size_changed),(gpointer)self);
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
	BtMachine *machine;
	
  g_assert(user_data);
	machine=bt_main_page_patterns_get_current_machine(self);
	// show new list of pattern in pattern menu
  pattern_menu_refresh(self,machine);
	// refresh context menu
	context_menu_refresh(self,machine);
	g_object_try_unref(machine);
}

static void on_song_changed(const BtEditApplication *app,GParamSpec *arg,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  BtSong *song;
  BtSetup *setup;

  g_assert(user_data);

  GST_INFO("song has changed : app=%p, self=%p",app,self);
  // get song from app and then setup from song
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
	g_return_if_fail(song);

  g_object_get(G_OBJECT(song),"setup",&setup,NULL);
  // update page
  machine_menu_refresh(self,setup);
  //pattern_menu_refresh(self); // should be triggered by machine_menu_refresh()
	wavetable_menu_refresh(self);
	g_signal_connect(G_OBJECT(setup),"machine-added",G_CALLBACK(on_machine_added),(gpointer)self);
	g_signal_connect(G_OBJECT(setup),"machine-removed",G_CALLBACK(on_machine_removed),(gpointer)self);
  // release the references
  g_object_try_unref(setup);
  g_object_try_unref(song);
}

static void on_context_menu_track_add_activate(GtkMenuItem *menuitem,gpointer user_data) {
  //BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);

	// @todo implement track_add
  g_assert(user_data);
}

static void on_context_menu_track_remove_activate(GtkMenuItem *menuitem,gpointer user_data) {
  //BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);

	// @todo implement track_remove
  g_assert(user_data);
}

static void on_context_menu_pattern_new_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
	BtSong *song;
	BtMachine *machine;
	BtPattern *pattern;
	gchar *mid,*id,*name;
	GtkWidget *dialog;

  g_assert(user_data);

	machine=bt_main_page_patterns_get_current_machine(self);
	g_return_if_fail(machine);
	
	g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
	g_object_get(G_OBJECT(machine),"id",&mid,NULL);

	name=bt_machine_get_unique_pattern_name(machine);
	id=g_strdup_printf("%s %s",mid,name);
	// new_pattern
	pattern=bt_pattern_new(song, id, name, /*length=*/16, machine);

	// pattern_properties
	dialog=GTK_WIDGET(bt_pattern_properties_dialog_new(self->priv->app,pattern));
	gtk_widget_show_all(dialog);

	if(gtk_dialog_run(GTK_DIALOG(dialog))==GTK_RESPONSE_ACCEPT) {
		bt_pattern_properties_dialog_apply(BT_PATTERN_PROPERTIES_DIALOG(dialog));

		GST_INFO("new pattern added : %p",pattern);
		pattern_menu_refresh(self,machine);
		context_menu_refresh(self,machine);
	}
	else {
		bt_machine_remove_pattern(machine,pattern);
	}
  gtk_widget_destroy(dialog);
	
	// free ressources
	g_free(mid);
	g_free(id);
	g_free(name);
	g_object_unref(pattern);
	g_object_unref(song);
	g_object_unref(machine);
	GST_DEBUG("new pattern done");
}

static void on_context_menu_pattern_properties_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
	BtPattern *pattern;
	GtkWidget *dialog;
	
  g_assert(user_data);
	
	pattern=bt_main_page_patterns_get_current_pattern(self);
	g_return_if_fail(pattern);

	// pattern_properties
	dialog=GTK_WIDGET(bt_pattern_properties_dialog_new(self->priv->app,pattern));
	gtk_widget_show_all(dialog);

	if(gtk_dialog_run(GTK_DIALOG(dialog))==GTK_RESPONSE_ACCEPT) {
		bt_pattern_properties_dialog_apply(BT_PATTERN_PROPERTIES_DIALOG(dialog));
	}
  gtk_widget_destroy(dialog);
	g_object_unref(pattern);
}

static void on_context_menu_pattern_remove_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  BtMainWindow *main_window;
	BtPattern *pattern;
	gchar *msg,*id;

  g_assert(user_data);

	pattern=bt_main_page_patterns_get_current_pattern(self);
	g_return_if_fail(pattern);
	
	g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
	g_object_get(pattern,"name",&id,NULL);
	msg=g_strdup_printf(_("Delete pattern '%s'"),id);
	g_free(id);
	
	if(bt_dialog_question(main_window,_("Delete pattern..."),msg,_("There is no undo for this."))) {
		BtMachine *machine;
		
		machine=bt_main_page_patterns_get_current_machine(self);
		bt_machine_remove_pattern(machine,pattern);
		pattern_menu_refresh(self,machine);
		context_menu_refresh(self,machine);

		g_object_unref(machine);
	}
	g_object_try_unref(main_window);
	g_object_unref(pattern);	// should finalize the pattern
	g_free(msg);
}

static void on_context_menu_pattern_copy_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
 	BtMachine *machine;
	BtPattern *pattern,*pattern_new;
	GtkWidget *dialog;

  g_assert(user_data);

	machine=bt_main_page_patterns_get_current_machine(self);
	g_return_if_fail(machine);

	pattern=bt_main_page_patterns_get_current_pattern(self);
	g_return_if_fail(pattern);
	
	// copy pattern
	pattern_new=bt_pattern_copy(pattern);
	g_object_unref(pattern);
	pattern=pattern_new;
	g_return_if_fail(pattern);

	// pattern_properties
	dialog=GTK_WIDGET(bt_pattern_properties_dialog_new(self->priv->app,pattern));
	gtk_widget_show_all(dialog);

	if(gtk_dialog_run(GTK_DIALOG(dialog))==GTK_RESPONSE_ACCEPT) {
		bt_pattern_properties_dialog_apply(BT_PATTERN_PROPERTIES_DIALOG(dialog));

		GST_INFO("new pattern added : %p",pattern);
		pattern_menu_refresh(self,machine);
		context_menu_refresh(self,machine);
	}
	else {
		bt_machine_remove_pattern(machine,pattern);
	}
  gtk_widget_destroy(dialog);
	
	g_object_unref(pattern);
	g_object_unref(machine);
}

//-- helper methods

static gboolean bt_main_page_patterns_init_ui(const BtMainPagePatterns *self) {
  GtkWidget *toolbar,*scrolled_window;
  GtkWidget *box,*tool_item;
	GtkWidget *menu_item,*image;
	GtkCellRenderer *renderer;
	GtkTreeSelection *tree_sel;
	gint i;
	
	GST_DEBUG("!!!! self=%p",self);
	
  // add toolbar
  toolbar=gtk_toolbar_new();
  gtk_widget_set_name(toolbar,_("pattern view tool bar"));
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
	g_signal_connect(G_OBJECT(self->priv->machine_menu), "changed", G_CALLBACK(on_machine_menu_changed), (gpointer)self);

  gtk_box_pack_start(GTK_BOX(box),gtk_label_new(_("Machine")),FALSE,FALSE,2);
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->priv->machine_menu),TRUE,TRUE,2);

	tool_item=GTK_WIDGET(gtk_tool_item_new());
  gtk_widget_set_name(tool_item,_("Machine"));
	gtk_container_add(GTK_CONTAINER(tool_item),box);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),GTK_TOOL_ITEM(tool_item),-1);

  gtk_toolbar_insert(GTK_TOOLBAR(toolbar),gtk_separator_tool_item_new(),-1);

  // pattern select
  box=gtk_hbox_new(FALSE,2);
  gtk_container_set_border_width(GTK_CONTAINER(box),4);
  self->priv->pattern_menu=GTK_COMBO_BOX(gtk_combo_box_new());
	renderer=gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(self->priv->pattern_menu),renderer,TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(self->priv->pattern_menu),renderer,"text", 0,NULL);
  gtk_box_pack_start(GTK_BOX(box),gtk_label_new(_("Pattern")),FALSE,FALSE,2);
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->priv->pattern_menu),TRUE,TRUE,2);
	self->priv->pattern_menu_changed=g_signal_connect(G_OBJECT(self->priv->pattern_menu), "changed", G_CALLBACK(on_pattern_menu_changed), (gpointer)self);

	tool_item=GTK_WIDGET(gtk_tool_item_new());
  gtk_widget_set_name(tool_item,_("Pattern"));
	gtk_container_add(GTK_CONTAINER(tool_item),box);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),GTK_TOOL_ITEM(tool_item),-1);

  gtk_toolbar_insert(GTK_TOOLBAR(toolbar),gtk_separator_tool_item_new(),-1);

  // add wavetable entry select
	box=gtk_hbox_new(FALSE,2);
  gtk_container_set_border_width(GTK_CONTAINER(box),4);
	self->priv->wavetable_menu=GTK_COMBO_BOX(gtk_combo_box_new());
	renderer=gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(self->priv->wavetable_menu),renderer,TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(self->priv->wavetable_menu),renderer,"text", 0,NULL);
  gtk_box_pack_start(GTK_BOX(box),gtk_label_new(_("Wave")),FALSE,FALSE,2);
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->priv->wavetable_menu),TRUE,TRUE,2);
	//self->priv->wavetable_menu_changed=g_signal_connect(G_OBJECT(self->priv->wavetable_menu), "changed", G_CALLBACK(on_wavetable_menu_changed), (gpointer)self);

	tool_item=GTK_WIDGET(gtk_tool_item_new());
  gtk_widget_set_name(tool_item,_("Wave"));
	gtk_container_add(GTK_CONTAINER(tool_item),box);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),GTK_TOOL_ITEM(tool_item),-1);

  gtk_toolbar_insert(GTK_TOOLBAR(toolbar),gtk_separator_tool_item_new(),-1);

	// add base octave (0-8)
	box=gtk_hbox_new(FALSE,2);
  gtk_container_set_border_width(GTK_CONTAINER(box),4);
	self->priv->base_octave_menu=GTK_COMBO_BOX(gtk_combo_box_new_text());
	for(i=0;i<8;i++) {
		gtk_combo_box_append_text(self->priv->base_octave_menu,g_strdup_printf("%1d",i));
	}
	gtk_combo_box_set_active(self->priv->base_octave_menu,3);
  gtk_box_pack_start(GTK_BOX(box),gtk_label_new(_("Octave")),FALSE,FALSE,2);
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->priv->base_octave_menu),TRUE,TRUE,2);
	//g_signal_connect(G_OBJECT(self->priv->base_octave_menu), "changed", G_CALLBACK(on_base_octave_menu_changed), (gpointer)self);

	tool_item=GTK_WIDGET(gtk_tool_item_new());
  gtk_widget_set_name(tool_item,_("Octave"));
	gtk_container_add(GTK_CONTAINER(tool_item),box);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),GTK_TOOL_ITEM(tool_item),-1);

  gtk_toolbar_insert(GTK_TOOLBAR(toolbar),gtk_separator_tool_item_new(),-1);

  // @todo add play notes checkbox or toggle tool button
  
	
	// @idea what about adding one control for global params and one for each voice, then these controls can be folded
  // add list-view for pattern
	scrolled_window=gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_ETCHED_IN);
  self->priv->pattern_table=GTK_TREE_VIEW(gtk_tree_view_new());
	tree_sel=gtk_tree_view_get_selection(self->priv->pattern_table);
	// GTK_SELECTION_BROWSE unfortunately selects whole rows, we rather need something that just outlines current row and column
	// we can set a gtk_tree_selection_set_select_function() to select ranges
	gtk_tree_selection_set_mode(tree_sel,GTK_SELECTION_NONE);
	g_object_set(self->priv->pattern_table,"enable-search",FALSE,"rules-hint",TRUE,"fixed-height-mode",TRUE,NULL);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window),GTK_WIDGET(self->priv->pattern_table));
  gtk_container_add(GTK_CONTAINER(self),scrolled_window);
  //g_signal_connect(G_OBJECT(self->priv->pattern_table), "move-cursor", G_CALLBACK(on_pattern_table_move_cursor), (gpointer)self);
  //g_signal_connect(G_OBJECT(self->priv->pattern_table), "cursor-changed", G_CALLBACK(on_pattern_table_cursor_changed), (gpointer)self);
	g_signal_connect(G_OBJECT(self->priv->pattern_table), "key-release-event", G_CALLBACK(on_pattern_table_key_release_event), (gpointer)self);
	g_signal_connect(G_OBJECT(self->priv->pattern_table), "button-press-event", G_CALLBACK(on_pattern_table_button_press_event), (gpointer)self);

	GST_DEBUG("  before context menu",self);

	// generate the context menu
  self->priv->context_menu=GTK_MENU(gtk_menu_new());
	
	self->priv->context_menu_track_add=gtk_image_menu_item_new_with_label(_("New track"));
	image=gtk_image_new_from_stock(GTK_STOCK_ADD,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(self->priv->context_menu_track_add),image);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),self->priv->context_menu_track_add);
  gtk_widget_show(self->priv->context_menu_track_add);
	g_signal_connect(G_OBJECT(self->priv->context_menu_track_add),"activate",G_CALLBACK(on_context_menu_track_add_activate),(gpointer)self);
	
	self->priv->context_menu_track_remove=gtk_image_menu_item_new_with_label(_("Remove last track"));
	image=gtk_image_new_from_stock(GTK_STOCK_REMOVE,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(self->priv->context_menu_track_remove),image);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),self->priv->context_menu_track_remove);
  gtk_widget_show(self->priv->context_menu_track_remove);
	g_signal_connect(G_OBJECT(self->priv->context_menu_track_remove),"activate",G_CALLBACK(on_context_menu_track_remove_activate),(gpointer)self);
	
	menu_item=gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_set_sensitive(menu_item,FALSE);
  gtk_widget_show(menu_item);

	menu_item=gtk_image_menu_item_new_with_label(_("New pattern  ..."));
	image=gtk_image_new_from_stock(GTK_STOCK_NEW,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
	g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_context_menu_pattern_new_activate),(gpointer)self);
	
	self->priv->context_menu_pattern_properties=gtk_image_menu_item_new_with_label(_("Pattern properties ..."));
	image=gtk_image_new_from_stock(GTK_STOCK_PROPERTIES,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(self->priv->context_menu_pattern_properties),image);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),self->priv->context_menu_pattern_properties);
  gtk_widget_show(self->priv->context_menu_pattern_properties);
	g_signal_connect(G_OBJECT(self->priv->context_menu_pattern_properties),"activate",G_CALLBACK(on_context_menu_pattern_properties_activate),(gpointer)self);
	
	self->priv->context_menu_pattern_remove=gtk_image_menu_item_new_with_label(_("Remove pattern ..."));
	image=gtk_image_new_from_stock(GTK_STOCK_DELETE,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(self->priv->context_menu_pattern_remove),image);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),self->priv->context_menu_pattern_remove);
  gtk_widget_show(self->priv->context_menu_pattern_remove);
	g_signal_connect(G_OBJECT(self->priv->context_menu_pattern_remove),"activate",G_CALLBACK(on_context_menu_pattern_remove_activate),(gpointer)self);
	
  self->priv->context_menu_pattern_copy=gtk_image_menu_item_new_with_label(_("Copy pattern ..."));
	image=gtk_image_new_from_stock(GTK_STOCK_COPY,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(self->priv->context_menu_pattern_copy),image);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),self->priv->context_menu_pattern_copy);
  gtk_widget_show(self->priv->context_menu_pattern_copy);
	g_signal_connect(G_OBJECT(self->priv->context_menu_pattern_copy),"activate",G_CALLBACK(on_context_menu_pattern_copy_activate),(gpointer)self);
	// --
	// @todo solo, mute, bypass
	// --
	// @todo cut, copy, paste

  // register event handlers
  g_signal_connect(G_OBJECT(self->priv->app), "notify::song", G_CALLBACK(on_song_changed), (gpointer)self);

	GST_DEBUG("  done");
	return(TRUE);
}

//-- constructor methods

/**
 * bt_main_page_patterns_new:
 * @app: the application the window belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtMainPagePatterns *bt_main_page_patterns_new(const BtEditApplication *app) {
  BtMainPagePatterns *self;

  if(!(self=BT_MAIN_PAGE_PATTERNS(g_object_new(BT_TYPE_MAIN_PAGE_PATTERNS,"app",app,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_main_page_patterns_init_ui(self)) {
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
 * Unref the machine, when done with it.
 *
 * Returns: the #BtMachine instance or %NULL in case of an error
 */
BtMachine *bt_main_page_patterns_get_current_machine(const BtMainPagePatterns *self) {
  BtMachine *machine=NULL;
  GtkTreeIter iter;
  GtkTreeModel *store;

  GST_INFO("get current machine");
  
  if(gtk_combo_box_get_active_iter(self->priv->machine_menu,&iter)) {
    store=gtk_combo_box_get_model(self->priv->machine_menu);
    gtk_tree_model_get(store,&iter,MACHINE_MENU_MACHINE,&machine,-1);
  }
  return(machine);
}

/**
 * bt_main_page_patterns_get_current_pattern:
 * @self: the pattern subpage
 *
 * Get the currently active #BtPattern as determined by the pattern option menu
 * in the toolbar.
 * Unref the pattern, when done with it.
 *
 * Returns: the #BtPattern instance or %NULL in case of an error
 */
BtPattern *bt_main_page_patterns_get_current_pattern(const BtMainPagePatterns *self) {
  BtMachine *machine;
	BtPattern *pattern=NULL;
  GtkTreeIter iter;
  GtkTreeModel *store;
  
  GST_INFO("get current pattern");
  
  if(gtk_combo_box_get_active_iter(self->priv->machine_menu,&iter)) {
    store=gtk_combo_box_get_model(self->priv->machine_menu);
    gtk_tree_model_get(store,&iter,MACHINE_MENU_MACHINE,&machine,-1);
    if(machine) {
      if(gtk_combo_box_get_active_iter(self->priv->pattern_menu,&iter)) {
        store=gtk_combo_box_get_model(self->priv->pattern_menu);
        gtk_tree_model_get(store,&iter,PATTERN_MENU_PATTERN,&pattern,-1);
      }
    }
	}
  return(g_object_ref(pattern));
}

/**
 * bt_main_page_patterns_show_pattern:
 * @self: the pattern subpage
 * @pattern: the pattern to show
 *
 * Show the given pattern. Will update machine and pattern menu.
 */
void bt_main_page_patterns_show_pattern(const BtMainPagePatterns *self,BtPattern *pattern) {
	BtMachine *machine;
  GtkTreeIter iter;
  GtkTreeModel *store;

	g_object_get(G_OBJECT(pattern),"machine",&machine,NULL);
	// update machine menu
  store=gtk_combo_box_get_model(self->priv->machine_menu);
  machine_model_get_iter_by_machine(store,&iter,machine);
  gtk_combo_box_set_active_iter(self->priv->machine_menu,&iter);
	// update pattern menu
  store=gtk_combo_box_get_model(self->priv->pattern_menu);
  pattern_model_get_iter_by_pattern(store,&iter,pattern);
  gtk_combo_box_set_active_iter(self->priv->pattern_menu,&iter);
	// focus pattern editor
	gtk_widget_grab_focus(GTK_WIDGET(self->priv->pattern_table));
  // release the references
	g_object_try_unref(machine);
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
      g_object_try_weak_unref(self->priv->app);
      self->priv->app = BT_EDIT_APPLICATION(g_value_get_object(value));
			g_object_try_weak_ref(self->priv->app);
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
	
  g_object_try_weak_unref(self->priv->app);
	g_object_try_unref(self->priv->pattern);
	
	gtk_object_destroy(GTK_OBJECT(self->priv->context_menu));

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
      G_STRUCT_SIZE(BtMainPagePatternsClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_main_page_patterns_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtMainPagePatterns),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_main_page_patterns_init, // instance_init
			NULL // value_table
    };
		type = g_type_register_static(GTK_TYPE_VBOX,"BtMainPagePatterns",&info,0);
  }
  return type;
}
