/* $Id: main-page-sequence.c,v 1.10 2004-09-10 17:10:43 ensonic Exp $
 * class for the editor main machines page
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
  GtkOptionMenu *bars_menu;
  
  /* the pattern list */
  GtkTreeView *sequence_table;
  /* the pattern list */
  GtkTreeView *pattern_list;
  
  /* colors */
  GdkColor source_bg1,source_bg2;
  GdkColor processor_bg1,processor_bg2;
  GdkColor sink_bg1,sink_bg2;
};

enum {
  SEQUENCE_TABLE_SOURCE_BG=0,
  SEQUENCE_TABLE_PROCESSOR_BG,
  SEQUENCE_TABLE_SINK_BG,
  SEQUENCE_TABLE_POS,
  SEQUENCE_TABLE_LABEL,
  SEQUENCE_TABLE_PRE_CT
};

//-- event handler helper

/**
 * sequence_table_init:
 *
 * removes old columns and re-inserts the Pos and Label columns.
 */
static void sequence_table_init(const BtMainPageSequence *self) {
  GtkCellRenderer *renderer;
  GList *columns,*node;
  
  // remove columns
  if((columns=gtk_tree_view_get_columns(self->private->sequence_table))) {
    node=g_list_first(columns);
    while(node) {
      gtk_tree_view_remove_column(self->private->sequence_table,GTK_TREE_VIEW_COLUMN(node->data));
      node=g_list_next(node);
    }
    g_list_free(columns);
  }
  // re-add static columns
  renderer=gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer),"editable",FALSE,"xalign",1.0,NULL);
  gtk_tree_view_insert_column_with_attributes(self->private->sequence_table,-1,_("Pos."),renderer,
    "text",SEQUENCE_TABLE_POS,
    NULL);
  renderer=gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer),"editable",TRUE,"xalign",1.0,NULL);
  gtk_tree_view_insert_column_with_attributes(self->private->sequence_table,-1,_("Labels"),renderer,
    "text",SEQUENCE_TABLE_LABEL,
    NULL);
}

/**
 * sequence_table_refresh:
 * rebuild the sequence table after a structural change
 */
