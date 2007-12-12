/* $Id: main-page-patterns.c,v 1.149 2007-12-04 21:51:58 ensonic Exp $
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
 * @see_also: #BtPattern, #BtPatternView
 *
 * Provides an editor for #BtPattern instances.
 */

/* @todo: main-page-patterns tasks
 * - cut/copy/paste
 * - add third view for eating remaining space
 * - need dividers for global and voice data (take care of cursor navi)
 *   - 2 pixel wide column?
 *   - extra views (needs dynamic number of view
 * - shortcuts
 *   - Ctrl-I/D : Insert/Delete rows
 *   - Ctrl-B : Blend
 *     - from min/max of parameter or content of start/end cell (also multi-column)
 *     - what if only start or end is given?
 *   - Ctrl-R : Randomize
 *     - from min/max of parameter or content of start/end cell (also multi-column)
 *   - Ctrl-S : Smooth
 *     - low pass median filter over changes
 *   - Ctrl-<num> :  Stepping
 *     - set increment for cursor-down on edit
 * - copy gtk_cell_renderer_progress -> bt_cell_renderer_pattern_value
 *   - limmit acceptable keys for value entries: http://www.gtk.org/faq/#AEN843
 * - add parameter info when inside cell
 * - move cursor down on edit
 *
 * - also do controller-assignments like in machine-property window
 */

#define BT_EDIT
#define BT_MAIN_PAGE_PATTERNS_C

#include "bt-edit.h"

#define USE_PATTERN_EDITOR 1

enum {
  MAIN_PAGE_PATTERNS_APP=1,
};

struct _BtMainPagePatternsPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  G_POINTER_ALIAS(BtEditApplication *,app);

  /* machine selection menu */
  GtkComboBox *machine_menu;
  /* pattern selection menu */
  GtkComboBox *pattern_menu;
  /* wavetable selection menu */
  GtkComboBox *wavetable_menu;
  /* base octave selection menu */
  GtkComboBox *base_octave_menu;

  /* the pattern table */
#ifdef USE_PATTERN_EDITOR
  BtPatternEditor *pattern_table;
#else
  GtkTreeView *pattern_pos_table;
  GtkTreeView *pattern_table;
#endif

  /* local commands */
  GtkAccelGroup *accel_group;

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

  /* octave menu */
  guint base_octave;

  /* the pattern that is currently shown */
  BtPattern *pattern;
#if 0
  gulong number_of_groups;
  PatternColumnGroup *param_groups;
#else
  PatternColumn *global_param_descs, *voice_param_descs;
#endif

  guint *column_keymode;

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
  PATTERN_MENU_PATTERN,
  PATTERN_MENU_COLOR_SET
};

enum {
  PATTERN_TABLE_POS=0,
  PATTERN_TABLE_PRE_CT
};

