/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 2; tab-width: 2 -*- */

/* $Id: main-page-sequence.c,v 1.49 2005-01-28 18:04:44 ensonic Exp $
 * class for the editor main sequence page
 */

/* @todo:
 *  - moving cursor around
 *  - disallowing to move to position column
 *  - adding/removing columns
 *  - add mute/solo/bypass buttons to column headers
 */

#define BT_EDIT
#define BT_MAIN_PAGE_SEQUENCE_C

#include "bt-edit.h"

enum {
  MAIN_PAGE_SEQUENCE_APP=1,
};


struct _BtMainPageSequencePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the application */
  BtEditApplication *app;
  
  /* bars selection menu */
  GtkComboBox *bars_menu;
	gulong bars;
  
  /* the sequence table */
  GtkTreeView *sequence_table;
  /* the pattern list */
  GtkTreeView *pattern_list;

  /* pattern context_menu */
  GtkMenu *context_menu;

  /* colors */
  GdkColor source_bg1,source_bg2;
  GdkColor processor_bg1,processor_bg2;
  GdkColor sink_bg1,sink_bg2;
	
	glong tick_pos;
};

static GtkVBoxClass *parent_class=NULL;

enum {
  SEQUENCE_TABLE_SOURCE_BG=0,
  SEQUENCE_TABLE_PROCESSOR_BG,
  SEQUENCE_TABLE_SINK_BG,
	SEQUENCE_TABLE_TICK_FG_SET,
  SEQUENCE_TABLE_POS,
  SEQUENCE_TABLE_LABEL,
  SEQUENCE_TABLE_PRE_CT
};

#define IS_SEQUENCE_POS_VISIBLE(pos,bars) ((pos&((bars)-1))==0)

static gchar *pattern_keys="0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

//-- tree filter func

static gboolean step_visible_filter(GtkTreeModel *store,GtkTreeIter *iter,gpointer user_data) {
	//gboolean visible=TRUE;
	BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
	gulong pos;
	
	g_assert(user_data);
	
	// determine row number and hide or show accordingly
	gtk_tree_model_get(store,iter,SEQUENCE_TABLE_POS,&pos,-1);
	//visible=IS_SEQUENCE_POS_VISIBLE(pos,self->priv->bars);
	//GST_INFO("bars=%d, pos=%d, -> visible=%1d",self->priv->bars,pos,visible);
	return(IS_SEQUENCE_POS_VISIBLE(pos,self->priv->bars));
}

//-- tree model helper

static gboolean sequence_model_get_iter_by_position(GtkTreeModel *store,GtkTreeIter *iter,gulong that_pos) {
	gulong this_pos;
	gboolean found=FALSE;

	gtk_tree_model_get_iter_first(store,iter);
	do {
		gtk_tree_model_get(store,iter,SEQUENCE_TABLE_POS,&this_pos,-1);
		if(this_pos==that_pos) { 
			found=TRUE;break;
		}
	} while(gtk_tree_model_iter_next(store,iter));
	return(found);
}

/*
 * sequence_model_recolorize:
 * alternate coloring for visible rows
 */
static void sequence_model_recolorize(BtMainPageSequence *self) {
	GtkTreeModel *store;
	GtkTreeModelFilter *filtered_store;
	GtkTreeIter iter;
	gboolean visible,odd_row=FALSE;

	GST_INFO("recolorize sequence tree view");
	
	if((filtered_store=GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(self->priv->sequence_table)))
		&& (store=gtk_tree_model_filter_get_model(filtered_store)))
	{
		gtk_tree_model_get_iter_first(store,&iter);
		do {
			if(step_visible_filter(store,&iter,self)) {
				if(odd_row) {
    		  gtk_list_store_set(GTK_LIST_STORE(store),&iter,
        		SEQUENCE_TABLE_SOURCE_BG   ,&self->priv->source_bg2,
        		SEQUENCE_TABLE_PROCESSOR_BG,&self->priv->processor_bg2,
        		SEQUENCE_TABLE_SINK_BG     ,&self->priv->sink_bg2,
        		-1);
				}
				else {
      		gtk_list_store_set(GTK_LIST_STORE(store),&iter,
        		SEQUENCE_TABLE_SOURCE_BG   ,&self->priv->source_bg1,
        		SEQUENCE_TABLE_PROCESSOR_BG,&self->priv->processor_bg1,
        		SEQUENCE_TABLE_SINK_BG     ,&self->priv->sink_bg1,
        		-1);
				}
				odd_row=!odd_row;
			}
		} while(gtk_tree_model_iter_next(store,&iter));
	}
	else {
		GST_WARNING("can't get tree model");
	}
}

//-- event handlers

