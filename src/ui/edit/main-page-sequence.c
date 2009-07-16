/* $Id$
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/**
 * SECTION:btmainpagesequence
 * @short_description: the editor main sequence page
 * @see_also: #BtSequence, #BtSequenceView
 *
 * Provides an editor for #BtSequence instances.
 */

/* @todo: main-page-sequence tasks
 * - cut/copy/paste
 * - add third view for eating remaining space
 *   - or block cursor moving there
 * - shortcuts
 *   - Ctrl-<num> :  Stepping
 *     - set increment for cursor-down on edit
 *   - Duplicate pattern: make a copy of pattern under cursor and go to pattern view
 *   - New pattern: open new pattern dialog, insert pattern under cursor and go to pattern view
 * - sequence header
 *   - add the same context menu as the machines have in machine view when
 *     clicking on track headers
 *   - allow to switch meters (off, level, scope, spectrum)
 * - when we move between tracks, switch the current-machine in pattern-view
 * - pattern list
 *   - go to next occurence when double clicking a pattern
 *   - show tick-length in pattern list
 * @todo: we should have a track-changed signal
 *  - allowd patternd to sync with selected machine and not passively syncing
 *    (bt_main_page_patterns_show_machine())
 * @idea: add a follow playback checkbox to toolbar to en/disable sequence scrolling
 *   - the scrolling causes quite some repaints and thus slowness
 *   - it would be good if we could deoouple the scolling and the events, so
 *     that we e.g. scroll 10 times a second to the latest position (same for level meters)
 * @idea: bold row,label for cursor row
 *   - makes it easier to follow position in wide sequences
 *     (same needed for pattern view)
 *
 * @bugs
 * - keyboard movement is broken: http://bugzilla.gnome.org/show_bug.cgi?id=371756
 * - hovering the mouse over the treeview causes rebraws for the whole lines
 *   - cells are asked to do prelight, even if they wouldn't draw anything else
 *   http://www.gtk.org/plan/meetings/20041025.txt
 */

#define BT_EDIT
#define BT_MAIN_PAGE_SEQUENCE_C

#include "bt-edit.h"
#include "gtkvumeter.h"

enum {
  MAIN_PAGE_SEQUENCE_APP=1,
  MAIN_PAGE_SEQUENCE_CURSOR_ROW
};

struct _BtMainPageSequencePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  G_POINTER_ALIAS(BtEditApplication *,app);
  /* the sequence we are showing */
  BtSequence *sequence;
  /* machine for current column */
  BtMachine *machine;

  /* bars selection menu */
  GtkComboBox *bars_menu;
  gulong bars;

  /* label selection menu */
  GtkComboBox *label_menu;

  /* pos unit selection menu */
  GtkComboBox *pos_menu;
  guint pos_format; // @todo: make enum

  /* the sequence table */
  GtkHBox *sequence_pos_table_header;
  GtkTreeView *sequence_pos_table;
  GtkHBox *sequence_table_header;
  GtkTreeView *sequence_table;
  /* the pattern list */
  GtkTreeView *pattern_list;

  /* position-table header label widget */
  GtkWidget *pos_header;

  /* local commands */
  GtkAccelGroup *accel_group;

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

  /* shortcut table */
  const char *pattern_keys;

  /* vumeter data */
  GHashTable *level_to_vumeter;
  GstClock *clock;

  /* step filtering */
  gulong sequence_length;     /* number of [dummy] rows contained in the model */
  glong row_filter_pos;   /* the number of visible (not-filtered) rows */

  /* signal handler id's */
  gulong pattern_added_handler, pattern_removed_handler;

  /* playback state */
  gboolean is_playing;
  
  /* lock for multithreaded access */
  GMutex        *lock;
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
  SEQUENCE_TABLE_POSSTR,
  SEQUENCE_TABLE_LABEL,
  SEQUENCE_TABLE_PRE_CT
};

enum {
  POSITION_MENU_POS=0,
  POSITION_MENU_POSSTR,
  POSITION_MENU_LABEL
};

enum {
  PATTERN_TABLE_KEY=0,
  PATTERN_TABLE_NAME,
  PATTERN_TABLE_USED
};

enum {
  PATTERN_POS_FORMAT_TICKS=0,
  PATTERN_POS_FORMAT_TIME
};

enum {
  SEQUENCE_VIEW_POS_PLAY=0,
  SEQUENCE_VIEW_POS_LOOP_START,
  SEQUENCE_VIEW_POS_LOOP_END
};

// this only works for 4/4 meassure
//#define IS_SEQUENCE_POS_VISIBLE(pos,bars) ((pos&((bars)-1))==0)
#define IS_SEQUENCE_POS_VISIBLE(pos,bars) ((pos%bars)==0)
#define SEQUENCE_CELL_WIDTH 100
#define SEQUENCE_CELL_HEIGHT 28
#define SEQUENCE_CELL_XPAD 0
#define SEQUENCE_CELL_YPAD 0
#define POSITION_CELL_WIDTH 65
#define HEADER_SPACING 2

#define LOW_VUMETER_VAL -60.0

// when setting the HEIGHT for one column, then the focus rect is visible for
// the other (smaller) columns

// keyboard shortcuts for sequence-table
// CLEAR       '.'
// MUTE        '-'
// BREAK       ','
// SOLO/BYPASS '_'
static const gchar sink_pattern_keys[]     = "-,0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const gchar source_pattern_keys[]   ="-,_0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const gchar processor_pattern_keys[]="-,_0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

static GQuark column_index_quark=0;

static void sequence_table_refresh_labels(const BtMainPageSequence *self);
static void sequence_table_refresh(const BtMainPageSequence *self,const BtSong *song);

static void on_track_add_activated(GtkMenuItem *menuitem, gpointer user_data);
static void on_pattern_changed(BtMachine *machine,BtPattern *pattern,gpointer user_data);

//-- tree filter func

static gboolean step_visible_filter(GtkTreeModel *store,GtkTreeIter *iter,gpointer user_data) {
  //gboolean visible=TRUE;
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  gulong pos;

  // determine row number and hide or show accordingly
  gtk_tree_model_get(store,iter,SEQUENCE_TABLE_POS,&pos,-1);

  if( pos < self->priv->row_filter_pos && IS_SEQUENCE_POS_VISIBLE(pos,self->priv->bars))
    return TRUE;
  else
    return FALSE;

  // determine row number and hide or show accordingly
  //gtk_tree_model_get(store,iter,SEQUENCE_TABLE_POS,&pos,-1);
  //visible=IS_SEQUENCE_POS_VISIBLE(pos,self->priv->bars);
  //GST_INFO("bars=%d, pos=%d, -> visible=%1d",self->priv->bars,pos,visible);

  //return(IS_SEQUENCE_POS_VISIBLE(pos,self->priv->bars));
}

//-- tree cell data functions

static void label_cell_data_function(GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  gulong row;
  GdkColor *bg_col=NULL;

  gtk_tree_model_get(model,iter,
    SEQUENCE_TABLE_POS,&row,
    -1);

  if((0==self->priv->cursor_column) && (row==self->priv->cursor_row)) {
    bg_col=self->priv->cursor_bg;
  }
  else if((0>=self->priv->selection_start_column) && (0<=self->priv->selection_end_column) &&
    (row>=self->priv->selection_start_row) && (row<=self->priv->selection_end_row)
  ) {
    bg_col=((row/self->priv->bars)&1)?self->priv->selection_bg2:self->priv->selection_bg1;
  }
  if(bg_col) {
    g_object_set(G_OBJECT(renderer),
      "background-gdk",bg_col,
      "background-set",TRUE,
      NULL);
  }
  else {
    g_object_set(G_OBJECT(renderer),
      "background-set",FALSE,
      NULL);
  }
}


static void source_machine_cell_data_function(GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  gulong row,column;
  GdkColor *bg_col,*def_bg_col;
  gchar *str=NULL;

  column=1+GPOINTER_TO_UINT(g_object_get_qdata(G_OBJECT(col),column_index_quark));

  gtk_tree_model_get(model,iter,
    SEQUENCE_TABLE_POS,&row,
    SEQUENCE_TABLE_SOURCE_BG,&def_bg_col,
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
    bg_col=(def_bg_col->pixel==self->priv->source_bg1->pixel)?self->priv->selection_bg1:self->priv->selection_bg2;
  }
  else {
    bg_col=def_bg_col;
  }
  g_object_set(G_OBJECT(renderer),
    "background-gdk",bg_col,
    "text",str,
     NULL);
  if(def_bg_col) gdk_color_free(def_bg_col);
  g_free(str);
}

static void processor_machine_cell_data_function(GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  gulong row,column;
  GdkColor *bg_col,*def_bg_col;
  gchar *str=NULL;

  column=1+GPOINTER_TO_UINT(g_object_get_qdata(G_OBJECT(col),column_index_quark));

  gtk_tree_model_get(model,iter,
    SEQUENCE_TABLE_POS,&row,
    SEQUENCE_TABLE_PROCESSOR_BG,&def_bg_col,
    SEQUENCE_TABLE_LABEL+column,&str,
    -1);

  if((column==self->priv->cursor_column) && (row==self->priv->cursor_row)) {
    bg_col=self->priv->cursor_bg;
  }
  else if((column>=self->priv->selection_start_column) && (column<=self->priv->selection_end_column) &&
    (row>=self->priv->selection_start_row) && (row<=self->priv->selection_end_row)
  ) {
    bg_col=(def_bg_col->pixel==self->priv->processor_bg1->pixel)?self->priv->selection_bg1:self->priv->selection_bg2;
  }
  else {
    bg_col=def_bg_col;
  }
  g_object_set(G_OBJECT(renderer),
    "background-gdk",bg_col,
    "text",str,
     NULL);
  if(def_bg_col) gdk_color_free(def_bg_col);
  g_free(str);
}

static void sink_machine_cell_data_function(GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  gulong row,column;
  GdkColor *bg_col,*def_bg_col;
  gchar *str=NULL;

  column=1+GPOINTER_TO_UINT(g_object_get_qdata(G_OBJECT(col),column_index_quark));

  gtk_tree_model_get(model,iter,
    SEQUENCE_TABLE_POS,&row,
    SEQUENCE_TABLE_SINK_BG,&def_bg_col,
    SEQUENCE_TABLE_LABEL+column,&str,
    -1);

  if((column==self->priv->cursor_column) && (row==self->priv->cursor_row)) {
    bg_col=self->priv->cursor_bg;
  }
  else if((column>=self->priv->selection_start_column) && (column<=self->priv->selection_end_column) &&
    (row>=self->priv->selection_start_row) && (row<=self->priv->selection_end_row)
  ) {
    bg_col=(def_bg_col->pixel==self->priv->sink_bg1->pixel)?self->priv->selection_bg1:self->priv->selection_bg2;
  }
  else {
    bg_col=def_bg_col;
  }
  g_object_set(G_OBJECT(renderer),
    "background-gdk",bg_col,
    "text",str,
     NULL);
  if(def_bg_col) gdk_color_free(def_bg_col);
  g_free(str);
}

//-- tree model helper

static gboolean sequence_view_get_cursor_pos(GtkTreeView *tree_view,GtkTreePath *path,GtkTreeViewColumn *column,gulong *col,gulong *row) {
  gboolean res=FALSE;
  GtkTreeModel *store;
  GtkTreeModelFilter *filtered_store;
  GtkTreeIter iter,filter_iter;

  g_return_val_if_fail(path,FALSE);

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
    else {
      GST_INFO("No iter for path");
    }
  }
  else {
    GST_WARNING("Can't get tree-model");
  }
  return(res);
}

static gboolean sequence_view_set_cursor_pos(const BtMainPageSequence *self) {
  GtkTreePath *path;
  gboolean res=FALSE;
  
  // @todo: http://bugzilla.gnome.org/show_bug.cgi?id=498010
  if(!GTK_IS_TREE_VIEW(self->priv->sequence_table) || !gtk_tree_view_get_model(self->priv->sequence_table)) return(FALSE);

  if((path=gtk_tree_path_new_from_indices((self->priv->cursor_row/self->priv->bars),-1))) {
    GList *columns;
    if((columns=gtk_tree_view_get_columns(self->priv->sequence_table))) {
      GtkTreeViewColumn *column=g_list_nth_data(columns,self->priv->cursor_column);
      // set cell focus
      gtk_tree_view_set_cursor(self->priv->sequence_table,path,column,FALSE);

      res=TRUE;
      g_list_free(columns);
    }
    else {
      GST_WARNING("Can't get columns for pos %ld:%ld",self->priv->cursor_row,self->priv->cursor_column);
    }
    gtk_tree_path_free(path);
  }
  else {
    GST_WARNING("Can't create treepath for pos %ld:%ld",self->priv->cursor_row,self->priv->cursor_column);
  }
  gtk_widget_grab_focus_savely(GTK_WIDGET(self->priv->sequence_table));
  return res;
}

/*
 * sequence_view_get_current_pos:
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
static gboolean sequence_view_get_current_pos(const BtMainPageSequence *self,gulong *time,gulong *track) {
  gboolean res=FALSE;
  GtkTreePath *path;
  GtkTreeViewColumn *column;

  //GST_INFO("get active sequence cell");

  gtk_tree_view_get_cursor(self->priv->sequence_table,&path,&column);
  if(column && path) {
    res=sequence_view_get_cursor_pos(self->priv->sequence_table,path,column,track,time);
  }
  else {
    GST_INFO("No cursor pos, column=%p, path=%p",column,path);
  }
  if(path) gtk_tree_path_free(path);
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
 * @self: the sequence subpage
 *
 * Apply alternate coloring for visible rows
 */
static void sequence_model_recolorize(const BtMainPageSequence *self) {
  GtkTreeModel *store;
  GtkTreeIter iter;
  gboolean odd_row=FALSE;
  gulong filter_pos;

  GST_INFO("recolorize sequence tree view");

  if((store=sequence_model_get_store(self))) {
    if(gtk_tree_model_get_iter_first(store,&iter)) {
      filter_pos=self->priv->row_filter_pos;
      self->priv->row_filter_pos=self->priv->sequence_length;
      do {
        if(step_visible_filter(store,&iter,(gpointer)self)) {
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
        }
      } while(gtk_tree_model_iter_next(store,&iter));
      self->priv->row_filter_pos=filter_pos;
    }
  }
  else {
    GST_WARNING("can't get tree model");
  }
}

static void sequence_calculate_visible_lines(const BtMainPageSequence *self) {
  gulong visible_rows,sequence_length;

  g_object_get(self->priv->sequence,"length",&sequence_length,NULL);

  visible_rows=sequence_length/self->priv->bars;
  GST_INFO("visible_rows=%lu = %lu / %lu",visible_rows,sequence_length,self->priv->bars);
  g_object_set(self->priv->sequence_table,"visible-rows",visible_rows,NULL);
  g_object_set(self->priv->sequence_pos_table,"visible-rows",visible_rows,NULL);
}