enum {
  PATTERN_KEYMODE_NOTE=0,
  PATTERN_KEYMODE_BOOL,
  PATTERN_KEYMODE_NUMBER
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

static void on_context_menu_pattern_new_activate(GtkMenuItem *menuitem,gpointer user_data);
static void on_context_menu_pattern_remove_activate(GtkMenuItem *menuitem,gpointer user_data);

//-- tree cell data functions
#ifndef USE_PATTERN_EDITOR
static void generic_selection_cell_data_function(GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {
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

static void enum_selection_cell_data_function(GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  gulong row,column;
  GdkColor *bg_col=NULL;
  GEnumClass *enum_class;
  GEnumValue *enum_value;
  gchar *str1=NULL;
  const gchar *str2=NULL;

  column=GPOINTER_TO_UINT(g_object_get_qdata(G_OBJECT(col),column_index_quark));

  gtk_tree_model_get(model,iter,
    PATTERN_TABLE_POS,&row,
    PATTERN_TABLE_PRE_CT+column,&str1,
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
  //enum_class=g_type_class_peek_static(gtk_tree_model_get_column_type(model,PATTERN_TABLE_PRE_CT+column));
  enum_class=g_type_class_peek_static(BT_TYPE_TRIGGER_SWITCH);
  if(BT_IS_STRING(str1) && (enum_value=g_enum_get_value(enum_class, atoi(str1)))) {
    str2=enum_value->value_nick;
  }
  g_free(str1);
  g_object_set(G_OBJECT(renderer),
    "background-gdk",bg_col,
    "text",str2,
     NULL);
}
#endif

//-- tree model helper

static void machine_model_get_iter_by_machine(GtkTreeModel *store,GtkTreeIter *iter,BtMachine *that_machine) {
  BtMachine *this_machine;

  GST_INFO("look up iter for machine : %p,ref_count=%d",that_machine,G_OBJECT(that_machine)->ref_count);

  gtk_tree_model_get_iter_first(store,iter);
  do {
    gtk_tree_model_get(store,iter,MACHINE_MENU_MACHINE,&this_machine,-1);
    if(this_machine==that_machine) {
      GST_INFO("found iter for machine : %p,ref_count=%d",that_machine,G_OBJECT(that_machine)->ref_count);
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

#ifndef USE_PATTERN_EDITOR
static gboolean pattern_view_get_cursor_pos(GtkTreeView *tree_view,GtkTreePath *path,GtkTreeViewColumn *column,gulong *col,gulong *row) {
  gboolean res=FALSE;
  GtkTreeModel *store;
  GtkTreeIter iter;

  g_return_val_if_fail(path,FALSE);

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
    else {
      GST_INFO("No iter for path");
    }
  }
  else {
    GST_WARNING("Can't get tree-model");
  }
  return(res);
}

static gboolean pattern_view_set_cursor_pos(BtMainPagePatterns *self) {
  GtkTreePath *path;
  gboolean res=FALSE;

  // @todo: http://bugzilla.gnome.org/show_bug.cgi?id=498010
  if(!GTK_IS_TREE_VIEW(self->priv->pattern_table) || !gtk_tree_view_get_model(self->priv->pattern_table)) return(FALSE);

  if((path=gtk_tree_path_new_from_indices(self->priv->cursor_row,-1))) {
    GList *columns;

    if((columns=gtk_tree_view_get_columns(self->priv->pattern_table))) {
      GtkTreeViewColumn *column=GTK_TREE_VIEW_COLUMN(g_list_nth_data(columns,self->priv->cursor_column));
      // set cell focus
      gtk_tree_view_set_cursor(self->priv->pattern_table,path,column,FALSE);

      res=TRUE;
      g_list_free(columns);
    }
    else {
      GST_WARNING("Can't get columns for pos %d:%d",self->priv->cursor_row,self->priv->cursor_column);
    }
    gtk_tree_path_free(path);
  }
  else {
    GST_WARNING("Can't create treepath for pos %d:%d",self->priv->cursor_row,self->priv->cursor_column);
  }
  if(GTK_WIDGET_REALIZED(self->priv->pattern_table)) {
    gtk_widget_grab_focus(GTK_WIDGET(self->priv->pattern_table));
  }
  return res;
}

/*
 * pattern_get_current_pos:
 * @self: the pattern subpage
 * @time: pointer for time result
 * @track: pointer for track result
 *
 * Get the currently cursor position in the pattern table.
 * The result will be place in the respective pointers.
 * If one is NULL, no value is returned for it.
 *
 * Returns: %TRUE if the cursor is at a valid track position
 */
static gboolean pattern_view_get_current_pos(const BtMainPagePatterns *self,gulong *time,gulong *track) {
  gboolean res=FALSE;
  GtkTreePath *path;
  GtkTreeViewColumn *column;

  if(!self->priv->pattern) return(FALSE);

  //GST_INFO("get active pattern cell");

  gtk_tree_view_get_cursor(self->priv->pattern_table,&path,&column);
  if(column && path) {
    res=pattern_view_get_cursor_pos(self->priv->pattern_table,path,column,track,time);
  }
  else {
    GST_INFO("No cursor pos, column=%p, path=%p",column,path);
  }
  if(path) gtk_tree_path_free(path);
  return(res);
}
#endif

//-- status bar helpers

static void pattern_view_update_column_description(const BtMainPagePatterns *self, BtPatternViewUpdateColumn mode) {
  BtMainWindow *main_window;
  BtMainStatusbar *statusbar;

  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
  // called too early
  if(!main_window) return;
  // our page is not the current
  if(mode&UPDATE_COLUMN_UPDATE) {
    BtMainPages *pages;
    gint page_num;

    g_object_get(G_OBJECT(main_window),"pages",&pages,NULL);
    page_num=gtk_notebook_get_current_page(GTK_NOTEBOOK(pages));
    g_object_try_unref(pages);

    if(page_num!=BT_MAIN_PAGES_PATTERNS_PAGE) return;
  }

  g_object_get(main_window,"statusbar",&statusbar,NULL);

  // pop previous text by passing str=NULL;
  if(mode&UPDATE_COLUMN_POP)
    g_object_set(statusbar,"status",NULL,NULL);

  if(mode&UPDATE_COLUMN_PUSH) {
    const gchar *str=BT_MAIN_STATUSBAR_DEFAULT;

    if(self->priv->pattern) {
      BtMachine *machine;
      GParamSpec *property=NULL;
      gulong global_params,voice_params,voices,col;

      g_object_get(self->priv->pattern,"machine",&machine,NULL);
      g_object_get(machine,
        "global-params",&global_params,
        "voice-params",&voice_params,
        "voices",&voices,
        NULL);
      col=self->priv->cursor_column;

      // get ParamSpec for global or voice param
      if(col<global_params)
        property=bt_machine_get_global_param_spec(machine,col);
      else {
        col-=global_params;
        if(col<voices*voice_params)
          property=bt_machine_get_voice_param_spec(machine,col%voice_params);
      }

      // get parameter description
      if(property)
        str=g_param_spec_get_blurb(property);

      g_object_unref(machine);
    }
    g_object_set(statusbar,"status",str,NULL);
  }

  g_object_unref(statusbar);
  g_object_unref(main_window);
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

#ifndef USE_PATTERN_EDITOR
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
    gtk_tree_view_scroll_to_cell(self->priv->pattern_table,path,column,FALSE,1.0,0.0);
    gtk_widget_queue_draw(GTK_WIDGET(self->priv->pattern_table));
  }
  if(path) gtk_tree_path_free(path);

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
  if(!GTK_WIDGET_REALIZED(self->priv->pattern_table)) return(FALSE);

  GST_INFO("pattern_table key : state 0x%x, keyval 0x%x, hw-code 0x%x, name %s",
    event->state,event->keyval,event->hardware_keycode,gdk_keyval_name(event->keyval));
  if(event->keyval==GDK_Return) {  /* GDK_KP_Enter */
    if(modifier==GDK_SHIFT_MASK) {
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
  else if(event->keyval==GDK_Menu) {
    gtk_menu_popup(self->priv->context_menu,NULL,NULL,NULL,NULL,3,gtk_get_current_event_time());   
  }
  else if(event->keyval==GDK_Up || event->keyval==GDK_Down || event->keyval==GDK_Left || event->keyval==GDK_Right) {
    if(self->priv->pattern) {
#if HAVE_GTK_2_10 && !HAVE_GTK_2_10_7
      gboolean changed=FALSE;
#endif
      BtMachine *machine;
      gulong length,column_ct,voices,global_params,voice_params;

      g_object_get(G_OBJECT(self->priv->pattern),"length",&length,"voices",&voices,"machine",&machine,NULL);
      g_object_get(G_OBJECT(machine),"global-params",&global_params,"voice-params",&voice_params,NULL);
      column_ct=global_params+voices*voice_params;
      g_object_unref(machine);

      // work around http://bugzilla.gnome.org/show_bug.cgi?id=371756
#if HAVE_GTK_2_10 && !HAVE_GTK_2_10_7
      switch(event->keyval) {
        case GDK_Up:
          if(self->priv->cursor_row>0) {
            self->priv->cursor_row--;
            changed=TRUE;
          }
          break;
        case GDK_Down:
          if((self->priv->cursor_row+1)<length) {
            self->priv->cursor_row++;
            changed=TRUE;
          }
          break;
      }
      if(changed) {
        pattern_view_set_cursor_pos(self);
      }
#endif

      if(modifier==GDK_SHIFT_MASK) {
        gboolean select=FALSE;

        GST_INFO("handling selection");

        // handle selection
        switch(event->keyval) {
          case GDK_Up:
            if((self->priv->cursor_row>=0)
#if HAVE_GTK_2_10 && !HAVE_GTK_2_10_7
              && changed
#endif
              ) {
              GST_INFO("up   : %3d,%3d -> %3d,%3d @ %3d,%3d",self->priv->selection_start_column,self->priv->selection_start_row,self->priv->selection_end_column,self->priv->selection_end_row,self->priv->cursor_column,self->priv->cursor_row);
              if(self->priv->selection_start_row==-1) {
                GST_INFO("up   : new selection");
                self->priv->selection_start_column=self->priv->cursor_column;
                self->priv->selection_end_column=self->priv->cursor_column;
                self->priv->selection_start_row=self->priv->cursor_row;
                self->priv->selection_end_row=self->priv->cursor_row+1;
              }
              else {
                if(self->priv->selection_start_row==(self->priv->cursor_row+1)) {
                  GST_INFO("up   : expand selection");
                  self->priv->selection_start_row-=1;
                }
                else {
                  GST_INFO("up   : shrink selection");
                  self->priv->selection_end_row-=1;
                }
              }
              GST_INFO("up   : %3d,%3d -> %3d,%3d",self->priv->selection_start_column,self->priv->selection_start_row,self->priv->selection_end_column,self->priv->selection_end_row);
              select=TRUE;
            }
            break;
          case GDK_Down:
            if((self->priv->cursor_row<=length)
#if HAVE_GTK_2_10 && !HAVE_GTK_2_10_7
              && changed
#endif
              ) {
              GST_INFO("down : %3d,%3d -> %3d,%3d @ %3d,%3d",self->priv->selection_start_column,self->priv->selection_start_row,self->priv->selection_end_column,self->priv->selection_end_row,self->priv->cursor_column,self->priv->cursor_row);
              if(self->priv->selection_end_row==-1) {
                GST_INFO("down : new selection");
                self->priv->selection_start_column=self->priv->cursor_column;
                self->priv->selection_end_column=self->priv->cursor_column;
                self->priv->selection_start_row=self->priv->cursor_row-1;
                self->priv->selection_end_row=self->priv->cursor_row;
              }
              else {
                if(self->priv->selection_end_row==(self->priv->cursor_row-1)) {
                  GST_INFO("down : expand selection");
                  self->priv->selection_end_row+=1;
                }
                else {
                  GST_INFO("down : shrink selection");
                  self->priv->selection_start_row+=1;
                }
              }
              GST_INFO("down : %3d,%3d -> %3d,%3d",self->priv->selection_start_column,self->priv->selection_start_row,self->priv->selection_end_column,self->priv->selection_end_row);
              select=TRUE;
            }
            break;
          case GDK_Left:
            if(self->priv->cursor_column>=0) {
              // move cursor
              self->priv->cursor_column--;
              pattern_view_set_cursor_pos(self);
              GST_INFO("left : %3d,%3d -> %3d,%3d @ %3d,%3d",self->priv->selection_start_column,self->priv->selection_start_row,self->priv->selection_end_column,self->priv->selection_end_row,self->priv->cursor_column,self->priv->cursor_row);
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
              GST_INFO("left : %3d,%3d -> %3d,%3d",self->priv->selection_start_column,self->priv->selection_start_row,self->priv->selection_end_column,self->priv->selection_end_row);
              select=TRUE;
            }
            break;
          case GDK_Right:
            if(self->priv->cursor_column<=column_ct) {
              // move cursor
              self->priv->cursor_column++;
              pattern_view_set_cursor_pos(self);
              GST_INFO("right: %3d,%3d -> %3d,%3d @ %3d,%3d",self->priv->selection_start_column,self->priv->selection_start_row,self->priv->selection_end_column,self->priv->selection_end_row,self->priv->cursor_column,self->priv->cursor_row);
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
              GST_INFO("right: %3d,%3d -> %3d,%3d",self->priv->selection_start_column,self->priv->selection_start_row,self->priv->selection_end_column,self->priv->selection_end_row);
              select=TRUE;
            }
            break;
        }
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
  }
  else {
    gulong tick,param;

    if(pattern_view_get_current_pos(self,&tick,&param)) {
      gchar *str=NULL;
      gchar oct_str[4];
      gboolean changed=FALSE;

      if(event->keyval==GDK_space || event->keyval == GDK_period) {
        changed=TRUE;
      }
      else {
        switch(self->priv->column_keymode[param]) {
          case PATTERN_KEYMODE_NOTE:
            /* handle y<->z of key-layouts (event->hardware_keycode)
               keyval 0x7a, hw-code 0x34, name z
               keyval 0x79, hw-code 0x1d, name y
             */
            /* @todo: handle h/b variation in notes (locale)
             * http://en.wikipedia.org/wiki/Note#Note_name
             */
            switch(event->hardware_keycode) {
              case 0x34: str="c-0";break;
              case 0x27: str="c#0";break;
              case 0x35: str="d-0";break;
              case 0x28: str="d#0";break;
              case 0x36: str="e-0";break;
              case 0x37: str="f-0";break;
              case 0x2a: str="f#0";break;
              case 0x38: str="g-0";break;
              case 0x2b: str="g#0";break;
              case 0x39: str="a-0";break;
              case 0x2c: str="a#0";break;
              case 0x3a: str="h-0";break;
              case 0x18: str="c-1";break;
              case 0x0b: str="c#1";break;
              case 0x19: str="d-1";break;
              case 0x0c: str="d#1";break;
              case 0x1a: str="e-1";break;
              case 0x1b: str="f-1";break;
              case 0x0e: str="f#1";break;
              case 0x1c: str="g-1";break;
              case 0x0f: str="g#1";break;
              case 0x1d: str="a-1";break;
              case 0x10: str="a#1";break;
              case 0x1e: str="h-1";break;
              case 0x1f: str="c-2";break;
              case 0x12: str="c#2";break;
              case 0x20: str="d-2";break;
              case 0x13: str="d#2";break;
              case 0x21: str="e-2";break;
            }
            if(str) {
              oct_str[0]=str[0];
              oct_str[1]=str[1];
              oct_str[2]=(gchar)(((guint)str[2])+self->priv->base_octave);
              oct_str[3]='\0';
              str=oct_str;
            }
            break;
          case PATTERN_KEYMODE_BOOL:
            switch(event->keyval) {
              case GDK_Delete:
              case GDK_BackSpace:
                str="0";
                break;
              case GDK_Insert:
                str="1";
                break;
            }
            break;
        }
        if(str) changed=TRUE;
      }
      if(changed) {
        GtkTreeModel *store;
        GtkTreePath *path;
        BtMachine *machine;
        gulong length,voices,global_params,voice_params;

        g_object_get(G_OBJECT(self->priv->pattern),"length",&length,"voices",&voices,"machine",&machine,NULL);
        g_object_get(G_OBJECT(machine),"global-params",&global_params,"voice-params",&voice_params,NULL);
        g_object_unref(machine);

        // change model and pattern
        store=gtk_tree_view_get_model(self->priv->pattern_table);
        gtk_tree_view_get_cursor(self->priv->pattern_table,&path,NULL);
        if(path) {
          GtkTreeIter iter;

          GST_INFO("  update model");

          if(gtk_tree_model_get_iter(GTK_TREE_MODEL(store),&iter,path)) {
            // store the changed text in the model and pattern
            gtk_list_store_set(GTK_LIST_STORE(store),&iter,PATTERN_TABLE_PRE_CT+param,g_strdup(str),-1);
            // @todo: need to use_voice_event too!
            if(param<global_params) {
              bt_pattern_set_global_event(self->priv->pattern,tick,param,(BT_IS_STRING(str)?str:NULL));
            }
            else {
              gulong voice,vparams,vparam;

              vparams=param-global_params;
              voice=vparams/voice_params;
              vparam=vparams-(voice*voice_params);

              bt_pattern_set_voice_event(self->priv->pattern,tick,voice,vparam,(BT_IS_STRING(str)?str:NULL));
            }
          }
          gtk_tree_path_free(path);
        }
        // move cursor down & set cell focus
        if((self->priv->cursor_row+1)<length) {
          GST_INFO("  move cursor down (row=%d/%d)",self->priv->cursor_row,length);
          self->priv->cursor_row++;
          pattern_view_set_cursor_pos(self);
        }
      }

    }
  }
  return(res);
}
#endif

static gboolean on_pattern_table_button_press_event(GtkWidget *widget,GdkEventButton *event,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  gboolean res=FALSE;

  g_assert(user_data);

  GST_INFO("pattern_table button_press : button 0x%x, type 0x%d",event->button,event->type);
#ifndef USE_PATTERN_EDITOR
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
  else
#endif
  if(event->button==3) {
    gtk_menu_popup(self->priv->context_menu,NULL,NULL,NULL,NULL,3,gtk_get_current_event_time());
    res=TRUE;
  }
  return(res);
}

#ifndef USE_PATTERN_EDITOR
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

  if((store=gtk_tree_view_get_model(self->priv->pattern_table))) {
    if(gtk_tree_model_get_iter_from_string(store,&iter,path_string)) {
      gulong param,tick;
      gulong length;

      param=GPOINTER_TO_UINT(g_object_get_qdata(G_OBJECT(cellrenderertext),column_index_quark));
      gtk_tree_model_get(store,&iter,PATTERN_TABLE_POS,&tick,-1);

      GST_INFO("%p : global cell edited: path='%s', content='%s', param=%lu, tick=%lu",self->priv->pattern,path_string,safe_string(new_text),param,tick);
      // store the changed text in the model and pattern
      g_object_get(G_OBJECT(self->priv->pattern),"length",&length,NULL);
      gtk_list_store_set(GTK_LIST_STORE(store),&iter,PATTERN_TABLE_PRE_CT+param,g_strdup(new_text),-1);
      bt_pattern_set_global_event(self->priv->pattern,tick,param,(BT_IS_STRING(new_text)?new_text:NULL));

      // move cursor down & set cell focus
      if((self->priv->cursor_row+1)<length) {
        GST_INFO("  move cursor down (row=%d/%d)",self->priv->cursor_row,length);
        self->priv->cursor_row++;
        pattern_view_set_cursor_pos(self);
      }
    }
    else {
      GST_INFO("No iter for path");
    }
  }
  else {
    GST_WARNING("Can't get tree-model");
  }
}

static void on_pattern_voice_cell_edited(GtkCellRendererText *cellrenderertext,gchar *path_string,gchar *new_text,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  GtkTreeModel *store;
  GtkTreeIter iter;

  if((store=gtk_tree_view_get_model(self->priv->pattern_table))) {
    if(gtk_tree_model_get_iter_from_string(store,&iter,path_string)) {
      BtMachine *machine;
      gulong voice,param,tick;
      gulong ix,length,global_params,voice_params;

      voice=GPOINTER_TO_UINT(g_object_get_qdata(G_OBJECT(cellrenderertext),voice_index_quark));
      param=GPOINTER_TO_UINT(g_object_get_qdata(G_OBJECT(cellrenderertext),column_index_quark));
      gtk_tree_model_get(store,&iter,PATTERN_TABLE_POS,&tick,-1);
      GST_INFO("%p, voice cell edited path='%s', content='%s', voice=%lu, param=%lu, tick=%lu",self->priv->pattern,path_string,safe_string(new_text),voice,param,tick);
      // store the changed text in the model and pattern
      g_object_get(G_OBJECT(self->priv->pattern),"length",&length,"machine",&machine,NULL);
      g_object_get(G_OBJECT(machine),"global-params",&global_params,"voice-params",&voice_params,NULL);

      ix=PATTERN_TABLE_PRE_CT+global_params+(voice*voice_params);
      gtk_list_store_set(GTK_LIST_STORE(store),&iter,ix+param,g_strdup(new_text),-1);
      bt_pattern_set_voice_event(self->priv->pattern,tick,voice,param,(BT_IS_STRING(new_text)?new_text:NULL));

      g_object_unref(machine);

      // move cursor down & set cell focus
      if((self->priv->cursor_row+1)<length) {
        GST_INFO("  move cursor down (row=%d/%d)",self->priv->cursor_row,length);
        self->priv->cursor_row++;
        pattern_view_set_cursor_pos(self);
      }
    }
    else {
      GST_INFO("No iter for path");
    }
  }
  else {
    GST_WARNING("Can't get tree-model");
  }
}
#endif

//-- event handler helper

static void machine_menu_add(const BtMainPagePatterns *self,BtMachine *machine,GtkListStore *store) {
  gchar *str;
  GtkTreeIter menu_iter;

  g_object_get(G_OBJECT(machine),"id",&str,NULL);
  GST_INFO("  adding %p, \"%s\"  (machine-refs: %d)",machine,str,(G_OBJECT(machine))->ref_count);

  gtk_list_store_append(store,&menu_iter);
  gtk_list_store_set(store,&menu_iter,
    MACHINE_MENU_ICON,bt_ui_ressources_get_pixbuf_by_machine(machine),
    MACHINE_MENU_LABEL,str,
    MACHINE_MENU_MACHINE,machine,
    -1);
  g_signal_connect(G_OBJECT(machine),"notify::id",G_CALLBACK(on_machine_id_changed),(gpointer)self);

  GST_DEBUG("  (machine-refs: %d)",(G_OBJECT(machine))->ref_count);
  g_free(str);
}

static void machine_menu_refresh(const BtMainPagePatterns *self,const BtSetup *setup) {
  BtMachine *machine=NULL;
  GtkListStore *store;
  GList *node,*list;
  gint index=-1;

  // update machine menu
  store=gtk_list_store_new(3,GDK_TYPE_PIXBUF,G_TYPE_STRING,BT_TYPE_MACHINE);
  g_object_get(G_OBJECT(setup),"machines",&list,NULL);
  for(node=list;node;node=g_list_next(node)) {
    machine=BT_MACHINE(node->data);
    machine_menu_add(self,machine,store);
    index++;  // count so that we can activate the last one
  }
  g_list_free(list);
  GST_INFO("machine menu refreshed");
  gtk_widget_set_sensitive(GTK_WIDGET(self->priv->machine_menu),(machine!=NULL));
  gtk_combo_box_set_model(self->priv->machine_menu,GTK_TREE_MODEL(store));
  gtk_combo_box_set_active(self->priv->machine_menu,((machine!=NULL)?index:-1));
  g_object_unref(store); // drop with comboxbox
}

static void pattern_menu_refresh(const BtMainPagePatterns *self,BtMachine *machine) {
  BtPattern *pattern=NULL;
  GList *node,*list;
  gchar *str;
  GtkListStore *store;
  GtkTreeIter menu_iter;
  gint index=-1;
  gboolean is_internal,is_used;

  // update pattern menu
  store=gtk_list_store_new(3,G_TYPE_STRING,BT_TYPE_PATTERN,G_TYPE_BOOLEAN);
  if(machine) {
    BtSong *song;
    BtSequence *sequence;

    g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
    g_object_get(G_OBJECT(song),"sequence",&sequence,NULL);
    g_object_get(G_OBJECT(machine),"patterns",&list,NULL);
    for(node=list;node;node=g_list_next(node)) {
      pattern=BT_PATTERN(node->data);
      g_object_get(G_OBJECT(pattern),"name",&str,"is-internal",&is_internal,NULL);
      if(!is_internal) {
        GST_INFO("  adding \"%s\"",str);
        is_used=bt_sequence_is_pattern_used(sequence,pattern);
        gtk_list_store_append(store,&menu_iter);
        gtk_list_store_set(store,&menu_iter,
          PATTERN_MENU_LABEL,str,
          PATTERN_MENU_PATTERN,pattern,
          PATTERN_MENU_COLOR_SET,!is_used,
          -1);
        g_signal_connect(G_OBJECT(pattern),"notify::name",G_CALLBACK(on_pattern_name_changed),(gpointer)self);
        index++;  // count so that we can activate the last one
      }
      g_free(str);
    }
    g_list_free(list);
    g_object_unref(sequence);
    g_object_unref(song);
    GST_INFO("pattern menu refreshed");
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

#ifndef USE_PATTERN_EDITOR
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
  GtkWidget *header;
  GtkCellRenderer *renderer;
  GtkListStore *store;
  GType *store_types;
  GtkTreeIter tree_iter;
  gchar *str;
  //GParamSpec *pspec;
  GtkTreeViewColumn *tree_col;
  GtkCellRendererMode cell_edit_mode;
  GtkTreeCellDataFunc cell_data_func;

  GST_INFO("refresh pattern table");
  g_assert(GTK_IS_TREE_VIEW(self->priv->pattern_table));

  // reset columns
  pattern_table_clear(self);

  if(pattern) {
    gint col_index;
    gulong i,j,k,pos=0,ix,col_ct;
    gulong number_of_ticks,voices,global_params,voice_params;
    BtMachine *machine;
    GType type;
    gboolean trigger;

    g_object_get(G_OBJECT(pattern),"length",&number_of_ticks,"voices",&voices,"machine",&machine,NULL);
    g_object_get(G_OBJECT(machine),"global-params",&global_params,"voice-params",&voice_params,NULL);
    GST_DEBUG("  size is %2d,%2d",number_of_ticks,global_params);

    // build model
    GST_DEBUG("  build model");
    col_ct=1+global_params+voices*voice_params;
    store_types=(GType *)g_new(GType,col_ct);
    store_types[0]=G_TYPE_LONG;
    for(i=1;i<col_ct;i++) {
      // @todo: use specific type depending on parameter
      store_types[i]=G_TYPE_STRING;
    }
    store=gtk_list_store_newv(col_ct,store_types);
    g_free(store_types);
    // edit modes
    g_free(self->priv->column_keymode);
    self->priv->column_keymode=(guint *)g_new(guint,col_ct);
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
          GST_INFO("  cell %d,%d : %s",i,j,str);
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
      // use specific cell-renderers by type
      type=bt_machine_get_global_param_type(machine,j);
      trigger=bt_machine_is_global_param_trigger(machine,j);
      cell_edit_mode=GTK_CELL_RENDERER_MODE_EDITABLE;
      cell_data_func=generic_selection_cell_data_function;
      if(trigger) {
        if(type==G_TYPE_STRING) {
          cell_edit_mode=GTK_CELL_RENDERER_MODE_ACTIVATABLE;
          self->priv->column_keymode[ix-PATTERN_TABLE_PRE_CT]=PATTERN_KEYMODE_NOTE;
        }
        else if(type==G_TYPE_BOOLEAN) {
          cell_edit_mode=GTK_CELL_RENDERER_MODE_ACTIVATABLE;
          self->priv->column_keymode[ix-PATTERN_TABLE_PRE_CT]=PATTERN_KEYMODE_BOOL;
        }
        else if(type==BT_TYPE_TRIGGER_SWITCH) {
          cell_edit_mode=GTK_CELL_RENDERER_MODE_ACTIVATABLE;
          cell_data_func=enum_selection_cell_data_function;
          self->priv->column_keymode[ix-PATTERN_TABLE_PRE_CT]=PATTERN_KEYMODE_BOOL;

        }
        /*
        else if(type==BT_TYPE_TRIGGER_NOTE) {
          cell_edit_mode=GTK_CELL_RENDERER_MODE_ACTIVATABLE;
          cell_data_func=enum_selection_cell_data_function
          self->priv->column_keymode[ix-PATTERN_TABLE_PRE_CT]=PATTERN_KEYMODE_NOTE;

        }
        */
        else {
          self->priv->column_keymode[ix-PATTERN_TABLE_PRE_CT]=PATTERN_KEYMODE_NUMBER;
        }
      }
      else {
        self->priv->column_keymode[ix-PATTERN_TABLE_PRE_CT]=PATTERN_KEYMODE_NUMBER;
      }
      /* @IDEA: we could also connect to
        "editing-started"
              void        user_function      (GtkCellRenderer *renderer,
                                              GtkCellEditable *editable,
                                              gchar           *path,
                                              gpointer         user_data)
        and then for the 'editable' (which is a GtkEntry) listen to the
        "key-release-event"
              gboolean    user_function      (GtkWidget   *widget,
                                              GdkEventKey *event,
                                              gpointer     user_data)
        there we map the key to the note,
          gtk_entry_set_text(GTK_ENTRY(widget),note_text);
          gtk_cell_editable_editing_done(GTK_CELL_EDITABLE(widget));
      */

      renderer=gtk_cell_renderer_text_new();
      g_object_set(G_OBJECT(renderer),
        "mode",cell_edit_mode,
        "xalign",1.0,
        "family","Monospace",
        "family-set",TRUE,
        "editable",((cell_edit_mode==GTK_CELL_RENDERER_MODE_EDITABLE)?TRUE:FALSE),
        "editable-set",TRUE,
        NULL);
      // set location data
      g_object_set_qdata(G_OBJECT(renderer),column_index_quark,GUINT_TO_POINTER(j));
      gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer),1);
      if(cell_edit_mode==GTK_CELL_RENDERER_MODE_EDITABLE) {
        g_signal_connect(G_OBJECT(renderer),"edited",G_CALLBACK(on_pattern_global_cell_edited),(gpointer)self);
      }
      if((tree_col=gtk_tree_view_column_new_with_attributes(NULL,renderer,
          "text",ix,
          NULL))
      ) {
        // use smaller font for the headers
        str=g_strdup_printf("<span size=\"small\">%s</span>",bt_machine_get_global_param_name(machine,j));
        header=gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(header),str);
        gtk_widget_show(header);
        g_free(str);

        g_object_set(tree_col,
          "widget",header,
          "sizing",GTK_TREE_VIEW_COLUMN_FIXED,
          "fixed-width",PATTERN_CELL_WIDTH,
          NULL);
        g_object_set_qdata(G_OBJECT(tree_col),column_index_quark,GUINT_TO_POINTER(j));
        gtk_tree_view_append_column(self->priv->pattern_table,tree_col);
        // @todo: also connect to different cell data function, depending on type
        gtk_tree_view_column_set_cell_data_func(tree_col, renderer, cell_data_func, (gpointer)self, NULL);
      }
      else GST_WARNING("can't create treeview column");
      ix++;
    }
    for(j=0;j<voices;j++) {
      for(k=0;k<voice_params;k++) {
        // use specific cell-renderers by type
        type=bt_machine_get_voice_param_type(machine,k);
        trigger=bt_machine_is_voice_param_trigger(machine,k);
        cell_edit_mode=GTK_CELL_RENDERER_MODE_EDITABLE;
        cell_data_func=generic_selection_cell_data_function;
        if(trigger) {
          if(type==G_TYPE_STRING) {
            cell_edit_mode=GTK_CELL_RENDERER_MODE_ACTIVATABLE;
            self->priv->column_keymode[ix-PATTERN_TABLE_PRE_CT]=PATTERN_KEYMODE_NOTE;
          }
          else if(type==G_TYPE_BOOLEAN) {
            cell_edit_mode=GTK_CELL_RENDERER_MODE_ACTIVATABLE;
            self->priv->column_keymode[ix-PATTERN_TABLE_PRE_CT]=PATTERN_KEYMODE_BOOL;
          }
          else if(type==BT_TYPE_TRIGGER_SWITCH) {
            cell_edit_mode=GTK_CELL_RENDERER_MODE_ACTIVATABLE;
            cell_data_func=enum_selection_cell_data_function;
            self->priv->column_keymode[ix-PATTERN_TABLE_PRE_CT]=PATTERN_KEYMODE_BOOL;

          }
          else {
            self->priv->column_keymode[ix-PATTERN_TABLE_PRE_CT]=PATTERN_KEYMODE_NUMBER;
          }
        }
        else {
          self->priv->column_keymode[ix-PATTERN_TABLE_PRE_CT]=PATTERN_KEYMODE_NUMBER;
        }

        renderer=gtk_cell_renderer_text_new();
        g_object_set(G_OBJECT(renderer),
          "mode",cell_edit_mode,
          "xalign",1.0,
          "family","Monospace",
          "family-set",TRUE,
          "editable",((cell_edit_mode==GTK_CELL_RENDERER_MODE_EDITABLE)?TRUE:FALSE),
          "editable-set",TRUE,
          NULL);
        // set location data
        g_object_set_qdata(G_OBJECT(renderer),voice_index_quark,GUINT_TO_POINTER(j));
        g_object_set_qdata(G_OBJECT(renderer),column_index_quark,GUINT_TO_POINTER(k));
        gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer),1);
        if(cell_edit_mode==GTK_CELL_RENDERER_MODE_EDITABLE) {
          g_signal_connect(G_OBJECT(renderer),"edited",G_CALLBACK(on_pattern_voice_cell_edited),(gpointer)self);
        }
        if((tree_col=gtk_tree_view_column_new_with_attributes(NULL,renderer,
          "text",ix,
          NULL))
        ) {
          // use smaller font for the headers
          str=g_strdup_printf("<span size=\"small\">%s</span>",bt_machine_get_voice_param_name(machine,k));
          header=gtk_label_new(NULL);
          gtk_label_set_markup(GTK_LABEL(header),str);
          gtk_widget_show(header);
          g_free(str);

          g_object_set(tree_col,
            "widget",header,
            "sizing",GTK_TREE_VIEW_COLUMN_FIXED,
            "fixed-width",PATTERN_CELL_WIDTH,
            NULL);
          g_object_set_qdata(G_OBJECT(tree_col),column_index_quark,GUINT_TO_POINTER(ix-PATTERN_TABLE_PRE_CT));
          gtk_tree_view_append_column(self->priv->pattern_table,tree_col);
          gtk_tree_view_column_set_cell_data_func(tree_col, renderer, cell_data_func, (gpointer)self, NULL);
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
#else

typedef struct {
  gfloat (*str_to_float)(gchar *in);
  const gchar *(*float_to_str)(gfloat in);
} PatternColumnConverters;

static float pattern_edit_get_data_at(gpointer pattern_data, gpointer column_data, int row, int track, int param) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(pattern_data);
  gchar *str;

#if 0
    switch (group->type) {
      case 0: {
        BtWirePattern *wire_pattern = bt_wire_get_pattern(group->user_data,self->priv->pattern);
        str=bt_wire_pattern_get_event(wire_pattern,row,param);
        g_object_unref(wire_pattern);
      } break;
      case 1:
        str=bt_pattern_get_global_event(self->priv->pattern,row,param);
        break;
      case 2:
        str=bt_pattern_get_voice_event(self->priv->pattern,row,group->user_data,param);
        break;
      default:
        GST_WARNING("invalid column group type");
    }
#else
  if (track == -1)
    str=bt_pattern_get_global_event(self->priv->pattern,row,param);
  else
    str=bt_pattern_get_voice_event(self->priv->pattern,row,track,param);
#endif

  if(str) {
    if(column_data) {
      return ((PatternColumnConverters *)column_data)->str_to_float(str);
    }
    else
      return g_ascii_strtod(str,NULL);
  }
  return 0.0;
}

static void pattern_edit_set_data_at(gpointer pattern_data, gpointer column_data, int row, int track, int param, float value) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(pattern_data);
  const gchar *str;
  
  if(column_data) {
    str=((PatternColumnConverters *)column_data)->float_to_str(value);
  }
  else
    str=bt_persistence_strfmt_double(value);

#if 0
    switch (group->type) {
      case 0: {
        BtWirePattern *wire_pattern = bt_wire_get_pattern(group->user_data,self->priv->pattern);
        bt_wire_pattern_set_event(wire_pattern,row,param,str);
        g_object_unref(wire_pattern);
      } break;
      case 1:
        bt_pattern_set_global_event(self->priv->pattern,row,param,str);
        break;
      case 2:
        bt_pattern_set_voice_event(self->priv->pattern,row,group->user_data,param,str);
        break;
      default:
        GST_WARNING("invalid column group type");
    }
#else
  if (track == -1)
    bt_pattern_set_global_event(self->priv->pattern,row,param,str);
  else
    bt_pattern_set_voice_event(self->priv->pattern,row,track,param,str);
#endif
}

// @todo: this is a copy of gstbml:src/common.c
static float  note_str_to_float(gchar *note) {
  guint tone, octave;

  g_return_val_if_fail(note,0);
  g_return_val_if_fail((strlen(note)==3),0);
  g_return_val_if_fail((note[1]=='-' || note[1]=='#'),0);

  // parse note
  switch(note[0]) {
    case 'c':
    case 'C':
      tone=(note[1]=='-')?0:1;
      break;
    case 'd':
    case 'D':
      tone=(note[1]=='-')?2:3;
      break;
    case 'e':
    case 'E':
      tone=4;
      break;
    case 'f':
    case 'F':
      tone=(note[1]=='-')?5:6;
      break;
    case 'g':
    case 'G':
      tone=(note[1]=='-')?7:8;
      break;
    case 'a':
    case 'A':
      tone=(note[1]=='-')?9:10;
      break;
    case 'b':
    case 'B':
    case 'h':
    case 'H':
      tone=11;
      break;
    default:
      g_return_val_if_reached(0.0);
  }
  octave=atoi(&note[2]);
  // 0 is no-value
  return (float)(1+(octave<<4)+tone);
}

static const gchar * note_float_to_str(gfloat in) {
  guint tone, octave, note=(gint)in;
  static gchar str[4];
  static const gchar *key[12]= { "c-", "c#", "d-", "d#", "e-", "f-", "f#", "g-", "g#", "a-", "a#", "b-" };

  // 0 is no-value
  if (note==0) return("");

  g_return_val_if_fail(note<((16*9)+12),0);

  note-=1;
  octave=note/16;
  tone=note-(octave*16);

  sprintf(str,"%2s%1d",key[tone],octave);
  return(str);
}

static PatternColumnConverters pcc[]={
  { note_str_to_float, note_float_to_str }
};

static void pattern_edit_fill_column_type(PatternColumn *col,GParamSpec *property, gboolean trigger) {
  GType type=bt_g_type_get_base_type(property->value_type);

  if(trigger) {
    // all triggers are off by default
    col->def=0;
    if(type==G_TYPE_STRING) {
      col->type=PCT_NOTE;
      col->min=0;
      col->max=((16*9)+12);
      col->user_data=&pcc[0];
    }
    else if(type==G_TYPE_BOOLEAN) {
      col->type=PCT_SWITCH;
      col->min=0;
      col->max=1;
      col->user_data=NULL;
    }
    else if(type==BT_TYPE_TRIGGER_SWITCH) {
      col->type=PCT_SWITCH;
      col->min=0;
      col->max=1;
      col->user_data=NULL;
    }
    else {
      GST_WARNING("unhandled trigger param type: '%s'",g_type_name(type));
      col->type=0;
      col->min=col->max=0;
      col->user_data=NULL;
    }
  }
  else {
    if(type==G_TYPE_BOOLEAN) {
      const GParamSpecBoolean *bool_property=G_PARAM_SPEC_BOOLEAN(property);
      col->type=PCT_BYTE;
      col->min=0;
      col->max=1;
      col->def=bool_property->default_value;
      col->user_data=NULL;
    }
    else if(type==G_TYPE_ENUM) {
      const GParamSpecEnum *enum_property=G_PARAM_SPEC_ENUM(property);
      const GEnumClass *enum_class=enum_property->enum_class;
 
      col->type=PCT_BYTE;
      col->min=enum_class->minimum;
      col->max=enum_class->maximum;
      col->def=enum_property->default_value;
      col->user_data=NULL;
    }
    else if(type==G_TYPE_INT) {
      const GParamSpecInt *int_property=G_PARAM_SPEC_INT(property);

      col->type=PCT_WORD;
      col->min=int_property->minimum;
      col->max=int_property->maximum;
      col->def=int_property->default_value;
      if(int_property->minimum>=0 && int_property->maximum<256) {
        col->type=PCT_BYTE;
      }
      col->user_data=NULL;
    }
    else if(type==G_TYPE_UINT) {
      const GParamSpecUInt *uint_property=G_PARAM_SPEC_UINT(property);

      col->type=PCT_WORD;
      col->min=uint_property->minimum;
      col->max=uint_property->maximum;
      col->def=uint_property->default_value;
      if(uint_property->minimum>=0 && uint_property->maximum<256) {
        col->type=PCT_BYTE;
      }
      col->user_data=NULL;
    }
    else if(type==G_TYPE_FLOAT) {
      const GParamSpecFloat *float_property=G_PARAM_SPEC_FLOAT(property);

      col->type=PCT_WORD;
      col->min=float_property->minimum;
      col->max=float_property->maximum;
      col->def=float_property->default_value;
      // @todo: need scaling
      // scaling factor =  
      col->user_data=NULL;
    }
    else if(type==G_TYPE_DOUBLE) {
      const GParamSpecDouble *double_property=G_PARAM_SPEC_DOUBLE(property);
      
      col->type=PCT_WORD;
      col->min=double_property->minimum;
      col->max=double_property->maximum;
      col->def=double_property->default_value;
      // @todo: need scaling: min->0, max->65536
      col->user_data=NULL;
    }
    else {
      GST_WARNING("unhandled continous param type: '%s'",g_type_name(type));
      col->type=0;
      col->min=col->max=col->def=0;
      col->user_data=NULL;
    }
  }
}

static void pattern_table_refresh(const BtMainPagePatterns *self,const BtPattern *pattern) {
  static BtPatternEditorCallbacks callbacks = {
    pattern_edit_get_data_at,
    pattern_edit_set_data_at,
    NULL
  };
  
  if(pattern) {
    gulong i;
    gulong number_of_ticks,voices,global_params,voice_params;
    BtMachine *machine;
    
    g_object_get(G_OBJECT(pattern),"length",&number_of_ticks,"voices",&voices,"machine",&machine,NULL);
    g_object_get(G_OBJECT(machine),"global-params",&global_params,"voice-params",&voice_params,NULL);
    GST_DEBUG("  size is %2d,%2d",number_of_ticks,global_params);

    // wire pattern data
#if 0
    PatternColumnGroup *group;
    
    self->priv->number_of_groups=(global_params>0?1:0)+voices;
    g_free(self->priv->param_groups);

    if(!BT_IS_SOURCE_MACHINE(machine)) {
      BtSong *song;
      BtSetup *setup;
      BtWire *wire;
      GList *wires,*node;
      gulong wire_params;
      
      // need to iterate over all inputs
      g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
      g_object_get(G_OBJECT(song),"setup",&setup,NULL);
      wires=bt_setup_get_wires_by_dst_machine(setup,machine);
      
      self->priv->number_of_groups+=g_list_length(wires);
      group=self->priv->param_groups=g_new(PatternColumnGroup,self->priv->number_of_groups);
      
      for(node=wires;node;node=g_list_next(node)) {
        wire=BT_WIRE(node->data);
        // check wire config
        g_object_get(G_OBJECT(wire),"num-params",&wire_params,NULL);
        group->type=0;
        group->name=wire.src.name
        group->user_data=wire;
        group->num_columns=wire_params;
        group->columns=g_new(PatternColumn,wire_params);
        for(i=0;i<wire_params;i++) {
          pattern_edit_fill_column_type(&group->columns[i],
            bt_wire_get_param_spec(wire,i),FALSE);
        }
        g_object_unref(wire);
        group++;
      }
      g_list_free(wires);
      g_object_unref(setup);
      g_object_unref(song);
    }
    else {
      group=self->priv->param_groups=g_new(PatternColumnGroup,self->priv->number_of_groups);  
    }
    if(global_params) {
      // create mapping for global params
      group->type=1;
      group->name=g_strdup("globals");
      group->user_data=0;
      group->num_columns=global_params;
      group->columns=g_new(PatternColumn,global_params);
      for(i=0;i<global_params;i++) {
        pattern_edit_fill_column_type(&group->columns[i],
          bt_machine_get_global_param_spec(machine,i),
          bt_machine_is_global_param_trigger(machine,i));
      }
      group++;
    }
    if(voices) {
      PatternColumnGroup *stamp=group;
      // create mapping for voice params
      group->type=2;
      group->name=g_strdup("voice 1");
      group->user_data=1;
      group->num_columns=voice_params;
      group->columns=g_new(PatternColumn,voice_params);
      for(i=0;i<voice_params;i++) {
        pattern_edit_fill_column_type(&group->columns[i],
          bt_machine_get_voice_param_spec(machine,i),
          bt_machine_is_voice_param_trigger(machine,i));
      }
      group++;
      for(i=1;i<voices;i++) {
        group->type=2;
        group->name=g_strdup_printf("voice %d",i);
        group->user_data=i;
        group->num_columns=voice_params;
        group->columns=g_memdup(stamp->columns,sizeof(PatternColumn)*voice_params);
        group++;        
      }
    }

    bt_pattern_editor_set_pattern(self->priv->pattern_table, (gpointer)self,
      number_of_ticks, self->priv->number_of_groups, self->priv->param_groups, &callbacks);

#else
    // machine pattern data    
    g_free(self->priv->global_param_descs);
    self->priv->global_param_descs=g_new(PatternColumn,global_params);
    g_free(self->priv->voice_param_descs);
    self->priv->voice_param_descs=g_new(PatternColumn,voice_params);
    // create mapping for global params
    for(i=0;i<global_params;i++) {
      pattern_edit_fill_column_type(&self->priv->global_param_descs[i],
        bt_machine_get_global_param_spec(machine,i),
        bt_machine_is_global_param_trigger(machine,i));
    }
    // create mapping for voice params
    for(i=0;i<voice_params;i++) {
      pattern_edit_fill_column_type(&self->priv->voice_param_descs[i],
        bt_machine_get_voice_param_spec(machine,i),
        bt_machine_is_voice_param_trigger(machine,i));
    }
      
    bt_pattern_editor_set_pattern(self->priv->pattern_table, (gpointer)self,
      number_of_ticks, voices,
      global_params, voice_params,
      self->priv->global_param_descs, self->priv->voice_param_descs,
      &callbacks);
#endif    
    g_object_unref(machine);
  }
  else {
    bt_pattern_editor_set_pattern(self->priv->pattern_table, (gpointer)self, 0, 0, 0, 0, NULL, NULL, &callbacks);
  }
}
#endif

/*
 * context_menu_refresh:
 * @self:  the pattern page
 * @machine: the currently selected machine
 *
 * enable/disable context menu items
 */
static void context_menu_refresh(const BtMainPagePatterns *self,BtMachine *machine) {
  if(machine) {
    gboolean has_patterns=bt_machine_has_patterns(machine);

    //gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu),TRUE);
    if(has_patterns) {
      if(bt_machine_is_polyphonic(machine)) {
        gint voices;

        g_object_get(machine,"voices",&voices,NULL);
        gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_track_add),TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_track_remove),(voices>0));
      }
      else {
        gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_track_add),FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_track_remove),FALSE);
      }
      gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_pattern_properties),TRUE);
      gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_pattern_remove),TRUE);
      gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_pattern_copy),TRUE);
    }
    else {
      GST_INFO("machine has no patterns");
      gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_track_add),FALSE);
      gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_track_remove),FALSE);
      gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_pattern_properties),FALSE);
      gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_pattern_remove),FALSE);
      gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_pattern_copy),FALSE);
    }
  }
  else {
    GST_WARNING("no machine, huh?");
    //gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu),FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_track_add),FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_track_remove),FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_pattern_properties),FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_pattern_remove),FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_pattern_copy),FALSE);
  }
}

