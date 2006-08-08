// $Id: main-page-sequence.c,v 1.124 2006-08-08 19:46:14 ensonic Exp $
/**
 * SECTION:btmainpagesequence
 * @short_description: the editor main sequence page
 * @see_also: #BtSequenceView
 */ 

/* @todo main-page-sequence tasks
 *  - cut/copy/paste
 *  - focus sequence_view after entering the page ?
 *  - sequence header
 *    - add table to separate scrollable window
 *      (no own adjustments, share x-adjustment with sequence-view, show full height)
 *      - add level meters
 *      - add the same context menu as the machines have in machine view
 *    - sequence view will have no visible column headers
 *  - support different rhythms
 *    - use different steps in the bars menu (e.g. 1,2,3,6,9,12,...)
 *    - use different highlighing (strong bar every start of a beat)
 *  - insert/remove rows
 *
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
  union {
    BtEditApplication *app;
    gpointer app_ptr;
  };
  
  /* bars selection menu */
  GtkComboBox *bars_menu;
  gulong bars;
  
  /* the sequence table */
  GtkTreeView *sequence_pos_table;
  GtkTreeView *sequence_table;
  /* the pattern list */
  GtkTreeView *pattern_list;
  
  /* position-table header label widget */
  GtkWidget *pos_header;

  /* sequence context_menu */
  GtkMenu *context_menu;
  GtkMenuItem *context_menu_add;

  /* colors */
  GdkColor *cursor_bg;
  GdkColor *selection_bg1,*selection_bg2;
  GdkColor *source_bg1,*source_bg2;
  GdkColor *processor_bg1,*processor_bg2;
  GdkColor *sink_bg1,*sink_bg2;
  
  /* some internal states */
  glong tick_pos;
  /* cursor */
  glong cursor_column;
  glong cursor_row;
  /* selection range */
  glong selection_start_column;
  glong selection_start_row;
  glong selection_end_column;
  glong selection_end_row;
  /* selection first cell */
  glong selection_column;
  glong selection_row;
  BtMachine *machine;
  
  /* signal handler id's */
  gulong pattern_added_handler, pattern_removed_handler;
};

static GtkVBoxClass *parent_class=NULL;

/* internal data model fields */
enum {
  SEQUENCE_TABLE_SOURCE_BG=0,
  SEQUENCE_TABLE_PROCESSOR_BG,
  SEQUENCE_TABLE_SINK_BG,
  SEQUENCE_TABLE_CURSOR_BG,
  SEQUENCE_TABLE_SELECTION_BG,
  SEQUENCE_TABLE_TICK_FG_SET,
  SEQUENCE_TABLE_POS,
  SEQUENCE_TABLE_LABEL,
  SEQUENCE_TABLE_PRE_CT
};

#define IS_SEQUENCE_POS_VISIBLE(pos,bars) ((pos&((bars)-1))==0)
#define SEQUENCE_CELL_WIDTH 100
#define SEQUENCE_CELL_HEIGHT 28
#define SEQUENCE_CELL_XPAD 0
#define SEQUENCE_CELL_YPAD 0
// when setting the HEIGHT for one column, then the focus rect is visible for
// the other (smaller) columns

static gchar sink_pattern_keys[]     = "-,0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static gchar source_pattern_keys[]   ="-,_0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static gchar processor_pattern_keys[]="-,_0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static gchar *pattern_keys;

static GQuark column_index_quark=0;

static void on_track_add_activated(GtkMenuItem *menuitem, gpointer user_data);
static void on_pattern_changed(BtMachine *machine,BtPattern *pattern,gpointer user_data);

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

//-- tree cell data functions

static void source_machine_cell_data_function(GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  gulong row,column;
  GdkColor *bg_col;
  gchar *str=NULL;

  column=1+GPOINTER_TO_UINT(g_object_get_qdata(G_OBJECT(col),column_index_quark));

  gtk_tree_model_get(model,iter,
    SEQUENCE_TABLE_POS,&row,
    SEQUENCE_TABLE_SOURCE_BG,&bg_col,
    SEQUENCE_TABLE_LABEL+column,&str,
    -1);

  //GST_INFO("col/row: %3d/%3d <-> %3d/%3d",column,row,self->priv->cursor_column,self->priv->cursor_row);
  //GST_INFO("bg_col %x <-> source_bg1,2 %x, %x",bg_col->pixel,self->priv->source_bg1.pixel,self->priv->source_bg2.pixel);
  
  if((column==self->priv->cursor_column) && (row==self->priv->cursor_row)) {
    bg_col=self->priv->cursor_bg;
  }
  else if((column>=self->priv->selection_start_column) && (column<=self->priv->selection_end_column) &&
    (row>=self->priv->selection_start_row) && (row<=self->priv->selection_end_row)
  ) {
    bg_col=(bg_col->pixel==self->priv->source_bg1->pixel)?self->priv->selection_bg1:self->priv->selection_bg2;
  }
  g_object_set(G_OBJECT(renderer),
    "background-gdk",bg_col,
    "text",str,
     NULL);
  g_free(str);
}

static void processor_machine_cell_data_function(GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  gulong row,column;
  GdkColor *bg_col;
  gchar *str=NULL;

  column=1+GPOINTER_TO_UINT(g_object_get_qdata(G_OBJECT(col),column_index_quark));

  gtk_tree_model_get(model,iter,
    SEQUENCE_TABLE_POS,&row,
    SEQUENCE_TABLE_PROCESSOR_BG,&bg_col,
    SEQUENCE_TABLE_LABEL+column,&str,
    -1);
  
  if((column==self->priv->cursor_column) && (row==self->priv->cursor_row)) {
    bg_col=self->priv->cursor_bg;
  }
  else if((column>=self->priv->selection_start_column) && (column<=self->priv->selection_end_column) &&
    (row>=self->priv->selection_start_row) && (row<=self->priv->selection_end_row)
  ) {
    bg_col=(bg_col->pixel==self->priv->processor_bg1->pixel)?self->priv->selection_bg1:self->priv->selection_bg2;
  }
  g_object_set(G_OBJECT(renderer),
    "background-gdk",bg_col,
    "text",str,
     NULL);
  g_free(str);
}

static void sink_machine_cell_data_function(GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  gulong row,column;
  GdkColor *bg_col;
  gchar *str=NULL;

  column=1+GPOINTER_TO_UINT(g_object_get_qdata(G_OBJECT(col),column_index_quark));

  gtk_tree_model_get(model,iter,
    SEQUENCE_TABLE_POS,&row,
    SEQUENCE_TABLE_SINK_BG,&bg_col,
    SEQUENCE_TABLE_LABEL+column,&str,
    -1);
 
  
  if((column==self->priv->cursor_column) && (row==self->priv->cursor_row)) {
    bg_col=self->priv->cursor_bg;
  }
  else if((column>=self->priv->selection_start_column) && (column<=self->priv->selection_end_column) &&
    (row>=self->priv->selection_start_row) && (row<=self->priv->selection_end_row)
  ) {
    bg_col=(bg_col->pixel==self->priv->sink_bg1->pixel)?self->priv->selection_bg1:self->priv->selection_bg2;
  }
  g_object_set(G_OBJECT(renderer),
    "background-gdk",bg_col,
    "text",str,
     NULL);
  g_free(str);
}

//-- tree model helper

static glong sequence_view_get_cursor_column(GtkTreeView *tree_view) {
  glong res=-1;
  GtkTreeViewColumn *column;

  // get table column number (column 0 is for positions and colum 1 for labels)
  gtk_tree_view_get_cursor(tree_view,NULL,&column);
  if(column) {
    GList *columns=gtk_tree_view_get_columns(tree_view);
    res=g_list_index(columns,(gpointer)column);
    g_list_free(columns);
    GST_DEBUG("  -> cursor column is %d",res);
  }
  return(res);
}

static gboolean sequence_view_get_cursor_pos(GtkTreeView *tree_view,GtkTreePath *path,GtkTreeViewColumn *column,gulong *col,gulong *row) {
  gboolean res=FALSE;
  GtkTreeModel *store;
  GtkTreeModelFilter *filtered_store;
  GtkTreeIter iter,filter_iter;
  
  if((filtered_store=GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(tree_view)))
    && (store=gtk_tree_model_filter_get_model(filtered_store))
  )  {
    if(gtk_tree_model_get_iter(GTK_TREE_MODEL(filtered_store),&filter_iter,path)) {
      if(col) {
        GList *columns=gtk_tree_view_get_columns(tree_view);
        *col=g_list_index(columns,(gpointer)column);
        g_list_free(columns);
      }
      if(row) {
        gtk_tree_model_filter_convert_iter_to_child_iter(filtered_store,&iter,&filter_iter);
        gtk_tree_model_get(store,&iter,SEQUENCE_TABLE_POS,row,-1);
      }
      res=TRUE;
    }
  }
  else {
    GST_WARNING("  can't get tree-model");
  }
  return(res);
}

/*
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
*/

static GtkTreeModel *sequence_model_get_store(const BtMainPageSequence *self) {
  GtkTreeModel *store=NULL;
  GtkTreeModelFilter *filtered_store;

  if((filtered_store=GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(self->priv->sequence_table)))) {
    store=gtk_tree_model_filter_get_model(filtered_store);
  }
  return(store);  
}

