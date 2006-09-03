/* $Id: main-page-patterns.c,v 1.98 2006-09-03 13:34:34 ensonic Exp $
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
 * SECTION:btmainpagepatterns
 * @short_description: the editor main pattern page
 * @see_also: #BtPatternView
 */
 
/* @todo: main-page-patterns tasks
 * - focus pattern_view after switching patterns, entering the page ?
 * - test wheter we can use pango-markup for tree-view labels to make font
 *   smaller
 * - need dividers for global and voice data (2 pixel wide column?)
 * - shortcuts
 *   - Ctrl-I : Interpolate
 *     - from min/max of parameter or content of start/end cell (also multi-column)
 *     - what if only start or end is given?
 *   - Ctrl-R : Randomize
 *     - from min/max of parameter or content of start/end cell (also multi-column)
 *   - Ctrl-S : Smooth
 *     - low pass median filter over changes
 * - copy gtk_cell_renderer_progress -> bt_cell_renderer_pattern_value
 * - add parameter info when inside cell
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
  union {
    BtEditApplication *app;
    gpointer app_ptr;
  };
  
  /* machine selection menu */
  GtkComboBox *machine_menu;
  /* pattern selection menu */
  GtkComboBox *pattern_menu;
  /* wavetable selection menu */
  GtkComboBox *wavetable_menu;
  /* base octave selection menu */
  GtkComboBox *base_octave_menu;

  /* the pattern table */
  GtkTreeView *pattern_pos_table;
  GtkTreeView *pattern_table;

  /* pattern context_menu */
  GtkMenu *context_menu;
  GtkWidget *context_menu_track_add,*context_menu_track_remove;
  GtkWidget *context_menu_pattern_properties,*context_menu_pattern_remove,*context_menu_pattern_copy;

  /* colors */
  GdkColor *cursor_bg;
  GdkColor *selection_bg1,*selection_bg2;

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

typedef enum {
  UPDATE_COLUMN_POP=1,
  UPDATE_COLUMN_PUSH,
  UPDATE_COLUMN_UPDATE
} BtPatternViewUpdateColumn;

#define PATTERN_CELL_WIDTH 70
#define PATTERN_CELL_HEIGHT 28

static GQuark column_index_quark=0;
static GQuark voice_index_quark=0;

//-- tree cell data functions

static void selection_cell_data_function(GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  gulong row,column;
  GdkColor *bg_col=NULL;
  gchar *str=NULL;

  column=GPOINTER_TO_UINT(g_object_get_qdata(G_OBJECT(col),column_index_quark));

  gtk_tree_model_get(model,iter,
    PATTERN_TABLE_POS,&row,
    PATTERN_TABLE_PRE_CT+column,&str,
    -1);

  //GST_INFO("col/row: %3d/%3d <-> %3d/%3d",column,row,self->priv->cursor_column,self->priv->cursor_row);
  
  if((column==self->priv->cursor_column) && (row==self->priv->cursor_row)) {
    bg_col=self->priv->cursor_bg;
  }
  else if((column>=self->priv->selection_start_column) && (column<=self->priv->selection_end_column) &&
    (row>=self->priv->selection_start_row) && (row<=self->priv->selection_end_row)
  ) {
    bg_col=(row&1)?self->priv->selection_bg2:self->priv->selection_bg1;
  }
  g_object_set(G_OBJECT(renderer),
    "background-gdk",bg_col,
    "text",str,
     NULL);
  g_free(str);
}


//-- tree model helper

static void machine_model_get_iter_by_machine(GtkTreeModel *store,GtkTreeIter *iter,BtMachine *that_machine) {
  BtMachine *this_machine;

  gtk_tree_model_get_iter_first(store,iter);
  do {
    gtk_tree_model_get(store,iter,MACHINE_MENU_MACHINE,&this_machine,-1);
    if(this_machine==that_machine) {
      g_object_unref(this_machine);
      break;
    }
    g_object_unref(this_machine);
  } while(gtk_tree_model_iter_next(store,iter));
}

static void pattern_model_get_iter_by_pattern(GtkTreeModel *store,GtkTreeIter *iter,BtPattern *that_pattern) {
  BtPattern *this_pattern;

  gtk_tree_model_get_iter_first(store,iter);
  do {
    gtk_tree_model_get(store,iter,PATTERN_MENU_PATTERN,&this_pattern,-1);
    if(this_pattern==that_pattern) {
      g_object_unref(this_pattern);
      break;
    }
    g_object_unref(this_pattern);
  } while(gtk_tree_model_iter_next(store,iter));
}