//-- event handler

static gboolean on_page_switched_idle(gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);

  if(GTK_WIDGET_REALIZED(self->priv->pattern_table)) {
    gtk_widget_grab_focus(GTK_WIDGET(self->priv->pattern_table));
  }
  return(FALSE);
}

static void on_page_switched(GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  BtMainWindow *main_window;
  static gint prev_page_num=-1;

  if(page_num==BT_MAIN_PAGES_PATTERNS_PAGE) {
    // only do this if the page really has changed
    if(prev_page_num != BT_MAIN_PAGES_PATTERNS_PAGE) {
      BtMachine *machine;
  
      GST_DEBUG("enter pattern page");
      // only set new text
      pattern_view_update_column_description(self,UPDATE_COLUMN_PUSH);
      if((machine=bt_main_page_patterns_get_current_machine(self))) {
        // refresh to update colors in the menu (as usage might have changed)
        pattern_menu_refresh(self,machine);
        g_object_unref(machine);
      }
      // add local commands
      g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
      if(main_window) {
        gtk_window_add_accel_group(GTK_WINDOW(main_window),self->priv->accel_group);
        // workaround for http://bugzilla.gnome.org/show_bug.cgi?id=469374
        g_signal_emit_by_name (main_window, "keys-changed", 0);
        g_object_unref(main_window);
      }
      // delay the pattern_table grab
      g_idle_add_full(G_PRIORITY_HIGH_IDLE,on_page_switched_idle,user_data,NULL);
    }
  }
  else {
    // only do this if the page was BT_MAIN_PAGES_PATTERNS_PAGE
    if(prev_page_num == BT_MAIN_PAGES_PATTERNS_PAGE) {
      // only reset old
      GST_DEBUG("leave pattern page");
      pattern_view_update_column_description(self,UPDATE_COLUMN_POP);
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
    GST_INFO("unref old pattern: %p,refs=%d", self->priv->pattern,(G_OBJECT(self->priv->pattern))->ref_count);
    g_object_unref(self->priv->pattern);
  }

  // refresh pattern view
  self->priv->pattern=bt_main_page_patterns_get_current_pattern(self);
  GST_INFO("ref'ed new pattern: %p,refs=%d",
    self->priv->pattern,(self->priv->pattern?(G_OBJECT(self->priv->pattern))->ref_count:-1));

  GST_INFO("new pattern selected : %p",self->priv->pattern);
  pattern_table_refresh(self,self->priv->pattern);
  pattern_view_update_column_description(self,UPDATE_COLUMN_UPDATE);
  if(GTK_WIDGET_REALIZED(self->priv->pattern_table)) {
    gtk_widget_grab_focus(GTK_WIDGET(self->priv->pattern_table));
  }
  if(self->priv->pattern) {
    // watch the pattern
    self->priv->pattern_length_changed=g_signal_connect(G_OBJECT(self->priv->pattern),"notify::length",G_CALLBACK(on_pattern_size_changed),(gpointer)self);
    self->priv->pattern_voices_changed=g_signal_connect(G_OBJECT(self->priv->pattern),"notify::voices",G_CALLBACK(on_pattern_size_changed),(gpointer)self);
  }
}

static void on_base_octave_menu_changed(GtkComboBox *menu, gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);

  g_assert(user_data);

  self->priv->base_octave=gtk_combo_box_get_active(self->priv->base_octave_menu);
  if(GTK_WIDGET_REALIZED(self->priv->pattern_table)) {
    gtk_widget_grab_focus(GTK_WIDGET(self->priv->pattern_table));
  }
}