static gchar *sequence_format_positions(const BtMainPageSequence *self,gulong pos) {
  static gchar pos_str[20];

  switch(self->priv->pos_format) {
    case PATTERN_POS_FORMAT_TICKS:
      g_snprintf(pos_str,5,"%lu",pos);
      break;
    case PATTERN_POS_FORMAT_TIME: {
      gulong msec,sec,min;
      const GstClockTime bar_time=bt_sequence_get_bar_time(self->priv->sequence);

      // update current statusbar
      msec=(gulong)((pos*bar_time)/G_USEC_PER_SEC);
      min=(gulong)(msec/60000);msec-=(min*60000);
      sec=(gulong)(msec/ 1000);msec-=(sec* 1000);
      // format
      g_sprintf(pos_str,"%02lu:%02lu.%03lu",min,sec,msec);
    } break;
    default:
      *pos_str='\0';
      GST_WARNING("unimplemented time format %d",self->priv->pos_format);
  }
  return(pos_str);
}

//-- gtk helpers

static void widget_shade_bg_color(GtkWidget *widget,GtkStateType state,gfloat rf,gfloat gf,gfloat bf) {
  GtkStyle *style=gtk_widget_get_style(widget);
  GdkColor color=style->bg[state];
  gfloat c;

  c=((gfloat)color.red*rf);
  color.red=(guint16)MIN(c,65535.0);
  c=((gfloat)color.green*gf);
  color.green=(guint16)MIN(c,65535.0);
  c=((gfloat)color.blue*bf);
  color.blue=(guint16)MIN(c,65535.0);
  gtk_widget_modify_bg(widget,state,&color);

}

static GtkWidget* make_mini_button(const gchar *txt,gfloat rf,gfloat gf,gfloat bf, gboolean toggled) {
  GtkWidget *button;

// the font get smaller, but the buttons don't :/
#define USE_MARKUP 0
#if USE_MARKUP
  GtkWidget *label;
  button=gtk_toggle_button_new_with_label("");
  label=gtk_bin_get_child(GTK_BIN(button));
  if(GTK_IS_LABEL(label)) {
    gchar *str=g_strconcat("<small>",txt,"</small>",NULL);
    gtk_label_set_markup (GTK_LABEL (label),str);
    g_free(str);
  }
  else {
    GST_WARNING("expecting a GtkLabel as a first child");
  }
#else
  button=gtk_toggle_button_new_with_label(txt);
#endif
  widget_shade_bg_color(button,GTK_STATE_ACTIVE  ,rf,gf,bf);
  widget_shade_bg_color(button,GTK_STATE_PRELIGHT,rf,gf,bf);
  gtk_container_set_border_width(GTK_CONTAINER(button),0);
  if(toggled)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),toggled);

  return(button);
}

//-- event handlers

static gboolean on_page_switched_idle(gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);

  //if(GTK_WIDGET_REALIZED(self->priv->sequence_table)) { 
    // do we need to set the cursor here?
    sequence_view_set_cursor_pos(self);
  //}
  return(FALSE);
}

static void on_page_switched(GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  BtMainWindow *main_window;
  static gint prev_page_num=-1;

  if(page_num==BT_MAIN_PAGES_SEQUENCE_PAGE) {
    // only do this if the page really has changed
    if(prev_page_num != BT_MAIN_PAGES_SEQUENCE_PAGE) {
      GST_DEBUG("enter sequence page");
      // add local commands
      g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
      if(main_window) {
        gtk_window_add_accel_group(GTK_WINDOW(main_window),self->priv->accel_group);
#if !GTK_CHECK_VERSION(2,12,0)
        // workaround for http://bugzilla.gnome.org/show_bug.cgi?id=469374
        g_signal_emit_by_name (main_window, "keys-changed", 0);
#endif
        g_object_unref(main_window);
      }
      // delay the sequence_table grab
      g_idle_add_full(G_PRIORITY_HIGH_IDLE,on_page_switched_idle,user_data,NULL);
    }
  }
  else {
    // only do this if the page was BT_MAIN_PAGES_SEQUENCE_PAGE
    if(prev_page_num == BT_MAIN_PAGES_SEQUENCE_PAGE) {
      GST_DEBUG("leave sequence page");
      // remove local commands
      g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
      if(main_window) {
        gtk_window_remove_accel_group(GTK_WINDOW(main_window),self->priv->accel_group);
        g_object_unref(main_window);
      }
    }
  }
  prev_page_num = page_num;
}

static void on_machine_id_changed(BtMachine *machine,GParamSpec *arg,gpointer user_data) {
  GtkLabel *label=GTK_LABEL(user_data);
  gchar *str;

  g_object_get(G_OBJECT(machine),"id",&str,NULL);
  GST_INFO("machine id changed to \"%s\"",str);
  gtk_label_set_text(label,str);
  g_free(str);
}

static void on_machine_id_changed_seq(BtMachine *machine,GParamSpec *arg,gpointer user_data) {
  on_machine_id_changed(machine,arg,user_data);
}

static void on_machine_id_changed_menu(BtMachine *machine,GParamSpec *arg,gpointer user_data) {
  on_machine_id_changed(machine,arg,user_data);
}

/*
 * on_header_size_allocate:
 *
 * Adjusts the height of the header widget of the first treeview (pos) to the
 * height of the second treeview.
 */
static void on_header_size_allocate(GtkWidget *widget,GtkAllocation *allocation,gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);

  GST_DEBUG("#### header label size %d x %d",allocation->width,allocation->height);

  gtk_widget_set_size_request(self->priv->pos_header,-1,allocation->height);
}

/* DEBUG
static void on_sequence_header_size_allocate(GtkWidget *widget,GtkAllocation *allocation,gpointer user_data) {
  GtkRequisition requisition;
  
  gtk_widget_size_request(widget,&requisition);
  GST_WARNING("#### header %s alloc:  %d x %d, req: %d x %d",
    (gchar *)user_data,
    allocation->width,allocation->height,
    requisition.width,requisition.height
    );
}
// DEBUG */

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

static gboolean on_delayed_idle_track_level_change(gpointer user_data) {
  gconstpointer * const params=(gconstpointer *)user_data;
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(params[0]);
  GstMessage *message=(GstMessage *)params[1];
  
  if(self) {
    GtkVUMeter *vumeter;

    g_mutex_lock(self->priv->lock);
    g_object_remove_weak_pointer(G_OBJECT(self),(gpointer *)&params[0]);
    g_mutex_unlock(self->priv->lock);

    if(!self->priv->is_playing)
      goto done;

    if((vumeter=g_hash_table_lookup(self->priv->level_to_vumeter,GST_MESSAGE_SRC(message)))) {
      const GstStructure *structure=gst_message_get_structure(message);
      const GValue *l_cur,*l_peak;
      gdouble cur=0.0, peak=0.0;
      guint i,size;
  
      //l_cur=(GValue *)gst_structure_get_value(structure, "rms");
      l_cur=(GValue *)gst_structure_get_value(structure, "peak");
      //l_peak=(GValue *)gst_structure_get_value(structure, "peak");
      l_peak=(GValue *)gst_structure_get_value(structure, "decay");
      size=gst_value_list_get_size(l_cur);
      for(i=0;i<size;i++) {
        cur+=g_value_get_double(gst_value_list_get_value(l_cur,i));
        peak+=g_value_get_double(gst_value_list_get_value(l_peak,i));
      }
      if(isinf(cur) || isnan(cur)) cur=LOW_VUMETER_VAL;
      else cur/=size;
      if(isinf(peak) || isnan(peak)) peak=LOW_VUMETER_VAL;
      else peak/=size;  
  
      //gtk_vumeter_set_levels(vumeter, (gint)cur, (gint)peak);
      gtk_vumeter_set_levels(vumeter, (gint)peak, (gint)cur);
    }
  }
done:
  gst_message_unref(message);
  g_free(params);
  return(FALSE);
}

static gboolean on_delayed_track_level_change(GstClock *clock,GstClockTime time,GstClockID id,gpointer user_data) {
  // the callback is called froma clock thread
  if(GST_CLOCK_TIME_IS_VALID(time))
    g_idle_add(on_delayed_idle_track_level_change,user_data);
  else
    g_free(user_data);
  return(TRUE);
}

static void on_track_level_change(GstBus * bus, GstMessage * message, gpointer user_data) {
  const GstStructure *structure=gst_message_get_structure(message);
  const gchar *name = gst_structure_get_name(structure);

  if(!strcmp(name,"level")) {
    BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
    GstElement *level=GST_ELEMENT(GST_MESSAGE_SRC(message));
    
    // check if its our element (we can have multiple level meters)
    if((g_hash_table_lookup(self->priv->level_to_vumeter,level))) {
      GstClockTime timestamp, duration;
      GstClockTime waittime=GST_CLOCK_TIME_NONE;
  
      if(gst_structure_get_clock_time (structure, "running-time", &timestamp) &&
        gst_structure_get_clock_time (structure, "duration", &duration)) {
        /* wait for middle of buffer */
        waittime=timestamp+duration/2;
      }
      else if(gst_structure_get_clock_time (structure, "endtime", &timestamp)) {
        /* level send endtime as stream_time and not as running_time */
        waittime=gst_segment_to_running_time(&GST_BASE_TRANSFORM(level)->segment, GST_FORMAT_TIME, timestamp);
      }
      if(GST_CLOCK_TIME_IS_VALID(waittime)) {
        gconstpointer *params=g_new(gconstpointer,2);
        GstClockID clock_id;
        GstClockTime basetime=gst_element_get_base_time(level);
  
        //GST_WARNING("target %"GST_TIME_FORMAT" %"GST_TIME_FORMAT, GST_TIME_ARGS(timestamp),GST_TIME_ARGS(waittime));
      
        params[0]=(gpointer)self;
        params[1]=(gpointer)gst_message_ref(message);
        g_mutex_lock(self->priv->lock);
        g_object_add_weak_pointer(G_OBJECT(self),(gpointer *)&params[0]);
        g_mutex_unlock(self->priv->lock);
        clock_id=gst_clock_new_single_shot_id(self->priv->clock,waittime+basetime);
        gst_clock_id_wait_async(clock_id,on_delayed_track_level_change,(gpointer)params);
        gst_clock_id_unref(clock_id);
      }
    }
  }
}

static void on_sequence_label_edited(GtkCellRendererText *cellrenderertext,gchar *path_string,gchar *new_text,gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  GtkTreeModelFilter *filtered_store;
  GtkTreeModel *store;
  gulong pos;
  gchar *old_text;

  GST_INFO("label edited: '%s': '%s'",path_string,new_text);

  if((filtered_store=GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(self->priv->sequence_table))) &&
    (store=gtk_tree_model_filter_get_model(filtered_store))
  ) {
    GtkTreeIter iter,filter_iter;

    if(gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(filtered_store),&filter_iter,path_string)) {
      gboolean changed=FALSE;

      gtk_tree_model_filter_convert_iter_to_child_iter(filtered_store,&iter,&filter_iter);

      gtk_tree_model_get(store,&iter,SEQUENCE_TABLE_POS,&pos,SEQUENCE_TABLE_LABEL,&old_text,-1);
      GST_INFO("old_text '%s'",old_text);

      if(old_text || new_text) {
        changed=TRUE;
        if(old_text && !*old_text) changed=FALSE;
        if(new_text && !*new_text) changed=FALSE;
      }
      else if(old_text && new_text && !strcmp(old_text,new_text)) changed=TRUE;
      if(changed) {
        gulong length;

        GST_INFO("label changed");
        g_object_get(G_OBJECT(self->priv->sequence),"length",&length,NULL);

        // need to change it in the model
        gtk_list_store_set(GTK_LIST_STORE(store),&iter,SEQUENCE_TABLE_LABEL,new_text,-1);
        // update the sequence
        if(pos>=length) {
          g_object_set(G_OBJECT(self->priv->sequence),"length",pos+1,NULL);
        }
        bt_sequence_set_label(self->priv->sequence,pos,new_text);
        // update label_menu
        sequence_table_refresh_labels(self);
      }
      g_free(old_text);
    }
  }
}