/*
 * sequence_model_recolorize:
 * alternate coloring for visible rows
 */
static void sequence_model_recolorize(BtMainPageSequence *self) {
  GtkTreeModel *store;
  GtkTreeIter iter;
  gboolean odd_row=FALSE;
  gulong rows=0;

  GST_INFO("recolorize sequence tree view");
  
  if((store=sequence_model_get_store(self))) {
    if(gtk_tree_model_get_iter_first(store,&iter)) {
      do {
        if(step_visible_filter(store,&iter,self)) {
          if(odd_row) {
            gtk_list_store_set(GTK_LIST_STORE(store),&iter,
              SEQUENCE_TABLE_SOURCE_BG   ,self->priv->source_bg2,
              SEQUENCE_TABLE_PROCESSOR_BG,self->priv->processor_bg2,
              SEQUENCE_TABLE_SINK_BG     ,self->priv->sink_bg2,
              -1);
          }
          else {
            gtk_list_store_set(GTK_LIST_STORE(store),&iter,
              SEQUENCE_TABLE_SOURCE_BG   ,self->priv->source_bg1,
              SEQUENCE_TABLE_PROCESSOR_BG,self->priv->processor_bg1,
              SEQUENCE_TABLE_SINK_BG     ,self->priv->sink_bg1,
              -1);
          }
          odd_row=!odd_row;
          rows++;
        }
      } while(gtk_tree_model_iter_next(store,&iter));
      g_object_set(self->priv->sequence_table,"visible-rows",rows,NULL);
      g_object_set(self->priv->sequence_pos_table,"visible-rows",rows,NULL);
    }
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

/*
 * on_header_size_allocate:
 *
 * Adjusts the height of the header widget of the first treeview (pos) to the
 * height of the second treeview.
 */
static void on_header_size_allocate(GtkWidget *widget,GtkAllocation *allocation,gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);

  GST_INFO("#### header label size %d x %d",
    allocation->width,allocation->height);
  
#ifdef HAVE_GTK_TREE_VIEW_COLUMN_PATCH
  gtk_widget_set_size_request(self->priv->pos_header,-1,(allocation->height-8));
#else
  gtk_widget_set_size_request(self->priv->pos_header,-1,allocation->height);
#endif
}

static void on_mute_toggled(GtkToggleButton *togglebutton,gpointer user_data) {
  BtMachine *machine=BT_MACHINE(user_data);

  if(gtk_toggle_button_get_active(togglebutton)) {
    g_object_set(machine,"state",BT_MACHINE_STATE_MUTE,NULL);
  }
  else {
    g_object_set(machine,"state",BT_MACHINE_STATE_NORMAL,NULL);
  }
}

static void on_solo_toggled(GtkToggleButton *togglebutton,gpointer user_data) {
  BtMachine *machine=BT_MACHINE(user_data);

  if(gtk_toggle_button_get_active(togglebutton)) {
    g_object_set(machine,"state",BT_MACHINE_STATE_SOLO,NULL);
  }
  else {
    g_object_set(machine,"state",BT_MACHINE_STATE_NORMAL,NULL);
  }
}

static void on_bypass_toggled(GtkToggleButton *togglebutton,gpointer user_data) {
  BtMachine *machine=BT_MACHINE(user_data);

  if(gtk_toggle_button_get_active(togglebutton)) {
    g_object_set(machine,"state",BT_MACHINE_STATE_BYPASS,NULL);
  }
  else {
    g_object_set(machine,"state",BT_MACHINE_STATE_NORMAL,NULL);
  }
}

static void on_machine_state_changed_mute(BtMachine *machine,GParamSpec *arg,gpointer user_data) {
  GtkToggleButton *button=GTK_TOGGLE_BUTTON(user_data);
  BtMachineState state;
  
  g_object_get(machine,"state",&state,NULL);
  gtk_toggle_button_set_active(button,(state==BT_MACHINE_STATE_MUTE));
}

static void on_machine_state_changed_solo(BtMachine *machine,GParamSpec *arg,gpointer user_data) {
  GtkToggleButton *button=GTK_TOGGLE_BUTTON(user_data);
  BtMachineState state;
  
  g_object_get(machine,"state",&state,NULL);
  gtk_toggle_button_set_active(button,(state==BT_MACHINE_STATE_SOLO));
}

static void on_machine_state_changed_bypass(BtMachine *machine,GParamSpec *arg,gpointer user_data) {
  GtkToggleButton *button=GTK_TOGGLE_BUTTON(user_data);
  BtMachineState state;
  
  g_object_get(machine,"state",&state,NULL);
  gtk_toggle_button_set_active(button,(state==BT_MACHINE_STATE_BYPASS));
}

//-- event handler helper

/*
 * sequence_pos_table_init:
 * @self: the sequence page
 *
 * inserts the 'Pos.' column into the first (left) treeview
 */
static void sequence_pos_table_init(const BtMainPageSequence *self) {
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *tree_col;
  gint col_index=0;
  
  // add static column
  self->priv->pos_header=gtk_label_new(_("Pos."));
  gtk_widget_show_all(self->priv->pos_header);
  
  renderer=gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer),
    "mode",GTK_CELL_RENDERER_MODE_INERT,
    "xalign",1.0,
    "yalign",0.5,
    "foreground","blue",
    NULL);
  gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer),1);
  if((tree_col=gtk_tree_view_column_new_with_attributes(NULL,renderer,
    "text",SEQUENCE_TABLE_POS,
    "foreground-set",SEQUENCE_TABLE_TICK_FG_SET,
    NULL))
  ) {
    g_object_set(tree_col,
      "widget",self->priv->pos_header,
      "sizing",GTK_TREE_VIEW_COLUMN_FIXED,
      "fixed-width",40,
      NULL);
    col_index=gtk_tree_view_append_column(self->priv->sequence_pos_table,tree_col);
  }
  else GST_WARNING("can't create treeview column");

  GST_DEBUG("    number of columns : %d",col_index);
}

/*
 * sequence_table_clear:
 * @self: the sequence page
 *
 * removes old columns
 */
static void sequence_table_clear(const BtMainPageSequence *self) {
  GList *columns,*node;
  
  // remove columns
  if((columns=gtk_tree_view_get_columns(self->priv->sequence_table))) {
    for(node=g_list_first(columns);node;node=g_list_next(node)) {
      gtk_tree_view_remove_column(self->priv->sequence_table,GTK_TREE_VIEW_COLUMN(node->data));
    }
    g_list_free(columns);
  }
}

/*
 * sequence_table_init:
 * @self: the sequence page
 *
 * inserts the Label columns.
 */
static void sequence_table_init(const BtMainPageSequence *self, gboolean connect_header) {
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *tree_col;
  GtkWidget *label,*combo;
  GtkWidget *header;
  gint col_index=0;

  header=gtk_vbox_new(FALSE,0);

  label=gtk_label_new(_("Labels"));
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.0);
  //gtk_misc_set_padding(GTK_MISC(label),0,0);  
  gtk_box_pack_start(GTK_BOX(header),label,TRUE,FALSE,0);
  
  combo=gtk_combo_box_new() ;
  gtk_box_pack_start(GTK_BOX(header),combo,TRUE,FALSE,0);

  gtk_widget_show_all(header);

  // re-add static columns    
  renderer=gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer),
    "mode",GTK_CELL_RENDERER_MODE_ACTIVATABLE,
    "xalign",1.0,
    "yalign",0.5,
    "foreground","blue",
    /*
    "width",80-4,
    "height",SEQUENCE_CELL_HEIGHT-4,
    "xpad",SEQUENCE_CELL_XPAD,
    "ypad",SEQUENCE_CELL_YPAD,
    */
    NULL);
  gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer),1);
  if((tree_col=gtk_tree_view_column_new_with_attributes(_("Labels"),renderer,
    "text",SEQUENCE_TABLE_LABEL,
    "foreground-set",SEQUENCE_TABLE_TICK_FG_SET,
    NULL))
  ) {
    g_object_set(tree_col,
      "sizing",GTK_TREE_VIEW_COLUMN_FIXED,
      "fixed-width",80,
      NULL);
    col_index=gtk_tree_view_append_column(self->priv->sequence_table,tree_col);

    if(connect_header)
      g_signal_connect(G_OBJECT(header),"size-allocate",G_CALLBACK(on_header_size_allocate),(gpointer)self);

    gtk_tree_view_column_set_widget(tree_col,header);
  }
  else GST_WARNING("can't create treeview column");
  
  GST_DEBUG("    number of columns : %d",col_index);
}