static void on_machine_added(BtSetup *setup,BtMachine *machine,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  GtkTreeModel *store;
  gint index;

  g_assert(user_data);

  GST_INFO("new machine %p,ref_count=%d has been added",machine,G_OBJECT(machine)->ref_count);
  store=gtk_combo_box_get_model(self->priv->machine_menu);
  machine_menu_add(self,machine,GTK_LIST_STORE(store));

  index=gtk_tree_model_iter_n_children(store,NULL);
  if(index==1) {
    gtk_widget_set_sensitive(GTK_WIDGET(self->priv->machine_menu),TRUE);
  }
  gtk_combo_box_set_active(self->priv->machine_menu,index-1);
}

static void on_machine_removed(BtSetup *setup,BtMachine *machine,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  GtkTreeModel *store;
  GtkTreeIter iter;
  gint index;

  g_assert(user_data);
  g_return_if_fail(BT_IS_MACHINE(machine));

  GST_INFO("machine %p,ref_count=%d has been removed",machine,G_OBJECT(machine)->ref_count);
  store=gtk_combo_box_get_model(self->priv->machine_menu);
  // get the row where row.machine==machine
  machine_model_get_iter_by_machine(store,&iter,machine);
  gtk_list_store_remove(GTK_LIST_STORE(store),&iter);
  index=gtk_tree_model_iter_n_children(store,NULL);
  if(index==0) {
    gtk_widget_set_sensitive(GTK_WIDGET(self->priv->machine_menu),FALSE);
  }
  gtk_combo_box_set_active(self->priv->machine_menu,index-1);
  GST_INFO("... machine %p,ref_count=%d has been removed",machine,G_OBJECT(machine)->ref_count);
}