static void on_pos_menu_changed(GtkComboBox *combo_box,gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  BtSong *song;

  self->priv->pos_format=gtk_combo_box_get_active(combo_box);

  // reformat pos-column and label-menu (this is inefficient)
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  // @todo: inefficient (we ideally just want to poke new stings into the model)
  sequence_table_refresh(self,song);
  sequence_table_refresh_labels(self);
  sequence_model_recolorize(self);
  g_object_unref(song);
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
  GtkWidget *label;
  GtkTreeViewColumn *tree_col;
  gint col_index=0;

  // empty header widget
  gtk_container_forall(GTK_CONTAINER(self->priv->sequence_pos_table_header),(GtkCallback)gtk_widget_destroy,NULL);

  // create header widget
  self->priv->pos_header=gtk_vbox_new(FALSE,HEADER_SPACING);
  // time line position
  label=gtk_label_new(_("Pos."));
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.0);
  gtk_box_pack_start(GTK_BOX(self->priv->pos_header),label,TRUE,FALSE,0);

  self->priv->pos_menu=GTK_COMBO_BOX(gtk_combo_box_new_text());
  gtk_combo_box_append_text(self->priv->pos_menu,_("Ticks"));
  gtk_combo_box_append_text(self->priv->pos_menu,_("Time"));
  gtk_combo_box_set_active(self->priv->pos_menu,0);
  gtk_box_pack_start(GTK_BOX(self->priv->pos_header),GTK_WIDGET(self->priv->pos_menu),TRUE,TRUE,0);
  //gtk_widget_set_size_request(self->priv->pos_header,POSITION_CELL_WIDTH,-1);
  g_signal_connect(G_OBJECT(self->priv->pos_menu),"changed",G_CALLBACK(on_pos_menu_changed), (gpointer)self);
  gtk_widget_show_all(self->priv->pos_header);

  gtk_box_pack_start(GTK_BOX(self->priv->sequence_pos_table_header),self->priv->pos_header,TRUE,TRUE,0);
  gtk_widget_set_size_request(GTK_WIDGET(self->priv->sequence_pos_table_header),POSITION_CELL_WIDTH,-1);

  // add static column
  renderer=gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer),
    "mode",GTK_CELL_RENDERER_MODE_INERT,
    "xalign",1.0,
    "yalign",0.5,
    "foreground","blue",
    NULL);
  gtk_cell_renderer_set_fixed_size(renderer, 1, -1);
  gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer),1);
  if((tree_col=gtk_tree_view_column_new_with_attributes(NULL,renderer,
    "text",SEQUENCE_TABLE_POSSTR,
    "foreground-set",SEQUENCE_TABLE_TICK_FG_SET,
    NULL))
  ) {
    g_object_set(tree_col,
      "sizing",GTK_TREE_VIEW_COLUMN_FIXED,
      "fixed-width",POSITION_CELL_WIDTH,
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
  gulong number_of_tracks;

  // remove columns
  if((columns=gtk_tree_view_get_columns(self->priv->sequence_table))) {
    for(node=g_list_first(columns);node;node=g_list_next(node)) {
      gtk_tree_view_remove_column(self->priv->sequence_table,GTK_TREE_VIEW_COLUMN(node->data));
    }
    g_list_free(columns);
  }

  // change number of tracks
  g_object_get(self->priv->sequence,"tracks",&number_of_tracks,NULL);
  if(number_of_tracks>0) {
    BtMachine *machine;
    guint i;

    // disconnect signal handlers
    for(i=0;i<number_of_tracks;i++) {
      if((machine=bt_sequence_get_machine(self->priv->sequence,i))) {
        g_signal_handlers_disconnect_matched(machine,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_machine_state_changed_mute,NULL);
        g_signal_handlers_disconnect_matched(machine,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_machine_state_changed_solo,NULL);
        g_signal_handlers_disconnect_matched(machine,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_machine_state_changed_bypass,NULL);
        // need to disconnect the label updates for the seq headers, unfortunately we don#t know the label
        // so we use a waek_ref and on_sequence_header_label_destroy()
        g_object_unref(machine);
      }
    }
  }
}

static void remove_container_widget(GtkWidget *widget,gpointer user_data) {
  GST_LOG("removing: %d, %s",G_OBJECT(widget)->ref_count,gtk_widget_get_name(widget));
  gtk_container_remove(GTK_CONTAINER(user_data),widget);
}

/*
 * sequence_table_init:
 * @self: the sequence page
 *
 * inserts the Label columns.
 */
static void sequence_table_init(const BtMainPageSequence *self) {
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *tree_col;
  GtkWidget *label;
  GtkWidget *header,*vbox;
  gint col_index=0;

  GST_INFO("preparing sequence table");

  // do not destroy when flushing the header
  if((vbox=gtk_widget_get_parent(GTK_WIDGET(self->priv->label_menu)))) {
    GST_INFO("holding label widget: %d",G_OBJECT(self->priv->label_menu)->ref_count);
    gtk_container_remove(GTK_CONTAINER(vbox),GTK_WIDGET(g_object_ref(self->priv->label_menu)));
    //gtk_widget_unparent(GTK_WIDGET(g_object_ref(self->priv->label_menu)));
    GST_INFO("                    : %d",G_OBJECT(self->priv->label_menu)->ref_count);
  }
  // empty header widget
  gtk_container_forall(GTK_CONTAINER(self->priv->sequence_table_header),(GtkCallback)remove_container_widget,GTK_CONTAINER(self->priv->sequence_table_header));

  // create header widget
  header=gtk_hbox_new(FALSE,HEADER_SPACING);
  vbox=gtk_vbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(header),vbox,TRUE,TRUE,0);
  gtk_box_pack_start(GTK_BOX(header),gtk_vseparator_new(),FALSE,FALSE,0);
  
  label=gtk_label_new(_("Labels"));
  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.0);
  gtk_box_pack_start(GTK_BOX(vbox),label,TRUE,TRUE,0);
  
  gtk_box_pack_start(GTK_BOX(vbox),GTK_WIDGET(self->priv->label_menu),TRUE,TRUE,0);

  /* FIXME: specifying 0, instead of -1, should yield 'as small as possible'
   * in reality it result in distorted overlapping widgets :(
   */
  gtk_widget_set_size_request(header,SEQUENCE_CELL_WIDTH,-1);
  gtk_widget_show_all(header);
  gtk_box_pack_start(GTK_BOX(self->priv->sequence_table_header),header,FALSE,FALSE,0);
  g_signal_connect(G_OBJECT(header),"size-allocate",G_CALLBACK(on_header_size_allocate),(gpointer)self);

  // re-add static columns
  renderer=gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer),
    "mode",GTK_CELL_RENDERER_MODE_EDITABLE,
    "xalign",1.0,
    "yalign",0.5,
    "foreground","blue",
    "editable",TRUE,
    /*
    "width",SEQUENCE_CELL_WIDTH-4,
    "height",SEQUENCE_CELL_HEIGHT-4,
    "xpad",SEQUENCE_CELL_XPAD,
    "ypad",SEQUENCE_CELL_YPAD,
    */
    NULL);
  gtk_cell_renderer_set_fixed_size(renderer, 1, -1);
  gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer),1);
  g_signal_connect(G_OBJECT(renderer),"edited",G_CALLBACK(on_sequence_label_edited),(gpointer)self);
  if((tree_col=gtk_tree_view_column_new_with_attributes(_("Labels"),renderer,
    "text",SEQUENCE_TABLE_LABEL,
    "foreground-set",SEQUENCE_TABLE_TICK_FG_SET,
    NULL))
  ) {
    g_object_set(tree_col,
      "sizing",GTK_TREE_VIEW_COLUMN_FIXED,
      "fixed-width",SEQUENCE_CELL_WIDTH,
      NULL);
    col_index=gtk_tree_view_append_column(self->priv->sequence_table,tree_col);
    gtk_tree_view_column_set_cell_data_func(tree_col, renderer, label_cell_data_function, (gpointer)self, NULL);
  }
  else GST_WARNING("can't create treeview column");

  if(self->priv->level_to_vumeter) g_hash_table_destroy(self->priv->level_to_vumeter);
  self->priv->level_to_vumeter=g_hash_table_new_full(NULL,NULL,(GDestroyNotify)gst_object_unref,NULL);

  GST_DEBUG("    number of columns : %d",col_index);
}

static void sequence_table_refresh_labels(const BtMainPageSequence *self) {
  gulong i,timeline_ct;
  gchar *str,*pos_str;
  GtkListStore *label_menu_store;
  GtkTreeIter label_menu_iter;

  // label menu will have 'position : label'
  label_menu_store=gtk_list_store_new(3,G_TYPE_ULONG,G_TYPE_STRING,G_TYPE_STRING);

  g_object_get(G_OBJECT(self->priv->sequence),"length",&timeline_ct,NULL);

  for(i=0;i<self->priv->sequence_length;i++) {
    pos_str=sequence_format_positions(self,i);
    if( i < timeline_ct ) {
      // set label
      str=bt_sequence_get_label(self->priv->sequence,i);
      if(BT_IS_STRING(str)) {
        gtk_list_store_append(label_menu_store,&label_menu_iter);
        gtk_list_store_set(label_menu_store,&label_menu_iter,
          POSITION_MENU_POS,i,
          POSITION_MENU_POSSTR,pos_str,
          POSITION_MENU_LABEL,str,
          -1);
        GST_DEBUG("adding : %s : %s",pos_str,str);
        g_free(str);
      }
    }
  }
  gtk_combo_box_set_model(self->priv->label_menu,GTK_TREE_MODEL(label_menu_store));
  gtk_combo_box_set_active(self->priv->label_menu,0);
  g_object_unref(label_menu_store); // drop with widget
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
  BtMachine *machine;
  BtPattern *pattern;
  GtkWidget *header;
  gchar *str,*pos_str;
  gulong i,j,col_ct,timeline_ct,track_ct,pos=0;
  gint col_index;
  GtkCellRenderer *renderer;
  GtkListStore *store;
  GtkTreeModel *filtered_store;
  GType *store_types;
  GtkTreeIter tree_iter;
  GtkTreeViewColumn *tree_col;
  gboolean free_str;
  GHashTable *machine_usage;

  GST_INFO("refresh sequence table");

  g_object_get(G_OBJECT(song),"setup",&setup,NULL);
  g_object_get(G_OBJECT(self->priv->sequence),"length",&timeline_ct,"tracks",&track_ct,NULL);
  GST_DEBUG("  size is %2lu,%2lu",timeline_ct,track_ct);

  // reset columns
  sequence_table_clear(self);

  // build model
  GST_DEBUG("  build model");
  col_ct=(SEQUENCE_TABLE_PRE_CT+track_ct);
  store_types=(GType *)g_new(GType,col_ct);
  // for background color columns
  store_types[SEQUENCE_TABLE_SOURCE_BG   ]=GDK_TYPE_COLOR;
  store_types[SEQUENCE_TABLE_PROCESSOR_BG]=GDK_TYPE_COLOR;
  store_types[SEQUENCE_TABLE_SINK_BG     ]=GDK_TYPE_COLOR;
  store_types[SEQUENCE_TABLE_CURSOR_BG   ]=GDK_TYPE_COLOR;
  store_types[SEQUENCE_TABLE_SELECTION_BG]=GDK_TYPE_COLOR;
  store_types[SEQUENCE_TABLE_TICK_FG_SET ]=G_TYPE_BOOLEAN;
  // for static display columns
  store_types[SEQUENCE_TABLE_POS         ]=G_TYPE_LONG;
  store_types[SEQUENCE_TABLE_POSSTR      ]=G_TYPE_STRING;
  // for track display columns
  for(i=SEQUENCE_TABLE_LABEL;i<col_ct;i++) {
    store_types[i]=G_TYPE_STRING;
  }
  store=gtk_list_store_newv(col_ct,store_types);
  g_free(store_types);

  // add patterns
  for(i=0;i<self->priv->sequence_length;i++) {
    gtk_list_store_append(store, &tree_iter);

    pos_str=sequence_format_positions(self,i);
    // set position, highlight-color
    gtk_list_store_set(store,&tree_iter,
      SEQUENCE_TABLE_POS,pos,
      SEQUENCE_TABLE_POSSTR,pos_str,
      SEQUENCE_TABLE_TICK_FG_SET,FALSE,
      -1);
    pos++;
    if( i < timeline_ct ) {
      // set label
      str=bt_sequence_get_label(self->priv->sequence,i);
      if(str) {
        gtk_list_store_set(store,&tree_iter,SEQUENCE_TABLE_LABEL,str,-1);
        g_free(str);
      }

      // set patterns
      for(j=0;j<track_ct;j++) {
        free_str=FALSE;
        if((pattern=bt_sequence_get_pattern(self->priv->sequence,i,j))) {
          g_object_get(pattern,"name",&str,NULL);
          free_str=TRUE;
          g_object_unref(pattern);
        }
        else {
          str=" ";
        }
        //GST_DEBUG("  %2d,%2d : adding \"%s\"",i,j,str);
        gtk_list_store_set(store,&tree_iter,SEQUENCE_TABLE_PRE_CT+j,str,-1);
        if(free_str)
          g_free(str);
      }
    }
  }
  // create a filterd model to realize step filtering
  filtered_store=gtk_tree_model_filter_new(GTK_TREE_MODEL(store),NULL);
  gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filtered_store),step_visible_filter,(gpointer)self,NULL);
  // active models
  gtk_tree_view_set_model(self->priv->sequence_table,filtered_store);
  gtk_tree_view_set_model(self->priv->sequence_pos_table,filtered_store);
  g_object_unref(filtered_store); // drop with widget

  // build dynamic sequence view
  GST_DEBUG("  build view");

  // add initial columns
  sequence_table_init(self);

  // add column for each machine
  machine_usage=g_hash_table_new(NULL,NULL);
  for(j=0;j<track_ct;j++) {
    machine=bt_sequence_get_machine(self->priv->sequence,j);
    renderer=gtk_cell_renderer_text_new();
    g_object_set(G_OBJECT(renderer),
      "mode",GTK_CELL_RENDERER_MODE_ACTIVATABLE,
      "xalign",0.0,
      "yalign",0.5,
      /*
      "editable",TRUE,
      "width",SEQUENCE_CELL_WIDTH-4,
      "height",SEQUENCE_CELL_HEIGHT-4,
      "xpad",SEQUENCE_CELL_XPAD,
      "ypad",SEQUENCE_CELL_YPAD,
      */
      NULL);
    gtk_cell_renderer_set_fixed_size(renderer, 1, -1);
    gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer),1);

    // setup column header
    if(machine) {
      GtkWidget *label,*button,*vbox,*box;
      GtkVUMeter *vumeter;
      GstElement *level;
      gchar *level_name="output-post-level";

      // enable level meters
      if(!BT_IS_SINK_MACHINE(machine)) {
        if(!bt_machine_enable_output_post_level(machine)) {
          GST_INFO("enabling output level for machine failed");
        }
      }
      else {
        // its the sink, which already has it enabled
        level_name="input-post-level";
      }
      g_object_get(G_OBJECT(machine),"id",&str,level_name,&level,NULL);

      // @todo: add context menu like that in the machine_view to the header

      // create header widget
      header=gtk_hbox_new(FALSE,HEADER_SPACING);
      vbox=gtk_vbox_new(FALSE,0);
      gtk_box_pack_start(GTK_BOX(header),vbox,TRUE,TRUE,0);
      gtk_box_pack_start(GTK_BOX(header),gtk_vseparator_new(),FALSE,FALSE,0);

      label=gtk_label_new(str);
      gtk_misc_set_alignment(GTK_MISC(label),0.0,0.0);
      g_free(str);
      gtk_box_pack_start(GTK_BOX(vbox),label,TRUE,TRUE,0);

      // disconnecting old handler here would be better, but then we need to differentiate
      g_signal_handlers_disconnect_matched(G_OBJECT(machine),G_SIGNAL_MATCH_FUNC,0,0,NULL,on_machine_id_changed_seq,NULL);
      g_signal_connect(G_OBJECT(machine),"notify::id",G_CALLBACK(on_machine_id_changed_seq),(gpointer)label);
      // we need to remove the signal handler when updating the labels
      //g_object_weak_ref(G_OBJECT(label),on_sequence_header_label_destroy,machine);
      /* we have the label column already
      if(j==0) {
        // connect to the size-allocate signal to adjust the height of the other treeview header
        g_signal_connect(G_OBJECT(header),"size-allocate",G_CALLBACK(on_header_size_allocate),(gpointer)self);
      }
      */

      box=gtk_hbox_new(FALSE,0);
      gtk_box_pack_start(GTK_BOX(vbox),GTK_WIDGET(box),TRUE,TRUE,0);
      
      /* only do this for first track of a machine
       * - multiple level-meter views for same machine don't work
       * - MSB buttons would need to be synced
       */
      if (!g_hash_table_lookup(machine_usage,machine)) {
        BtMachineState state;

        g_object_get(machine,"state",&state,NULL);
        
        g_hash_table_insert(machine_usage,machine,machine);
        // add M/S/B butons and connect signal handlers
        // @todo: use colors from ui-resources
        button=make_mini_button("M",1.2, 1.0/1.25, 1.0/1.25,(state==BT_MACHINE_STATE_MUTE)); // red
        gtk_box_pack_start(GTK_BOX(box),button,FALSE,FALSE,0);
        g_signal_connect(G_OBJECT(button),"toggled",G_CALLBACK(on_mute_toggled),(gpointer)machine);
        g_signal_connect(G_OBJECT(machine),"notify::state", G_CALLBACK(on_machine_state_changed_mute), (gpointer)button);

        if(BT_IS_SOURCE_MACHINE(machine)) {
          button=make_mini_button("S",1.0/1.2,1.0/1.2,1.1,(state==BT_MACHINE_STATE_SOLO)); // blue
          gtk_box_pack_start(GTK_BOX(box),button,FALSE,FALSE,0);
          g_signal_connect(G_OBJECT(button),"toggled",G_CALLBACK(on_solo_toggled),(gpointer)machine);
          g_signal_connect(G_OBJECT(machine),"notify::state", G_CALLBACK(on_machine_state_changed_solo), (gpointer)button);
        }

        if(BT_IS_PROCESSOR_MACHINE(machine)) {
          button=make_mini_button("B",1.2,1.0/1.1,1.0/1.4,(state==BT_MACHINE_STATE_BYPASS)); // orange
          gtk_box_pack_start(GTK_BOX(box),button,FALSE,FALSE,0);
          g_signal_connect(G_OBJECT(button),"toggled",G_CALLBACK(on_bypass_toggled),(gpointer)machine);
          g_signal_connect(G_OBJECT(machine),"notify::state", G_CALLBACK(on_machine_state_changed_bypass), (gpointer)button);
        }
        vumeter=GTK_VUMETER(gtk_vumeter_new(FALSE));
        gtk_vumeter_set_min_max(vumeter, LOW_VUMETER_VAL, 0);
        gtk_vumeter_set_levels(vumeter, LOW_VUMETER_VAL, LOW_VUMETER_VAL);
        // no falloff in widget, we have falloff in GstLevel
        //gtk_vumeter_set_peaks_falloff(vumeter, GTK_VUMETER_PEAKS_FALLOFF_MEDIUM);
        gtk_vumeter_set_scale(vumeter, GTK_VUMETER_SCALE_LINEAR);
        gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(vumeter),TRUE,TRUE,0);
  
        // add level meters to hashtable
        if(level) {
          g_hash_table_insert(self->priv->level_to_vumeter,level,vumeter);
        }
      }
    }
    else {
      header=gtk_label_new("???");
      GST_WARNING("can't get machine for column %lu",j);
    }
    gtk_widget_set_size_request(header,SEQUENCE_CELL_WIDTH,-1);
    gtk_widget_show_all(header);
    gtk_box_pack_start(GTK_BOX(self->priv->sequence_table_header),header,FALSE,FALSE,0);

    if((tree_col=gtk_tree_view_column_new_with_attributes(NULL,renderer, NULL))) {
      g_object_set(tree_col,
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
  g_hash_table_destroy(machine_usage);

  // add a final column that eats remaining space
  renderer=gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer),
    "mode",GTK_CELL_RENDERER_MODE_INERT,
    NULL);
  gtk_cell_renderer_set_fixed_size(renderer, 1, -1);
  gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer), 1);

  header=gtk_label_new("");
  gtk_widget_show_all(header);
  gtk_box_pack_start(GTK_BOX(self->priv->sequence_table_header),header,TRUE,TRUE,0);
  if((tree_col=gtk_tree_view_column_new_with_attributes(/*title=*/NULL,renderer,NULL))) {
    g_object_set(tree_col,
      "sizing",GTK_TREE_VIEW_COLUMN_FIXED,
      NULL);
    col_index=gtk_tree_view_append_column(self->priv->sequence_table,tree_col);
    GST_DEBUG("    number of columns : %d",col_index);
  }
  else GST_WARNING("can't create treeview column");

  // release the references
  g_object_unref(setup);
}