static gboolean pattern_view_get_cursor_pos(GtkTreeView *tree_view,GtkTreePath *path,GtkTreeViewColumn *column,gulong *col,gulong *row) {
  gboolean res=FALSE;
  GtkTreeModel *store;
  GtkTreeIter iter;
  
  if((store=gtk_tree_view_get_model(tree_view))) {
    if(gtk_tree_model_get_iter(GTK_TREE_MODEL(store),&iter,path)) {
      if(col) {
        GList *columns=gtk_tree_view_get_columns(tree_view);
        *col=g_list_index(columns,(gpointer)column);
        g_list_free(columns);
      }
      if(row) {
        gtk_tree_model_get(store,&iter,PATTERN_TABLE_POS,row,-1);
      }
      res=TRUE;
    }
  }
  else {
    GST_WARNING("  can't get tree-model");
  }
  return(res);
}

//-- status bar helpers

static void pattern_view_update_column_description(const BtMainPagePatterns *self, BtPatternViewUpdateColumn mode) {
  BtMainWindow *main_window;
  BtMainStatusbar *statusbar;
   
  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
  // called too early
  if(!main_window) return;
  g_object_get(main_window,"statusbar",&statusbar,NULL);
  
  // pop previous test by passing str=NULL;
  if(mode&UPDATE_COLUMN_POP)
    g_object_set(statusbar,"status",NULL,NULL);

  if(mode&UPDATE_COLUMN_PUSH) {
    const gchar *str;

    if(self->priv->pattern) {
      BtMachine *machine;
      GParamSpec *property;
      gulong global_params,voice_params,col;

      g_object_get(self->priv->pattern,"machine",&machine,NULL);
      g_object_get(machine,
        "global-params",&global_params,
        "voice-params",&voice_params,
        NULL);
      col=self->priv->cursor_column;
      
      if(col<global_params)
        property=bt_machine_get_global_param_spec(machine,col);
      else
        property=bt_machine_get_voice_param_spec(machine,(col-global_params)%voice_params);
      str=g_param_spec_get_blurb(property);

      g_object_unref(machine);
    }
    else {
      str=BT_MAIN_STATUSBAR_DEFAULT;
    }
    g_object_set(statusbar,"status",str,NULL);

  }

  g_object_unref(statusbar);
  g_object_unref(main_window);
}

//-- event handlers

static gboolean on_page_switched_idle(gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);

  gtk_widget_grab_focus(GTK_WIDGET(self->priv->pattern_table));
  return(FALSE);
}
  
static void on_page_switched(GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  static gint prev_page_num=-1;

  if(page_num==BT_MAIN_PAGES_PATTERNS_PAGE) {
    GST_DEBUG("enter pattern page");
    // only set new
    pattern_view_update_column_description(self,UPDATE_COLUMN_PUSH);
    // delay the pattern-table grab
    g_idle_add_full(G_PRIORITY_HIGH_IDLE,on_page_switched_idle,user_data,NULL);
  }
  else {
    // only do this if the page was BT_MAIN_PAGES_PATTERNS_PAGE
    if(prev_page_num == BT_MAIN_PAGES_PATTERNS_PAGE) {
      // only reset old
      GST_DEBUG("leave pattern page");
      pattern_view_update_column_description(self,UPDATE_COLUMN_POP);
    }
  }
  prev_page_num = page_num;
}

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

static gboolean on_pattern_table_cursor_changed_idle(gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  GtkTreePath *path;
  GtkTreeViewColumn *column;
  gulong cursor_column,cursor_row;

  g_return_val_if_fail(user_data,FALSE);

  //GST_INFO("pattern cursor has changed : self=%p",user_data);
  
  gtk_tree_view_get_cursor(self->priv->pattern_table,&path,&column);
  if(pattern_view_get_cursor_pos(self->priv->pattern_table,path,column,&cursor_column,&cursor_row)) {
    if(self->priv->cursor_column!=cursor_column) {
      // update statusbar
      self->priv->cursor_column=cursor_column;
      pattern_view_update_column_description(self,UPDATE_COLUMN_UPDATE);
    }
    self->priv->cursor_row=cursor_row;
    GST_INFO("cursor has changed: %3d,%3d",self->priv->cursor_column,self->priv->cursor_row);
    gtk_widget_queue_draw(GTK_WIDGET(self->priv->pattern_table));
  }
  return(FALSE);
}

static void on_pattern_table_cursor_changed(GtkTreeView *treeview, gpointer user_data) {
  /* delay the action */
  g_idle_add_full(G_PRIORITY_HIGH_IDLE,on_pattern_table_cursor_changed_idle,user_data,NULL);
}