static void sequence_table_refresh(const BtMainPageSequence *self,const BtSong *song) {
  BtSetup *setup;
  BtSequence *sequence;
  BtSongInfo *song_info;
  BtMachine *machine;
  BtTimeLine *timeline;
  BtTimeLineTrack *timelinetrack;
  BtPattern *pattern;
  GtkCellRenderer *renderer;
  gchar *str;
  gulong i,j,col_ct,timeline_ct,track_ct,bars,pos=1;
  GtkListStore *store;
  GType *store_types;
  GtkTreeIter tree_iter;
  GtkTreeViewColumn *tree_col;
  GValue pattern_type={0,};

  GST_INFO("refresh sequence table");
  
  setup=bt_song_get_setup(song);
  sequence=bt_song_get_sequence(song);
  song_info=bt_song_get_song_info(song);

  // reset columns
  sequence_table_init(self);
  timeline_ct=bt_g_object_get_long_property(G_OBJECT(sequence),"length");
  track_ct=bt_g_object_get_long_property(G_OBJECT(sequence),"tracks");
  bars=bt_g_object_get_long_property(G_OBJECT(song_info),"bars");
  GST_INFO("  size is %2d,%2d",timeline_ct,track_ct);
  // add column for each machine
  for(j=0;j<track_ct;j++) {
    machine=bt_sequence_get_machine_by_track(sequence,j);
    str=bt_g_object_get_string_property(G_OBJECT(machine),"id");
    renderer=gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(renderer),"editable",TRUE,NULL);
    i=gtk_tree_view_insert_column_with_attributes(self->private->sequence_table,-1,str,renderer,
      "text",SEQUENCE_TABLE_PRE_CT+j,
      NULL);
    tree_col=gtk_tree_view_get_column(self->private->sequence_table,i-1);
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
  GST_DEBUG("  build model");
  col_ct=(SEQUENCE_TABLE_PRE_CT+track_ct);
  store_types=(GType *)g_new(GType *,col_ct);
  // for background color columns
  store_types[SEQUENCE_TABLE_SOURCE_BG   ]=GDK_TYPE_COLOR;
  store_types[SEQUENCE_TABLE_PROCESSOR_BG]=GDK_TYPE_COLOR;
  store_types[SEQUENCE_TABLE_SINK_BG     ]=GDK_TYPE_COLOR;
  // for static display columns
  store_types[SEQUENCE_TABLE_POS         ]=G_TYPE_LONG;
  // for track display columns
  for(i=SEQUENCE_TABLE_LABEL;i<col_ct;i++) {
    store_types[i]=G_TYPE_STRING;
  }
  store=gtk_list_store_newv(col_ct,store_types);
  // add patterns
  g_value_init(&pattern_type,BT_TYPE_TIMELINETRACK_TYPE);
  for(i=0;i<timeline_ct;i++) {
    timeline=bt_sequence_get_timeline_by_time(sequence,i);
    gtk_list_store_append(store, &tree_iter);
    // set colors
    if(i&1) {
      gtk_list_store_set(store,&tree_iter,
        SEQUENCE_TABLE_SOURCE_BG   ,&self->private->source_bg2,
        SEQUENCE_TABLE_PROCESSOR_BG,&self->private->processor_bg2,
        SEQUENCE_TABLE_SINK_BG     ,&self->private->sink_bg2,
          -1);
    }
    else {
      gtk_list_store_set(store,&tree_iter,
        SEQUENCE_TABLE_SOURCE_BG   ,&self->private->source_bg1,
        SEQUENCE_TABLE_PROCESSOR_BG,&self->private->processor_bg1,
        SEQUENCE_TABLE_SINK_BG     ,&self->private->sink_bg1,
          -1);
    }
    // set position
    gtk_list_store_set(store,&tree_iter,SEQUENCE_TABLE_POS,pos,-1);
    pos+=bars;
    // set label
    if((str=bt_g_object_get_string_property(G_OBJECT(timeline),"label"))) {
      gtk_list_store_set(store,&tree_iter,SEQUENCE_TABLE_LABEL,str,-1);
    }
    // set patterns
    for(j=0;j<track_ct;j++) {
      timelinetrack=bt_timeline_get_timelinetrack_by_index(timeline,j);
      g_object_get_property(G_OBJECT(timelinetrack),"type", &pattern_type);
      switch(g_value_get_enum(&pattern_type)) {
        case BT_TIMELINETRACK_TYPE_EMPTY:
          str=" ";
          break;
        case BT_TIMELINETRACK_TYPE_PATTERN:
          pattern=BT_PATTERN(bt_g_object_get_object_property(G_OBJECT(timelinetrack),"pattern"));
          str=bt_g_object_get_string_property(G_OBJECT(pattern),"name");
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
      GST_INFO("  %2d,%2d : adding \"%s\"",i,j,str);
      gtk_list_store_set(store,&tree_iter,SEQUENCE_TABLE_PRE_CT+j,str,-1);
    }
  }
  gtk_tree_view_set_model(self->private->sequence_table,GTK_TREE_MODEL(store));
  g_object_unref(store); // drop with treeview
}

/**
 * pattern_list_refresh:
 * When the user moves the cursor in the sequence, update the list of patterns
 * so that it shows the patterns that belong to the machine in the current
 * sequence row
 */
static void pattern_list_refresh(const BtMainPageSequence *self,const BtMachine *machine) {
  BtPattern *pattern=NULL;
  GtkListStore *store;
  GtkTreeIter tree_iter;
  gpointer *iter;
  gchar *str;

  GST_INFO("refresh pattern list");
  // @todo if machine is NULL add label columns
  
  store=gtk_list_store_new(1,G_TYPE_STRING);

  //-- append default rows
  gtk_list_store_append(store, &tree_iter);
  gtk_list_store_set(store,&tree_iter,0,_("  mute"),-1);
  gtk_list_store_append(store, &tree_iter);
  gtk_list_store_set(store,&tree_iter,0,_("  break"),-1);
  if(machine) {
    //-- append pattern rows
    iter=bt_machine_pattern_iterator_new(machine);
    while(iter) {
      pattern=bt_machine_pattern_iterator_get_pattern(iter);
      str=bt_g_object_get_string_property(G_OBJECT(pattern),"name");
      GST_INFO("  adding \"%s\"",str);
      gtk_list_store_append(store, &tree_iter);
      gtk_list_store_set(store,&tree_iter,0,str,-1);
      iter=bt_machine_pattern_iterator_next(iter);
    }
  }
  gtk_tree_view_set_model(self->private->pattern_list,GTK_TREE_MODEL(store));
  g_object_unref(store); // drop with treeview
}

//-- event handler

void on_sequence_table_cursor_changed(GtkTreeView *treeview, gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);

  GST_INFO("sequence_table cursor has changed : treeview=%p, page=%p",treeview,user_data);
  pattern_list_refresh(self,bt_main_page_sequence_get_current_machine(self));
}