static void pattern_list_refresh(const BtMainPageSequence *self) {
  GtkListStore *store;
  GtkTreeIter tree_iter;
  
  // refresh the pattern list
  GST_INFO("refresh pattern list for machine : %p",self->priv->machine);

  store=gtk_list_store_new(3,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_BOOLEAN);

  if(self->priv->machine) {
    BtPattern *pattern;
    gulong index;
    GList *node,*list;
    gboolean is_internal,is_used;
    gchar *str,key[2]={0,};
  
    //-- append default rows
    self->priv->pattern_keys=sink_pattern_keys;
    index=2;
    gtk_list_store_append(store, &tree_iter);
    // use spaces to avoid clashes with normal patterns?
    gtk_list_store_set(store,&tree_iter,PATTERN_TABLE_KEY,".",PATTERN_TABLE_NAME,_("  clear"),PATTERN_TABLE_USED,TRUE,-1);
    gtk_list_store_append(store, &tree_iter);
    gtk_list_store_set(store,&tree_iter,PATTERN_TABLE_KEY,"-",PATTERN_TABLE_NAME,_("  mute"),PATTERN_TABLE_USED,TRUE,-1);
    gtk_list_store_append(store, &tree_iter);
    gtk_list_store_set(store,&tree_iter,PATTERN_TABLE_KEY,",",PATTERN_TABLE_NAME,_("  break"),PATTERN_TABLE_USED,TRUE,-1);
    if(BT_IS_PROCESSOR_MACHINE(self->priv->machine)) {
      gtk_list_store_append(store, &tree_iter);
      gtk_list_store_set(store,&tree_iter,PATTERN_TABLE_KEY,"_",PATTERN_TABLE_NAME,_("  bypass"),PATTERN_TABLE_USED,TRUE,-1);
      self->priv->pattern_keys=processor_pattern_keys;
      index++;
    }
    if(BT_IS_SOURCE_MACHINE(self->priv->machine)) {
      gtk_list_store_append(store, &tree_iter);
      gtk_list_store_set(store,&tree_iter,PATTERN_TABLE_KEY,"_",PATTERN_TABLE_NAME,_("  solo"),PATTERN_TABLE_USED,TRUE,-1);
      self->priv->pattern_keys=source_pattern_keys;
      index++;
    }
  
    //-- append pattern rows
    g_object_get(G_OBJECT(self->priv->machine),"patterns",&list,NULL);
    for(node=list;node;node=g_list_next(node)) {
      pattern=BT_PATTERN(node->data);
      g_object_get(G_OBJECT(pattern),"name",&str,"is-internal",&is_internal,NULL);
      if(!is_internal) {
        //GST_DEBUG("  adding \"%s\" at index %d -> '%c'",str,index,self->priv->pattern_keys[index]);
        key[0]=(index<64)?self->priv->pattern_keys[index]:' ';
        //if(index<64) key[0]=self->priv->pattern_keys[index];
        //else key[0]=' ';

        //GST_DEBUG("  with shortcut \"%s\"",key);
        // use gray color for unused patterns in pattern list
        is_used=bt_sequence_is_pattern_used(self->priv->sequence,pattern);
        gtk_list_store_append(store, &tree_iter);
        gtk_list_store_set(store,&tree_iter,
          PATTERN_TABLE_KEY,key,
          PATTERN_TABLE_NAME,str,
          PATTERN_TABLE_USED,is_used,
          -1);
        index++;
      }
      g_free(str);
    }
    g_list_free(list);
    
    // sync machine in pattern page
    BtMainWindow *main_window;
    BtMainPages *pages;
    BtMainPagePatterns *patterns_page;
  
    g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
    g_object_get(G_OBJECT(main_window),"pages",&pages,NULL);
    g_object_get(G_OBJECT(pages),"patterns-page",&patterns_page,NULL);
  
    bt_main_page_patterns_show_machine(patterns_page,self->priv->machine);
    
    g_object_unref(patterns_page);
    g_object_unref(pages);
    g_object_unref(main_window);     
  }
  else {
    GST_INFO("no machine for cursor_column: %ld",self->priv->cursor_column);
  }
  gtk_tree_view_set_model(self->priv->pattern_list,GTK_TREE_MODEL(store));
  
  g_object_unref(store); // drop with treeview
}


/*
 * update_after_track_changed:
 * @self: the sequence page
 *
 * When the user moves the cursor in the sequence, update the list of patterns
 * so that it shows the patterns that belong to the machine in the current
 * sequence row.
 * Also update the current selected machine in pattern view.
 */
static void update_after_track_changed(const BtMainPageSequence *self) {
  BtMachine *machine;

  GST_INFO("change active track");

  machine=bt_main_page_sequence_get_current_machine(self);
  if(machine!=self->priv->machine) {
    if(self->priv->machine) {
      GST_INFO("unref old cur-machine %p,refs=%d",self->priv->machine,(G_OBJECT(self->priv->machine))->ref_count);
      g_signal_handler_disconnect(G_OBJECT(self->priv->machine),self->priv->pattern_added_handler);
      g_signal_handler_disconnect(G_OBJECT(self->priv->machine),self->priv->pattern_removed_handler);
      // unref the old machine
      g_object_unref(self->priv->machine);
      self->priv->machine=NULL;
      self->priv->pattern_added_handler=0;
      self->priv->pattern_removed_handler=0;
    }
    if(machine) {
      GST_INFO("ref new cur-machine: refs: %d",(G_OBJECT(machine))->ref_count);
      self->priv->pattern_added_handler=g_signal_connect(G_OBJECT(machine),"pattern-added",G_CALLBACK(on_pattern_changed),(gpointer)self);
      self->priv->pattern_removed_handler=g_signal_connect(G_OBJECT(machine),"pattern-removed",G_CALLBACK(on_pattern_changed),(gpointer)self);
      // remember the new machine
      self->priv->machine=machine;
    }  
    pattern_list_refresh(self);    
  }
  else {
    g_object_try_unref(machine);
  }
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

  // (re)create a new menu
  submenu=gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(self->priv->context_menu_add),submenu);

  // fill machine menu
  g_object_get(G_OBJECT(setup),"machines",&list,NULL);
  for(node=list;node;node=g_list_next(node)) {
    machine=BT_MACHINE(node->data);
    g_object_get(G_OBJECT(machine),"id",&str,NULL);

    menu_item=gtk_image_menu_item_new_with_label(str);
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),bt_ui_resources_get_icon_image_by_machine(machine));
    gtk_menu_shell_append(GTK_MENU_SHELL(submenu),menu_item);
    gtk_widget_show(menu_item);
    widgets=gtk_container_get_children(GTK_CONTAINER(menu_item));
    label=g_list_nth_data(widgets,0);
    if(GTK_IS_LABEL(label)) {
      GST_DEBUG("menu item for machine %p,ref_count=%d",machine,G_OBJECT(machine)->ref_count);
      g_signal_handlers_disconnect_matched(G_OBJECT(machine),G_SIGNAL_MATCH_FUNC,0,0,NULL,on_machine_id_changed_menu,NULL);
      g_signal_connect(G_OBJECT(machine),"notify::id",G_CALLBACK(on_machine_id_changed_menu),(gpointer)label);
      // we need to remove the signal handler when updating the labels
      //g_object_weak_ref(G_OBJECT(label),on_sequence_header_label_destroy,machine);
    }
    g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_track_add_activated),(gpointer)self);
    g_list_free(widgets);
    g_free(str);
  }
  g_list_free(list);
}

/*
 * sequence_view_set_pos:
 *
 * set play, loop-start/end or length bars
 */
static void sequence_view_set_pos(const BtMainPageSequence *self,gulong type,glong row) {
  BtSong *song;
  gulong sequence_length;
  gdouble pos;
  glong play_pos,loop_start,loop_end;

  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(song,"play-pos",&play_pos,NULL);
  g_object_get(self->priv->sequence,"length",&sequence_length,"loop-end",&loop_end,NULL);
  if(row==-1) row=sequence_length;
  // use a keyboard qualifier to set loop_start and end
  /* @todo should the sequence-view listen to notify::xxx ? */
  switch(type) {
    case SEQUENCE_VIEW_POS_PLAY:
      g_object_set(song,"play-pos",row,NULL);
      break;
    case SEQUENCE_VIEW_POS_LOOP_START:
      // set and read back, as sequence might clamp the value
      g_object_set(self->priv->sequence,"loop-start",row,NULL);
      g_object_get(self->priv->sequence,"loop-start",&row,NULL);
      loop_start=row;
      
      pos=(gdouble)row/(gdouble)sequence_length;
      g_object_set(self->priv->sequence_table,"loop-start",pos,NULL);
      g_object_set(self->priv->sequence_pos_table,"loop-start",pos,NULL);

      GST_INFO("adjusted loop-start = %ld",row);

      g_object_get(self->priv->sequence,"loop-end",&loop_end,NULL);
      if((loop_end!=-1) && (loop_end<=row)) {
        loop_end=loop_start+self->priv->bars;
        g_object_set(self->priv->sequence,"loop-end",loop_end,NULL);
        
        GST_INFO("and adjusted loop-end = %ld",loop_end);

        pos=(gdouble)loop_end/(gdouble)sequence_length;
        g_object_set(self->priv->sequence_table,"loop-end",pos,NULL);
        g_object_set(self->priv->sequence_pos_table,"loop-end",pos,NULL);
      }
      break;
    case SEQUENCE_VIEW_POS_LOOP_END:
      // pos is beyond length or is on loop-end already -> adjust length
      if((row>sequence_length) || (row==loop_end)) {
        GST_INFO("adjusted length = %ld -> %ld",sequence_length,row);
        sequence_length=row;
        g_object_set(self->priv->sequence,"length",sequence_length,NULL);
        g_object_get(self->priv->sequence,"loop-start",&loop_start,NULL);
        // this triggers redraw
        sequence_table_refresh(self,song);
        sequence_table_refresh_labels(self);
        sequence_calculate_visible_lines(self);
        sequence_model_recolorize(self);
        sequence_view_set_cursor_pos(self);
      }
      else {
        // set and read back, as sequence might clamp the value
        g_object_set(self->priv->sequence,"loop-end",row,NULL);
        g_object_get(self->priv->sequence,"loop-end",&row,NULL);
        loop_end=row;

        GST_INFO("adjusted loop-end = %ld",row);

        g_object_get(self->priv->sequence,"loop-start",&loop_start,NULL);
        if((loop_start!=-1) && (loop_start>=row)) {
          loop_start=loop_end-self->priv->bars;
          if(loop_start<0) loop_start=0;
          g_object_set(self->priv->sequence,"loop-start",loop_start,NULL);
          
          GST_INFO("and adjusted loop-start = %ld",loop_start);
        }
      }
      pos=(loop_end>-1)?(gdouble)loop_end/(gdouble)sequence_length:1.0;
      g_object_set(self->priv->sequence_table,"loop-end",pos,NULL);
      g_object_set(self->priv->sequence_pos_table,"loop-end",pos,NULL);

      pos=(loop_start>-1)?(gdouble)loop_start/(gdouble)sequence_length:0.0;
      g_object_set(self->priv->sequence_table,"loop-start",pos,NULL);
      g_object_set(self->priv->sequence_pos_table,"loop-start",pos,NULL);
      break;
  }
  g_object_unref(song);
}