static void on_wire_added_or_removed(BtSetup *setup,BtWire *wire,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  BtMachine *this_machine,*that_machine;
  
  if(!self->priv->pattern) return;
  
  g_object_get(self->priv->pattern,"machine",&this_machine,NULL);
  g_object_get(wire,"dst",&that_machine,NULL);
  if(this_machine==that_machine) {
    pattern_table_refresh(self,self->priv->pattern);
  }
  g_object_unref(this_machine);
  g_object_unref(that_machine);

}

static void on_machine_menu_changed(GtkComboBox *menu, gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  BtMachine *machine;

  g_assert(user_data);
  GST_INFO("machine_menu changed");
  if((machine=bt_main_page_patterns_get_current_machine(self))) {
    GST_INFO("refreshing menues for machine %p,ref_count=%d",machine,G_OBJECT(machine)->ref_count);
    // show new list of pattern in pattern menu
    pattern_menu_refresh(self,machine);
    GST_INFO("1st done for  machine %p,ref_count=%d",machine,G_OBJECT(machine)->ref_count);
    // refresh context menu
    context_menu_refresh(self,machine);
    GST_INFO("2nd done for  machine %p,ref_count=%d",machine,G_OBJECT(machine)->ref_count);
    g_object_try_unref(machine);
  }
  else {
    GST_WARNING("current machine == NULL");
  }
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
#ifndef USE_PATTERN_EDITOR
              g_object_set(self->priv->pattern_table,"play-position",play_pos,NULL);
              g_object_set(self->priv->pattern_pos_table,"play-position",play_pos,NULL);
#else
              // @todo: need to set playpos
#endif
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
#ifndef USE_PATTERN_EDITOR
    g_object_set(self->priv->pattern_table,"play-position",0.0,NULL);
    g_object_set(self->priv->pattern_pos_table,"play-position",0.0,NULL);
#else
    // @todo: need to set playpos
#endif
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
  if(!song) return;
  GST_INFO("song->ref_ct=%d",G_OBJECT(song)->ref_count);

  g_object_get(G_OBJECT(song),"setup",&setup,NULL);
  // update page
  machine_menu_refresh(self,setup);
  //pattern_menu_refresh(self); // should be triggered by machine_menu_refresh()
  wavetable_menu_refresh(self);
  g_signal_connect(G_OBJECT(setup),"machine-added",G_CALLBACK(on_machine_added),(gpointer)self);
  g_signal_connect(G_OBJECT(setup),"machine-removed",G_CALLBACK(on_machine_removed),(gpointer)self);
  g_signal_connect(G_OBJECT(setup),"wire-added",G_CALLBACK(on_wire_added_or_removed),(gpointer)self);
  g_signal_connect(G_OBJECT(setup),"wire-removed",G_CALLBACK(on_wire_added_or_removed),(gpointer)self);
  // subscribe to play-pos changes of song->sequence
  g_signal_connect(G_OBJECT(song), "notify::play-pos", G_CALLBACK(on_sequence_tick), (gpointer)self);
  // release the references
  g_object_unref(setup);
  g_object_unref(song);
  GST_INFO("song has changed done");
}