gboolean on_sequence_table_cursor_moved(GtkTreeView *treeview, GtkMovementStep arg1, gint arg2, gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);

  GST_INFO("sequence_table cursor has moved : treeview=%p, page=%p, arg1=%d, arg2=%d",treeview,user_data,arg1,arg2);
  if(arg1==GTK_MOVEMENT_VISUAL_POSITIONS) {
    pattern_list_refresh(self,bt_main_page_sequence_get_current_machine(self));
  }
}

static void on_song_changed(const BtEditApplication *app, gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  BtSong *song;
  glong index,bars;

  GST_INFO("song has changed : song=%p, page=%p",song,user_data);
  // get song from app and then setup from song
  song=BT_SONG(bt_g_object_get_object_property(G_OBJECT(self->private->app),"song"));
  // update page
  // update toolbar
  bars=bt_g_object_get_long_property(G_OBJECT(bt_song_get_song_info(song)),"bars");
  // find out to which entry it belongs and set the index
  // 1 -> 0, 2 -> 1, 4 -> 2 , 8 -> 3
  if(bars<4) {
    index=bars-1;
  }
  else {
    index=1+(bars>>2);
  }
  //GST_INFO("  bars=%d, index=%d",bars,index);
  gtk_option_menu_set_history(GTK_OPTION_MENU(self->private->bars_menu),index);
  // update sequence and pattern list
  sequence_table_refresh(self,song);
  pattern_list_refresh(self,bt_main_page_sequence_get_current_machine(self));
}

//-- helper methods