static void sequence_add_track(const BtMainPageSequence *self,BtMachine *machine) {
  BtSong *song;
  GList *columns;

  // get song from app and then setup from song
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);

  bt_sequence_add_track(self->priv->sequence,machine);

  // reset selection
  self->priv->selection_start_column=self->priv->selection_start_row=self->priv->selection_end_column=self->priv->selection_end_row=-1;

  // reinit the view
  sequence_table_refresh(self,song);
  sequence_model_recolorize(self);

  // update cursor_column and focus cell
  // (-2 because last column is empty and first is label)
  columns=gtk_tree_view_get_columns(self->priv->sequence_table);
  self->priv->cursor_column=g_list_length(columns)-2;
  GST_INFO("new cursor column: %ld",self->priv->cursor_column);
  g_list_free(columns);

  sequence_view_set_cursor_pos(self);
  update_after_track_changed(self);

  g_object_unref(song);
}

//-- event handler

static void on_track_add_activated(GtkMenuItem *menu_item, gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  GList *widgets;
  GtkWidget *label;

  // get the machine by the menuitems name
  //id=(gchar *)gtk_widget_get_name(GTK_WIDGET(menu_item));
  widgets=gtk_container_get_children(GTK_CONTAINER(menu_item));
  label=g_list_nth_data(widgets,0);
  if(GTK_IS_LABEL(label)) {
    BtSong *song;
    BtSetup *setup;
    BtMachine *machine;
    const gchar *id;

    // get song from app and then setup from song
    g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
    g_object_get(song,"setup",&setup,NULL);
  
    id=gtk_label_get_text(GTK_LABEL(label));
    GST_INFO("adding track for machine \"%s\"",id);
    if((machine=bt_setup_get_machine_by_id(setup,id))) {
      sequence_add_track(self,machine);
      g_object_unref(machine);
    }
    g_object_unref(setup);
    g_object_unref(song);
  }
  g_list_free(widgets);
}

static void on_track_remove_activated(GtkMenuItem *menuitem, gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  BtSong *song;
  gulong number_of_tracks;

  // get song from app and then setup from song
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);

  // change number of tracks
  g_object_get(self->priv->sequence,"tracks",&number_of_tracks,NULL);
  if(number_of_tracks>0) {
    BtMachine *machine;

    machine=bt_sequence_get_machine(self->priv->sequence,number_of_tracks-1);
    // even though we can have multiple tracks per machine, we can disconnect them all, as we rebuild the treeview anyway
    g_signal_handlers_disconnect_matched(machine,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_machine_state_changed_mute,NULL);
    g_signal_handlers_disconnect_matched(machine,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_machine_state_changed_solo,NULL);
    g_signal_handlers_disconnect_matched(machine,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_machine_state_changed_bypass,NULL);
    g_object_unref(machine);

    GST_DEBUG("old cursor column: %ld, tracks: %lu",self->priv->cursor_column,number_of_tracks);
    // remove the track where the cursor is 
    bt_sequence_remove_track_by_ix(self->priv->sequence,self->priv->cursor_column-1);

    // reset selection
    self->priv->selection_start_column=self->priv->selection_start_row=self->priv->selection_end_column=self->priv->selection_end_row=-1;

    // reinit the view
    sequence_table_refresh(self,song);
    sequence_model_recolorize(self);

    if(self->priv->cursor_column>=number_of_tracks) {
      // update cursor_column and focus cell
      self->priv->cursor_column--;
      sequence_view_set_cursor_pos(self);
      GST_DEBUG("new cursor column: %ld",self->priv->cursor_column);
    }

    update_after_track_changed(self);
  }

  g_object_unref(song);
}

static void on_track_move_left_activated(GtkMenuItem *menuitem, gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  BtSong *song;
  gulong track=self->priv->cursor_column-1;
  
  GST_INFO("move track %ld to left",self->priv->cursor_column);

  if(track>0) {
    // get song from app and then setup from song
    g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
    
    if(bt_sequence_move_track_left(self->priv->sequence,track)) {
      self->priv->cursor_column--;
      // reinit the view
      sequence_table_refresh(self,song);
      sequence_model_recolorize(self);
      sequence_view_set_cursor_pos(self);
    }

    g_object_unref(song);
  }
}

static void on_track_move_right_activated(GtkMenuItem *menuitem, gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  BtSong *song;
  gulong track=self->priv->cursor_column-1,number_of_tracks;

  GST_INFO("move track %ld to right",self->priv->cursor_column);

  // get song from app and then setup from song
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(self->priv->sequence),"tracks",&number_of_tracks,NULL);

  if(track<number_of_tracks) {
    if(bt_sequence_move_track_right(self->priv->sequence,track)) {
      self->priv->cursor_column++;
      // reinit the view
      sequence_table_refresh(self,song);
      sequence_model_recolorize(self);
      sequence_view_set_cursor_pos(self);
    }
  }
  g_object_unref(song);
}

static void on_song_play_pos_notify(const BtSong *song,GParamSpec *arg,gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  gdouble play_pos;
  gulong sequence_length,pos;
  GtkTreePath *path;

  g_assert(user_data);

  // calculate fractional pos and set into sequence-viewer
  g_object_get(G_OBJECT(song),"play-pos",&pos,NULL);
  g_object_get(G_OBJECT(self->priv->sequence),"length",&sequence_length,NULL);
  play_pos=(gdouble)pos/(gdouble)sequence_length;
  if(play_pos<=1.0) {
    g_object_set(self->priv->sequence_table,"play-position",play_pos,NULL);
    g_object_set(self->priv->sequence_pos_table,"play-position",play_pos,NULL);
  }

  //GST_DEBUG("sequence tick received : %d",pos);

  // do nothing for invisible rows
  if(IS_SEQUENCE_POS_VISIBLE(pos,self->priv->bars)) {
    // scroll  to make play pos visible
    if((path=gtk_tree_path_new_from_indices((pos/self->priv->bars),-1))) {
      // that would try to keep the cursor in the middle (means it will scroll more)
      if(GTK_WIDGET_REALIZED(self->priv->sequence_table)) {
        gtk_tree_view_scroll_to_cell(self->priv->sequence_table,path,NULL,TRUE,0.5,0.5);
        //gtk_tree_view_scroll_to_cell(self->priv->sequence_table,path,NULL,FALSE,0.0,0.0);
      }
      if(GTK_WIDGET_REALIZED(self->priv->sequence_pos_table)) {
        gtk_tree_view_scroll_to_cell(self->priv->sequence_pos_table,path,NULL,TRUE,0.5,0.5);
      }
      gtk_tree_path_free(path);
    }
  }
}

static void reset_level_meter(gpointer key, gpointer value, gpointer user_data) {
  gtk_vumeter_set_levels(GTK_VUMETER(value), LOW_VUMETER_VAL, LOW_VUMETER_VAL);
}

static void on_song_is_playing_notify(const BtSong *song,GParamSpec *arg,gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);

  g_assert(user_data);

  g_object_get(G_OBJECT(song),"is-playing",&self->priv->is_playing,NULL);
  // stop all level meters
  if(!self->priv->is_playing) {
    g_hash_table_foreach(self->priv->level_to_vumeter,reset_level_meter,NULL);
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

    sequence_calculate_visible_lines(self);
    sequence_model_recolorize(self);
    //GST_INFO("  bars = %d",self->priv->bars);
    if((filtered_store=GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(self->priv->sequence_table)))) {
      gtk_tree_model_filter_refilter(filtered_store);
    }
    gtk_widget_grab_focus_savely(GTK_WIDGET(self->priv->sequence_table));
  }
}

static void on_label_menu_changed(GtkComboBox *combo_box,gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  GtkTreeModel *store;
  GtkTreeIter iter;

  g_assert(user_data);

  GST_INFO("bars_menu has changed : page=%p",user_data);

  if((store=gtk_combo_box_get_model(self->priv->label_menu))
    && gtk_combo_box_get_active_iter(self->priv->label_menu,&iter))
  {
    GtkTreePath *path;
    gulong pos;

    gtk_tree_model_get(store,&iter,POSITION_MENU_POS,&pos,-1);
    GST_INFO("  move to = %lu",pos);
    if((path=gtk_tree_path_new_from_indices((pos/self->priv->bars),-1))) {
      // that would try to keep the cursor in the middle (means it will scroll more)
      if(GTK_WIDGET_REALIZED(self->priv->sequence_table)) {
        gtk_tree_view_scroll_to_cell(self->priv->sequence_table,path,NULL,TRUE,0.5,0.5);
      }
      gtk_tree_path_free(path);
    }
  }
}

static gboolean on_sequence_table_cursor_changed_idle(gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  GtkTreePath *path;
  GtkTreeViewColumn *column;
  gulong cursor_column,cursor_row;
  GList *columns;

  g_return_val_if_fail(user_data,FALSE);

  //GST_INFO("sequence_table cursor has changed : self=%p",user_data);

  gtk_tree_view_get_cursor(self->priv->sequence_table,&path,&column);
  if(column && path) {
    if(sequence_view_get_cursor_pos(self->priv->sequence_table,path,column,&cursor_column,&cursor_row)) {
      gulong lastbar,lastcolumn;

      columns=gtk_tree_view_get_columns(self->priv->sequence_table);
      lastcolumn=g_list_length(columns)-2;
      g_list_free(columns);

      GST_INFO("new row = %3lu <-> old row = %3ld",cursor_row,self->priv->cursor_row);
      self->priv->cursor_row=cursor_row;
      
      if(cursor_column>lastcolumn) {
        cursor_column=lastcolumn;
        sequence_view_set_cursor_pos(self);
      }
        
      GST_INFO("new col = %3lu <-> old col = %3ld",cursor_column,self->priv->cursor_column);
      if(cursor_column!=self->priv->cursor_column) {       
        self->priv->cursor_column=cursor_column;
        update_after_track_changed(self);        
      }
      GST_INFO("cursor has changed: %3ld,%3ld",self->priv->cursor_column,self->priv->cursor_row);

      // calculate the last visible row from step-filter and scroll-filter
      lastbar=self->priv->row_filter_pos-1-((self->priv->row_filter_pos-1)%self->priv->bars);

      // do we need to extend sequence?
      if( cursor_row >= lastbar ) {
        GtkTreeModelFilter *filtered_store;

        self->priv->row_filter_pos += self->priv->bars;
        if( self->priv->row_filter_pos > self->priv->sequence_length ) {
          BtSong *song;

          g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);

          self->priv->sequence_length+=SEQUENCE_ROW_ADDITION_INTERVAL;
          sequence_table_refresh(self,song);
          sequence_model_recolorize(self);
          // this got invalidated by _refresh()
          column=gtk_tree_view_get_column(self->priv->sequence_table,cursor_column);

          g_object_unref(song);
        }

        if((filtered_store=GTK_TREE_MODEL_FILTER(gtk_tree_view_get_model(self->priv->sequence_table)))) {
          gtk_tree_model_filter_refilter(filtered_store);
        }
        gtk_tree_view_set_cursor(self->priv->sequence_table,path,column,FALSE);
        gtk_widget_grab_focus_savely(GTK_WIDGET(self->priv->sequence_table));
      }
      gtk_tree_view_scroll_to_cell(self->priv->sequence_table,path,column,FALSE,1.0,0.0);
      gtk_widget_queue_draw(GTK_WIDGET(self->priv->sequence_table));
    }
  }
  else {
    GST_INFO("No cursor pos, column=%p, path=%p",column,path);
  }
  if(path) gtk_tree_path_free(path);

  return(FALSE);
}

static void on_sequence_table_cursor_changed(GtkTreeView *treeview, gpointer user_data) {
  /* delay the action */
  g_idle_add_full(G_PRIORITY_HIGH_IDLE,on_sequence_table_cursor_changed_idle,user_data,NULL);
}