static void on_machine_id_changed(BtMachine *machine,GParamSpec *arg,gpointer user_data) {
	GtkLabel *label=GTK_LABEL(user_data);
	gchar *str;
	
	g_object_get(G_OBJECT(machine),"id",&str,NULL);
  GST_INFO("machine id changed to \"%s\"",str);
  gtk_label_set_text(label,str);
	g_free(str);
}

//-- event handler helper

/**
 * sequence_table_clear:
 * @self: the sequence page
 *
 * removes old columns
 */
static void sequence_table_clear(const BtMainPageSequence *self) {
  GList *columns,*node;
  gint col_index;
	
  // remove columns
  if((columns=gtk_tree_view_get_columns(self->priv->sequence_table))) {
		for(node=g_list_first(columns);node;node=g_list_next(node)) {
      gtk_tree_view_remove_column(self->priv->sequence_table,GTK_TREE_VIEW_COLUMN(node->data));
    }
    g_list_free(columns);
  }
}

/**
 * sequence_table_init:
 * @self: the sequence page
 *
 * inserts the Pos and Label columns.
 */
static void sequence_table_init(const BtMainPageSequence *self) {
  GtkCellRenderer *renderer;
	gint col_index;
	
  // re-add static columns
  renderer=gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer),"mode",GTK_CELL_RENDERER_MODE_INERT,"xalign",1.0,"foreground","red",NULL);
  gtk_tree_view_insert_column_with_attributes(self->priv->sequence_table,-1,_("Pos."),renderer,
    "text",SEQUENCE_TABLE_POS,
		"foreground-set",SEQUENCE_TABLE_TICK_FG_SET,
    NULL);
  renderer=gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer),"xalign",1.0,"foreground","red",NULL);
  col_index=gtk_tree_view_insert_column_with_attributes(self->priv->sequence_table,-1,_("Labels"),renderer,
    "text",SEQUENCE_TABLE_LABEL,
		"foreground-set",SEQUENCE_TABLE_TICK_FG_SET,
    NULL);
	
	GST_DEBUG("    number of columns : %d",col_index);
}

/**
 * sequence_table_refresh:
 * @self:  the sequence page
 * @song: the newly created song
 *
 * rebuild the sequence table after a structural change
 */