static void on_context_menu_track_add_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  BtMachine *machine;
  gint voices;

  g_assert(user_data);

  machine=bt_main_page_patterns_get_current_machine(self);
  g_return_if_fail(machine);
  g_object_get(machine,"voices",&voices,NULL);
  voices++;
  g_object_set(machine,"voices",voices,NULL);

  pattern_menu_refresh(self,machine);
  context_menu_refresh(self,machine);

  g_object_unref(machine);
  g_assert(user_data);
}

static void on_context_menu_track_remove_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  BtMachine *machine;
  gint voices;

  g_assert(user_data);

  machine=bt_main_page_patterns_get_current_machine(self);
  g_return_if_fail(machine);
  g_object_get(machine,"voices",&voices,NULL);
  voices--;
  g_object_set(machine,"voices",voices,NULL);

  pattern_menu_refresh(self,machine);
  context_menu_refresh(self,machine);

  g_object_unref(machine);
  g_assert(user_data);
}

static void on_context_menu_pattern_new_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  BtMainWindow *main_window;
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

  g_object_get(G_OBJECT(self->priv->app),"song",&song,"main-window",&main_window,NULL);
  g_object_get(G_OBJECT(song),"song-info",&song_info,NULL);
  g_object_get(G_OBJECT(song_info),"bars",&bars,NULL);
  g_object_get(G_OBJECT(machine),"id",&mid,NULL);

  name=bt_machine_get_unique_pattern_name(machine);
  id=g_strdup_printf("%s %s",mid,name);
  // new_pattern
  pattern=bt_pattern_new(song, id, name, bars, machine);

  // pattern_properties
  if((dialog=GTK_WIDGET(bt_pattern_properties_dialog_new(self->priv->app,pattern)))) {
    gtk_window_set_transient_for(GTK_WINDOW(dialog),GTK_WINDOW(main_window));
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
  }

  // free ressources
  g_free(mid);
  g_free(id);
  g_free(name);
  g_object_unref(pattern);
  g_object_unref(song_info);
  g_object_unref(song);
  g_object_unref(machine);
  g_object_unref(main_window);
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
  if((dialog=GTK_WIDGET(bt_pattern_properties_dialog_new(self->priv->app,pattern)))) {
    BtMainWindow *main_window;

    g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
    gtk_window_set_transient_for(GTK_WINDOW(dialog),GTK_WINDOW(main_window));
    g_object_unref(main_window);
    gtk_widget_show_all(dialog);

    if(gtk_dialog_run(GTK_DIALOG(dialog))==GTK_RESPONSE_ACCEPT) {
      bt_pattern_properties_dialog_apply(BT_PATTERN_PROPERTIES_DIALOG(dialog));
    }
    gtk_widget_destroy(dialog);
  }
  g_object_unref(pattern);
}