static gboolean on_pattern_table_key_release_event(GtkWidget *widget,GdkEventKey *event,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  gboolean res=FALSE;
  gulong modifier=(gulong)event->state&gtk_accelerator_get_default_mod_mask();
  //gulong modifier=(gulong)event->state&(GDK_SHIFT_MASK|GDK_CONTROL_MASK|GDK_MOD4_MASK);
  
  g_assert(user_data);
  
  GST_INFO("pattern_table key : state 0x%x, keyval 0x%x",event->state,event->keyval);
  if(event->keyval==GDK_Return) {  /* GDK_KP_Enter */
    if(modifier==GDK_CONTROL_MASK) {
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
            (self->priv->selection_start_row!=(self->priv->cursor_row-1))
          ) {
            GST_INFO("up   : new selection");
            self->priv->selection_start_column=self->priv->cursor_column;
            self->priv->selection_end_column=self->priv->cursor_column;
            self->priv->selection_start_row=self->priv->cursor_row;
            self->priv->selection_end_row=self->priv->cursor_row+1;
          }
          else {
            GST_INFO("up   : expand selection");
            self->priv->selection_start_row-=1;
          }
          GST_INFO("up   : %3d,%3d -> %3d,%3d",self->priv->selection_start_column,self->priv->selection_start_row,self->priv->selection_end_column,self->priv->selection_end_row);
          select=TRUE;
          break;
        case GDK_Down:
          GST_INFO("down : %3d,%3d -> %3d,%3d @ %3d,%3d",self->priv->selection_start_column,self->priv->selection_start_row,self->priv->selection_end_column,self->priv->selection_end_row,self->priv->cursor_column,self->priv->cursor_row);
          if((self->priv->selection_end_column!=self->priv->cursor_column) &&
            (self->priv->selection_end_row!=(self->priv->cursor_row+1))
          ) {
            GST_INFO("down : new selection");
            self->priv->selection_start_column=self->priv->cursor_column;
            self->priv->selection_end_column=self->priv->cursor_column;
            self->priv->selection_start_row=self->priv->cursor_row-1;
            self->priv->selection_end_row=self->priv->cursor_row;
          }
          else {
            GST_INFO("down : expand selection");
            self->priv->selection_end_row+=1;
          }
          GST_INFO("down : %3d,%3d -> %3d,%3d",self->priv->selection_start_column,self->priv->selection_start_row,self->priv->selection_end_column,self->priv->selection_end_row);
          select=TRUE;
          break;
        case GDK_Left:
          // move cursor
          self->priv->cursor_column--;
          path=gtk_tree_path_new_from_indices(self->priv->cursor_row,-1);
          columns=gtk_tree_view_get_columns(self->priv->pattern_table);
          column=g_list_nth_data(columns,self->priv->cursor_column);
          // set cell focus
          gtk_tree_view_set_cursor(self->priv->pattern_table,path,column,FALSE);
          gtk_widget_grab_focus(GTK_WIDGET(self->priv->pattern_table));
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
          path=gtk_tree_path_new_from_indices(self->priv->cursor_row,-1);
          columns=gtk_tree_view_get_columns(self->priv->pattern_table);
          column=g_list_nth_data(columns,self->priv->cursor_column);
          // set cell focus
          gtk_tree_view_set_cursor(self->priv->pattern_table,path,column,FALSE);
          gtk_widget_grab_focus(GTK_WIDGET(self->priv->pattern_table));
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
        gtk_widget_queue_draw(GTK_WIDGET(self->priv->pattern_table));
        res=TRUE;
      }
    }
    else {
      if(self->priv->selection_start_column!=-1) {
        self->priv->selection_start_column=self->priv->selection_start_row=self->priv->selection_end_column=self->priv->selection_end_row=-1;
        gtk_widget_queue_draw(GTK_WIDGET(self->priv->pattern_table));
      }
    }
  }
  return(res);
}

static gboolean on_pattern_table_button_press_event(GtkWidget *widget,GdkEventButton *event,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  gboolean res=FALSE;

  g_assert(user_data);
  
  GST_INFO("pattern_table button_press : button 0x%x, type 0x%d",event->button,event->type);
  if(event->button==1) {
    if(gtk_tree_view_get_bin_window(self->priv->pattern_table)==(event->window)) {
      GtkTreePath *path;
      GtkTreeViewColumn *column;
      // determine sequence position from mouse coordinates
      if(gtk_tree_view_get_path_at_pos(self->priv->pattern_table,event->x,event->y,&path,&column,NULL,NULL)) {
        // set cell focus
        gtk_tree_view_set_cursor_on_cell(self->priv->pattern_table,path,column,NULL,FALSE);
        gtk_widget_grab_focus(GTK_WIDGET(self->priv->pattern_table));
        // reset selection
        self->priv->selection_start_column=self->priv->selection_start_row=self->priv->selection_end_column=self->priv->selection_end_row=-1;
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

static gboolean on_pattern_table_motion_notify_event(GtkWidget *widget,GdkEventMotion *event,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
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
          self->priv->selection_column=cursor_column;
          self->priv->selection_row=cursor_row;
        }
        gtk_tree_view_set_cursor(self->priv->pattern_table,path,column,FALSE);
        gtk_widget_grab_focus(GTK_WIDGET(self->priv->pattern_table));
        // cursor updates are not yet processed
        on_pattern_table_cursor_changed_idle(self);
        GST_INFO("cursor new/old: %3d,%3d -> %3d,%3d",cursor_column,cursor_row,self->priv->cursor_column,self->priv->cursor_row);
        if((cursor_column!=self->priv->cursor_column) || (cursor_row!=self->priv->cursor_row)) {
          GST_INFO("  selection before : %3d,%3d -> %3d,%3d [%3d,%3d]",self->priv->selection_start_column,self->priv->selection_start_row,self->priv->selection_end_column,self->priv->selection_end_row,self->priv->selection_column,self->priv->selection_row);
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
          GST_INFO("  selection after  : %3d,%3d -> %3d,%3d [%3d,%3d]",self->priv->selection_start_column,self->priv->selection_start_row,self->priv->selection_end_column,self->priv->selection_end_row,self->priv->selection_column,self->priv->selection_row);
          gtk_widget_queue_draw(GTK_WIDGET(self->priv->pattern_table));
        }
        res=TRUE;
      }
      if(path) gtk_tree_path_free(path);
    }
  }  
  return(res);
}