static void sequence_table_refresh(const BtMainPageSequence *self,const BtSong *song) {
  BtSetup *setup;
  BtSequence *sequence;
  BtMachine *machine;
  BtTimeLine *timeline;
  BtTimeLineTrack *timelinetrack;
  BtPattern *pattern;
	GtkWidget *label;
  gchar *str;
  gulong i,j,col_ct,timeline_ct,track_ct,pos=0;
	gint col_index;
  GtkCellRenderer *renderer;
  GtkListStore *store;
	GtkTreeModel *filtered_store;
  GType *store_types;
  GtkTreeIter tree_iter;
  GtkTreeViewColumn *tree_col;
  gulong pattern_type;
  gboolean free_str;

  GST_INFO("refresh sequence table");
  
  g_object_get(G_OBJECT(song),"setup",&setup,"sequence",&sequence,NULL);
  g_object_get(G_OBJECT(sequence),"length",&timeline_ct,"tracks",&track_ct,NULL);
  GST_INFO("  size is %2d,%2d",timeline_ct,track_ct);
	
  // reset columns
	sequence_table_clear(self);

	// build model
	GST_DEBUG("  build model");
  col_ct=(SEQUENCE_TABLE_PRE_CT+track_ct);
  store_types=(GType *)g_new(GType *,col_ct);
  // for background color columns
  store_types[SEQUENCE_TABLE_SOURCE_BG   ]=GDK_TYPE_COLOR;
  store_types[SEQUENCE_TABLE_PROCESSOR_BG]=GDK_TYPE_COLOR;
  store_types[SEQUENCE_TABLE_SINK_BG     ]=GDK_TYPE_COLOR;
  store_types[SEQUENCE_TABLE_TICK_FG_SET ]=G_TYPE_BOOLEAN;
  // for static display columns
  store_types[SEQUENCE_TABLE_POS         ]=G_TYPE_LONG;
  // for track display columns
  for(i=SEQUENCE_TABLE_LABEL;i<col_ct;i++) {
    store_types[i]=G_TYPE_STRING;
  }
  store=gtk_list_store_newv(col_ct,store_types);
  g_free(store_types);
  // add patterns
  for(i=0;i<timeline_ct;i++) {
    timeline=bt_sequence_get_timeline_by_time(sequence,i);
    gtk_list_store_append(store, &tree_iter);
    // set position, highlight-color
    gtk_list_store_set(store,&tree_iter,
			SEQUENCE_TABLE_POS,pos,
			SEQUENCE_TABLE_TICK_FG_SET,FALSE,
			-1);
		pos++;
    // set label
    g_object_get(G_OBJECT(timeline),"label",&str,NULL);
    if(str) {
      gtk_list_store_set(store,&tree_iter,SEQUENCE_TABLE_LABEL,str,-1);
      g_free(str);
    }
    // set patterns
    for(j=0;j<track_ct;j++) {
      timelinetrack=bt_timeline_get_timelinetrack_by_index(timeline,j);
      g_object_get(G_OBJECT(timelinetrack),"type",&pattern_type,NULL);
      free_str=FALSE;
      switch(pattern_type) {
        case BT_TIMELINETRACK_TYPE_EMPTY:
          str=" ";
          break;
        case BT_TIMELINETRACK_TYPE_PATTERN:
          g_object_get(G_OBJECT(timelinetrack),"pattern",&pattern,NULL);
          g_object_get(G_OBJECT(pattern),"name",&str,NULL);
          g_object_try_unref(pattern);
          free_str=TRUE;
          break;
        case BT_TIMELINETRACK_TYPE_MUTE:
          str="---";
          break;
        case BT_TIMELINETRACK_TYPE_STOP:
          str="===";
          break;
        default:
          str="???";
          GST_ERROR("implement me");
      }
      //GST_INFO("  %2d,%2d : adding \"%s\"",i,j,str);
      gtk_list_store_set(store,&tree_iter,SEQUENCE_TABLE_PRE_CT+j,str,-1);
      if(free_str) g_free(str);
      g_object_unref(timelinetrack);
    }
    g_object_unref(timeline);
  }
	// create a filterd model to realize step filtering
	filtered_store=gtk_tree_model_filter_new(GTK_TREE_MODEL(store),NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filtered_store),step_visible_filter,(gpointer)self,NULL);
	GST_INFO("  filter = %p",filtered_store);
	// should we use the filtered_store here?
  //gtk_tree_view_set_model(self->priv->sequence_table,GTK_TREE_MODEL(store));
	gtk_tree_view_set_model(self->priv->sequence_table,filtered_store);

  // build view
	GST_DEBUG("  build view");
	// add initial columns
  sequence_table_init(self);
  // add column for each machine
  for(j=0;j<track_ct;j++) {
    machine=bt_sequence_get_machine_by_track(sequence,j);
    renderer=gtk_cell_renderer_text_new();
    //g_object_set(G_OBJECT(renderer),"editable",TRUE,NULL);

    // set machine name as column header
		if(machine) {
    	g_object_get(G_OBJECT(machine),"id",&str,NULL);
			label=gtk_label_new(str);
			g_free(str);
		}
		else {
			label=gtk_label_new("???");
			GST_WARNING("can't get machine for column %d",j);
		}
		// @todo here we can add hbox that containts Mute, Solo, Bypass buttons as well
		// or popup button that shows the whole context menu like that in the machine_view
		gtk_widget_show(label);
    col_index=gtk_tree_view_insert_column_with_attributes(self->priv->sequence_table,-1,NULL,renderer,
      "text",SEQUENCE_TABLE_PRE_CT+j,
      NULL);
    
    if((tree_col=gtk_tree_view_get_column(self->priv->sequence_table,col_index-1))) {
			gtk_tree_view_column_set_widget(tree_col,label);
			g_signal_connect(G_OBJECT(machine),"notify::id",(GCallback)on_machine_id_changed,(gpointer)label);
		
			// color code columns
	    if(BT_IS_SOURCE_MACHINE(machine)) {
  	    gtk_tree_view_column_add_attribute(tree_col,renderer,"background-gdk",SEQUENCE_TABLE_SOURCE_BG);
    	}
    	else if(BT_IS_PROCESSOR_MACHINE(machine)) {
      	gtk_tree_view_column_add_attribute(tree_col,renderer,"background-gdk",SEQUENCE_TABLE_PROCESSOR_BG);
    	}
    	else if(BT_IS_SINK_MACHINE(machine)) {
      	gtk_tree_view_column_add_attribute(tree_col,renderer,"background-gdk",SEQUENCE_TABLE_SINK_BG);
    	}
		}
		else GST_WARNING("can't get treeview column");
    g_object_unref(machine);
  }
  GST_INFO("    number of columns : %d",col_index);

  // release the references
  g_object_try_unref(sequence);
  g_object_try_unref(setup);    
	//g_object_unref(store); // drop with treeview
  g_object_unref(filtered_store); // drop with treeview
}

/**
 * pattern_list_refresh:
 * @self: the sequence page
 *
 * When the user moves the cursor in the sequence, update the list of patterns
 * so that it shows the patterns that belong to the machine in the current
 * sequence row.
 */