/*
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
  BtPattern *pattern;
  GtkWidget *header;
  gchar *str;
  gulong i,j,col_ct,timeline_ct,track_ct,pos=0;
  gint col_index;
  GtkCellRenderer *renderer;
  GtkListStore *store;
  GtkTreeModel *filtered_store;
  GType *store_types;
  GtkTreeIter tree_iter;
  GtkTreeViewColumn *tree_col;
  gboolean free_str;
  gboolean is_internal;
  gboolean connect_handler;

  GST_INFO("refresh sequence table");
  
  g_object_get(G_OBJECT(song),"setup",&setup,"sequence",&sequence,NULL);
  g_object_get(G_OBJECT(sequence),"length",&timeline_ct,"tracks",&track_ct,NULL);
  GST_DEBUG("  size is %2d,%2d",timeline_ct,track_ct);
  
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
  store_types[SEQUENCE_TABLE_CURSOR_BG   ]=GDK_TYPE_COLOR;
  store_types[SEQUENCE_TABLE_SELECTION_BG]=GDK_TYPE_COLOR;
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
    gtk_list_store_append(store, &tree_iter);
    // set position, highlight-color
    gtk_list_store_set(store,&tree_iter,
      SEQUENCE_TABLE_POS,pos,
      SEQUENCE_TABLE_TICK_FG_SET,FALSE,
      -1);
    pos++;
    // set label
    if((str=bt_sequence_get_label(sequence,i))) {
      gtk_list_store_set(store,&tree_iter,SEQUENCE_TABLE_LABEL,str,-1);
      g_free(str);
    }
    // set patterns
    for(j=0;j<track_ct;j++) {
      free_str=FALSE;
      if((pattern=bt_sequence_get_pattern(sequence,i,j))) {
        g_object_get(pattern,"is-internal",&is_internal,NULL);
        if(!is_internal) {
          g_object_get(pattern,"name",&str,NULL);
          free_str=TRUE;
        }
        else {
          BtPatternCmd cmd=bt_pattern_get_cmd(pattern,0);
          switch(cmd) {
            case BT_PATTERN_CMD_BREAK:
              str="---";
              break;
            case BT_PATTERN_CMD_MUTE:
              str="===";
              break;
            case BT_PATTERN_CMD_SOLO:
              str="***";
              break;
            case BT_PATTERN_CMD_BYPASS:
              str="###";
              break;
            default:
              str="???";
              GST_ERROR("implement me");
          }
        }
        g_object_try_unref(pattern);        
      }
      else {
        str=" ";
      }
      //GST_DEBUG("  %2d,%2d : adding \"%s\"",i,j,str);
      gtk_list_store_set(store,&tree_iter,SEQUENCE_TABLE_PRE_CT+j,str,-1);
      if(free_str) g_free(str);
    }
  }
  // create a filterd model to realize step filtering
  filtered_store=gtk_tree_model_filter_new(GTK_TREE_MODEL(store),NULL);
  gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filtered_store),step_visible_filter,(gpointer)self,NULL);
  // active model
  gtk_tree_view_set_model(self->priv->sequence_table,filtered_store);
  gtk_tree_view_set_model(self->priv->sequence_pos_table,filtered_store);

  // build dynamic sequence view
  GST_DEBUG("  build view");

  // add initial columns
  // if no machines present, connect header size change handler to Labels column
  connect_handler=(track_ct==0)?TRUE:FALSE;
  sequence_table_init(self,connect_handler);

  // add column for each machine
  for(j=0;j<track_ct;j++) {
    machine=bt_sequence_get_machine(sequence,j);
    renderer=gtk_cell_renderer_text_new();
    //g_object_set(G_OBJECT(renderer),"editable",TRUE,NULL);
    g_object_set(G_OBJECT(renderer),
      "mode",GTK_CELL_RENDERER_MODE_ACTIVATABLE,
      "xalign",0.0,
      "yalign",0.5,
      /*
      "width",SEQUENCE_CELL_WIDTH-4,
      "height",SEQUENCE_CELL_HEIGHT-4,
      "xpad",SEQUENCE_CELL_XPAD,
      "ypad",SEQUENCE_CELL_YPAD,
      */
      NULL);
    gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer),1);

    // setup column header
    if(machine) {
      GtkWidget *label,*button,*box;
      // add hbox that contains the machine-name + Mute, Solo and Bypass buttons
      // @todo: add context menu like that in the machine_view
      // @todo: this is still to spacy (to much padding)
      // @todo: buttons don't work
      
      header=gtk_vbox_new(TRUE,0);
      
      g_object_get(G_OBJECT(machine),"id",&str,NULL);
      label=gtk_label_new(str);
      gtk_misc_set_alignment(GTK_MISC(label),0.0,0.0);
      //gtk_misc_set_padding(GTK_MISC(label),0,0);
      g_free(str);
      gtk_box_pack_start(GTK_BOX(header),label,TRUE,TRUE,0);
      
      box=gtk_hbox_new(TRUE,0);
      // add M/S/B butons and connect signal handlers
      button=gtk_toggle_button_new_with_label("M");
      gtk_container_set_border_width(GTK_CONTAINER(button),0);
      gtk_box_pack_start(GTK_BOX(box),button,FALSE,FALSE,0);
      g_signal_connect(G_OBJECT(button),"toggled",G_CALLBACK(on_mute_toggled),(gpointer)machine);
      g_signal_connect(G_OBJECT(machine),"notify::state", G_CALLBACK(on_machine_state_changed_mute), (gpointer)button);
      if(!BT_IS_SINK_MACHINE(machine)) {
        button=gtk_toggle_button_new_with_label("S");
        gtk_container_set_border_width(GTK_CONTAINER(button),0);
        gtk_box_pack_start(GTK_BOX(box),button,FALSE,FALSE,0);
        g_signal_connect(G_OBJECT(button),"toggled",G_CALLBACK(on_solo_toggled),(gpointer)machine);
        g_signal_connect(G_OBJECT(machine),"notify::state", G_CALLBACK(on_machine_state_changed_solo), (gpointer)button);
      }
      if(BT_IS_PROCESSOR_MACHINE(machine)) {
        button=gtk_toggle_button_new_with_label("B");
        gtk_container_set_border_width(GTK_CONTAINER(button),0);
        gtk_box_pack_start(GTK_BOX(box),button,FALSE,FALSE,0);
        g_signal_connect(G_OBJECT(button),"toggled",G_CALLBACK(on_bypass_toggled),(gpointer)machine);
        g_signal_connect(G_OBJECT(machine),"notify::state", G_CALLBACK(on_machine_state_changed_bypass), (gpointer)button);
      }
      // @todo: add level-meter instead and connect
      gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(gtk_label_new(" ")),TRUE,TRUE,0);
      
      gtk_box_pack_start(GTK_BOX(header),GTK_WIDGET(box),TRUE,TRUE,0);

      g_signal_connect(G_OBJECT(machine),"notify::id",G_CALLBACK(on_machine_id_changed),(gpointer)label);
      if(j==0) {
        // connect to the size-allocate signal to adjust the height of the other treeview header
        g_signal_connect(G_OBJECT(header),"size-allocate",G_CALLBACK(on_header_size_allocate),(gpointer)self);
      }
    }
    else {
      header=gtk_label_new("???");
      GST_WARNING("can't get machine for column %d",j);
    }
    gtk_widget_show_all(header);
    if((tree_col=gtk_tree_view_column_new_with_attributes(NULL,renderer,
    /*  "text",SEQUENCE_TABLE_PRE_CT+j,*/
      NULL))
    ) {
      g_object_set(tree_col,
        "widget",header,
#ifdef HAVE_GTK_TREE_VIEW_COLUMN_PATCH
        /* @todo: this requires patch in gtk+ */
        "wrap-header-widget",FALSE,
#endif
        "sizing",GTK_TREE_VIEW_COLUMN_FIXED,
        "fixed-width",SEQUENCE_CELL_WIDTH,
        NULL);
      g_object_set_qdata(G_OBJECT(tree_col),column_index_quark,GUINT_TO_POINTER(j));
      gtk_tree_view_append_column(self->priv->sequence_table,tree_col);
    
      // color code columns
      if(BT_IS_SOURCE_MACHINE(machine)) {
        gtk_tree_view_column_set_cell_data_func(tree_col, renderer, source_machine_cell_data_function, (gpointer)self, NULL);
      }
      else if(BT_IS_PROCESSOR_MACHINE(machine)) {
        gtk_tree_view_column_set_cell_data_func(tree_col, renderer, processor_machine_cell_data_function, (gpointer)self, NULL);
      }
      else if(BT_IS_SINK_MACHINE(machine)) {
        gtk_tree_view_column_set_cell_data_func(tree_col, renderer, sink_machine_cell_data_function, (gpointer)self, NULL);
      }
    }
    else GST_WARNING("can't create treeview column");
    g_object_try_unref(machine);
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
    col_index=gtk_tree_view_append_column(self->priv->sequence_table,tree_col);
    GST_DEBUG("    number of columns : %d",col_index);
  }
  else GST_WARNING("can't create treeview column");

  // release the references
  g_object_try_unref(sequence);
  g_object_try_unref(setup);    
  //g_object_unref(store); // drop with treeview
  g_object_unref(filtered_store); // drop with treeview
}