static void on_context_menu_pattern_remove_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  BtMainWindow *main_window;
  BtPattern *pattern;
  BtSong *song;
  BtSequence *sequence;
  gchar *msg,*id;

  g_assert(user_data);

  pattern=bt_main_page_patterns_get_current_pattern(self);
  g_return_if_fail(pattern);

  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,"song",&song,NULL);
  g_object_get(G_OBJECT(song),"sequence",&sequence,NULL);
  g_object_get(pattern,"name",&id,NULL);

  if(bt_sequence_is_pattern_used(sequence,pattern)) {
    msg=g_strdup_printf(_("Delete used pattern '%s'"),id);
  }
  else {
    msg=g_strdup_printf(_("Delete unused pattern '%s'"),id);
  }
  g_free(id);

  if(bt_dialog_question(main_window,_("Delete pattern..."),msg,_("There is no undo for this."))) {
    BtMachine *machine;

    machine=bt_main_page_patterns_get_current_machine(self);
    bt_machine_remove_pattern(machine,pattern);
    pattern_menu_refresh(self,machine);
    context_menu_refresh(self,machine);

    g_object_unref(machine);
  }
  g_object_unref(sequence);
  g_object_unref(main_window);
  g_object_unref(song);
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
  if((dialog=GTK_WIDGET(bt_pattern_properties_dialog_new(self->priv->app,pattern)))) {
    BtMainWindow *main_window;

    g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
    gtk_window_set_transient_for(GTK_WINDOW(dialog),GTK_WINDOW(main_window));
    g_object_unref(main_window);
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
  }

  g_object_unref(pattern);
  g_object_unref(machine);
}

//-- helper methods

static gboolean bt_main_page_patterns_init_ui(const BtMainPagePatterns *self,const BtMainPages *pages) {
  GtkWidget *toolbar,*tool_item,*box;
  GtkWidget *scrolled_window;
  GtkWidget *menu_item,*image;
  GtkCellRenderer *renderer;
  gint i;
  gchar oct_str[2];
#ifndef USE_PATTERN_EDITOR
  GtkWidget *scrolled_sync_window;
  GtkTreeSelection *tree_sel;
  GtkAdjustment *vadjust;
#endif

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
  /*
   * this won't work, as we can't pass anything to the event handler
   * gtk_widget_add_accelerator(self->priv->machine_menu, "key-press-event", accel_group, GDK_Cursor_Up, GDK_CONTROL_MASK, 0);
   * so, we need to subclass the combobox and add two signals: select-next, select-prev
   */

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
  g_object_set(G_OBJECT(renderer),
    "foreground","gray",
    NULL);
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(self->priv->pattern_menu),renderer,TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(self->priv->pattern_menu),renderer,
    "text",PATTERN_MENU_LABEL,
    "foreground-set",PATTERN_MENU_COLOR_SET,
    NULL);
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
  gtk_combo_box_set_active(self->priv->base_octave_menu,self->priv->base_octave);
  gtk_box_pack_start(GTK_BOX(box),gtk_label_new(_("Octave")),FALSE,FALSE,2);
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->priv->base_octave_menu),TRUE,TRUE,2);
  g_signal_connect(G_OBJECT(self->priv->base_octave_menu), "changed", G_CALLBACK(on_base_octave_menu_changed), (gpointer)self);

  tool_item=GTK_WIDGET(gtk_tool_item_new());
  gtk_widget_set_name(tool_item,_("Octave"));
  gtk_container_add(GTK_CONTAINER(tool_item),box);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar),GTK_TOOL_ITEM(tool_item),-1);

  gtk_toolbar_insert(GTK_TOOLBAR(toolbar),gtk_separator_tool_item_new(),-1);

  // @todo add play notes checkbox or toggle tool button
  /*gtk_check_button_new();*/

  // get colors
  self->priv->cursor_bg=bt_ui_ressources_get_gdk_color(BT_UI_RES_COLOR_CURSOR);
  self->priv->selection_bg1=bt_ui_ressources_get_gdk_color(BT_UI_RES_COLOR_SELECTION1);
  self->priv->selection_bg2=bt_ui_ressources_get_gdk_color(BT_UI_RES_COLOR_SELECTION2);