static void pattern_list_refresh(const BtMainPageSequence *self) {
  BtPattern *pattern;
  BtMachine *machine;
  GtkListStore *store;
  GtkTreeIter tree_iter;
  GList *node,*list;
  gchar *str,key[2]={0,};
	gulong index=0;

  GST_INFO("refresh pattern list");
  store=gtk_list_store_new(2,G_TYPE_STRING,G_TYPE_STRING);

  //-- append default rows
  gtk_list_store_append(store, &tree_iter);
  gtk_list_store_set(store,&tree_iter,0,"-",1,_("  mute"),-1);
  gtk_list_store_append(store, &tree_iter);
  gtk_list_store_set(store,&tree_iter,0,",",1,_("  break"),-1);
  if((machine=bt_main_page_sequence_get_current_machine(self))) {
    //-- append pattern rows
		g_object_get(G_OBJECT(machine),"patterns",&list,NULL);
		for(node=list;node;node=g_list_next(node)) {
      pattern=BT_PATTERN(node->data);
      g_object_get(G_OBJECT(pattern),"name",&str,NULL);
			GST_INFO("  adding \"%s\" at index %d -> '%c'",str,index,pattern_keys[index]);
			key[0]=(index<64)?pattern_keys[index]:' ';
			//if(index<64) key[0]=pattern_keys[index];
			//else key[0]=' ';
      GST_INFO("  with shortcut \"%s\"",key);
      gtk_list_store_append(store, &tree_iter);
      gtk_list_store_set(store,&tree_iter,0,key,1,str,-1);
      g_free(str);
			index++;
    }
		g_list_free(list);
    g_object_unref(machine);
  }
  gtk_tree_view_set_model(self->priv->pattern_list,GTK_TREE_MODEL(store));
  g_object_unref(store); // drop with treeview
}

//-- event handler

static void on_sequence_tick(const BtSequence *sequence, glong pos, gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
	GtkTreeModel *store;
	GtkTreeModelFilter *filtered_store;
	GtkTreeIter iter;
  
  g_assert(user_data);

	// reset old tick pos
	if(!pos) self->priv->tick_pos=-1;
	// do nothing for invisible rows
	if(!IS_SEQUENCE_POS_VISIBLE(pos,self->priv->bars)) return;

  //GST_INFO("sequence tick received : %d",pos);
	if((filtered_store=GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(self->priv->sequence_table)))
		&& (store=gtk_tree_model_filter_get_model(filtered_store)))
	{
	  // update sequence table highlight
		gdk_threads_enter();
		// set color for new pos
		if(sequence_model_get_iter_by_position(store,&iter,pos)) {
			gtk_list_store_set(GTK_LIST_STORE(store),&iter,SEQUENCE_TABLE_TICK_FG_SET,TRUE,-1);
		}
		// unset color for old pos
		if(self->priv->tick_pos!=-1) {
			if(sequence_model_get_iter_by_position(store,&iter,self->priv->tick_pos)) {
				gtk_list_store_set(GTK_LIST_STORE(store),&iter,SEQUENCE_TABLE_TICK_FG_SET,FALSE,-1);
			}
		}
		self->priv->tick_pos=pos;
		gdk_threads_leave();
	}
	else {
		GST_WARNING("can't get tree model");
	}
}

static void on_bars_menu_changed(GtkComboBox *combo_box,gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
	GtkTreeModel *store;
	GtkTreeIter iter;

  g_assert(user_data);

  GST_INFO("bars_menu has changed : page=%p",user_data);
	
	if((store=gtk_combo_box_get_model(self->priv->bars_menu))
		&& gtk_combo_box_get_active_iter(self->priv->bars_menu,&iter))
	{
		gchar *str;
		GtkTreeModelFilter *filtered_store;

		gtk_tree_model_get(store,&iter,0,&str,-1);
		self->priv->bars=atoi(str);
		g_free(str);
		sequence_model_recolorize(self);
		//GST_INFO("  bars = %d",self->priv->bars);
		if((filtered_store=GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(self->priv->sequence_table)))) {
			gtk_tree_model_filter_refilter(filtered_store);
		}
	}
}

static void on_sequence_table_cursor_changed(GtkTreeView *treeview, gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);

  g_assert(user_data);

  GST_INFO("sequence_table cursor has changed : treeview=%p, page=%p",treeview,user_data);
  pattern_list_refresh(self);
}

static gboolean on_sequence_table_cursor_moved(GtkTreeView *treeview, GtkMovementStep arg1, gint arg2, gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  BtMachine *machine;

  g_assert(user_data);

  GST_INFO("sequence_table cursor has moved : treeview=%p, page=%p, arg1=%d, arg2=%d",treeview,user_data,arg1,arg2);
  if(arg1==GTK_MOVEMENT_VISUAL_POSITIONS) {
    pattern_list_refresh(self);
  }
}