static gboolean on_sequence_table_key_release_event(GtkWidget *widget,GdkEventKey *event,gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  gboolean res=FALSE;
  gulong row,track;

  g_assert(user_data);
  if(!GTK_WIDGET_REALIZED(self->priv->sequence_table)) return(FALSE);

  GST_INFO("sequence_table key key : state 0x%x, keyval 0x%x, hw-code 0x%x, name %s",
    event->state,event->keyval,event->hardware_keycode,gdk_keyval_name(event->keyval));

  // determine timeline and timelinetrack from cursor pos
  if(sequence_view_get_current_pos(self,&row,&track)) {
    BtSong *song;
    gulong length,tracks;
    gchar *str=NULL;
    gboolean free_str=FALSE;
    gboolean change=FALSE;
    gboolean pattern_usage_changed=FALSE;
    gulong modifier=(gulong)event->state&gtk_accelerator_get_default_mod_mask();

    g_object_get(G_OBJECT(self->priv->sequence),"length",&length,"tracks",&tracks,NULL);
    
    GST_DEBUG("cursor pos : %lu/%lu, %lu/%lu",row,length,track,tracks);
    if(track>tracks) return(FALSE);

    g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);

    // look up pattern for key
    if(event->keyval==GDK_space || event->keyval == GDK_period) {
      // first column is label
      if((track>0) && (row<length)) {
        BtPattern *pattern=bt_sequence_get_pattern(self->priv->sequence,row,track-1);

        bt_sequence_set_pattern(self->priv->sequence,row,track-1,NULL);
        if(pattern) {
          pattern_usage_changed=!bt_sequence_is_pattern_used(self->priv->sequence,pattern);
          g_object_unref(pattern);
        }
        str=" ";
        change=TRUE;
        res=TRUE;
      }
    }
    else if(event->keyval==GDK_Return) {  /* GDK_KP_Enter */
      // first column is label
      if(track>0) {
        BtMainWindow *main_window;
        BtMainPages *pages;
        BtMainPagePatterns *patterns_page;
        BtPattern *pattern;
        BtMachine *machine;

        g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
        g_object_get(G_OBJECT(main_window),"pages",&pages,NULL);
        g_object_get(G_OBJECT(pages),"patterns-page",&patterns_page,NULL);

        gtk_notebook_set_current_page(GTK_NOTEBOOK(pages),BT_MAIN_PAGES_PATTERNS_PAGE);
        if((row<length) && (pattern=bt_sequence_get_pattern(self->priv->sequence,row,track-1))) {
          GST_INFO("show pattern");
          bt_main_page_patterns_show_pattern(patterns_page,pattern);
          g_object_unref(pattern);
        }
        else if((machine=bt_main_page_sequence_get_current_machine(self))) {
          GST_INFO("show machine");
          bt_main_page_patterns_show_machine(patterns_page,machine);
          g_object_unref(machine);
        }

        g_object_unref(patterns_page);
        g_object_unref(pages);
        g_object_unref(main_window);

        res=TRUE;
      }
    }
    else if(event->keyval==GDK_Menu) {
      gtk_menu_popup(self->priv->context_menu,NULL,NULL,NULL,NULL,3,gtk_get_current_event_time());   
    }
    else if(event->keyval==GDK_Up || event->keyval==GDK_Down || event->keyval==GDK_Left || event->keyval==GDK_Right) {
      // work around http://bugzilla.gnome.org/show_bug.cgi?id=371756
#if GTK_CHECK_VERSION(2,10,0) && !GTK_CHECK_VERSION(2,10,7)
      gboolean changed=FALSE;

      switch(event->keyval) {
        case GDK_Up:
          if(self->priv->cursor_row>0) {
            self->priv->cursor_row-=self->priv->bars;
            changed=TRUE;
          }
          break;
        case GDK_Down:
          // we expand length
          //if(self->priv->cursor_row<self->priv->sequence_length) {
            self->priv->cursor_row+=self->priv->bars;
            changed=TRUE;
          //}
          break;
      }
      if(changed) {
        sequence_view_set_cursor_pos(self);
      }
#endif

      if(modifier==GDK_SHIFT_MASK) {
        gboolean select=FALSE;

        GST_INFO("handling selection");

        // handle selection
        switch(event->keyval) {
          case GDK_Up:
            if((self->priv->cursor_row>=0)
#if GTK_CHECK_VERSION(2,10,0) && !GTK_CHECK_VERSION(2,10,7)
              && changed
#endif
              ) {
              GST_INFO("up   : %3ld,%3ld -> %3ld,%3ld @ %3ld,%3ld",self->priv->selection_start_column,self->priv->selection_start_row,self->priv->selection_end_column,self->priv->selection_end_row,self->priv->cursor_column,self->priv->cursor_row);
              if(self->priv->selection_start_row==-1) {
                GST_INFO("up   : new selection");
                self->priv->selection_start_column=self->priv->cursor_column;
                self->priv->selection_end_column=self->priv->cursor_column;
                self->priv->selection_start_row=self->priv->cursor_row;
                self->priv->selection_end_row=self->priv->cursor_row+self->priv->bars;
              }
              else {
                if(self->priv->selection_start_row==(self->priv->cursor_row+self->priv->bars)) {
                  GST_INFO("up   : expand selection");
                  self->priv->selection_start_row-=self->priv->bars;
                }
                else {
                  GST_INFO("up   : shrink selection");
                  self->priv->selection_end_row-=self->priv->bars;
                }
              }
              GST_INFO("up   : %3ld,%3ld -> %3ld,%3ld",self->priv->selection_start_column,self->priv->selection_start_row,self->priv->selection_end_column,self->priv->selection_end_row);
              select=TRUE;
            }
            break;
          case GDK_Down:
            /* we expand length */
            GST_INFO("down : %3ld,%3ld -> %3ld,%3ld @ %3ld,%3ld",self->priv->selection_start_column,self->priv->selection_start_row,self->priv->selection_end_column,self->priv->selection_end_row,self->priv->cursor_column,self->priv->cursor_row);
            if(self->priv->selection_end_row==-1) {
              GST_INFO("down : new selection");
              self->priv->selection_start_column=self->priv->cursor_column;
              self->priv->selection_end_column=self->priv->cursor_column;
              self->priv->selection_start_row=self->priv->cursor_row-self->priv->bars;
              self->priv->selection_end_row=self->priv->cursor_row;
            }
            else {
              if(self->priv->selection_end_row==(self->priv->cursor_row-self->priv->bars)) {
                GST_INFO("down : expand selection");
                self->priv->selection_end_row+=self->priv->bars;
              }
              else {
                GST_INFO("down : shrink selection");
                self->priv->selection_start_row+=self->priv->bars;
              }
            }
            GST_INFO("down : %3ld,%3ld -> %3ld,%3ld",self->priv->selection_start_column,self->priv->selection_start_row,self->priv->selection_end_column,self->priv->selection_end_row);
            select=TRUE;
            break;
          case GDK_Left:
            if(self->priv->cursor_column>=0) {
              // move cursor
              self->priv->cursor_column--;
              sequence_view_set_cursor_pos(self);
              GST_INFO("left : %3ld,%3ld -> %3ld,%3ld @ %3ld,%3ld",self->priv->selection_start_column,self->priv->selection_start_row,self->priv->selection_end_column,self->priv->selection_end_row,self->priv->cursor_column,self->priv->cursor_row);
              if(self->priv->selection_start_column==-1) {
                GST_INFO("left : new selection");
                self->priv->selection_start_column=self->priv->cursor_column;
                self->priv->selection_end_column=self->priv->cursor_column+1;
                self->priv->selection_start_row=self->priv->cursor_row;
                self->priv->selection_end_row=self->priv->cursor_row;
              }
              else {
                if(self->priv->selection_start_column==(self->priv->cursor_column+1)) {
                  GST_INFO("left : expand selection");
                  self->priv->selection_start_column--;
                }
                else {
                  GST_INFO("left : shrink selection");
                  self->priv->selection_end_column--;
                }
              }
              GST_INFO("left : %3ld,%3ld -> %3ld,%3ld",self->priv->selection_start_column,self->priv->selection_start_row,self->priv->selection_end_column,self->priv->selection_end_row);
              select=TRUE;
            }
            break;
          case GDK_Right:
            if(self->priv->cursor_column<=tracks) {
              // move cursor
              self->priv->cursor_column++;
              sequence_view_set_cursor_pos(self);
              GST_INFO("right: %3ld,%3ld -> %3ld,%3ld @ %3ld,%3ld",self->priv->selection_start_column,self->priv->selection_start_row,self->priv->selection_end_column,self->priv->selection_end_row,self->priv->cursor_column,self->priv->cursor_row);
              if(self->priv->selection_end_column==-1) {
                GST_INFO("right: new selection");
                self->priv->selection_start_column=self->priv->cursor_column-1;
                self->priv->selection_end_column=self->priv->cursor_column;
                self->priv->selection_start_row=self->priv->cursor_row;
                self->priv->selection_end_row=self->priv->cursor_row;
              }
              else {
                if(self->priv->selection_end_column==(self->priv->cursor_column-1)) {
                  GST_INFO("right: expand selection");
                  self->priv->selection_end_column++;
                }
                else {
                  GST_INFO("right: shrink selection");
                  self->priv->selection_start_column++;
                }
              }
              GST_INFO("right: %3ld,%3ld -> %3ld,%3ld",self->priv->selection_start_column,self->priv->selection_start_row,self->priv->selection_end_column,self->priv->selection_end_row);
              select=TRUE;
            }
            break;
        }
        if(select) {
          gtk_widget_queue_draw(GTK_WIDGET(self->priv->sequence_table));
          res=TRUE;
        }
      }
      else {
        // remove selection
        if(self->priv->selection_start_column!=-1) {
          self->priv->selection_start_column=self->priv->selection_start_row=self->priv->selection_end_column=self->priv->selection_end_row=-1;
          gtk_widget_queue_draw(GTK_WIDGET(self->priv->sequence_table));
        }
      }
    }
    else if(event->keyval == GDK_b) {
      if(modifier==GDK_CONTROL_MASK) {
        GST_INFO("ctrl-b pressed, row %lu",row);
        sequence_view_set_pos(self,SEQUENCE_VIEW_POS_LOOP_START,(glong)row);
        res=TRUE;
      }
    }
    else if(event->keyval == GDK_e) {
      if(modifier==GDK_CONTROL_MASK) {
        GST_INFO("ctrl-e pressed, row %lu",row);
        sequence_view_set_pos(self,SEQUENCE_VIEW_POS_LOOP_END,(glong)row);
        res=TRUE;
      }
    }
    else if(event->keyval == GDK_Insert) {
      if(modifier==0) {
        GST_INFO("insert pressed, row %lu, track %lu",row,track-1);
        bt_sequence_insert_rows(self->priv->sequence,row,track-1,self->priv->bars);
        //self->priv->sequence_length+=self->priv->bars;
        // reinit the view
        sequence_table_refresh(self,song);
        //sequence_calculate_visible_lines(self);
        sequence_model_recolorize(self);
        sequence_view_set_cursor_pos(self);
        res=TRUE;
      }
      else if(modifier==GDK_SHIFT_MASK) {
        GST_INFO("shift-insert pressed, row %lu",row);
        bt_sequence_insert_full_rows(self->priv->sequence,row,self->priv->bars);
        self->priv->sequence_length+=self->priv->bars;
        // reinit the view
        sequence_table_refresh(self,song);
        sequence_calculate_visible_lines(self);
        sequence_model_recolorize(self);
        sequence_view_set_cursor_pos(self);
        res=TRUE;
      }
    }
    else if(event->keyval == GDK_Delete) {
      if(modifier==0) {
        GST_INFO("delete pressed, row %lu, track %lu",row,track-1);
        bt_sequence_delete_rows(self->priv->sequence,row,track-1,self->priv->bars);
        //self->priv->sequence_length-=self->priv->bars;
        // reinit the view
        sequence_table_refresh(self,song);
        //sequence_calculate_visible_lines(self);
        sequence_model_recolorize(self);
        sequence_view_set_cursor_pos(self);
        res=TRUE;
      }
      else if(modifier==GDK_SHIFT_MASK) {
        GST_INFO("shift-delete pressed, row %lu",row);
        bt_sequence_delete_full_rows(self->priv->sequence,row,self->priv->bars);
        self->priv->sequence_length-=self->priv->bars;
        // reinit the view
        sequence_table_refresh(self,song);
        sequence_calculate_visible_lines(self);
        sequence_model_recolorize(self);
        sequence_view_set_cursor_pos(self);
        res=TRUE;
      }
    }

    if(event->keyval<='z' && (modifier==0)) {
      // first column is label
      if((track>0) && (row<length)) {
        BtMachine *machine;

        if((machine=bt_sequence_get_machine(self->priv->sequence,track-1))) {
          gchar *pos=strchr(self->priv->pattern_keys,(gchar)(event->keyval&0xff));

          // reset selection
          self->priv->selection_start_column=-1;
          self->priv->selection_start_row=-1;
          self->priv->selection_end_column=-1;
          self->priv->selection_end_row=-1;

          if(pos) {
            BtPattern *pattern;
            gulong index=(gulong)pos-(gulong)self->priv->pattern_keys;

            GST_INFO("pattern key pressed: '%c' > index: %lu",*pos,index);

            if((pattern=bt_machine_get_pattern_by_index(machine,index))) {
              pattern_usage_changed=!bt_sequence_is_pattern_used(self->priv->sequence,pattern);
              bt_sequence_set_pattern(self->priv->sequence,row,track-1,pattern);
              g_object_get(G_OBJECT(pattern),"name",&str,NULL);
              g_object_unref(pattern);
              free_str=TRUE;
              change=TRUE;
              res=TRUE;
            } else {
              GST_WARNING("no pattern for index %d",index);
            }
          }
          else {
            GST_WARNING("keyval %c not used by machine",(gchar)(event->keyval&0xff));
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
            glong col=g_list_index(columns,(gpointer)column)-1;
            GtkTreePath *cpath;
            //glong row;

            g_list_free(columns);
            gtk_tree_model_filter_convert_iter_to_child_iter(filtered_store,&iter,&filter_iter);
            //gtk_tree_model_get(store,&iter,SEQUENCE_TABLE_POS,&row,-1);
            //GST_INFO("  position is %d,%d -> ",row,col,SEQUENCE_TABLE_PRE_CT+col);

            gtk_list_store_set(GTK_LIST_STORE(store),&iter,SEQUENCE_TABLE_PRE_CT+col,str,-1);
            // move cursor down & set cell focus
            self->priv->cursor_row+=self->priv->bars;
            if((cpath=gtk_tree_path_new_from_indices((self->priv->cursor_row/self->priv->bars),-1))) {
              gtk_tree_view_set_cursor(self->priv->sequence_table,cpath,column,FALSE);
              gtk_tree_path_free(cpath);
            }

            if(pattern_usage_changed) {
              pattern_list_refresh(self);
            }
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

    // release the references
    g_object_unref(song);
    if(free_str) g_free(str);
  }
  return(res);
}

static gboolean on_sequence_header_button_press_event(GtkWidget *widget,GdkEventButton *event,gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  gboolean res=FALSE;

  g_assert(user_data);

  GST_INFO("sequence_header button_press : button 0x%x, type 0x%d",event->button,event->type);
  if(event->button==3) {
    gtk_menu_popup(self->priv->context_menu,NULL,NULL,NULL,NULL,3,gtk_get_current_event_time());
    res=TRUE;
  }
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
          GST_INFO("  left click to column %lu, row %lu",track,row);
          if(widget==GTK_WIDGET(self->priv->sequence_pos_table)) {
            switch(modifier) {
            case 0:
              sequence_view_set_pos(self,SEQUENCE_VIEW_POS_PLAY,(glong)row);
              break;
            case GDK_CONTROL_MASK:
              sequence_view_set_pos(self,SEQUENCE_VIEW_POS_LOOP_START,(glong)row);
              break;
            case GDK_MOD4_MASK:
              sequence_view_set_pos(self,SEQUENCE_VIEW_POS_LOOP_END,(glong)row);
              break;
            }
          }
          else {
            // set cell focus
            gtk_tree_view_set_cursor(self->priv->sequence_table,path,column,FALSE);
            gtk_widget_grab_focus_savely(GTK_WIDGET(self->priv->sequence_table));
            // reset selection
            self->priv->selection_start_column=self->priv->selection_start_row=self->priv->selection_end_column=self->priv->selection_end_row=-1;
          }
          res=TRUE;
        }
      }
      else {
        GST_INFO("clicked outside data area - #1");
        switch(modifier) {
        case 0:
          sequence_view_set_pos(self,SEQUENCE_VIEW_POS_PLAY,-1);
          break;
        case GDK_CONTROL_MASK:
          sequence_view_set_pos(self,SEQUENCE_VIEW_POS_LOOP_START,-1);
          break;
        case GDK_MOD4_MASK:
          sequence_view_set_pos(self,SEQUENCE_VIEW_POS_LOOP_END,-1);
          break;
        }
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
        gtk_widget_grab_focus_savely(GTK_WIDGET(self->priv->sequence_table));
        // cursor updates are not yet processed
        on_sequence_table_cursor_changed_idle(self);
        GST_INFO("cursor new/old: %3ld,%3ld -> %3ld,%3ld",cursor_column,cursor_row,self->priv->cursor_column,self->priv->cursor_row);
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

static gboolean on_sequence_table_scroll_event( GtkWidget *widget, GdkEventScroll *event, gpointer user_data ) {
  //BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);

  if(event) {
    static GdkEventKey keyevent={0,};

    if( event->direction == GDK_SCROLL_UP ) {
      keyevent.keyval = GDK_Up;
      keyevent.hardware_keycode = 98;
    }
    else if( event->direction == GDK_SCROLL_DOWN ) {
      keyevent.keyval = GDK_Down;
      keyevent.hardware_keycode = 104;
    }
    else
      return FALSE;

    keyevent.window = event->window;
    keyevent.state = event->state;
    keyevent.time = GDK_CURRENT_TIME;
    /*
    keyevent.send_event = 0;
    keyevent.length = 0;
    keyevent.string = 0;
    keyevent.group =  0;
    */
    
    keyevent.type = GDK_KEY_PRESS;
    //g_signal_emit_by_name(G_OBJECT(self->priv->sequence_table), "key-press-event", &keyevent );
    gtk_main_do_event((GdkEvent *)&keyevent);
    
    keyevent.type = GDK_KEY_RELEASE;
    //g_signal_emit_by_name(G_OBJECT(self->priv->sequence_table), "key-release-event", &keyevent );
    gtk_main_do_event((GdkEvent *)&keyevent);

    return TRUE;
  }

  return FALSE;
}


static void on_machine_added(BtSetup *setup,BtMachine *machine,gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);

  g_assert(user_data);

  GST_INFO("new machine %p,ref_count=%d has been added",machine,G_OBJECT(machine)->ref_count);
  machine_menu_refresh(self,setup);
  if(BT_IS_SOURCE_MACHINE(machine)) {
    sequence_add_track(self,machine);
  }
  GST_INFO("... new machine %p,ref_count=%d has been added",machine,G_OBJECT(machine)->ref_count);
}

static void on_machine_removed(BtSetup *setup,BtMachine *machine,gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  BtSong *song;

  g_assert(user_data);
  g_return_if_fail(BT_IS_MACHINE(machine));

  GST_INFO("machine %p,ref_count=%d has been removed",machine,G_OBJECT(machine)->ref_count);

  // reinit the menu
  machine_menu_refresh(self,setup);

  // get song from app and then setup from song
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);

  bt_sequence_remove_track_by_machine(self->priv->sequence,machine);
  // reinit the view
  sequence_table_refresh(self,song);
  sequence_model_recolorize(self);

  g_object_unref(song);
  //GST_INFO("... machine %p,ref_count=%d has been removed",machine,G_OBJECT(machine)->ref_count);
}

static void on_pattern_changed(BtMachine *machine,BtPattern *pattern,gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  BtSong *song;

  g_assert(user_data);

  GST_INFO("pattern has been added/removed");
  // reinit the list
  pattern_list_refresh(self);

  // get song from app and then setup from song
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  // reinit the sequence view
  sequence_table_refresh(self,song);
  sequence_model_recolorize(self);
  g_object_unref(song);
}

//-- helper methods

static gboolean update_bars_menu(const BtMainPageSequence *self,gulong bars) {
  GtkListStore *store;
  GtkTreeIter iter;
  gchar str[5];
  gulong i,j;
  /* the useful stepping depends on the rythm
     beats=bars/tpb
     bars=16, beats=4, tpb=4 : 4/4 -> 1,8, 16,32,64
     bars=12, beats=3, tpb=4 : 3/4 -> 1,6, 12,24,48
     bars=18, beats=3, tpb=6 : 3/6 -> 1,9, 18,36,72
  */
  store=gtk_list_store_new(1,G_TYPE_STRING);

  // single steps
  gtk_list_store_append(store,&iter);
  gtk_list_store_set(store,&iter,0,"1",-1);
  // half bars
  sprintf(str,"%lu",bars/2);
  gtk_list_store_append(store,&iter);
  gtk_list_store_set(store,&iter,0,str,-1);
  // add multiple of bars
  for(j=0,i=bars;j<4;i*=2,j++) {
    sprintf(str,"%lu",i);
    gtk_list_store_append(store,&iter);
    gtk_list_store_set(store,&iter,0,str,-1);
  }
  gtk_combo_box_set_model(self->priv->bars_menu,GTK_TREE_MODEL(store));
  // @todo: we should remember the bars-filter with the song
  gtk_combo_box_set_active(self->priv->bars_menu,2);
  g_object_unref(store); // drop with combobox

  return(TRUE);
}

static void on_song_info_bars_changed(const BtSongInfo *song_info,GParamSpec *arg,gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  glong bars;

  g_assert(user_data);

  g_object_get(G_OBJECT(song_info),"bars",&bars,NULL);
  // this also recolors the sequence
  update_bars_menu(self,bars);
}

static void on_song_changed(const BtEditApplication *app,GParamSpec *arg,gpointer user_data) {
  BtMainPageSequence *self=BT_MAIN_PAGE_SEQUENCE(user_data);
  BtSong *song;
  BtSongInfo *song_info;
  BtSetup *setup;
  GstBin *bin;
  GstBus *bus;
  glong bars;
  glong loop_start_pos,loop_end_pos;
  gulong sequence_length;
  gdouble loop_start,loop_end;

  g_assert(user_data);

  GST_INFO("song has changed : app=%p, self=%p",app,self);
  // get song from app and then setup from song
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  if(!song) return;
  GST_INFO("song->ref_ct=%d",G_OBJECT(song)->ref_count);

  g_object_try_unref(self->priv->sequence);
  g_object_get(G_OBJECT(song),"song-info",&song_info,"setup",&setup,"sequence",&self->priv->sequence,"bin", &bin,NULL);
  g_object_get(G_OBJECT(self->priv->sequence),"length",&sequence_length,"loop-start",&loop_start_pos,"loop-end",&loop_end_pos,NULL);
  // make sequence_length and step_filter_pos accord to song length
  self->priv->sequence_length=self->priv->row_filter_pos=sequence_length;

  // reset vu-meter hash (rebuilt below)
  if(self->priv->level_to_vumeter) g_hash_table_destroy(self->priv->level_to_vumeter);
  self->priv->level_to_vumeter=g_hash_table_new_full(NULL,NULL,(GDestroyNotify)gst_object_unref,NULL);

  // reset cursor pos
  self->priv->cursor_column=1;
  self->priv->cursor_row=0;
  
  // update page
  // update sequence and pattern list
  sequence_table_refresh(self,song);
  sequence_table_refresh_labels(self);
  update_after_track_changed(self);
  machine_menu_refresh(self,setup);
  g_signal_connect(G_OBJECT(setup),"machine-added",G_CALLBACK(on_machine_added),(gpointer)self);
  g_signal_connect(G_OBJECT(setup),"machine-removed",G_CALLBACK(on_machine_removed),(gpointer)self);
  // update toolbar
  g_object_get(G_OBJECT(song_info),"bars",&bars,NULL);
  update_bars_menu(self,bars);

  // update sequence view
  sequence_calculate_visible_lines(self);
  sequence_model_recolorize(self);
  loop_start=(loop_start_pos>-1)?(gdouble)loop_start_pos/(gdouble)sequence_length:0.0;
  loop_end  =(loop_end_pos  >-1)?(gdouble)loop_end_pos  /(gdouble)sequence_length:1.0;
  g_object_set(self->priv->sequence_table,"play-position",0.0,"loop-start",loop_start,"loop-end",loop_end,NULL);
  g_object_set(self->priv->sequence_pos_table,"play-position",0.0,"loop-start",loop_start,"loop-end",loop_end,NULL);
  // vumeters
  bus=gst_element_get_bus(GST_ELEMENT(bin));
  g_signal_connect(bus, "message::element", G_CALLBACK(on_track_level_change), (gpointer)self);
  gst_object_unref(bus);
  if(self->priv->clock) gst_object_unref(self->priv->clock);
  self->priv->clock=gst_pipeline_get_clock (GST_PIPELINE(bin));

  // subscribe to play-pos changes of song->sequence
  g_signal_connect(G_OBJECT(song), "notify::play-pos", G_CALLBACK(on_song_play_pos_notify), (gpointer)self);
  g_signal_connect(G_OBJECT(song),"notify::is-playing",G_CALLBACK(on_song_is_playing_notify),(gpointer)self);
  // subscribe to changes in the rythm
  g_signal_connect(G_OBJECT(song_info), "notify::bars", G_CALLBACK(on_song_info_bars_changed), (gpointer)self);
  //-- release the references
  gst_object_unref(bin);
  g_object_unref(song_info);
  g_object_unref(setup);
  g_object_unref(song);
  GST_INFO("song has changed done");
}

static gboolean bt_main_page_sequence_init_ui(const BtMainPageSequence *self,const BtMainPages *pages) {
  GtkWidget *toolbar;
  GtkWidget *split_box,*box,*vbox,*tool_item;
  GtkWidget *scrolled_window,*scrolled_vsync_window,*scrolled_hsync_window;
  GtkWidget *hsync_viewport;
  GtkWidget *menu_item,*image;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *tree_col;
  GtkTreeSelection *tree_sel;
  GtkAdjustment *vadjust, *hadjust;
#if !GTK_CHECK_VERSION(2,12,0)
  GtkTooltips *tips=gtk_tooltips_new();
#endif

  GST_DEBUG("!!!! self=%p",self);

  gtk_widget_set_name(GTK_WIDGET(self),"sequence view");

  // add toolbar
  toolbar=gtk_toolbar_new();
  gtk_widget_set_name(toolbar,"sequence view toolbar");
  gtk_box_pack_start(GTK_BOX(self),toolbar,FALSE,FALSE,0);
  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar),GTK_TOOLBAR_BOTH);
  // add toolbar widgets
  // steps
  box=gtk_hbox_new(FALSE,2);
  gtk_container_set_border_width(GTK_CONTAINER(box),4);
  // build the menu
  self->priv->bars_menu=GTK_COMBO_BOX(gtk_combo_box_new());
  gtk_widget_set_tooltip_text(GTK_WIDGET(self->priv->bars_menu),_("Show every n-th line"));
  renderer=gtk_cell_renderer_text_new();
  //gtk_cell_renderer_set_fixed_size(renderer, 1, -1);
  gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer), 1);
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(self->priv->bars_menu),renderer,TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(self->priv->bars_menu),renderer,"text", 0,NULL);
  g_signal_connect(G_OBJECT(self->priv->bars_menu),"changed",G_CALLBACK(on_bars_menu_changed), (gpointer)self);
  gtk_box_pack_start(GTK_BOX(box),gtk_label_new(_("Steps")),FALSE,FALSE,2);
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->priv->bars_menu),TRUE,TRUE,2);

  tool_item=GTK_WIDGET(gtk_tool_item_new());
  gtk_widget_set_name(tool_item,"Steps");
  gtk_container_add(GTK_CONTAINER(tool_item),box);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar),GTK_TOOL_ITEM(tool_item),-1);

  // get colors
  self->priv->cursor_bg=bt_ui_resources_get_gdk_color(BT_UI_RES_COLOR_CURSOR);
  self->priv->selection_bg1=bt_ui_resources_get_gdk_color(BT_UI_RES_COLOR_SELECTION1);
  self->priv->selection_bg2=bt_ui_resources_get_gdk_color(BT_UI_RES_COLOR_SELECTION2);
  self->priv->source_bg1=bt_ui_resources_get_gdk_color(BT_UI_RES_COLOR_SOURCE_MACHINE_BRIGHT1);
  self->priv->source_bg2=bt_ui_resources_get_gdk_color(BT_UI_RES_COLOR_SOURCE_MACHINE_BRIGHT2);
  self->priv->processor_bg1=bt_ui_resources_get_gdk_color(BT_UI_RES_COLOR_PROCESSOR_MACHINE_BRIGHT1);
  self->priv->processor_bg2=bt_ui_resources_get_gdk_color(BT_UI_RES_COLOR_PROCESSOR_MACHINE_BRIGHT2);
  self->priv->sink_bg1=bt_ui_resources_get_gdk_color(BT_UI_RES_COLOR_SINK_MACHINE_BRIGHT1);
  self->priv->sink_bg2=bt_ui_resources_get_gdk_color(BT_UI_RES_COLOR_SINK_MACHINE_BRIGHT2);

  // generate the context menu
  self->priv->accel_group=gtk_accel_group_new();
  self->priv->context_menu=GTK_MENU(g_object_ref_sink(G_OBJECT(gtk_menu_new())));
  gtk_menu_set_accel_group(GTK_MENU(self->priv->context_menu), self->priv->accel_group);
  gtk_menu_set_accel_path(GTK_MENU(self->priv->context_menu),"<Buzztard-Main>/SequenceView/SequenceContext");

  self->priv->context_menu_add=GTK_MENU_ITEM(gtk_image_menu_item_new_with_label(_("Add track")));
  image=gtk_image_new_from_stock(GTK_STOCK_ADD,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(self->priv->context_menu_add),image);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),GTK_WIDGET(self->priv->context_menu_add));
  gtk_widget_show(GTK_WIDGET(self->priv->context_menu_add));

  menu_item=gtk_image_menu_item_new_with_label(_("Remove track"));
  image=gtk_image_new_from_stock(GTK_STOCK_REMOVE,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_menu_item_set_accel_path (GTK_MENU_ITEM (menu_item), "<Buzztard-Main>/SequenceView/SequenceContext/RemoveTrack");
  gtk_accel_map_add_entry ("<Buzztard-Main>/SequenceView/SequenceContext/RemoveTrack", GDK_Delete, GDK_CONTROL_MASK);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_track_remove_activated),(gpointer)self);

  menu_item=gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_set_sensitive(menu_item,FALSE);
  gtk_widget_show(menu_item);

  menu_item=gtk_image_menu_item_new_with_label(_("Move track left"));
  image=gtk_image_new_from_stock(GTK_STOCK_GO_BACK,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_menu_item_set_accel_path (GTK_MENU_ITEM (menu_item), "<Buzztard-Main>/SequenceView/SequenceContext/MoveTrackLeft");
  gtk_accel_map_add_entry ("<Buzztard-Main>/SequenceView/SequenceContext/MoveTrackLeft", GDK_Left, GDK_CONTROL_MASK);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_track_move_left_activated),(gpointer)self);

  menu_item=gtk_image_menu_item_new_with_label(_("Move track right"));
  image=gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_menu_item_set_accel_path (GTK_MENU_ITEM (menu_item), "<Buzztard-Main>/SequenceView/SequenceContext/MoveTrackRight");
  gtk_accel_map_add_entry ("<Buzztard-Main>/SequenceView/SequenceContext/MoveTrackRight", GDK_Right, GDK_CONTROL_MASK);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_track_move_right_activated),(gpointer)self);

  // --
  // @todo cut, copy, paste


  // add a hpaned
  split_box=gtk_hpaned_new();
  gtk_container_add(GTK_CONTAINER(self),split_box);

  // add hbox for sequence view
  box=gtk_hbox_new(FALSE,0);
  gtk_paned_pack1(GTK_PANED(split_box),box,TRUE,TRUE);

  // add sequence-pos list-view
  vbox=gtk_vbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(box), vbox, FALSE, FALSE, 0);
  self->priv->sequence_pos_table_header=GTK_HBOX(gtk_hbox_new(FALSE,0));
  gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(self->priv->sequence_pos_table_header), FALSE, FALSE, 0);

  scrolled_vsync_window=gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_vsync_window),GTK_POLICY_NEVER,GTK_POLICY_NEVER);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_vsync_window),GTK_SHADOW_NONE);
  self->priv->sequence_pos_table=GTK_TREE_VIEW(bt_sequence_view_new(self->priv->app));
  g_object_set(self->priv->sequence_pos_table,
    "enable-search",FALSE,
    "rules-hint",TRUE,
    "fixed-height-mode",TRUE,
    "headers-visible", FALSE,
    NULL);
  // set a minimum size, otherwise the window can't be shrinked (we need this because of GTK_POLICY_NEVER)
  gtk_widget_set_size_request(GTK_WIDGET(self->priv->sequence_pos_table),POSITION_CELL_WIDTH,40);
  tree_sel=gtk_tree_view_get_selection(self->priv->sequence_pos_table);
  gtk_tree_selection_set_mode(tree_sel,GTK_SELECTION_NONE);
  sequence_pos_table_init(self);
  gtk_container_add(GTK_CONTAINER(scrolled_vsync_window),GTK_WIDGET(self->priv->sequence_pos_table));
  gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(scrolled_vsync_window), TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(self->priv->sequence_pos_table), "button-press-event", G_CALLBACK(on_sequence_table_button_press_event), (gpointer)self);

  // add vertical separator
  gtk_box_pack_start(GTK_BOX(box), gtk_vseparator_new(), FALSE, FALSE, 0);

  // build label menu
  self->priv->label_menu=GTK_COMBO_BOX(gtk_combo_box_new());
  gtk_widget_set_tooltip_text(GTK_WIDGET(self->priv->label_menu),_("Browse to labels in the sequence"));
  renderer=gtk_cell_renderer_text_new();
  gtk_cell_renderer_set_fixed_size(renderer, 1, -1);
  gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer), 1);
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(self->priv->label_menu),renderer,FALSE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(self->priv->label_menu),renderer,"text",POSITION_MENU_POSSTR,NULL);
  renderer=gtk_cell_renderer_text_new();
  gtk_cell_renderer_set_fixed_size(renderer, 1, -1);
  gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer), 1);
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(self->priv->label_menu),renderer,TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(self->priv->label_menu),renderer,"text",POSITION_MENU_LABEL,NULL);
  g_signal_connect(G_OBJECT(self->priv->label_menu),"changed",G_CALLBACK(on_label_menu_changed), (gpointer)self);

  vbox=gtk_vbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(box), vbox, TRUE, TRUE, 0);
  
  // add sequence header list-view
  scrolled_hsync_window=gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_hsync_window),GTK_POLICY_NEVER,GTK_POLICY_NEVER);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_hsync_window),GTK_SHADOW_NONE);
  self->priv->sequence_table_header=GTK_HBOX(gtk_hbox_new(FALSE,0));
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_hsync_window),GTK_WIDGET(self->priv->sequence_table_header));
  hsync_viewport=gtk_bin_get_child(GTK_BIN(scrolled_hsync_window));
  gtk_viewport_set_shadow_type(GTK_VIEWPORT(hsync_viewport),GTK_SHADOW_NONE);
  // set a minimum size, otherwise the window can't be shrinked (we need this because of GTK_POLICY_NEVER)
  gtk_widget_set_size_request(GTK_WIDGET(hsync_viewport),SEQUENCE_CELL_WIDTH,-1);
  /* DEBUG
  g_signal_connect(G_OBJECT(self->priv->sequence_table_header),"size-allocate",G_CALLBACK(on_sequence_header_size_allocate),(gpointer)"box");
  g_signal_connect(G_OBJECT(self->priv->sequence_table_header),"size-allocate",G_CALLBACK(on_sequence_header_size_allocate),(gpointer)"vport");
  // DEBUG */
  
  gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(scrolled_hsync_window), FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(scrolled_hsync_window), "button-press-event", G_CALLBACK(on_sequence_header_button_press_event), (gpointer)self);

  // add sequence list-view
  scrolled_window=gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_NONE);
  self->priv->sequence_table=GTK_TREE_VIEW(bt_sequence_view_new(self->priv->app));
  g_object_set(self->priv->sequence_table,
    "enable-search",FALSE,
    "rules-hint",TRUE,
    "fixed-height-mode",TRUE,
    "headers-visible", FALSE,
    NULL);
  tree_sel=gtk_tree_view_get_selection(self->priv->sequence_table);
  gtk_tree_selection_set_mode(tree_sel,GTK_SELECTION_NONE);
  sequence_table_init(self);
  gtk_container_add(GTK_CONTAINER(scrolled_window),GTK_WIDGET(self->priv->sequence_table));
  gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(scrolled_window), TRUE, TRUE, 0);
  g_signal_connect_after(G_OBJECT(self->priv->sequence_table), "cursor-changed", G_CALLBACK(on_sequence_table_cursor_changed), (gpointer)self);
  g_signal_connect(G_OBJECT(self->priv->sequence_table), "key-release-event", G_CALLBACK(on_sequence_table_key_release_event), (gpointer)self);
  g_signal_connect(G_OBJECT(self->priv->sequence_table), "button-press-event", G_CALLBACK(on_sequence_table_button_press_event), (gpointer)self);
  g_signal_connect(G_OBJECT(self->priv->sequence_table), "motion-notify-event", G_CALLBACK(on_sequence_table_motion_notify_event), (gpointer)self);
  g_signal_connect(G_OBJECT(self->priv->sequence_table), "scroll-event", G_CALLBACK(on_sequence_table_scroll_event), (gpointer)self);
  gtk_widget_set_name(GTK_WIDGET(self->priv->sequence_table),"sequence editor");

  // make pos scrolled-window also use the vertical-scrollbar of the sequence scrolled-window
  vadjust=gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled_window));
  gtk_scrolled_window_set_vadjustment(GTK_SCROLLED_WINDOW(scrolled_vsync_window),vadjust);
  // make header scrolled-window also use the horizontal-scrollbar of the sequence scrolled-window
  hadjust=gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scrolled_window));
  gtk_scrolled_window_set_hadjustment(GTK_SCROLLED_WINDOW(scrolled_hsync_window),hadjust);
  //GST_DEBUG("pos_view=%p, data_view=%p", self->priv->sequence_pos_table,self->priv->sequence_table);

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
  gtk_widget_set_name(GTK_WIDGET(self->priv->pattern_list),"sequence-pattern-list");

  renderer=gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer),
    "xalign",1.0,
    NULL);
  gtk_cell_renderer_set_fixed_size(renderer, 1, -1);
  gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer), 1);
  if((tree_col=gtk_tree_view_column_new_with_attributes(_("Key"),renderer,
    "text",PATTERN_TABLE_KEY,
    "sensitive",PATTERN_TABLE_USED,
    NULL))
  ) {
    g_object_set(tree_col,"sizing",GTK_TREE_VIEW_COLUMN_FIXED,"fixed-width",30,NULL);
    gtk_tree_view_insert_column(self->priv->pattern_list,tree_col,-1);
  }
  else GST_WARNING("can't create treeview column");

  renderer=gtk_cell_renderer_text_new();
  gtk_cell_renderer_set_fixed_size(renderer, 1, -1);
  gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer), 1);
  if((tree_col=gtk_tree_view_column_new_with_attributes(_("Patterns"),renderer,
    "text",PATTERN_TABLE_NAME,
    "sensitive",PATTERN_TABLE_USED,
    NULL))
  ) {
    g_object_set(tree_col,"sizing",GTK_TREE_VIEW_COLUMN_FIXED,"fixed-width",70,NULL);
    gtk_tree_view_insert_column(self->priv->pattern_list,tree_col,-1);
  }
  else GST_WARNING("can't create treeview column");

  gtk_container_add(GTK_CONTAINER(scrolled_window),GTK_WIDGET(self->priv->pattern_list));
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(scrolled_window),TRUE,TRUE,0);
  //gtk_paned_pack2(GTK_PANED(split_box),GTK_WIDGET(scrolled_window),FALSE,FALSE);

  // set default widget
  //g_signal_connect_after(GTK_WIDGET(self->priv->sequence_table),"realize",G_CALLBACK(on_sequence_view_realized),(gpointer)self);
  gtk_container_set_focus_child(GTK_CONTAINER(self),GTK_WIDGET(self->priv->sequence_table));
  // register event handlers
  g_signal_connect(G_OBJECT(self->priv->app), "notify::song", G_CALLBACK(on_song_changed), (gpointer)self);
  // listen to page changes
  g_signal_connect(G_OBJECT(pages), "switch-page", G_CALLBACK(on_page_switched), (gpointer)self);

  GST_DEBUG("  done");
  return(TRUE);
}