static gboolean bt_main_page_sequence_init_ui(const BtMainPageSequence *self, const BtEditApplication *app) {
  GtkWidget *toolbar;
  GtkWidget *box,*menu,*menu_item,*button,*scrolled_window;
  GtkCellRenderer *renderer;
  GdkColormap *colormap;
  glong i;
  gchar str[4];

  // add toolbar
  toolbar=gtk_toolbar_new();
  gtk_widget_set_name(toolbar,_("machine view tool bar"));
  gtk_box_pack_start(GTK_BOX(self),toolbar,FALSE,FALSE,0);
  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar),GTK_TOOLBAR_BOTH);
  // add toolbar widgets
  // steps
  box=gtk_hbox_new(FALSE,2);
  gtk_container_set_border_width(GTK_CONTAINER(box),4);
  // build list of values for the menu
  menu=gtk_menu_new();
  sprintf(str,"1");gtk_menu_shell_append(GTK_MENU_SHELL(menu),gtk_menu_item_new_with_label(str));
  sprintf(str,"2");gtk_menu_shell_append(GTK_MENU_SHELL(menu),gtk_menu_item_new_with_label(str));
  for(i=4;i<=64;i+=4) {
    sprintf(str,"%d",i);
    menu_item=gtk_menu_item_new_with_label(str);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
    gtk_widget_show(menu_item);
  }
  self->private->bars_menu=GTK_OPTION_MENU(gtk_option_menu_new());
  gtk_option_menu_set_menu(self->private->bars_menu,menu);
  
  // @todo do we really have to add the label by our self
  gtk_box_pack_start(GTK_BOX(box),gtk_label_new(_("Steps")),FALSE,FALSE,2);
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->private->bars_menu),TRUE,TRUE,2);
  
  button=gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
                                GTK_TOOLBAR_CHILD_WIDGET,
                                box,
                                NULL,
                                NULL,NULL,
                                NULL,NULL,NULL);
  //gtk_label_set_use_underline(GTK_LABEL(((GtkToolbarChild*)(g_list_last(GTK_TOOLBAR(toolbar)->children)->data))->label),TRUE);
  gtk_widget_set_name(button,_("Steps"));

  // allocate out colors
  colormap=gdk_colormap_get_system();
  self->private->source_bg1.red=  (guint16)(1.0*65535);
  self->private->source_bg1.green=(guint16)(0.9*65535);
  self->private->source_bg1.blue= (guint16)(0.9*65535);
  gdk_colormap_alloc_color(colormap,&self->private->source_bg1,FALSE,TRUE);
  self->private->source_bg2.red=  (guint16)(1.0*65535);
  self->private->source_bg2.green=(guint16)(0.8*65535);
  self->private->source_bg2.blue= (guint16)(0.8*65535);
  gdk_colormap_alloc_color(colormap,&self->private->source_bg2,FALSE,TRUE);
  self->private->processor_bg1.red=  (guint16)(0.9*65535);
  self->private->processor_bg1.green=(guint16)(1.0*65535);
  self->private->processor_bg1.blue= (guint16)(0.9*65535);
  gdk_colormap_alloc_color(colormap,&self->private->processor_bg1,FALSE,TRUE);
  self->private->processor_bg2.red=  (guint16)(0.8*65535);
  self->private->processor_bg2.green=(guint16)(1.0*65535);
  self->private->processor_bg2.blue= (guint16)(0.8*65535);
  gdk_colormap_alloc_color(colormap,&self->private->processor_bg2,FALSE,TRUE);
  self->private->sink_bg1.red=  (guint16)(0.9*65535);
  self->private->sink_bg1.green=(guint16)(0.9*65535);
  self->private->sink_bg1.blue= (guint16)(1.0*65535);
  gdk_colormap_alloc_color(colormap,&self->private->sink_bg1,FALSE,TRUE);
  self->private->sink_bg2.red=  (guint16)(0.8*65535);
  self->private->sink_bg2.green=(guint16)(0.8*65535);
  self->private->sink_bg2.blue= (guint16)(1.0*65535);
  gdk_colormap_alloc_color(colormap,&self->private->sink_bg2,FALSE,TRUE);
  
  // add a hbox
  box=gtk_hpaned_new();
  gtk_container_add(GTK_CONTAINER(self),box);
  // add sequence list-view
  scrolled_window=gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_ETCHED_IN);
  self->private->sequence_table=GTK_TREE_VIEW(gtk_tree_view_new());
  gtk_tree_view_set_rules_hint(self->private->sequence_table,TRUE);
  gtk_tree_selection_set_mode(gtk_tree_view_get_selection(self->private->sequence_table),GTK_SELECTION_BROWSE);
  sequence_table_init(self);
  //gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window),GTK_WIDGET(self->private->sequence_table));
  gtk_container_add(GTK_CONTAINER(scrolled_window),GTK_WIDGET(self->private->sequence_table));
  gtk_paned_pack1(GTK_PANED(box),GTK_WIDGET(scrolled_window),TRUE,TRUE);
  // add pattern list-view
  scrolled_window=gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_ETCHED_IN);
  self->private->pattern_list=GTK_TREE_VIEW(gtk_tree_view_new());
  renderer=gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(self->private->pattern_list,-1,_("Patterns"),renderer,"text",0,NULL);
  //gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window),GTK_WIDGET(self->private->pattern_list));
  gtk_container_add(GTK_CONTAINER(scrolled_window),GTK_WIDGET(self->private->pattern_list));
  gtk_paned_pack2(GTK_PANED(box),GTK_WIDGET(scrolled_window),FALSE,FALSE);

  // register event handlers
  g_signal_connect(G_OBJECT(self->private->sequence_table), "move-cursor", (GCallback)on_sequence_table_cursor_moved, (gpointer)self);
  g_signal_connect(G_OBJECT(self->private->sequence_table), "cursor-changed", (GCallback)on_sequence_table_cursor_changed, (gpointer)self);
  g_signal_connect(G_OBJECT(app), "song-changed", (GCallback)on_song_changed, (gpointer)self);
  return(TRUE);
}