static gboolean on_sequence_table_key_release_event(GtkWidget *widget,GdkEventKey *event,gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
	gboolean res=FALSE;
	BtTimeLineTrack *timelinetrack;
	gchar *str=NULL;
	gboolean free_str=FALSE;

  g_assert(user_data);
	
	GST_INFO("sequence_table key : state 0x%x, keyval 0x%x",event->state,event->keyval);
	// determine timeline and timelinetrack from cursor pos
	if((timelinetrack=bt_main_page_sequence_get_current_timelinetrack(self))) {
		// look up pattern for key
		if(event->keyval=='-') {
			g_object_set(timelinetrack,"type",BT_TIMELINETRACK_TYPE_MUTE,"pattern",NULL,NULL);
			str="---";
			res=TRUE;
		}
		else if(event->keyval==',') {
			g_object_set(timelinetrack,"type",BT_TIMELINETRACK_TYPE_STOP,"pattern",NULL,NULL);
      str="===";
			res=TRUE;
		}
		else if(event->keyval==' ') {
			g_object_set(timelinetrack,"type",BT_TIMELINETRACK_TYPE_EMPTY,"pattern",NULL,NULL);
			str=" ";
			res=TRUE;
		}
		else {
			gchar *pos=strchr(pattern_keys,event->keyval);
			
			if(pos) {
				BtMachine *machine;

				if((machine=bt_main_page_sequence_get_current_machine(self))) {
					BtPattern *pattern;
				
					if((pattern=bt_machine_get_pattern_by_index(machine,((gulong)pos-(gulong)pattern_keys)))) {
						g_object_set(timelinetrack,"type",BT_TIMELINETRACK_TYPE_PATTERN,"pattern",pattern,NULL);
						g_object_get(G_OBJECT(pattern),"name",&str,NULL);
          	g_object_try_unref(pattern);
          	free_str=TRUE;
						res=TRUE;
					}
					g_object_unref(machine);
				}
			}
		}
		// update tree-view model
		if(res) {
			GtkTreeModelFilter *filtered_store;
			GtkTreeModel *store;
			GtkTreePath *path;
			GtkTreeViewColumn *column;
			GtkTreeIter iter,filter_iter;
			
			GST_INFO("  update model");

			if((filtered_store=GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(self->priv->sequence_table)))
				&& (store=gtk_tree_model_filter_get_model(filtered_store))
			)	{
				gtk_tree_view_get_cursor(self->priv->sequence_table,&path,&column);
				if(path && column && gtk_tree_model_get_iter(GTK_TREE_MODEL(filtered_store),&filter_iter,path)) {
			    GList *columns=gtk_tree_view_get_columns(self->priv->sequence_table);
		  	  glong row,track=g_list_index(columns,(gpointer)column)-2;
				
					g_list_free(columns);
					gtk_tree_model_filter_convert_iter_to_child_iter(filtered_store,&iter,&filter_iter);
					gtk_tree_model_get(store,&iter,SEQUENCE_TABLE_POS,&row,-1);
					GST_INFO("  position is %d,%d -> ",row,track,SEQUENCE_TABLE_PRE_CT+track);
					
					gtk_list_store_set(GTK_LIST_STORE(store),&iter,SEQUENCE_TABLE_PRE_CT+track,str,-1);
				}
				else {
					GST_WARNING("  can't evaluate cursor pos");
				}
				if(free_str) g_free(str);
			}
			else {
				GST_WARNING("  can't get tree-model");
			}
		}
		else {
			GST_WARNING("  nothing assgned to this key");
		}
	}
	else {
		GST_WARNING("  can't locate timelinetrack related to curos pos");
	}
	return(res);
}

static gboolean on_sequence_table_button_press_event(GtkWidget *widget,GdkEventButton *event,gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
	gboolean res=FALSE;

  g_assert(user_data);
	
	GST_INFO("sequence_table button : button 0x%x",event->button);
	if(event->button==3) {
		gtk_menu_popup(self->priv->context_menu,NULL,NULL,NULL,NULL,3,gtk_get_current_event_time());
		res=TRUE;
	}	
	return(res);
}

static void on_song_changed(const BtEditApplication *app,GParamSpec *arg,gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  BtSong *song;
  BtSongInfo *song_info;
	BtSequence *sequence;
  glong index,bars;

  g_assert(user_data);

  GST_INFO("song has changed : app=%p, self=%p",app,self);
  // get song from app and then setup from song
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"song-info",&song_info,"sequence",&sequence,NULL);
  // update page
  // update sequence and pattern list
  sequence_table_refresh(self,song);
  pattern_list_refresh(self);
  // update toolbar
  g_object_get(G_OBJECT(song_info),"bars",&bars,NULL);
  // find out to which entry it belongs and set the index
  // 1 -> 0, 2 -> 1, 4 -> 2 , 8 -> 3
  if(bars<4) {
    index=bars-1;
  }
  else {
    index=1+(bars>>2);
  }
  //GST_INFO("  bars=%d, index=%d",bars,index);
	if(gtk_combo_box_get_active(self->priv->bars_menu)!=index) {
  	gtk_combo_box_set_active(self->priv->bars_menu,index);
	}
	else {
		sequence_model_recolorize(self);
	}
	// connect to the tick signal
	g_signal_connect(G_OBJECT(sequence), "tick", (GCallback)on_sequence_tick, (gpointer)self);
  //-- release the references
  g_object_try_unref(song_info);
	g_object_try_unref(sequence);
  g_object_try_unref(song);
}