/*
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
  gboolean is_internal;

  GST_INFO("refresh pattern list");
  store=gtk_list_store_new(2,G_TYPE_STRING,G_TYPE_STRING);

  machine=bt_main_page_sequence_get_current_machine(self);
  if(machine!=self->priv->machine) {
    if(self->priv->machine) {
      GST_INFO("unref old cur-machine: refs: %d",(G_OBJECT(self->priv->machine))->ref_count);
      g_signal_handler_disconnect(G_OBJECT(self->priv->machine),self->priv->pattern_added_handler);
      g_signal_handler_disconnect(G_OBJECT(self->priv->machine),self->priv->pattern_removed_handler);
      g_object_unref(self->priv->machine);
      self->priv->pattern_added_handler=0;
      self->priv->pattern_removed_handler=0;
    }
    if(machine) {
      GST_INFO("ref new cur-machine: refs: %d",(G_OBJECT(self->priv->machine))->ref_count);
      self->priv->pattern_added_handler=g_signal_connect(G_OBJECT(machine),"pattern-added",G_CALLBACK(on_pattern_changed),(gpointer)self);
      self->priv->pattern_removed_handler=g_signal_connect(G_OBJECT(machine),"pattern-removed",G_CALLBACK(on_pattern_changed),(gpointer)self);
    }
    self->priv->machine=machine;
  }

  //-- append default rows
  pattern_keys=sink_pattern_keys;
  gtk_list_store_append(store, &tree_iter);
  gtk_list_store_set(store,&tree_iter,0,".",1,_("  clear"),-1);
  gtk_list_store_append(store, &tree_iter);
  gtk_list_store_set(store,&tree_iter,0,"-",1,_("  mute"),-1);
  gtk_list_store_append(store, &tree_iter);
  gtk_list_store_set(store,&tree_iter,0,",",1,_("  break"),-1);
  if(BT_IS_PROCESSOR_MACHINE(machine)) {
    gtk_list_store_append(store, &tree_iter);
    gtk_list_store_set(store,&tree_iter,0,"_",1,_("  thru"),-1);
    pattern_keys=processor_pattern_keys;
  }
  if(BT_IS_SOURCE_MACHINE(machine)) {
    gtk_list_store_append(store, &tree_iter);
    gtk_list_store_set(store,&tree_iter,0,"_",1,_("  solo"),-1);
    pattern_keys=source_pattern_keys;
  }
  
  if(machine) {
    //-- append pattern rows
    g_object_get(G_OBJECT(machine),"patterns",&list,NULL);
    for(node=list;node;node=g_list_next(node)) {
      pattern=BT_PATTERN(node->data);
      g_object_get(G_OBJECT(pattern),"name",&str,"is-internal",&is_internal,NULL);
      if(!is_internal) {
        //GST_DEBUG("  adding \"%s\" at index %d -> '%c'",str,index,pattern_keys[index]);
        key[0]=(index<64)?pattern_keys[index]:' ';
        //if(index<64) key[0]=pattern_keys[index];
        //else key[0]=' ';
        //GST_DEBUG("  with shortcut \"%s\"",key);
        gtk_list_store_append(store, &tree_iter);
        gtk_list_store_set(store,&tree_iter,0,key,1,str,-1);
        index++;
      }
      g_free(str);
    }
    g_list_free(list);
  }
  gtk_tree_view_set_model(self->priv->pattern_list,GTK_TREE_MODEL(store));
  
  g_object_unref(store); // drop with treeview
}

/*
 * machine_menu_refresh:
 * add all machines from setup to self->priv->context_menu_add
 */
static void machine_menu_refresh(const BtMainPageSequence *self,const BtSetup *setup) {
  BtMachine *machine=NULL;
  GList *node,*list,*widgets;
  GtkWidget *menu_item,*submenu,*label;
  gchar *str;

  GST_INFO("refreshing track menu");
  
  // create a new menu
  submenu=gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(self->priv->context_menu_add),submenu);
  
  // fill machine menu
  g_object_get(G_OBJECT(setup),"machines",&list,NULL);
  for(node=list;node;node=g_list_next(node)) {
    machine=BT_MACHINE(node->data);
    g_object_get(G_OBJECT(machine),"id",&str,NULL);
    
    menu_item=gtk_image_menu_item_new_with_label(str);
    gtk_widget_set_name(GTK_WIDGET(menu_item),str);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),bt_ui_ressources_get_image_by_machine(machine));
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu),menu_item);
    gtk_widget_show(menu_item);
    widgets=gtk_container_get_children(GTK_CONTAINER(menu_item));
    label=g_list_nth_data(widgets,0);
    if(GTK_IS_LABEL(label)) {
      g_signal_connect(G_OBJECT(machine),"notify::id",G_CALLBACK(on_machine_id_changed),(gpointer)label);
    }
    g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_track_add_activated),(gpointer)self);
    g_list_free(widgets);
    g_free(str);
  }
  g_list_free(list);
}

static void sequence_view_set_pos(const BtMainPageSequence *self,gulong modifier,glong row) {
  // set play or loop bars
  BtSong *song;
  BtSequence *sequence;
  gulong sequence_length;
  gdouble loop_pos;
  
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(song,"sequence",&sequence,NULL);
  g_object_get(sequence,"length",&sequence_length,NULL);
  if(row==-1) row=sequence_length;
  // use a keyboard qualifier to set loop_start and end
  /* @todo should the sequence-view listen to notify::xxx ? */
  switch(modifier) {
    case 0:
      g_object_set(song,"play-pos",row,NULL);
      break;
    case GDK_CONTROL_MASK:
      g_object_set(sequence,"loop-start",row,NULL);
      loop_pos=(gdouble)row/(gdouble)sequence_length;
      g_object_set(self->priv->sequence_table,"loop-start",loop_pos,NULL);
      g_object_set(self->priv->sequence_pos_table,"loop-start",loop_pos,NULL);
      break;
    case GDK_MOD4_MASK:
      g_object_set(sequence,"loop-end",row,NULL);
      loop_pos=(gdouble)row/(gdouble)sequence_length;
      g_object_set(self->priv->sequence_table,"loop-end",loop_pos,NULL);
      g_object_set(self->priv->sequence_pos_table,"loop-start",loop_pos,NULL);
      break;
  }
  g_object_unref(sequence);
  g_object_unref(song);
}

//-- event handler

static void on_track_add_activated(GtkMenuItem *menuitem, gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  BtSong *song;
  BtSequence *sequence;
  BtSetup *setup;
  BtMachine *machine;
  gchar *id;

  // get song from app and then setup from song
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(song,"setup",&setup,"sequence",&sequence,NULL);

  // get the machine by the menuitems name
  id=(gchar *)gtk_widget_get_name(GTK_WIDGET(menuitem));
  GST_INFO("adding track for machine \"%s\"",id);
  if((machine=bt_setup_get_machine_by_id(setup,id))) {
    bt_sequence_add_track(sequence,machine);
    // reinit the view
    sequence_table_refresh(self,song);
    sequence_model_recolorize(self);
    g_object_unref(machine);
  }
  
  g_object_unref(sequence);
  g_object_unref(setup);
  g_object_unref(song);
}

static void on_track_remove_activated(GtkMenuItem *menuitem, gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  BtSong *song;
  BtSequence *sequence;
  gulong number_of_tracks;

  // get song from app and then setup from song
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(song,"sequence",&sequence,NULL);

  // change number of tracks
  g_object_get(sequence,"tracks",&number_of_tracks,NULL);
  if(number_of_tracks>0) {
    bt_sequence_remove_track_by_ix(sequence,number_of_tracks-1);
    // reinit the view
    sequence_table_refresh(self,song);
    sequence_model_recolorize(self);
  }
  
  g_object_unref(sequence);
  g_object_unref(song);
}