#ifndef USE_PATTERN_EDITOR
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
#endif

  /* @idea what about adding one control for global params and one for each voice,
   * - then these controls can be folded (hidden)
   */
  // add pattern list-view
  scrolled_window=gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_NONE);
#ifdef USE_PATTERN_EDITOR
  self->priv->pattern_table=BT_PATTERN_EDITOR(bt_pattern_editor_new());
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window),GTK_WIDGET(self->priv->pattern_table));
  g_signal_connect(G_OBJECT(self->priv->pattern_table), "button-press-event", G_CALLBACK(on_pattern_table_button_press_event), (gpointer)self);
  gtk_container_add(GTK_CONTAINER(self), GTK_WIDGET(scrolled_window));
#else
  self->priv->pattern_table=GTK_TREE_VIEW(bt_pattern_view_new(self->priv->app));
  tree_sel=gtk_tree_view_get_selection(self->priv->pattern_table);
  gtk_tree_selection_set_mode(tree_sel,GTK_SELECTION_NONE);
  g_object_set(self->priv->pattern_table,"enable-search",FALSE,"rules-hint",TRUE,"fixed-height-mode",TRUE,NULL);
  gtk_container_add(GTK_CONTAINER(scrolled_window),GTK_WIDGET(self->priv->pattern_table));
  g_signal_connect_after(G_OBJECT(self->priv->pattern_table), "cursor-changed", G_CALLBACK(on_pattern_table_cursor_changed), (gpointer)self);
  g_signal_connect(G_OBJECT(self->priv->pattern_table), "key-release-event", G_CALLBACK(on_pattern_table_key_release_event), (gpointer)self);
  g_signal_connect(G_OBJECT(self->priv->pattern_table), "motion-notify-event", G_CALLBACK(on_pattern_table_motion_notify_event), (gpointer)self);
  gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(scrolled_window), TRUE, TRUE, 0);
#endif

#ifndef USE_PATTERN_EDITOR
  // make first scrolled-window also use the horiz-scrollbar of the second scrolled-window
  vadjust=gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled_window));
  gtk_scrolled_window_set_vadjustment(GTK_SCROLLED_WINDOW(scrolled_sync_window),vadjust);
#endif

  GST_DEBUG("  before context menu",self);
  // generate the context menu
  self->priv->accel_group=gtk_accel_group_new();
  self->priv->context_menu=GTK_MENU(gtk_menu_new());
  gtk_menu_set_accel_group(GTK_MENU(self->priv->context_menu), self->priv->accel_group);
  gtk_menu_set_accel_path(GTK_MENU(self->priv->context_menu),"<Buzztard-Main>/PatternView/PatternContext");

  self->priv->context_menu_track_add=menu_item=gtk_image_menu_item_new_with_label(_("New track"));
  image=gtk_image_new_from_stock(GTK_STOCK_ADD,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_menu_item_set_accel_path (GTK_MENU_ITEM (menu_item), "<Buzztard-Main>/PatternView/PatternContext/AddTrack");
  gtk_accel_map_add_entry ("<Buzztard-Main>/PatternView/PatternContext/AddTrack", GDK_plus, GDK_CONTROL_MASK);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_context_menu_track_add_activate),(gpointer)self);

  self->priv->context_menu_track_remove=menu_item=gtk_image_menu_item_new_with_label(_("Remove last track"));
  image=gtk_image_new_from_stock(GTK_STOCK_REMOVE,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_menu_item_set_accel_path (GTK_MENU_ITEM (menu_item), "<Buzztard-Main>/PatternView/PatternContext/RemoveTrack");
  gtk_accel_map_add_entry ("<Buzztard-Main>/PatternView/PatternContext/RemoveTrack", GDK_minus, GDK_CONTROL_MASK);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_context_menu_track_remove_activate),(gpointer)self);

  menu_item=gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_set_sensitive(menu_item,FALSE);
  gtk_widget_show(menu_item);

  menu_item=gtk_image_menu_item_new_with_label(_("New pattern  ..."));
  image=gtk_image_new_from_stock(GTK_STOCK_NEW,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_menu_item_set_accel_path (GTK_MENU_ITEM (menu_item), "<Buzztard-Main>/PatternView/PatternContext/NewPattern");
  gtk_accel_map_add_entry ("<Buzztard-Main>/PatternView/PatternContext/NewPattern", GDK_Return, GDK_CONTROL_MASK);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_context_menu_pattern_new_activate),(gpointer)self);

  self->priv->context_menu_pattern_properties=menu_item=gtk_image_menu_item_new_with_label(_("Pattern properties ..."));
  image=gtk_image_new_from_stock(GTK_STOCK_PROPERTIES,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_menu_item_set_accel_path (GTK_MENU_ITEM (menu_item), "<Buzztard-Main>/PatternView/PatternContext/PatternProperties");
  gtk_accel_map_add_entry ("<Buzztard-Main>/PatternView/PatternContext/PatternProperties", GDK_BackSpace, GDK_CONTROL_MASK);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_context_menu_pattern_properties_activate),(gpointer)self);

  self->priv->context_menu_pattern_remove=menu_item=gtk_image_menu_item_new_with_label(_("Remove pattern ..."));
  image=gtk_image_new_from_stock(GTK_STOCK_DELETE,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_menu_item_set_accel_path (GTK_MENU_ITEM (menu_item), "<Buzztard-Main>/PatternView/PatternContext/RemovePattern");
  gtk_accel_map_add_entry ("<Buzztard-Main>/PatternView/PatternContext/RemovePattern", GDK_Delete, GDK_CONTROL_MASK);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_context_menu_pattern_remove_activate),(gpointer)self);

  self->priv->context_menu_pattern_copy=menu_item=gtk_image_menu_item_new_with_label(_("Copy pattern ..."));
  image=gtk_image_new_from_stock(GTK_STOCK_COPY,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_menu_item_set_accel_path (GTK_MENU_ITEM (menu_item), "<Buzztard-Main>/PatternView/PatternContext/CopyPattern");
  gtk_accel_map_add_entry ("<Buzztard-Main>/PatternView/PatternContext/CopyPattern", GDK_Return, GDK_CONTROL_MASK|GDK_SHIFT_MASK);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_context_menu_pattern_copy_activate),(gpointer)self);
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
 * @pages: the page collection
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
      GST_DEBUG("  got machine: %p,machine-refs: %d",machine,(G_OBJECT(machine))->ref_count);
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
      GST_DEBUG("  got machine: %p,machine-refs: %d",machine,(G_OBJECT(machine))->ref_count);
      if(gtk_combo_box_get_active_iter(self->priv->pattern_menu,&iter)) {
        store=gtk_combo_box_get_model(self->priv->pattern_menu);
        gtk_tree_model_get(store,&iter,PATTERN_MENU_PATTERN,&pattern,-1);
        if(pattern) {
          g_object_unref(machine);
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
 * Show the given @pattern. Will update machine and pattern menu.
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

/**
 * bt_main_page_patterns_show_machine:
 * @self: the pattern subpage
 * @machine: the machine to show
 *
 * Show the given @machine. Will update machine menu.
 */
void bt_main_page_patterns_show_machine(const BtMainPagePatterns *self,BtMachine *machine) {
  GtkTreeIter iter;
  GtkTreeModel *store;

  // update machine menu
  store=gtk_combo_box_get_model(self->priv->machine_menu);
  machine_model_get_iter_by_machine(store,&iter,machine);
  gtk_combo_box_set_active_iter(self->priv->machine_menu,&iter);
  // focus pattern editor
  gtk_widget_grab_focus(GTK_WIDGET(self->priv->pattern_table));
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

  // @bug: http://bugzilla.gnome.org/show_bug.cgi?id=414712
  gtk_container_set_focus_child(GTK_CONTAINER(self),NULL);

  g_object_try_weak_unref(self->priv->app);
  GST_INFO("unref pattern: %p,refs=%d",
    self->priv->pattern,(self->priv->pattern?(G_OBJECT(self->priv->pattern))->ref_count:-1));
  g_object_try_unref(self->priv->pattern);

  gtk_object_destroy(GTK_OBJECT(self->priv->context_menu));
  gtk_object_destroy(GTK_OBJECT(self->priv->machine_menu));
  gtk_object_destroy(GTK_OBJECT(self->priv->pattern_menu));
  gtk_object_destroy(GTK_OBJECT(self->priv->wavetable_menu));
  gtk_object_destroy(GTK_OBJECT(self->priv->base_octave_menu));

  g_object_try_unref(self->priv->accel_group);

  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_main_page_patterns_finalize(GObject *object) {
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS(object);

  g_free(self->priv->column_keymode);
#if 0
  gulong i;
  
  for(i=0;i<self->priv->number_of_groups;i++) {
    g_free(self->priv->param_descs[i].name;
    g_free(self->priv->param_descs[i].columns;
  }
  g_free(self->priv->param_descs);
#else
  g_free(self->priv->global_param_descs);
  g_free(self->priv->voice_param_descs);
#endif

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

  self->priv->base_octave=2;
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
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));
}

GType bt_main_page_patterns_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof(BtMainPagePatternsClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_main_page_patterns_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtMainPagePatterns),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_main_page_patterns_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(GTK_TYPE_VBOX,"BtMainPagePatterns",&info,0);
  }
  return type;
}