//-- helper methods

static gboolean bt_main_page_sequence_init_bars_menu(const BtMainPageSequence *self,gulong bars) {
	GtkListStore *store;
	GtkTreeIter iter;
	gchar str[4];
	gulong i;
	/* @todo the useful stepping depends on the rythm
	   4/4 -> 1,4,8,16,32
	   3/4 -> 1,3,6,12,24
	*/
	store=gtk_list_store_new(1,G_TYPE_STRING);
	
	gtk_list_store_append(store,&iter);
	gtk_list_store_set(store,&iter,0,"1",-1);
	gtk_list_store_append(store,&iter);
	gtk_list_store_set(store,&iter,0,"2",-1);
  for(i=4;i<=64;i+=4) {
    sprintf(str,"%d",i);
		gtk_list_store_append(store,&iter);
		gtk_list_store_set(store,&iter,0,str,-1);
  }
	gtk_combo_box_set_model(self->priv->bars_menu,GTK_TREE_MODEL(store));
  gtk_combo_box_set_active(self->priv->bars_menu,2);
	g_object_unref(store); // drop with combobox
}

static gboolean bt_main_page_sequence_init_ui(const BtMainPageSequence *self) {
  GtkWidget *toolbar;
  GtkWidget *box,*button,*scrolled_window;
  GtkWidget *menu_item;
  GtkCellRenderer *renderer;
  GdkColormap *colormap;

  // add toolbar
  toolbar=gtk_toolbar_new();
  gtk_widget_set_name(toolbar,_("machine view tool bar"));
  gtk_box_pack_start(GTK_BOX(self),toolbar,FALSE,FALSE,0);
  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar),GTK_TOOLBAR_BOTH);
  // add toolbar widgets
  // steps
  box=gtk_hbox_new(FALSE,2);
  gtk_container_set_border_width(GTK_CONTAINER(box),4);
  // build the menu
	self->priv->bars_menu=GTK_COMBO_BOX(gtk_combo_box_new());
	bt_main_page_sequence_init_bars_menu(self,4);
	renderer=gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(self->priv->bars_menu),renderer,TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(self->priv->bars_menu),renderer,"text", 0,NULL);
	g_signal_connect(G_OBJECT(self->priv->bars_menu),"changed",(GCallback)on_bars_menu_changed, (gpointer)self);
  gtk_box_pack_start(GTK_BOX(box),gtk_label_new(_("Steps")),FALSE,FALSE,2);
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->priv->bars_menu),TRUE,TRUE,2);
  
  button=gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
                                GTK_TOOLBAR_CHILD_WIDGET,
                                box,
                                NULL,
                                NULL,NULL,
                                NULL,NULL,NULL);
  //gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("Steps"));

  // allocate our colors
  colormap=gdk_colormap_get_system();
  self->priv->source_bg1.red=  (guint16)(1.0*65535);
  self->priv->source_bg1.green=(guint16)(0.9*65535);
  self->priv->source_bg1.blue= (guint16)(0.9*65535);
  gdk_colormap_alloc_color(colormap,&self->priv->source_bg1,FALSE,TRUE);
  self->priv->source_bg2.red=  (guint16)(1.0*65535);
  self->priv->source_bg2.green=(guint16)(0.8*65535);
  self->priv->source_bg2.blue= (guint16)(0.8*65535);
  gdk_colormap_alloc_color(colormap,&self->priv->source_bg2,FALSE,TRUE);
  self->priv->processor_bg1.red=  (guint16)(0.9*65535);
  self->priv->processor_bg1.green=(guint16)(1.0*65535);
  self->priv->processor_bg1.blue= (guint16)(0.9*65535);
  gdk_colormap_alloc_color(colormap,&self->priv->processor_bg1,FALSE,TRUE);
  self->priv->processor_bg2.red=  (guint16)(0.8*65535);
  self->priv->processor_bg2.green=(guint16)(1.0*65535);
  self->priv->processor_bg2.blue= (guint16)(0.8*65535);
  gdk_colormap_alloc_color(colormap,&self->priv->processor_bg2,FALSE,TRUE);
  self->priv->sink_bg1.red=  (guint16)(0.9*65535);
  self->priv->sink_bg1.green=(guint16)(0.9*65535);
  self->priv->sink_bg1.blue= (guint16)(1.0*65535);
  gdk_colormap_alloc_color(colormap,&self->priv->sink_bg1,FALSE,TRUE);
  self->priv->sink_bg2.red=  (guint16)(0.8*65535);
  self->priv->sink_bg2.green=(guint16)(0.8*65535);
  self->priv->sink_bg2.blue= (guint16)(1.0*65535);
  gdk_colormap_alloc_color(colormap,&self->priv->sink_bg2,FALSE,TRUE);

  // generate the context menu  
  self->priv->context_menu=GTK_MENU(gtk_menu_new());

	// @todo add more items (Machine for Add Track) and disable Remove Track when tracks=0
  menu_item=gtk_menu_item_new_with_label(_("Add Track"));
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
  menu_item=gtk_menu_item_new_with_label(_("Remove Track"));
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);

  // add a hpaned
  box=gtk_hpaned_new();
  gtk_container_add(GTK_CONTAINER(self),box);
  // add sequence list-view
  scrolled_window=gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_ETCHED_IN);
  self->priv->sequence_table=GTK_TREE_VIEW(gtk_tree_view_new());
	g_object_set(self->priv->sequence_table,"enable-search",FALSE,"rules-hint",TRUE,NULL);
  //gtk_tree_view_set_rules_hint(self->priv->sequence_table,TRUE);
	// GTK_SELECTION_BROWSE unfortunately selects whole rows, we rather need something that just outlines current row and column 
  //gtk_tree_selection_set_mode(gtk_tree_view_get_selection(self->priv->sequence_table),GTK_SELECTION_BROWSE);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(self->priv->sequence_table),GTK_SELECTION_NONE);
  sequence_table_init(self);
  gtk_container_add(GTK_CONTAINER(scrolled_window),GTK_WIDGET(self->priv->sequence_table));
  gtk_paned_pack1(GTK_PANED(box),GTK_WIDGET(scrolled_window),TRUE,TRUE);
  // add pattern list-view
  scrolled_window=gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_ETCHED_IN);
  self->priv->pattern_list=GTK_TREE_VIEW(gtk_tree_view_new());
  renderer=gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer),"xalign",1.0,NULL);
  gtk_tree_view_insert_column_with_attributes(self->priv->pattern_list,-1,_("Key"),renderer,"text",0,NULL);
  renderer=gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(self->priv->pattern_list,-1,_("Patterns"),renderer,"text",1,NULL);
  gtk_container_add(GTK_CONTAINER(scrolled_window),GTK_WIDGET(self->priv->pattern_list));
  gtk_paned_pack2(GTK_PANED(box),GTK_WIDGET(scrolled_window),FALSE,FALSE);

  // register event handlers
  g_signal_connect(G_OBJECT(self->priv->sequence_table), "move-cursor", (GCallback)on_sequence_table_cursor_moved, (gpointer)self);
  g_signal_connect(G_OBJECT(self->priv->sequence_table), "cursor-changed", (GCallback)on_sequence_table_cursor_changed, (gpointer)self);
	g_signal_connect(G_OBJECT(self->priv->sequence_table), "key-release-event", (GCallback)on_sequence_table_key_release_event, (gpointer)self);
	g_signal_connect(G_OBJECT(self->priv->sequence_table), "button-press-event", (GCallback)on_sequence_table_button_press_event, (gpointer)self);
  g_signal_connect(G_OBJECT(self->priv->app), "notify::song", (GCallback)on_song_changed, (gpointer)self);
  return(TRUE);
}