static void on_sequence_tick(const BtSong *song,GParamSpec *arg,gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  BtSequence *sequence;
  gdouble play_pos;
  gulong sequence_length,pos;
  GtkTreePath *path;
  
  g_assert(user_data);

  // calculate fractional pos and set into sequence-viewer
  g_object_get(G_OBJECT(song),"sequence",&sequence,"play-pos",&pos,NULL);
  g_object_get(G_OBJECT(sequence),"length",&sequence_length,NULL);
  play_pos=(gdouble)pos/(gdouble)sequence_length;
  if(play_pos<=1.0) {
    g_object_set(self->priv->sequence_table,"play-position",play_pos,NULL);
    g_object_set(self->priv->sequence_pos_table,"play-position",play_pos,NULL);
  }

  //GST_DEBUG("sequence tick received : %d",pos);
  
  // do nothing for invisible rows
  if(IS_SEQUENCE_POS_VISIBLE(pos,self->priv->bars) || GTK_WIDGET_REALIZED(self->priv->sequence_table)) {
    // scroll  to make play pos visible
    if((path=gtk_tree_path_new_from_indices(pos,-1))) {
      // that would try to keep the cursor in the middle (means it will scroll more)
      gtk_tree_view_scroll_to_cell(self->priv->sequence_table,path,NULL,TRUE,0.5,0.5);
      //gtk_tree_view_scroll_to_cell(self->priv->sequence_table,path,NULL,FALSE,0.0,0.0);
      gtk_tree_path_free(path);
    }
  }
  g_object_unref(sequence);
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

static gboolean on_sequence_table_cursor_changed_idle(gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  GtkTreePath *path;
  GtkTreeViewColumn *column;
  gulong cursor_column,cursor_row;

  g_return_val_if_fail(user_data,FALSE);

  //GST_INFO("sequence_table cursor has changed : self=%p",user_data);
  //cursor_column=sequence_view_get_cursor_column(treeview);
  
  gtk_tree_view_get_cursor(self->priv->sequence_table,&path,&column);
  if(sequence_view_get_cursor_pos(self->priv->sequence_table,path,column,&cursor_column,&cursor_row)) {
    //GST_INFO("new row = %3d <-> old row = %3d",cursor_row,self->priv->cursor_row);
    self->priv->cursor_row=cursor_row;
    //GST_INFO("new col = %3d <-> old col = %3d",cursor_column,self->priv->cursor_column);
    if(cursor_column!=self->priv->cursor_column) {
      self->priv->cursor_column=cursor_column;
      pattern_list_refresh(self);
    }
    GST_INFO("cursor has changed: %3d,%3d",self->priv->cursor_column,self->priv->cursor_row);
    gtk_widget_queue_draw(GTK_WIDGET(self->priv->sequence_table));
  }
  return(FALSE);
}

static void on_sequence_table_cursor_changed(GtkTreeView *treeview, gpointer user_data) {
  /* delay the action */
  g_idle_add_full(G_PRIORITY_HIGH_IDLE,on_sequence_table_cursor_changed_idle,user_data,NULL);
}

static gboolean on_sequence_table_key_release_event(GtkWidget *widget,GdkEventKey *event,gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  BtSong *song;
  BtSequence *sequence;
  gboolean res=FALSE;
  gchar *str=NULL;
  gboolean free_str=FALSE;
  gboolean change=FALSE;
  gulong time,track;

  g_assert(user_data);

  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"sequence",&sequence,NULL);
  
  GST_INFO("sequence_table key : state 0x%x, keyval 0x%x",event->state,event->keyval);
  // determine timeline and timelinetrack from cursor pos
  if(bt_main_page_sequence_get_current_pos(self,&time,&track)) {
    gulong modifier=(gulong)event->state&gtk_accelerator_get_default_mod_mask();
    //gulong modifier=(gulong)event->state&(GDK_SHIFT_MASK|GDK_CONTROL_MASK|GDK_MOD4_MASK);
    // look up pattern for key
    if(event->keyval==' ') {
      bt_sequence_set_pattern(sequence,time,track,NULL);
      str=" ";
      change=TRUE;
      res=TRUE;
    }
    else if(event->keyval==GDK_Return) {  /* GDK_KP_Enter */
      if(modifier==GDK_CONTROL_MASK) {
        BtPattern *pattern;
        if((pattern=bt_sequence_get_pattern(sequence,time,track))) {
          BtMainWindow *main_window;
          BtMainPages *pages;
          BtMainPagePatterns *patterns_page;
  
          g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
          g_object_get(G_OBJECT(main_window),"pages",&pages,NULL);
          g_object_get(G_OBJECT(pages),"patterns-page",&patterns_page,NULL);
    
          gtk_notebook_set_current_page(GTK_NOTEBOOK(pages),BT_MAIN_PAGES_PATTERNS_PAGE);
          bt_main_page_patterns_show_pattern(patterns_page,pattern);
  
          g_object_try_unref(patterns_page);
          g_object_try_unref(pages);
          g_object_try_unref(main_window);
  
          g_object_unref(pattern);
          res=TRUE;
        }
      }
    }
    else if(event->keyval==GDK_Up || event->keyval==GDK_Down || event->keyval==GDK_Left || event->keyval==GDK_Right) {
      if(modifier==GDK_SHIFT_MASK) {
        GtkTreePath *path=NULL;
        GtkTreeViewColumn *column;
        GList *columns=NULL;
        gboolean select=FALSE;
        
        GST_INFO("handling selection");
        
        // handle selection
        switch(event->keyval) {
          case GDK_Up:
            GST_INFO("up   : %3d,%3d -> %3d,%3d @ %3d,%3d",self->priv->selection_start_column,self->priv->selection_start_row,self->priv->selection_end_column,self->priv->selection_end_row,self->priv->cursor_column,self->priv->cursor_row);
            if((self->priv->selection_start_column!=self->priv->cursor_column) &&
              (self->priv->selection_start_row!=(self->priv->cursor_row-self->priv->bars))
            ) {
              GST_INFO("up   : new selection");
              self->priv->selection_start_column=self->priv->cursor_column;
              self->priv->selection_end_column=self->priv->cursor_column;
              self->priv->selection_start_row=self->priv->cursor_row;
              self->priv->selection_end_row=self->priv->cursor_row+self->priv->bars;
            }
            else {
              GST_INFO("up   : expand selection");
              self->priv->selection_start_row-=self->priv->bars;
            }
            GST_INFO("up   : %3d,%3d -> %3d,%3d",self->priv->selection_start_column,self->priv->selection_start_row,self->priv->selection_end_column,self->priv->selection_end_row);
            select=TRUE;
            break;
          case GDK_Down:
            GST_INFO("down : %3d,%3d -> %3d,%3d @ %3d,%3d",self->priv->selection_start_column,self->priv->selection_start_row,self->priv->selection_end_column,self->priv->selection_end_row,self->priv->cursor_column,self->priv->cursor_row);
            if((self->priv->selection_end_column!=self->priv->cursor_column) &&
              (self->priv->selection_end_row!=(self->priv->cursor_row+self->priv->bars))
            ) {
              GST_INFO("down : new selection");
              self->priv->selection_start_column=self->priv->cursor_column;
              self->priv->selection_end_column=self->priv->cursor_column;
              self->priv->selection_start_row=self->priv->cursor_row-self->priv->bars;
              self->priv->selection_end_row=self->priv->cursor_row;
            }
            else {
              GST_INFO("down : expand selection");
              self->priv->selection_end_row+=self->priv->bars;
            }
            GST_INFO("down : %3d,%3d -> %3d,%3d",self->priv->selection_start_column,self->priv->selection_start_row,self->priv->selection_end_column,self->priv->selection_end_row);
            select=TRUE;
            break;
          case GDK_Left:
            // move cursor
            self->priv->cursor_column--;
            path=gtk_tree_path_new_from_indices((self->priv->cursor_row/self->priv->bars),-1);
            columns=gtk_tree_view_get_columns(self->priv->sequence_table);
            column=g_list_nth_data(columns,self->priv->cursor_column);
            // set cell focus
            gtk_tree_view_set_cursor(self->priv->sequence_table,path,column,FALSE);
            gtk_widget_grab_focus(GTK_WIDGET(self->priv->sequence_table));
            GST_INFO("left : %3d,%3d -> %3d,%3d @ %3d,%3d",self->priv->selection_start_column,self->priv->selection_start_row,self->priv->selection_end_column,self->priv->selection_end_row,self->priv->cursor_column,self->priv->cursor_row);
            if((self->priv->selection_start_column!=(self->priv->cursor_column+1)) &&
              (self->priv->selection_start_row!=self->priv->cursor_row)
            ) {
              GST_INFO("left : new selection");
              self->priv->selection_start_column=self->priv->cursor_column;
              self->priv->selection_end_column=self->priv->cursor_column+1;
              self->priv->selection_start_row=self->priv->cursor_row;
              self->priv->selection_end_row=self->priv->cursor_row;
            }
            else {
              GST_INFO("left : expand selection");
              self->priv->selection_start_column--;
            }
            GST_INFO("left : %3d,%3d -> %3d,%3d",self->priv->selection_start_column,self->priv->selection_start_row,self->priv->selection_end_column,self->priv->selection_end_row);
            select=TRUE;
            break;
          case GDK_Right:
            // move cursor
            self->priv->cursor_column++;
            path=gtk_tree_path_new_from_indices((self->priv->cursor_row/self->priv->bars),-1);
            columns=gtk_tree_view_get_columns(self->priv->sequence_table);
            column=g_list_nth_data(columns,self->priv->cursor_column);
            // set cell focus
            gtk_tree_view_set_cursor(self->priv->sequence_table,path,column,FALSE);
            gtk_widget_grab_focus(GTK_WIDGET(self->priv->sequence_table));
            GST_INFO("right: %3d,%3d -> %3d,%3d @ %3d,%3d",self->priv->selection_start_column,self->priv->selection_start_row,self->priv->selection_end_column,self->priv->selection_end_row,self->priv->cursor_column,self->priv->cursor_row);
            if((self->priv->selection_end_column!=(self->priv->cursor_column-1)) &&
              (self->priv->selection_end_row!=self->priv->cursor_row)
            ) {
              GST_INFO("right: new selection");
              self->priv->selection_start_column=self->priv->cursor_column-1;
              self->priv->selection_end_column=self->priv->cursor_column;
              self->priv->selection_start_row=self->priv->cursor_row;
              self->priv->selection_end_row=self->priv->cursor_row;
            }
            else {
              GST_INFO("right: expand selection");
              self->priv->selection_end_column++;
            }
            GST_INFO("right: %3d,%3d -> %3d,%3d",self->priv->selection_start_column,self->priv->selection_start_row,self->priv->selection_end_column,self->priv->selection_end_row);
            select=TRUE;
            break;
        }
        if(path) gtk_tree_path_free(path);
        if(columns) g_list_free(columns);
        if(select) {
          gtk_widget_queue_draw(GTK_WIDGET(self->priv->sequence_table));
          res=TRUE;
        }
      }
      else {
        if(self->priv->selection_start_column!=-1) {
          self->priv->selection_start_column=self->priv->selection_start_row=self->priv->selection_end_column=self->priv->selection_end_row=-1;
          gtk_widget_queue_draw(GTK_WIDGET(self->priv->sequence_table));
        }
      }
    }
    else if(event->keyval<0x100) {
      gchar *pos=strchr(pattern_keys,(gchar)(event->keyval&0xff));
      
      // reset selection
      self->priv->selection_start_column=-1;
      self->priv->selection_start_row=-1;
      self->priv->selection_end_column=-1;
      self->priv->selection_end_row=-1;
      
      GST_INFO("pattern key pressed: '%c'",*pos);
      
      if(pos) {
        BtMachine *machine;

        if((machine=bt_sequence_get_machine(sequence,track))) {
          BtPattern *pattern;
          gulong index=(gulong)pos-(gulong)pattern_keys;
        
          if((pattern=bt_machine_get_pattern_by_index(machine,index))) {
            bt_sequence_set_pattern(sequence,time,track,pattern);
            g_object_get(G_OBJECT(pattern),"name",&str,NULL);
            g_object_unref(pattern);
            free_str=TRUE;
            change=TRUE;
            res=TRUE;
          }
          g_object_unref(machine);
        }
      }
    }
    // update tree-view model
    if(change) {
      GtkTreeModelFilter *filtered_store;
      GtkTreeModel *store;
      
      if((filtered_store=GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(self->priv->sequence_table))) &&
        (store=gtk_tree_model_filter_get_model(filtered_store))
      ) {
        GtkTreePath *path;
        GtkTreeViewColumn *column;

        gtk_tree_view_get_cursor(self->priv->sequence_table,&path,&column);
        if(path && column) {
          GtkTreeIter iter,filter_iter;
          
          GST_INFO("  update model");
          
          if(gtk_tree_model_get_iter(GTK_TREE_MODEL(filtered_store),&filter_iter,path)) {
            GList *columns=gtk_tree_view_get_columns(self->priv->sequence_table);
            glong track=g_list_index(columns,(gpointer)column)-1;
            //glong row;
          
            g_list_free(columns);
            gtk_tree_model_filter_convert_iter_to_child_iter(filtered_store,&iter,&filter_iter);
            //gtk_tree_model_get(store,&iter,SEQUENCE_TABLE_POS,&row,-1);
            //GST_INFO("  position is %d,%d -> ",row,track,SEQUENCE_TABLE_PRE_CT+track);
            
            gtk_list_store_set(GTK_LIST_STORE(store),&iter,SEQUENCE_TABLE_PRE_CT+track,str,-1);
          }
          else {
            GST_WARNING("  can't get tree-iter");
          }
        }
        else {
          GST_WARNING("  can't evaluate cursor pos");
        }
        if(path) gtk_tree_path_free(path);
      }
      else {
        GST_WARNING("  can't get tree-model");
      }
    }
    //else if(!select) GST_INFO("  nothing assgned to this key");
    if(free_str) g_free(str);
  }
  else {
    GST_WARNING("  can't locate timelinetrack related to curos pos");
  }
  // release the references
  g_object_try_unref(sequence);
  g_object_try_unref(song);
  return(res);
}