//-- constructor methods

/**
 * bt_main_page_sequence_new:
 * @app: the application the window belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtMainPageSequence *bt_main_page_sequence_new(const BtEditApplication *app) {
  BtMainPageSequence *self;

  if(!(self=BT_MAIN_PAGE_SEQUENCE(g_object_new(BT_TYPE_MAIN_PAGE_SEQUENCE,"app",app,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_main_page_sequence_init_ui(self,app)) {
    goto Error;
  }
  return(self);
Error:
  if(self) g_object_unref(self);
  return(NULL);
}

//-- methods

/**
 * bt_main_page_sequence_get_current_machine:
 * @self: the pattern subpage
 *
 * Get the currently active #BtMachine as determined by the cursor position in
 * the sequence table.
 *
 * Returns: the #BtMachine instance or NULL in case of an error
 */
BtMachine *bt_main_page_sequence_get_current_machine(const BtMainPageSequence *self) {
  BtMachine *machine=NULL;
  BtSong *song;
  BtSequence *sequence;
  //GtkTreeSelection *selection;
  GtkTreePath *path;
  GtkTreeViewColumn *column;

  GST_INFO("get active machine");
  
  song=BT_SONG(bt_g_object_get_object_property(G_OBJECT(self->private->app),"song"));
  sequence=bt_song_get_sequence(song);

  // get table column number (column 0 is for labels)
  gtk_tree_view_get_cursor(self->private->sequence_table,&path,&column);
  if(column) {
    GList *columns=gtk_tree_view_get_columns(self->private->sequence_table);
    glong track=g_list_index(columns,(gpointer)column);

    g_list_free(columns);
    GST_INFO("  active track is %d",track);
    if(track) {
      machine=bt_sequence_get_machine_by_track(sequence,track-1);
    }
  }
  return(machine);
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
      g_value_set_object(value, G_OBJECT(self->private->app));
    } break;
    default: {
 			g_assert(FALSE);
      break;
    }
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
      self->private->app = g_object_ref(G_OBJECT(g_value_get_object(value)));
      //GST_DEBUG("set the app for MAIN_PAGE_SEQUENCE: %p",self->private->app);
    } break;
    default: {
			g_assert(FALSE);
      break;
    }
  }
}

static void bt_main_page_sequence_dispose(GObject *object) {
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE(object);
	return_if_disposed();
  self->private->dispose_has_run = TRUE;
}

static void bt_main_page_sequence_finalize(GObject *object) {
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE(object);
  
  g_object_unref(G_OBJECT(self->private->app));
  g_free(self->private);
}

static void bt_main_page_sequence_init(GTypeInstance *instance, gpointer g_class) {
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE(instance);
  self->private = g_new0(BtMainPageSequencePrivate,1);
  self->private->dispose_has_run = FALSE;
}

static void bt_main_page_sequence_class_init(BtMainPageSequenceClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;
  
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
      sizeof (BtMainPageSequenceClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_main_page_sequence_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtMainPageSequence),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_main_page_sequence_init, // instance_init
    };
		type = g_type_register_static(GTK_TYPE_VBOX,"BtMainPageSequence",&info,0);
  }
  return type;
}