//-- constructor methods

/**
 * bt_main_page_sequence_new:
 * @app: the application the window belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtMainPageSequence *bt_main_page_sequence_new(const BtEditApplication *app) {
  BtMainPageSequence *self;

  if(!(self=BT_MAIN_PAGE_SEQUENCE(g_object_new(BT_TYPE_MAIN_PAGE_SEQUENCE,"app",app,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_main_page_sequence_init_ui(self)) {
    goto Error;
  }
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods

/**
 * bt_main_page_sequence_get_current_machine:
 * @self: the pattern subpage
 *
 * Get the currently active #BtMachine as determined by the cursor position in
 * the sequence table.
 * Unref the machine, when done with it.
 *
 * Returns: the #BtMachine instance or %NULL in case of an error
 */
BtMachine *bt_main_page_sequence_get_current_machine(const BtMainPageSequence *self) {
  BtMachine *machine=NULL;
  BtSong *song;
  BtSequence *sequence;
  GtkTreePath *path;
  GtkTreeViewColumn *column;

  GST_INFO("get active machine");
  
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"sequence",&sequence,NULL);

  // get table column number (column 0 is for labels)
  gtk_tree_view_get_cursor(self->priv->sequence_table,&path,&column);
  if(column) {
    GList *columns=gtk_tree_view_get_columns(self->priv->sequence_table);
    glong track=g_list_index(columns,(gpointer)column);

    g_list_free(columns);
    GST_INFO("  active track is %d",track);
    if(track>1) { // first two tracks are pos and label
      machine=bt_sequence_get_machine_by_track(sequence,track-2);
    }
  }
  // release the references
  g_object_try_unref(sequence);
  g_object_try_unref(song);
  return(machine);
}