static gboolean on_sequence_table_button_press_event(GtkWidget *widget,GdkEventButton *event,gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  gboolean res=FALSE;

  g_assert(user_data);
  
  GST_INFO("sequence_table button_press : button 0x%x, type 0x%d",event->button,event->type);
  if(event->button==1) {
    if(gtk_tree_view_get_bin_window(GTK_TREE_VIEW(widget))==(event->window)) {
      GtkTreePath *path;
      GtkTreeViewColumn *column;
      gulong modifier=(gulong)event->state&(GDK_CONTROL_MASK|GDK_MOD4_MASK);
      // determine sequence position from mouse coordinates
      if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget),event->x,event->y,&path,&column,NULL,NULL)) {
        gulong track,row;
        if(sequence_view_get_cursor_pos(GTK_TREE_VIEW(widget),path,column,&track,&row)) {
          GST_INFO("  left click to column %d, row %d",track,row);
          if(widget==GTK_WIDGET(self->priv->sequence_pos_table)) {
            sequence_view_set_pos(self,modifier,(glong)row);
          }
          else {
            // set cell focus
            gtk_tree_view_set_cursor(self->priv->sequence_table,path,column,FALSE);
            gtk_widget_grab_focus(GTK_WIDGET(self->priv->sequence_table));
            // reset selection
            self->priv->selection_start_column=self->priv->selection_start_row=self->priv->selection_end_column=self->priv->selection_end_row=-1;
          }
          res=TRUE;
        }
      }
      else {
        GST_INFO("clicked outside data area - #1");
        sequence_view_set_pos(self,modifier,-1);
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

static gboolean on_sequence_table_motion_notify_event(GtkWidget *widget,GdkEventMotion *event,gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  gboolean res=FALSE;

  g_assert(user_data);
  
  // only activate in button_press ?
  if(event->state&GDK_BUTTON1_MASK) {
    if(gtk_tree_view_get_bin_window(GTK_TREE_VIEW(widget))==(event->window)) {
      GtkTreePath *path;
      GtkTreeViewColumn *column;
      // determine sequence position from mouse coordinates
      if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget),event->x,event->y,&path,&column,NULL,NULL)) {
        // handle selection
        glong cursor_column=self->priv->cursor_column;
        glong cursor_row=self->priv->cursor_row;
        if(self->priv->selection_start_column==-1) {
          self->priv->selection_column=self->priv->cursor_column;
          self->priv->selection_row=self->priv->cursor_row;
        }
        gtk_tree_view_set_cursor(self->priv->sequence_table,path,column,FALSE);
        gtk_widget_grab_focus(GTK_WIDGET(self->priv->sequence_table));
        // cursor updates are not yet processed
        on_sequence_table_cursor_changed_idle(self);
        GST_INFO("cursor new/old: %3d,%3d -> %3d,%3d",cursor_column,cursor_row,self->priv->cursor_column,self->priv->cursor_row);
        if((cursor_column!=self->priv->cursor_column) || (cursor_row!=self->priv->cursor_row)) {
          if(self->priv->selection_start_column==-1) {
            self->priv->selection_start_column=MIN(cursor_column,self->priv->selection_column);
            self->priv->selection_start_row=MIN(cursor_row,self->priv->selection_row);
            self->priv->selection_end_column=MAX(cursor_column,self->priv->selection_column);
            self->priv->selection_end_row=MAX(cursor_row,self->priv->selection_row);
          }
          else {
            if(self->priv->cursor_column<self->priv->selection_column) {
              self->priv->selection_start_column=self->priv->cursor_column;
              self->priv->selection_end_column=self->priv->selection_column;
            }
            else {
              self->priv->selection_start_column=self->priv->selection_column;
              self->priv->selection_end_column=self->priv->cursor_column;
            }
            if(self->priv->cursor_row<self->priv->selection_row) {
              self->priv->selection_start_row=self->priv->cursor_row;
              self->priv->selection_end_row=self->priv->selection_row;
            }
            else {
              self->priv->selection_start_row=self->priv->selection_row;
              self->priv->selection_end_row=self->priv->cursor_row;
            }
          }
          gtk_widget_queue_draw(GTK_WIDGET(self->priv->sequence_table));
        }
        res=TRUE;
      }
      if(path) gtk_tree_path_free(path);
    }
  }  
  return(res);
}

static void on_machine_added(BtSetup *setup,BtMachine *machine,gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  
  g_assert(user_data);
  
  GST_INFO("new machine has been added");
  machine_menu_refresh(self,setup);
}

static void on_machine_removed(BtSetup *setup,BtMachine *machine,gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  BtSong *song;
  BtSequence *sequence;

  g_assert(user_data);
  
  GST_INFO("machine has been removed");

  machine_menu_refresh(self,setup);

  // get song from app and then setup from song
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(song,"sequence",&sequence,NULL);

  bt_sequence_remove_track_by_machine(sequence,machine);
  // reinit the view
  sequence_table_refresh(self,song);
  sequence_model_recolorize(self);

  g_object_unref(sequence);
  g_object_unref(song);
}

static void on_pattern_changed(BtMachine *machine,BtPattern *pattern,gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);

  g_assert(user_data);
  
  GST_INFO("pattern has been added/removed");
  pattern_list_refresh(self);
}