static void on_pattern_global_cell_edited(GtkCellRendererText *cellrenderertext,gchar *path_string,gchar *new_text,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  GtkTreeModel *store;
  GtkTreeIter iter;
  
  store=gtk_tree_view_get_model(self->priv->pattern_table);
  if(gtk_tree_model_get_iter_from_string(store,&iter,path_string)) {
    gulong param,tick;
    
    param=GPOINTER_TO_UINT(g_object_get_qdata(G_OBJECT(cellrenderertext),column_index_quark));
    gtk_tree_model_get(store,&iter,PATTERN_TABLE_POS,&tick,-1);
    
    GST_INFO("%p : global cell edited: path='%s', content='%s', param=%lu, tick=%lu",self->priv->pattern,path_string,safe_string(new_text),param,tick);
    // store the changed text in the model and pattern
    gtk_list_store_set(GTK_LIST_STORE(store),&iter,PATTERN_TABLE_PRE_CT+param,g_strdup(new_text),-1);
    bt_pattern_set_global_event(self->priv->pattern,tick,param,new_text);
  }
}

static void on_pattern_voice_cell_edited(GtkCellRendererText *cellrenderertext,gchar *path_string,gchar *new_text,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  GtkTreeModel *store;
  GtkTreeIter iter;

  store=gtk_tree_view_get_model(self->priv->pattern_table);
  if(gtk_tree_model_get_iter_from_string(store,&iter,path_string)) {
    BtMachine *machine;
    gulong voice,param,tick;
    gulong ix,global_params,voice_params;
    
    voice=GPOINTER_TO_UINT(g_object_get_qdata(G_OBJECT(cellrenderertext),voice_index_quark));
    param=GPOINTER_TO_UINT(g_object_get_qdata(G_OBJECT(cellrenderertext),column_index_quark));
    gtk_tree_model_get(store,&iter,PATTERN_TABLE_POS,&tick,-1);
    GST_INFO("%p, voice cell edited path='%s', content='%s', voice=%lu, param=%lu, tick=%lu",self->priv->pattern,path_string,safe_string(new_text),voice,param,tick);
    // store the changed text in the model and pattern
    g_object_get(G_OBJECT(self->priv->pattern),"machine",&machine,NULL);
    g_object_get(G_OBJECT(machine),"global-params",&global_params,"voice-params",&voice_params,NULL);

    ix=PATTERN_TABLE_PRE_CT+global_params+(voice*voice_params);
    gtk_list_store_set(GTK_LIST_STORE(store),&iter,ix+param,g_strdup(new_text),-1);
    bt_pattern_set_voice_event(self->priv->pattern,tick,voice,param,new_text);    
    
    g_object_unref(machine);
  }
}

//-- event handler helper

static void machine_menu_add(const BtMainPagePatterns *self,BtMachine *machine,GtkListStore *store) {
  gchar *str;
  GtkTreeIter menu_iter;

  g_object_get(G_OBJECT(machine),"id",&str,NULL);
  GST_INFO("  adding \"%s\"  (machine-refs: %d)",str,(G_OBJECT(machine))->ref_count);

  gtk_list_store_append(store,&menu_iter);
  gtk_list_store_set(store,&menu_iter,
    MACHINE_MENU_ICON,bt_ui_ressources_get_pixbuf_by_machine(machine),
    MACHINE_MENU_LABEL,str,
    MACHINE_MENU_MACHINE,machine,
    -1);
  g_signal_connect(G_OBJECT(machine),"notify::id",G_CALLBACK(on_machine_id_changed),(gpointer)self);
  
  GST_INFO("  (machine-refs: %d)",(G_OBJECT(machine))->ref_count);
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
        index++;  // count so that we can activate the last one
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
  
  // @todo: scan wavetable list for waves
  
  gtk_widget_set_sensitive(GTK_WIDGET(self->priv->wavetable_menu),(wave!=NULL));
  gtk_combo_box_set_model(self->priv->wavetable_menu,GTK_TREE_MODEL(store));
  gtk_combo_box_set_active(self->priv->wavetable_menu,((wave!=NULL)?index:-1));
  g_object_unref(store); // drop with comboxbox
}