/**
 * bt_main_page_sequence_get_current_timelinetrack:
 * @self: the pattern subpage
 *
 * Get the currently active #BtTimeLineTrack as determined by the cursor position in
 * the sequence table.
 * Unref the timelinetrack, when done with it.
 *
 * Returns: the #BtTimeLineTrack instance or %NULL in case of an error
 */
BtTimeLineTrack *bt_main_page_sequence_get_current_timelinetrack(const BtMainPageSequence *self) {
	BtTimeLineTrack *timelinetrack=NULL;
  BtSong *song;
  BtSequence *sequence;
	BtTimeLine *timeline;
  GtkTreePath *path;
  GtkTreeViewColumn *column;

  GST_INFO("get active sequence cell");

  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"sequence",&sequence,NULL);

  // get table column number (column 0 is for labels)
  gtk_tree_view_get_cursor(self->priv->sequence_table,&path,&column);
  if(column && path) {
    GList *columns=gtk_tree_view_get_columns(self->priv->sequence_table);
    glong row,track=g_list_index(columns,(gpointer)column);
		GtkTreeModel *store;
		GtkTreeIter iter;

    g_list_free(columns);
		if(track>1) { // first two tracks are pos and label
			// get iter from path
			store=gtk_tree_view_get_model(self->priv->sequence_table);
			if(gtk_tree_model_get_iter(store,&iter,path)) {
				gulong row;
				// get pos from iter and then the timeline
				gtk_tree_model_get(store,&iter,SEQUENCE_TABLE_POS,&row,-1);
				if(timeline=bt_sequence_get_timeline_by_time(sequence,row)) {
					timelinetrack=bt_timeline_get_timelinetrack_by_index(timeline,track-2);
					GST_INFO("  found one at %d,%d",track-2,row);
					g_object_unref(timeline);
				}
			}
		}
  }
  // release the references
  g_object_try_unref(sequence);
  g_object_try_unref(song);	
	return(timelinetrack);
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_main_page_sequence_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_PAGE_SEQUENCE_APP: {
      g_value_set_object(value, self->priv->app);
    } break;
    default: {
 			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_main_page_sequence_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE(object);
  return_if_disposed();
  switch (property_id) {
    case MAIN_PAGE_SEQUENCE_APP: {
      g_object_try_weak_unref(self->priv->app);
      self->priv->app = BT_EDIT_APPLICATION(g_value_get_object(value));
			g_object_try_weak_ref(self->priv->app);
      //GST_DEBUG("set the app for MAIN_PAGE_SEQUENCE: %p",self->priv->app);
    } break;
    default: {
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_main_page_sequence_dispose(GObject *object) {
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE(object);
	return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  g_object_try_weak_unref(self->priv->app);
	
	gtk_object_destroy(GTK_OBJECT(self->priv->context_menu));

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_main_page_sequence_finalize(GObject *object) {
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE(object);
  
  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_main_page_sequence_init(GTypeInstance *instance, gpointer g_class) {
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE(instance);
  self->priv = g_new0(BtMainPageSequencePrivate,1);
  self->priv->dispose_has_run = FALSE;
	self->priv->bars=1;
}

static void bt_main_page_sequence_class_init(BtMainPageSequenceClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  
  parent_class=g_type_class_ref(GTK_TYPE_VBOX);

  gobject_class->set_property = bt_main_page_sequence_set_property;
  gobject_class->get_property = bt_main_page_sequence_get_property;
  gobject_class->dispose      = bt_main_page_sequence_dispose;
  gobject_class->finalize     = bt_main_page_sequence_finalize;

  g_object_class_install_property(gobject_class,MAIN_PAGE_SEQUENCE_APP,
                                  g_param_spec_object("app",
                                     "app contruct prop",
                                     "Set application object, the window belongs to",
                                     BT_TYPE_EDIT_APPLICATION, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));
}

GType bt_main_page_sequence_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtMainPageSequenceClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_main_page_sequence_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtMainPageSequence),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_main_page_sequence_init, // instance_init
			NULL // value_table
    };
		type = g_type_register_static(GTK_TYPE_VBOX,"BtMainPageSequence",&info,0);
  }
  return type;
}