static void on_song_changed(const BtEditApplication *app,GParamSpec *arg,gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  BtSong *song;
  BtSongInfo *song_info;
  BtSetup *setup;
  BtSequence *sequence;
  glong index,bars;
  glong loop_start_pos,loop_end_pos;
  gulong sequence_length;
  gdouble loop_start,loop_end;

  g_assert(user_data);

  GST_INFO("song has changed : app=%p, self=%p",app,self);
  // get song from app and then setup from song
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_return_if_fail(song);

  g_object_get(G_OBJECT(song),"song-info",&song_info,"setup",&setup,"sequence",&sequence,NULL);
  // update page
  // update sequence and pattern list
  sequence_table_refresh(self,song);
  pattern_list_refresh(self);
  machine_menu_refresh(self,setup);
  g_signal_connect(G_OBJECT(setup),"machine-added",G_CALLBACK(on_machine_added),(gpointer)self);
  g_signal_connect(G_OBJECT(setup),"machine-removed",G_CALLBACK(on_machine_removed),(gpointer)self);
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
  // update sequence view
  g_object_get(G_OBJECT(sequence),"length",&sequence_length,"loop-start",&loop_start_pos,"loop-end",&loop_end_pos,NULL);
  loop_start=(loop_start_pos>-1)?(gdouble)loop_start_pos/(gdouble)sequence_length:0.0;
  loop_end  =(loop_end_pos  >-1)?(gdouble)loop_end_pos  /(gdouble)sequence_length:1.0;
  g_object_set(self->priv->sequence_table,"play-position",0.0,"loop-start",loop_start,"loop-end",loop_end,NULL);
  g_object_set(self->priv->sequence_pos_table,"play-position",0.0,"loop-start",loop_start,"loop-end",loop_end,NULL);
  // subscribe to play-pos changes of song->sequence
  g_signal_connect(G_OBJECT(song), "notify::play-pos", G_CALLBACK(on_sequence_tick), (gpointer)self);
  //-- release the references
  g_object_try_unref(song_info);
  g_object_try_unref(setup);
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
    sprintf(str,"%lu",i);
    gtk_list_store_append(store,&iter);
    gtk_list_store_set(store,&iter,0,str,-1);
  }
  gtk_combo_box_set_model(self->priv->bars_menu,GTK_TREE_MODEL(store));
  gtk_combo_box_set_active(self->priv->bars_menu,2);
  g_object_unref(store); // drop with combobox
  
  return(TRUE);
}

static gboolean bt_main_page_sequence_init_ui(const BtMainPageSequence *self) {
  GtkWidget *toolbar;
  GtkWidget *split_box,*box,*tool_item;
  GtkWidget *scrolled_window,*scrolled_sync_window;
  GtkWidget *menu_item,*image;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *tree_col;
  GtkTreeSelection *tree_sel;
  GtkAdjustment *vadjust;

  GST_DEBUG("!!!! self=%p",self);
  
  gtk_widget_set_name(GTK_WIDGET(self),_("sequence view"));
  
  // add toolbar
  toolbar=gtk_toolbar_new();
  gtk_widget_set_name(toolbar,_("sequence view tool bar"));
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
  g_signal_connect(G_OBJECT(self->priv->bars_menu),"changed",G_CALLBACK(on_bars_menu_changed), (gpointer)self);
  gtk_box_pack_start(GTK_BOX(box),gtk_label_new(_("Steps")),FALSE,FALSE,2);
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->priv->bars_menu),TRUE,TRUE,2);
  
  tool_item=GTK_WIDGET(gtk_tool_item_new());
  gtk_widget_set_name(tool_item,_("Steps"));
  gtk_container_add(GTK_CONTAINER(tool_item),box);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar),GTK_TOOL_ITEM(tool_item),-1);

  // get colors
  self->priv->cursor_bg=bt_ui_ressources_get_gdk_color(BT_UI_RES_COLOR_CURSOR);
  self->priv->selection_bg1=bt_ui_ressources_get_gdk_color(BT_UI_RES_COLOR_SELECTION1);
  self->priv->selection_bg2=bt_ui_ressources_get_gdk_color(BT_UI_RES_COLOR_SELECTION2);
  self->priv->source_bg1=bt_ui_ressources_get_gdk_color(BT_UI_RES_COLOR_SOURCE_MACHINE_BRIGHT1);
  self->priv->source_bg2=bt_ui_ressources_get_gdk_color(BT_UI_RES_COLOR_SOURCE_MACHINE_BRIGHT2);
  self->priv->processor_bg1=bt_ui_ressources_get_gdk_color(BT_UI_RES_COLOR_PROCESSOR_MACHINE_BRIGHT1);
  self->priv->processor_bg2=bt_ui_ressources_get_gdk_color(BT_UI_RES_COLOR_PROCESSOR_MACHINE_BRIGHT2);
  self->priv->sink_bg1=bt_ui_ressources_get_gdk_color(BT_UI_RES_COLOR_SINK_MACHINE_BRIGHT1);
  self->priv->sink_bg2=bt_ui_ressources_get_gdk_color(BT_UI_RES_COLOR_SINK_MACHINE_BRIGHT2);

  // generate the context menu  
  self->priv->context_menu=GTK_MENU(gtk_menu_new());
  self->priv->context_menu_add=GTK_MENU_ITEM(gtk_image_menu_item_new_with_label(_("Add track")));
  image=gtk_image_new_from_stock(GTK_STOCK_ADD,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(self->priv->context_menu_add),image);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),GTK_WIDGET(self->priv->context_menu_add));
  gtk_widget_show(GTK_WIDGET(self->priv->context_menu_add));

  // @idea should that be in the context menu of table headers?
  menu_item=gtk_image_menu_item_new_with_label(_("Remove track"));
  image=gtk_image_new_from_stock(GTK_STOCK_REMOVE,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_track_remove_activated),(gpointer)self);

  // --
  // @todo cut, copy, paste
  
 
  // add a hpaned
  split_box=gtk_hpaned_new();
  gtk_container_add(GTK_CONTAINER(self),split_box);
  
  // add hbox for sequence view
  box=gtk_hbox_new(FALSE,0);
  gtk_paned_pack1(GTK_PANED(split_box),box,TRUE,TRUE);
  
  // add sequence-pos list-view
  scrolled_sync_window=gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_sync_window),GTK_POLICY_NEVER,GTK_POLICY_NEVER);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_sync_window),GTK_SHADOW_NONE);
  self->priv->sequence_pos_table=GTK_TREE_VIEW(bt_sequence_view_new(self->priv->app));
  g_object_set(self->priv->sequence_pos_table,"enable-search",FALSE,"rules-hint",TRUE,"fixed-height-mode",TRUE,NULL);
  // set a minimum size, otherwise the window can't be shrinked (we need this because of GTK_POLICY_NEVER)
  gtk_widget_set_size_request(GTK_WIDGET(self->priv->sequence_pos_table),40,40);
  tree_sel=gtk_tree_view_get_selection(self->priv->sequence_pos_table);
  gtk_tree_selection_set_mode(tree_sel,GTK_SELECTION_NONE);
  sequence_pos_table_init(self);
  gtk_container_add(GTK_CONTAINER(scrolled_sync_window),GTK_WIDGET(self->priv->sequence_pos_table));
  gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(scrolled_sync_window), FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(self->priv->sequence_pos_table), "button-press-event", G_CALLBACK(on_sequence_table_button_press_event), (gpointer)self);
  
  // add vertical separator
  gtk_box_pack_start(GTK_BOX(box), gtk_vseparator_new(), FALSE, FALSE, 0);

  // add sequence list-view
  scrolled_window=gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_NONE);
  self->priv->sequence_table=GTK_TREE_VIEW(bt_sequence_view_new(self->priv->app));
  g_object_set(self->priv->sequence_table,"enable-search",FALSE,"rules-hint",TRUE,"fixed-height-mode",TRUE,NULL);
  tree_sel=gtk_tree_view_get_selection(self->priv->sequence_table);
  gtk_tree_selection_set_mode(tree_sel,GTK_SELECTION_NONE);
  sequence_table_init(self,FALSE);
  gtk_container_add(GTK_CONTAINER(scrolled_window),GTK_WIDGET(self->priv->sequence_table));
  gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(scrolled_window), TRUE, TRUE, 0);
  g_signal_connect_after(G_OBJECT(self->priv->sequence_table), "cursor-changed", G_CALLBACK(on_sequence_table_cursor_changed), (gpointer)self);
  g_signal_connect(G_OBJECT(self->priv->sequence_table), "key-release-event", G_CALLBACK(on_sequence_table_key_release_event), (gpointer)self);
  g_signal_connect(G_OBJECT(self->priv->sequence_table), "button-press-event", G_CALLBACK(on_sequence_table_button_press_event), (gpointer)self);
  g_signal_connect(G_OBJECT(self->priv->sequence_table), "motion-notify-event", G_CALLBACK(on_sequence_table_motion_notify_event), (gpointer)self);

  // make first scrolled-window also use the horiz-scrollbar of the second scrolled-window
  vadjust=gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled_window));
  gtk_scrolled_window_set_vadjustment(GTK_SCROLLED_WINDOW(scrolled_sync_window),vadjust);

  // add vertical separator
  gtk_box_pack_start(GTK_BOX(box), gtk_vseparator_new(), FALSE, FALSE, 0);

  // add hbox for pattern list
  box=gtk_hbox_new(FALSE,0);
  gtk_paned_pack2(GTK_PANED(split_box),box,FALSE,FALSE);

  // add vertical separator
  gtk_box_pack_start(GTK_BOX(box), gtk_vseparator_new(), FALSE, FALSE, 0);

  // add pattern list-view
  scrolled_window=gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_NONE);
  self->priv->pattern_list=GTK_TREE_VIEW(gtk_tree_view_new());
  g_object_set(self->priv->pattern_list,"enable-search",FALSE,"rules-hint",TRUE,"fixed-height-mode",TRUE,NULL);
  
  renderer=gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer),"xalign",1.0,NULL);
  if((tree_col=gtk_tree_view_column_new_with_attributes(_("Key"),renderer,"text",0,NULL))) {
    g_object_set(tree_col,"sizing",GTK_TREE_VIEW_COLUMN_FIXED,"fixed-width",30,NULL);
    gtk_tree_view_insert_column(self->priv->pattern_list,tree_col,-1);
  }
  else GST_WARNING("can't create treeview column");

  renderer=gtk_cell_renderer_text_new();
  if((tree_col=gtk_tree_view_column_new_with_attributes(_("Patterns"),renderer,"text",1,NULL))) {
    g_object_set(tree_col,"sizing",GTK_TREE_VIEW_COLUMN_FIXED,"fixed-width",70,NULL);
    gtk_tree_view_insert_column(self->priv->pattern_list,tree_col,-1);
  }
  else GST_WARNING("can't create treeview column");
  
  gtk_container_add(GTK_CONTAINER(scrolled_window),GTK_WIDGET(self->priv->pattern_list));
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(scrolled_window),FALSE,FALSE,0);
  //gtk_paned_pack2(GTK_PANED(split_box),GTK_WIDGET(scrolled_window),FALSE,FALSE);

  // register event handlers
  g_signal_connect(G_OBJECT(self->priv->app), "notify::song", G_CALLBACK(on_song_changed), (gpointer)self);

  GST_DEBUG("  done");
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
 * @self: the sequence subpage
 *
 * Get the currently active #BtMachine as determined by the cursor position in
 * the sequence table.
 * Unref the machine, when done with it.
 *
 * Returns: the #BtMachine instance or %NULL in case of an error
 */