/*
 * pattern_pos_table_init:
 * @self: the pattern page
 *
 * inserts the Pos column.
 */
static void pattern_pos_table_init(const BtMainPagePatterns *self) {
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *tree_col;
  gint col_index=0;
  
  // add static column
  renderer=gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer),
    "mode",GTK_CELL_RENDERER_MODE_INERT,
    "xalign",1.0,
    "yalign",0.5,
    NULL);
  gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer),1);
  if((tree_col=gtk_tree_view_column_new_with_attributes(_("Pos."),renderer,
    "text",PATTERN_TABLE_POS,
    NULL))
  ) {
    g_object_set(tree_col,
      "sizing",GTK_TREE_VIEW_COLUMN_FIXED,
      "fixed-width",40,
      NULL);
    col_index=gtk_tree_view_append_column(self->priv->pattern_pos_table,tree_col);
  }
  else GST_WARNING("can't create treeview column");

  GST_DEBUG("    number of columns : %d",col_index);
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
  gulong i,j,k,number_of_ticks,pos=0,ix;
  gulong col_ct,voices,global_params,voice_params;
  gint col_index;
  GtkCellRenderer *renderer;
  GtkListStore *store;
  GType *store_types;
  GtkTreeIter tree_iter;
  gchar *str;
  //GParamSpec *pspec;
  GtkTreeViewColumn *tree_col;

  GST_INFO("refresh pattern table");
  g_assert(GTK_IS_TREE_VIEW(self->priv->pattern_table));
  
  // reset columns
  pattern_table_clear(self);

  if(pattern) {
    g_object_get(G_OBJECT(pattern),"length",&number_of_ticks,"voices",&voices,"machine",&machine,NULL);
    g_object_get(G_OBJECT(machine),"global-params",&global_params,"voice-params",&voice_params,NULL);
    GST_DEBUG("  size is %2d,%2d",number_of_ticks,global_params);
    
    // build model
    GST_DEBUG("  build model");
    col_ct=1+global_params+voices*voice_params;
    store_types=(GType *)g_new(GType *,col_ct);
    store_types[0]=G_TYPE_LONG;
    for(i=1;i<col_ct;i++) {
      store_types[i]=G_TYPE_STRING;
    }
    store=gtk_list_store_newv(col_ct,store_types);
    g_free(store_types);
    // add events
    for(i=0;i<number_of_ticks;i++) {
      ix=PATTERN_TABLE_PRE_CT;
      gtk_list_store_append(store, &tree_iter);
      // set position
      gtk_list_store_set(store,&tree_iter,
        PATTERN_TABLE_POS,pos,
        -1);
      // global params
      for(j=0;j<global_params;j++) {
        if((str=bt_pattern_get_global_event(pattern,i,j))) {
          gtk_list_store_set(store,&tree_iter,
            ix,str,
            -1);
          g_free(str);
        }
        ix++;
      }
      // voice params
      for(j=0;j<voices;j++) {
        for(k=0;k<voice_params;k++) {
          if((str=bt_pattern_get_voice_event(pattern,i,j,k))) {
            gtk_list_store_set(store,&tree_iter,
              ix,str,
              -1);
            g_free(str);
          }
          ix++;
        }
      }
      pos++;
    }
    GST_DEBUG("    activating store: %p",store);
    gtk_tree_view_set_model(self->priv->pattern_table,GTK_TREE_MODEL(store));
    gtk_tree_view_set_model(self->priv->pattern_pos_table,GTK_TREE_MODEL(store));

    // build view
    GST_DEBUG("  build view");
    ix=PATTERN_TABLE_PRE_CT;
    for(j=0;j<global_params;j++) {
      renderer=gtk_cell_renderer_text_new();
      g_object_set(G_OBJECT(renderer),
        //"mode",GTK_CELL_RENDERER_MODE_ACTIVATABLE,
        "mode",GTK_CELL_RENDERER_MODE_EDITABLE,
        "xalign",1.0,
        "family","Monospace",
        "family-set",TRUE,
        "editable",TRUE,
        "editable-set",TRUE,
        NULL);
      // set location data
      g_object_set_qdata(G_OBJECT(renderer),column_index_quark,GUINT_TO_POINTER(j));
      gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer),1);
      g_signal_connect(G_OBJECT(renderer),"edited",G_CALLBACK(on_pattern_global_cell_edited),(gpointer)self);
      if((tree_col=gtk_tree_view_column_new_with_attributes(
        bt_machine_get_global_param_name(machine,j),renderer,
          "text",ix,
          NULL))
      ) {
        g_object_set(tree_col,
          "sizing",GTK_TREE_VIEW_COLUMN_FIXED,
          "fixed-width",PATTERN_CELL_WIDTH,
          NULL);
        g_object_set_qdata(G_OBJECT(tree_col),column_index_quark,GUINT_TO_POINTER(j));
        gtk_tree_view_append_column(self->priv->pattern_table,tree_col);
        gtk_tree_view_column_set_cell_data_func(tree_col, renderer, selection_cell_data_function, (gpointer)self, NULL);
      }
      else GST_WARNING("can't create treeview column");
      ix++;
    }
    for(j=0;j<voices;j++) {
      for(k=0;k<voice_params;k++) {
        renderer=gtk_cell_renderer_text_new();
        g_object_set(G_OBJECT(renderer),
          //"mode",GTK_CELL_RENDERER_MODE_ACTIVATABLE,
          "mode",GTK_CELL_RENDERER_MODE_EDITABLE,
          "xalign",1.0,
          "family","Monospace",
          "family-set",TRUE,
          "editable",TRUE,
          "editable-set",TRUE,
          NULL);
        // set location data
        g_object_set_qdata(G_OBJECT(renderer),voice_index_quark,GUINT_TO_POINTER(j));
        g_object_set_qdata(G_OBJECT(renderer),column_index_quark,GUINT_TO_POINTER(k));
        gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer),1);
        g_signal_connect(G_OBJECT(renderer),"edited",G_CALLBACK(on_pattern_voice_cell_edited),(gpointer)self);
        if((tree_col=gtk_tree_view_column_new_with_attributes(
          bt_machine_get_voice_param_name(machine,k),renderer,
          "text",ix,
          NULL))
        ) {
          g_object_set(tree_col,
            "sizing",GTK_TREE_VIEW_COLUMN_FIXED,
            "fixed-width",PATTERN_CELL_WIDTH,
            NULL);
          g_object_set_qdata(G_OBJECT(tree_col),column_index_quark,GUINT_TO_POINTER(ix-PATTERN_TABLE_PRE_CT));
          gtk_tree_view_append_column(self->priv->pattern_table,tree_col);
          gtk_tree_view_column_set_cell_data_func(tree_col, renderer, selection_cell_data_function, (gpointer)self, NULL);
        }
        else GST_WARNING("can't create treeview column");
        ix++;
      }
    }    
    g_object_unref(machine);
    
    g_object_set(self->priv->pattern_table,"visible-rows",number_of_ticks,NULL);
    g_object_set(self->priv->pattern_pos_table,"visible-rows",number_of_ticks,NULL);
  }
  else {
    // create empty list model, so that the context menu works
    store=gtk_list_store_new(1,G_TYPE_STRING);
    // one empty field
    gtk_list_store_append(store, &tree_iter);
    gtk_list_store_set(store,&tree_iter, 0, "",-1);

    GST_DEBUG("    activating dummy store: %p",store);
    gtk_tree_view_set_model(self->priv->pattern_table,GTK_TREE_MODEL(store));
    gtk_tree_view_set_model(self->priv->pattern_pos_table,GTK_TREE_MODEL(store));
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
  pattern_view_update_column_description(self,UPDATE_COLUMN_UPDATE);
  gtk_widget_grab_focus(GTK_WIDGET(self->priv->pattern_table));
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

static void on_sequence_tick(const BtSong *song,GParamSpec *arg,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  BtSequence *sequence;
  BtMachine *machine,*cur_machine;
  BtPattern *pattern;
  gulong i,j,pos;
  gulong tracks,length,spos,sequence_length;
  gdouble play_pos;
  gboolean found=FALSE;
  
  if(!self->priv->pattern) return;
    
  g_object_get(G_OBJECT(self->priv->pattern),"machine",&cur_machine,"length",&length,NULL);
  g_object_get(G_OBJECT(song),"sequence",&sequence,"play-pos",&pos,NULL);
  g_object_get(G_OBJECT(sequence),"tracks",&tracks,"length",&sequence_length,NULL);
  
  if(pos<sequence_length) { 
    // check all tracks
    for(i=0;((i<tracks) && !found);i++) {
      machine=bt_sequence_get_machine(sequence,i);
      if(machine==cur_machine) {
        // find pattern start
        spos=(pos>length)?(pos-length):0;
        for(j=spos;((j<pos) && !found);j++) {
          // get pattern for current machine and current tick from sequence
          if((pattern=bt_sequence_get_pattern(sequence,j,i))) {
            // if it is the pattern we currently show, set play-line
            if(pattern==self->priv->pattern) {
              play_pos=(gdouble)(pos-j)/(gdouble)length;
              g_object_set(self->priv->pattern_table,"play-position",play_pos,NULL);
              g_object_set(self->priv->pattern_pos_table,"play-position",play_pos,NULL);
              found=TRUE;
            }
            g_object_unref(pattern);
          }
        }
      }
      g_object_unref(machine);
    }
  }
  if(!found) {
    // unfortunately the 2nd widget may lag behind with redrawing itself :(
    g_object_set(self->priv->pattern_table,"play-position",0.0,NULL);
    g_object_set(self->priv->pattern_pos_table,"play-position",0.0,NULL);
  }
  // release the references
  g_object_try_unref(sequence);
  g_object_try_unref(cur_machine);
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
  // subscribe to play-pos changes of song->sequence
  g_signal_connect(G_OBJECT(song), "notify::play-pos", G_CALLBACK(on_sequence_tick), (gpointer)self);
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
  BtSongInfo *song_info;
  BtMachine *machine;
  BtPattern *pattern;
  gchar *mid,*id,*name;
  GtkWidget *dialog;
  gulong bars;

  g_assert(user_data);

  machine=bt_main_page_patterns_get_current_machine(self);
  g_return_if_fail(machine);
  
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"song-info",&song_info,NULL);
  g_object_get(G_OBJECT(song_info),"bars",&bars,NULL);
  g_object_get(G_OBJECT(machine),"id",&mid,NULL);

  name=bt_machine_get_unique_pattern_name(machine);
  id=g_strdup_printf("%s %s",mid,name);
  // new_pattern
  pattern=bt_pattern_new(song, id, name, bars, machine);

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
  g_object_unref(song_info);
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
  g_object_unref(pattern);  // should finalize the pattern
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

static gboolean bt_main_page_patterns_init_ui(const BtMainPagePatterns *self,const BtMainPages *pages) {
  GtkWidget *toolbar;
  GtkWidget *box,*tool_item;
  GtkWidget *scrolled_window,*scrolled_sync_window;
  GtkWidget *menu_item,*image;
  GtkCellRenderer *renderer;
  GtkTreeSelection *tree_sel;
  GtkAdjustment *vadjust;
  gint i;
  gchar oct_str[2];
  
  GST_DEBUG("!!!! self=%p",self);
  
  gtk_widget_set_name(GTK_WIDGET(self),_("pattern view"));
  
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
    sprintf(oct_str,"%1d",i);
    gtk_combo_box_append_text(self->priv->base_octave_menu,oct_str);
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
    
  // get colors
  self->priv->cursor_bg=bt_ui_ressources_get_gdk_color(BT_UI_RES_COLOR_CURSOR);
  self->priv->selection_bg1=bt_ui_ressources_get_gdk_color(BT_UI_RES_COLOR_SELECTION1);
  self->priv->selection_bg2=bt_ui_ressources_get_gdk_color(BT_UI_RES_COLOR_SELECTION2);

  // add hbox for pattern view
  box=gtk_hbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(self),box);
  
  // add pattern-pos list-view
  scrolled_sync_window=gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_sync_window),GTK_POLICY_NEVER,GTK_POLICY_NEVER);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_sync_window),GTK_SHADOW_NONE);
  self->priv->pattern_pos_table=GTK_TREE_VIEW(bt_pattern_view_new(self->priv->app));
  g_object_set(self->priv->pattern_pos_table,"enable-search",FALSE,"rules-hint",TRUE,"fixed-height-mode",TRUE,NULL);
  // set a minimum size, otherwise the window can't be shrinked (we need this because of GTK_POLICY_NEVER)
  gtk_widget_set_size_request(GTK_WIDGET(self->priv->pattern_pos_table),40,40);
  tree_sel=gtk_tree_view_get_selection(self->priv->pattern_pos_table);
  gtk_tree_selection_set_mode(tree_sel,GTK_SELECTION_NONE);
  pattern_pos_table_init(self);
  gtk_container_add(GTK_CONTAINER(scrolled_sync_window),GTK_WIDGET(self->priv->pattern_pos_table));
  gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(scrolled_sync_window), FALSE, FALSE, 0);
  //g_signal_connect(G_OBJECT(self->priv->pattern_pos_table), "button-press-event", G_CALLBACK(on_pattern_table_button_press_event), (gpointer)self);

  // add vertical separator
  gtk_box_pack_start(GTK_BOX(box), gtk_vseparator_new(), FALSE, FALSE, 0);

  /* @idea what about adding one control for global params and one for each voice,
   * - then these controls can be folded (hidden)
   */
  // add pattern list-view
  scrolled_window=gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_NONE);
  self->priv->pattern_table=GTK_TREE_VIEW(bt_pattern_view_new(self->priv->app));
  tree_sel=gtk_tree_view_get_selection(self->priv->pattern_table);
  gtk_tree_selection_set_mode(tree_sel,GTK_SELECTION_NONE);
  g_object_set(self->priv->pattern_table,"enable-search",FALSE,"rules-hint",TRUE,"fixed-height-mode",TRUE,NULL);
  gtk_container_add(GTK_CONTAINER(scrolled_window),GTK_WIDGET(self->priv->pattern_table));
  gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(scrolled_window), TRUE, TRUE, 0);
  g_signal_connect_after(G_OBJECT(self->priv->pattern_table), "cursor-changed", G_CALLBACK(on_pattern_table_cursor_changed), (gpointer)self);
  g_signal_connect(G_OBJECT(self->priv->pattern_table), "key-release-event", G_CALLBACK(on_pattern_table_key_release_event), (gpointer)self);
  g_signal_connect(G_OBJECT(self->priv->pattern_table), "button-press-event", G_CALLBACK(on_pattern_table_button_press_event), (gpointer)self);
  g_signal_connect(G_OBJECT(self->priv->pattern_table), "motion-notify-event", G_CALLBACK(on_pattern_table_motion_notify_event), (gpointer)self);

  // make first scrolled-window also use the horiz-scrollbar of the second scrolled-window
  vadjust=gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled_window));
  gtk_scrolled_window_set_vadjustment(GTK_SCROLLED_WINDOW(scrolled_sync_window),vadjust);
  //GST_DEBUG("pos_view=%p, data_view=%p", self->priv->pattern_pos_table,self->priv->pattern_table);

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

  // set default widget
  gtk_container_set_focus_child(GTK_CONTAINER(self),GTK_WIDGET(self->priv->pattern_table));
  // register event handlers
  g_signal_connect(G_OBJECT(self->priv->app), "notify::song", G_CALLBACK(on_song_changed), (gpointer)self);
  // listen to page changes
  g_signal_connect(G_OBJECT(pages), "switch-page", G_CALLBACK(on_page_switched), (gpointer)self);

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
BtMainPagePatterns *bt_main_page_patterns_new(const BtEditApplication *app,const BtMainPages *pages) {
  BtMainPagePatterns *self;

  if(!(self=BT_MAIN_PAGE_PATTERNS(g_object_new(BT_TYPE_MAIN_PAGE_PATTERNS,"app",app,NULL)))) {
    goto Error;
  }
  // generate UI
  if(!bt_main_page_patterns_init_ui(self,pages)) {
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
  BtMachine *machine;
  GtkTreeIter iter;
  GtkTreeModel *store;

  GST_INFO("get current machine");
  
  if(gtk_combo_box_get_active_iter(self->priv->machine_menu,&iter)) {
    store=gtk_combo_box_get_model(self->priv->machine_menu);
    gtk_tree_model_get(store,&iter,MACHINE_MENU_MACHINE,&machine,-1);
    if(machine) {
      // gtk_tree_model_get already refs()
      return(machine);
      //return(g_object_ref(machine));
    }
  }
  return(NULL);
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
  BtPattern *pattern;
  GtkTreeIter iter;
  GtkTreeModel *store;
  
  GST_INFO("get current pattern");
  
  if(gtk_combo_box_get_active_iter(self->priv->machine_menu,&iter)) {
    store=gtk_combo_box_get_model(self->priv->machine_menu);
    gtk_tree_model_get(store,&iter,MACHINE_MENU_MACHINE,&machine,-1);
    if(machine) {
      GST_DEBUG("  got machine: (machine-refs: %d)",(G_OBJECT(machine))->ref_count);
      if(gtk_combo_box_get_active_iter(self->priv->pattern_menu,&iter)) {
        store=gtk_combo_box_get_model(self->priv->pattern_menu);
        gtk_tree_model_get(store,&iter,PATTERN_MENU_PATTERN,&pattern,-1);
        if(pattern) {
          // gtk_tree_model_get already refs()
          return(pattern);
          //return(g_object_ref(pattern));
        }
      }
      g_object_unref(machine);
    }
  }
  return(NULL);
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
   
  GST_DEBUG("!!!! self=%p",self);  
  g_object_try_weak_unref(self->priv->app);
  g_object_try_unref(self->priv->pattern);
  
  gtk_object_destroy(GTK_OBJECT(self->priv->context_menu));
  gtk_object_destroy(GTK_OBJECT(self->priv->machine_menu));
  gtk_object_destroy(GTK_OBJECT(self->priv->pattern_menu));
  gtk_object_destroy(GTK_OBJECT(self->priv->wavetable_menu));
	gtk_object_destroy(GTK_OBJECT(self->priv->base_octave_menu));

  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_main_page_patterns_finalize(GObject *object) {
  //BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS(object);
  
  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_main_page_patterns_init(GTypeInstance *instance, gpointer g_class) {
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS(instance);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_MAIN_PAGE_PATTERNS, BtMainPagePatternsPrivate);

  //self->priv->cursor_column=0;
  //self->priv->cursor_row=0;
  self->priv->selection_start_column=-1;
  self->priv->selection_start_row=-1;
  self->priv->selection_end_column=-1;
  self->priv->selection_end_row=-1;
}

static void bt_main_page_patterns_class_init(BtMainPagePatternsClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  column_index_quark=g_quark_from_static_string("BtMainPagePattern::column-index");
  voice_index_quark=g_quark_from_static_string("BtMainPagePattern::voice-index");
  
  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtMainPagePatternsPrivate));
    
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
  if (G_UNLIKELY(type == 0)) {
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