//-- constructor methods

/**
 * bt_main_page_sequence_new:
 * @app: the application the window belongs to
 * @pages: the page collection
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtMainPageSequence *bt_main_page_sequence_new(const BtEditApplication *app,const BtMainPages *pages) {
  BtMainPageSequence *self;

  if(!(self=BT_MAIN_PAGE_SEQUENCE(g_object_new(BT_TYPE_MAIN_PAGE_SEQUENCE,"app",app,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_main_page_sequence_init_ui(self,pages)) {
    goto Error;
  }
  return(self);
Error:
  if(self) gtk_object_destroy(GTK_OBJECT(self));
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
  return(bt_sequence_get_machine(self->priv->sequence,self->priv->cursor_column-1));
}

//-- cut/copy/paste

/**
 * bt_main_page_sequence_delete_selection:
 * @self: the sequence subpage
 *
 * Delete (clear) the selected area.
 */
void bt_main_page_sequence_delete_selection(const BtMainPageSequence *self) {
  GtkTreeModel *store;
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

  GST_INFO("delete sequence region: %3ld,%3ld -> %3ld,%3ld",selection_start_column,selection_start_row,selection_end_column,selection_end_row);

  if((store=sequence_model_get_store(self))) {
    GtkTreePath *path;

    if((path=gtk_tree_path_new_from_indices(selection_start_row,-1))) {
      GtkTreeIter iter;

      if(gtk_tree_model_get_iter(store,&iter,path)) {
        glong i,j;

        for(i=selection_start_row;i<=selection_end_row;i++) {
          for(j=selection_start_column-1;j<selection_end_column;j++) {
            GST_DEBUG("  delete sequence cell: %3ld,%3ld",j,i);
            bt_sequence_set_pattern(self->priv->sequence,i,j,NULL);
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
        GST_WARNING("  can't get tree-iter for row %ld",selection_start_row);
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
}

/**
 * bt_main_page_sequence_cut_selection:
 * @self: the sequence subpage
 *
 * Cut selected area.
 * <note>not yet working</note>
 */
void bt_main_page_sequence_cut_selection(const BtMainPageSequence *self) {
  /* @todo implement me */
#if 0
- like copy, but clear pattern cells afterwards
#endif
  bt_main_page_sequence_delete_selection(self);
}

/**
 * bt_main_page_sequence_copy_selection:
 * @self: the sequence subpage
 *
 * Copy selected area.
 * <note>not yet working</note>
 */
void bt_main_page_sequence_copy_selection(const BtMainPageSequence *self) {
  /* @todo implement me */
#if 0
- store BtPattern **patterns;
- remember selection (track start/end and time start/end)
#endif
}

/**
 * bt_main_page_sequence_paste_selection:
 * @self: the sequence subpage
 *
 * Paste at the top of the selected area.
 * <note>not yet working</note>
 */
void bt_main_page_sequence_paste_selection(const BtMainPageSequence *self) {
  /* @todo implement me */
#if 0
- we can paste at any timeoffset
  - maybe extend sequence if pos+selection.length> sequence.length)
- we need to check if the tracks match
#endif
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
    case MAIN_PAGE_SEQUENCE_CURSOR_ROW: {
      g_value_set_long(value, self->priv->cursor_row);
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
  BtSong *song;

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);

  // @bug: http://bugzilla.gnome.org/show_bug.cgi?id=414712
  gtk_container_set_focus_child(GTK_CONTAINER(self),NULL);

  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  if(song) {
    BtSongInfo *song_info;
    GstBin *bin;
    GstBus *bus;

    GST_DEBUG("disconnect handlers from song=%p",song);
    g_object_get(G_OBJECT(song),"song-info",&song_info,"bin", &bin,NULL);

    g_signal_handlers_disconnect_matched(song,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_song_play_pos_notify,NULL);
    g_signal_handlers_disconnect_matched(song,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_song_is_playing_notify,NULL);
    g_signal_handlers_disconnect_matched(song_info,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_song_info_bars_changed,NULL);

    bus=gst_element_get_bus(GST_ELEMENT(bin));
    g_signal_handlers_disconnect_matched(bus, G_SIGNAL_MATCH_FUNC,0,0,NULL,on_track_level_change,NULL);
    gst_object_unref(bus);

    gst_object_unref(bin);
    g_object_unref(song_info);
    g_object_unref(song);
  }

  g_object_try_unref(self->priv->sequence);

  g_object_try_weak_unref(self->priv->app);
  if(self->priv->machine) {
    GST_INFO("unref old cur-machine: %p,refs=%d",self->priv->machine,(G_OBJECT(self->priv->machine))->ref_count);
    if(self->priv->pattern_added_handler)
      g_signal_handler_disconnect(G_OBJECT(self->priv->machine),self->priv->pattern_added_handler);
    if(self->priv->pattern_removed_handler)
      g_signal_handler_disconnect(G_OBJECT(self->priv->machine),self->priv->pattern_removed_handler);
    g_object_unref(self->priv->machine);
  }

  gtk_widget_destroy(GTK_WIDGET(self->priv->context_menu));
  g_object_unref(G_OBJECT(self->priv->context_menu));

  g_object_try_unref(self->priv->accel_group);

  g_hash_table_destroy(self->priv->level_to_vumeter);
  if(self->priv->clock) gst_object_unref(self->priv->clock);

  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_main_page_sequence_finalize(GObject *object) {
  BtMainPageSequence *self = BT_MAIN_PAGE_SEQUENCE(object);

  GST_DEBUG("!!!! self=%p",self);
  g_mutex_free (self->priv->lock);

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
  self->priv->row_filter_pos=SEQUENCE_ROW_ADDITION_INTERVAL;
  self->priv->sequence_length=SEQUENCE_ROW_ADDITION_INTERVAL;
  
  self->priv->lock=g_mutex_new ();
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
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,MAIN_PAGE_SEQUENCE_CURSOR_ROW,
                                  g_param_spec_long("cursor-row",
                                     "cursor-row prop",
                                     "position of the cursor in the sequence view in bars",
                                     0,
                                     G_MAXLONG,  // loop-positions are LONG as well
                                     0,
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));
}

GType bt_main_page_sequence_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof(BtMainPageSequenceClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_main_page_sequence_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtMainPageSequence),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_main_page_sequence_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(GTK_TYPE_VBOX,"BtMainPageSequence",&info,0);
  }
  return type;
}