BtMachine *bt_main_page_sequence_get_current_machine(const BtMainPageSequence *self) {
  BtMachine *machine=NULL;
  glong col;

  GST_INFO("get active machine");
  
  // get table column number (col 0 is for for labels)
  if((col=sequence_view_get_cursor_column(self->priv->sequence_table))>0) {
    BtSong *song;
    BtSequence *sequence;

    GST_INFO(">>> active col is %d",col);
    g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
    g_object_get(G_OBJECT(song),"sequence",&sequence,NULL);
    machine=bt_sequence_get_machine(sequence,col-1);
    // release the references
    g_object_try_unref(sequence);
    g_object_try_unref(song);
  }
  return(machine);
}

/**
 * bt_main_page_sequence_get_current_pos:
 * @self: the sequence subpage
 * @time: pointer for time result
 * @track: pointer for track result
 *
 * Get the currently cursor position in the sequence table.
 * The result will be place in the respective pointers.
 * If one is NULL, no value is returned for it.
 *
 * Returns: %TRUE if the cursor is at a valid track position
 */
gboolean bt_main_page_sequence_get_current_pos(const BtMainPageSequence *self,gulong *time,gulong *track) {
  gboolean res=FALSE;
  BtSong *song;
  BtSequence *sequence;
  GtkTreePath *path;
  GtkTreeViewColumn *column;

  //GST_INFO("get active sequence cell");

  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"sequence",&sequence,NULL);

  // get table column number (column 0 is for labels)
  gtk_tree_view_get_cursor(self->priv->sequence_table,&path,&column);
  if(column && path) {
    GList *columns=gtk_tree_view_get_columns(self->priv->sequence_table);
    glong col=g_list_index(columns,(gpointer)column);

    g_list_free(columns);
    if(col>0) { // first two tracks are pos and label
      GtkTreeModel *store;
      GtkTreeIter iter;
      // get iter from path
      store=gtk_tree_view_get_model(self->priv->sequence_table);
      if(gtk_tree_model_get_iter(store,&iter,path)) {
        gulong row;
        // get pos from iter and then the timeline
        gtk_tree_model_get(store,&iter,SEQUENCE_TABLE_POS,&row,-1);
        GST_INFO("  found active cell at %d,%d",col-1,row);
        if(time) *time=row;
        if(track) *track=col-1;
        res=TRUE;
      }
    }
  }
  if(path) gtk_tree_path_free(path);
  // release the references
  g_object_try_unref(sequence);
  g_object_try_unref(song);
  return(res);
}

//-- cut/copy/paste

/*
enum {
  BT_SEQUENCE_TARGET
} targets_id;

#define BT_SEQUENCE_TARGET_ID "application/x-buzztard-sequence"

static const GtkTargetEntry targets[] = 
{
  { BT_SEQUENCE_TARGET_ID, 0, BT_SEQUENCE_TARGET }
};

static int ntargets = G_N_ELEMENTS (targets);
*/

void bt_main_page_sequence_cut_selection(const BtMainPageSequence *self) {
  /* @todo implement me
   * - where do we store this?
   * - how do we store this?
   * - later we might have multiple songs open, and like to copy from one-to-another
   *   thus, we should not store the copy inside the song
   * - we can use GtkClipboard
  GtkClipboard *clipboard=gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
  // and where do we put out data in ?
  gtk_clipboard_set_with_data(clipboard, targets, ntargets, 
                gb_clipboard_get_cb, gb_clipboard_clear_cb, 
                user_data);
    */
}

void bt_main_page_sequence_copy_selection(const BtMainPageSequence *self) {
  /* @todo implement me */
}

void bt_main_page_sequence_paste_selection(const BtMainPageSequence *self) {
  /* @todo implement me */
}

void bt_main_page_sequence_delete_selection(const BtMainPageSequence *self) {
  GtkTreeModel *store;
  BtSong *song;
  BtSequence *sequence;
  glong selection_start_column,selection_start_row;
  glong selection_end_column,selection_end_row;
  
  if(self->priv->selection_start_column==-1) {
    selection_start_column=selection_end_column=self->priv->cursor_column;
    selection_start_row=selection_end_row=self->priv->cursor_row;
  }
  else {
    selection_start_column=self->priv->selection_start_column;
    selection_start_row=self->priv->selection_start_row;
    selection_end_column=self->priv->selection_end_column;
    selection_end_row=self->priv->selection_end_row;
  }

  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"sequence",&sequence,NULL);

  GST_INFO("delete sequence region: %3d,%3d -> %3d,%3d",selection_start_column,selection_start_row,selection_end_column,selection_end_row);

  if((store=sequence_model_get_store(self))) {
    GtkTreePath *path;
    
    if((path=gtk_tree_path_new_from_indices(selection_start_row,-1))) {
      GtkTreeIter iter;

      if(gtk_tree_model_get_iter(store,&iter,path)) {
        glong i,j;
        
        
        for(i=selection_start_row;i<=selection_end_row;i++) {
          for(j=selection_start_column-1;j<selection_end_column;j++) {
            GST_DEBUG("  delete sequence cell: %3d,%3d",j,i);
            bt_sequence_set_pattern(sequence,i,j,NULL);
            gtk_list_store_set(GTK_LIST_STORE(store),&iter,SEQUENCE_TABLE_PRE_CT+j," ",-1);
          }
          if(!gtk_tree_model_iter_next(store,&iter)) {
            if(j<self->priv->selection_end_column) {
              GST_WARNING("  can't get next tree-iter");
            }
            break;
          }
        }
      }
      else {
        GST_WARNING("  can't get tree-iter for row %d",selection_start_row);
      }
      gtk_tree_path_free(path);
    }
    else {
      GST_WARNING("  can't get tree-path");
    }
  }
  else {
    GST_WARNING("  can't get tree-model");
  }
  // reset selection
  self->priv->selection_start_column=self->priv->selection_start_row=self->priv->selection_end_column=self->priv->selection_end_row=-1;
  // release the references
  g_object_try_unref(sequence);
  g_object_try_unref(song);
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

  GST_DEBUG("!!!! self=%p",self);  
  g_object_try_weak_unref(self->priv->app);
  if(self->priv->machine) {
    GST_INFO("unref old cur-machine: refs: %d",(G_OBJECT(self->priv->machine))->ref_count);
    g_signal_handler_disconnect(G_OBJECT(self->priv->machine),self->priv->pattern_added_handler);
    g_signal_handler_disconnect(G_OBJECT(self->priv->machine),self->priv->pattern_removed_handler);
    g_object_unref(self->priv->machine);
  }
  
  gtk_object_destroy(GTK_OBJECT(self->priv->context_menu));
  gtk_object_destroy(GTK_OBJECT(self->priv->bars_menu));

  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_main_page_sequence_finalize(GObject *object) {
  //BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE(object);
  
  //GST_DEBUG("!!!! self=%p",self);  

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_main_page_sequence_init(GTypeInstance *instance, gpointer g_class) {
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE(instance);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_MAIN_PAGE_SEQUENCE, BtMainPageSequencePrivate);

  self->priv->bars=1;
  //self->priv->cursor_column=0;
  //self->priv->cursor_row=0;
  self->priv->selection_start_column=-1;
  self->priv->selection_start_row=-1;
  self->priv->selection_end_column=-1;
  self->priv->selection_end_row=-1;
}

static void bt_main_page_sequence_class_init(BtMainPageSequenceClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  column_index_quark=g_quark_from_static_string("BtMainPageSequence::column-index");

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtMainPageSequencePrivate));

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
  if (G_UNLIKELY(type == 0)) {
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
