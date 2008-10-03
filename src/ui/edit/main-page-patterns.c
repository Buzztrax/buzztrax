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
 *   - prev/next for combobox entries
 *     - trigger "move-active" action signal with GTK_SCROLL_STEP_UP/GTK_SCROLL_STEP_DOWN
 *     - what mechanism to use:
 *       - gtk_binding_entry_add_signal (do bindings work when not focused?)
 *       - gtk_widget_add_accelerator (can't specify signal params)
 * - copy gtk_cell_renderer_progress -> bt_cell_renderer_pattern_value
 *   - limmit acceptable keys for value entries: http://www.gtk.org/faq/#AEN843
 *
 * - also do controller-assignments like in machine-property window
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
  BtPatternEditor *pattern_table;

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
  guint cursor_group, cursor_param;
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
  gulong number_of_groups;
  PatternColumnGroup *param_groups;
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
    //g_object_get(G_OBJECT(pages),"page",&page_num,NULL);
    page_num=gtk_notebook_get_current_page(GTK_NOTEBOOK(pages));
    g_object_unref(pages);

    if(page_num!=BT_MAIN_PAGES_PATTERNS_PAGE) return;
  }

  g_object_get(main_window,"statusbar",&statusbar,NULL);

  // pop previous text by passing str=NULL;
  if(mode&UPDATE_COLUMN_POP)
    g_object_set(statusbar,"status",NULL,NULL);

  if(mode&UPDATE_COLUMN_PUSH) {
    gchar *str=NULL,*desc="\0";
    const gchar *blurb="\0";

    if(self->priv->pattern && self->priv->number_of_groups) {
      GParamSpec *property=NULL;
      PatternColumnGroup *group;
      
      g_object_get(G_OBJECT(self->priv->pattern_table), "cursor-row", &self->priv->cursor_row, "cursor-param", &self->priv->cursor_param, "cursor-group", &self->priv->cursor_group, NULL);

      
      group = &self->priv->param_groups[self->priv->cursor_group];
      switch (group->type) {
        case 0: {
          property=bt_wire_get_param_spec(group->user_data, self->priv->cursor_param);
          /* there is no describe here
          bt_wire_pattern_get_event_data(group->user_data,self->priv->cursor_row,self->priv->cursor_param)
          */
        } break;
        case 1: {
          BtMachine *machine;
          GValue *gval;
          
          g_object_get(self->priv->pattern,"machine",&machine,NULL);
          property=bt_machine_get_global_param_spec(machine,self->priv->cursor_param);
          if((gval=bt_pattern_get_global_event_data(self->priv->pattern,self->priv->cursor_row,self->priv->cursor_param)) && G_IS_VALUE(gval)) {
            if(!(desc=bt_machine_describe_global_param_value(machine,self->priv->cursor_param,gval)))
              desc="\0";
          }
          g_object_unref(machine);
        } break;
        case 2: {
          BtMachine *machine;
          GValue *gval;
          
          g_object_get(self->priv->pattern,"machine",&machine,NULL);
          property=bt_machine_get_voice_param_spec(machine,self->priv->cursor_param);
          if((gval=bt_pattern_get_voice_event_data(self->priv->pattern,self->priv->cursor_row,GPOINTER_TO_UINT(group->user_data),self->priv->cursor_param)) && G_IS_VALUE(gval)) {
            if(!(desc=bt_machine_describe_voice_param_value(machine,self->priv->cursor_param,gval)))
              desc="\0";
          }
          g_object_unref(machine);
        } break;
        default:
          GST_WARNING("invalid column group type");
      }
      // get parameter description
      if(property)
        blurb=g_param_spec_get_blurb(property);
      str=g_strdup_printf("%s%s%s",blurb,(*desc?": ":""),desc);
    }
    g_object_set(statusbar,"status",(str?str:BT_MAIN_STATUSBAR_DEFAULT),NULL);
    g_free(str);
  }

  g_object_unref(statusbar);
  g_object_unref(main_window);
}

//-- selection helpers
#if 0
static boolean pattern_selection_apply(const BtMainPagePatterns *self,pattern_fn,wire_pattern_fn) {
  /* @todo: use pattern_fn / wire_pattern_fn */
  gint beg,end,group,param;
  if(bt_pattern_editor_get_selection(self->priv->pattern_table,&beg,&end,&group,&param)) {
    PatternColumnGroup group;
    GST_INFO("blending : %d %d , %d %d",beg,end,group,param);
    if(group==-1 && param==-1) {
      // blend full pattern
      BtWirePattern *wire_pattern;
      guint i=0;

      group=self->priv->param_groups[0];
      while(group.type==0) {
        wire_pattern = bt_wire_get_pattern(group.user_data,self->priv->pattern);
        if(!wire_pattern) {
          BtSong *song;
  
          g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
          wire_pattern=bt_wire_pattern_new(song,group.user_data,self->priv->pattern);
          g_object_unref(song);
        }
        if(wire_pattern) {
          bt_wire_pattern_blend_columns(wire_pattern,beg,end);
          g_object_unref(wire_pattern);
        }
        i++;
        group=self->priv->param_groups[i];
      }
      bt_pattern_blend_columns(self->priv->pattern,beg,end);
      gtk_widget_queue_draw(GTK_WIDGET(self->priv->pattern_table));
      res=TRUE;
    }
    if(group!=-1 && param==-1) {
      // blend whole group
      group=self->priv->param_groups[group];
      switch(group.type) {
        case 0: {
          BtWirePattern *wire_pattern = bt_wire_get_pattern(group.user_data,self->priv->pattern);
          if(!wire_pattern) {
            BtSong *song;
    
            g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
            wire_pattern=bt_wire_pattern_new(song,group.user_data,self->priv->pattern);
            g_object_unref(song);
          }
          if(wire_pattern) {
            // 0,group.num_columns-1
            bt_wire_pattern_blend_columns(wire_pattern,beg,end);
            g_object_unref(wire_pattern);
          }
        } break;
        case 1: {
          guint i;
          
          for(i=0;i<group.num_columns;i++) {
            bt_pattern_blend_column(self->priv->pattern,beg,end,i);
          }
        } break;
        case 2: {
          BtMachine *machine;
          gulong global_params, voice_params, params;
          guint i;
          
          g_object_get(G_OBJECT(self->priv->pattern),"machine",&machine,NULL);
          g_object_get(G_OBJECT(machine),"global-params",&global_params,"voice-params",&voice_params,NULL);
          params=global_params+(GPOINTER_TO_UINT(group.user_data)*voice_params);
          for(i=0;i<group.num_columns;i++) {
            bt_pattern_blend_column(self->priv->pattern,beg,end,params+i);
          }
          g_object_unref(machine);
        } break;
      }
      gtk_widget_queue_draw(GTK_WIDGET(self->priv->pattern_table));
      res=TRUE;
    }
    if(group!=-1 && param!=-1) {
      // blend one param in one group
      group=self->priv->param_groups[group];
      switch(group.type) {
        case 0: {
          BtWirePattern *wire_pattern = bt_wire_get_pattern(group.user_data,self->priv->pattern);
          if(!wire_pattern) {
            BtSong *song;
    
            g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
            wire_pattern=bt_wire_pattern_new(song,group.user_data,self->priv->pattern);
            g_object_unref(song);
          }
          if(wire_pattern) {
            bt_wire_pattern_blend_column(wire_pattern,beg,end,param);
            g_object_unref(wire_pattern);
          }
        } break;
        case 1:
          bt_pattern_blend_column(self->priv->pattern,beg,end,param);
          break;
        case 2: {
          BtMachine *machine;
          gulong global_params, voice_params, params;
          
          g_object_get(G_OBJECT(self->priv->pattern),"machine",&machine,NULL);
          g_object_get(G_OBJECT(machine),"global-params",&global_params,"voice-params",&voice_params,NULL);
          params=global_params+(GPOINTER_TO_UINT(group.user_data)*voice_params);
          bt_pattern_blend_column(self->priv->pattern,beg,end,params+param);
          g_object_unref(machine);
        } break;
      }
      gtk_widget_queue_draw(GTK_WIDGET(self->priv->pattern_table));
      res=TRUE;
    }
  }
}
#endif

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
  gulong modifier=(gulong)event->state&gtk_accelerator_get_default_mod_mask();
  //gulong modifier=(gulong)event->state&(GDK_SHIFT_MASK|GDK_CONTROL_MASK|GDK_MOD4_MASK);

  g_assert(user_data);
  if(!GTK_WIDGET_REALIZED(self->priv->pattern_table)) return(FALSE);

  GST_INFO("pattern_table key : state 0x%x, keyval 0x%x, hw-code 0x%x, name %s",
    event->state,event->keyval,event->hardware_keycode,gdk_keyval_name(event->keyval));
  if(event->keyval==GDK_Return) {  /* GDK_KP_Enter */
    //if(modifier==GDK_SHIFT_MASK) {
      BtMainWindow *main_window;
      BtMainPages *pages;
      //BtMainPageSequence *sequence_page;

      g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
      g_object_get(G_OBJECT(main_window),"pages",&pages,NULL);
      //g_object_get(G_OBJECT(pages),"sequence-page",&sequence_page,NULL);

      gtk_notebook_set_current_page(GTK_NOTEBOOK(pages),BT_MAIN_PAGES_SEQUENCE_PAGE);
      //bt_main_page_sequence_goto_???(sequence_page,pattern);

      //g_object_unref(sequence_page);
      g_object_unref(pages);
      g_object_unref(main_window);

      res=TRUE;
    //}
  }
  else if(event->keyval==GDK_Menu) {
    gtk_menu_popup(self->priv->context_menu,NULL,NULL,NULL,NULL,3,gtk_get_current_event_time());
    res=TRUE;
  }
  else if (event->keyval == GDK_Insert) {
    if((modifier&(GDK_CONTROL_MASK|GDK_SHIFT_MASK))==(GDK_CONTROL_MASK|GDK_SHIFT_MASK)) {
      // insert full row
      BtWirePattern *wire_pattern;
      guint i=0;

      g_object_get(G_OBJECT(self->priv->pattern_table), "cursor-row", &self->priv->cursor_row, NULL);
      GST_INFO("ctrl-shift-insert pressed, row %lu",self->priv->cursor_row);
      while(self->priv->param_groups[i].type==0) {
        if((wire_pattern = bt_wire_get_pattern(self->priv->param_groups[i].user_data,self->priv->pattern))) {
          bt_wire_pattern_insert_full_row(wire_pattern,self->priv->cursor_row);
          g_object_unref(wire_pattern);
        }
        i++;
      }
      bt_pattern_insert_full_row(self->priv->pattern,self->priv->cursor_row);
      gtk_widget_queue_draw(GTK_WIDGET(self->priv->pattern_table));
      res=TRUE;
    }
    else if(modifier&GDK_SHIFT_MASK) {
      // insert group
      g_object_get(G_OBJECT(self->priv->pattern_table), "cursor-row", &self->priv->cursor_row, "cursor-group", &self->priv->cursor_group, NULL);
      GST_INFO("shift-insert pressed, row %lu, group %lu",self->priv->cursor_row,self->priv->cursor_group);
      switch(self->priv->param_groups[self->priv->cursor_group].type) {
        case 0: {
          BtWirePattern *wire_pattern = bt_wire_get_pattern(self->priv->param_groups[self->priv->cursor_group].user_data,self->priv->pattern);
          if(wire_pattern) {
            bt_wire_pattern_insert_full_row(wire_pattern,self->priv->cursor_row);
            g_object_unref(wire_pattern);
          }
        } break;
        case 1: {
          guint i;
          
          for(i=0;i<self->priv->param_groups[self->priv->cursor_group].num_columns;i++) {
            bt_pattern_insert_row(self->priv->pattern,self->priv->cursor_row, i);
          }
        } break;
        case 2: {
          BtMachine *machine;
          gulong global_params, voice_params, params;
          guint i;
          
          g_object_get(G_OBJECT(self->priv->pattern),"machine",&machine,NULL);
          g_object_get(G_OBJECT(machine),"global-params",&global_params,"voice-params",&voice_params,NULL);
          params=global_params+(GPOINTER_TO_UINT(self->priv->param_groups[self->priv->cursor_group].user_data)*voice_params);
          for(i=0;i<self->priv->param_groups[self->priv->cursor_group].num_columns;i++) {
            bt_pattern_insert_row(self->priv->pattern,self->priv->cursor_row, params+i);
          }
          g_object_unref(machine);
        } break;
      }
      gtk_widget_queue_draw(GTK_WIDGET(self->priv->pattern_table));
      res=TRUE;
    }
    else {
      // insert column
      g_object_get(G_OBJECT(self->priv->pattern_table), "cursor-row", &self->priv->cursor_row, "cursor-group", &self->priv->cursor_group, "cursor-param", &self->priv->cursor_param, NULL);
      GST_INFO("insert pressed, row %lu, group %lu, param %lu",self->priv->cursor_row,self->priv->cursor_group, self->priv->cursor_param);
      switch(self->priv->param_groups[self->priv->cursor_group].type) {
        case 0: {
          BtWirePattern *wire_pattern = bt_wire_get_pattern(self->priv->param_groups[self->priv->cursor_group].user_data,self->priv->pattern);
          if(wire_pattern) {
            bt_wire_pattern_insert_row(wire_pattern,self->priv->cursor_row, self->priv->cursor_param);
            g_object_unref(wire_pattern);
          }
        } break;
        case 1:
          bt_pattern_insert_row(self->priv->pattern,self->priv->cursor_row, self->priv->cursor_param);
          break;
        case 2: {
          BtMachine *machine;
          gulong global_params, voice_params, params;
          
          g_object_get(G_OBJECT(self->priv->pattern),"machine",&machine,NULL);
          g_object_get(G_OBJECT(machine),"global-params",&global_params,"voice-params",&voice_params,NULL);
          params=global_params+(GPOINTER_TO_UINT(self->priv->param_groups[self->priv->cursor_group].user_data)*voice_params);
          bt_pattern_insert_row(self->priv->pattern,self->priv->cursor_row, params+self->priv->cursor_param);
          g_object_unref(machine);
        } break;
      }
      gtk_widget_queue_draw(GTK_WIDGET(self->priv->pattern_table));
      res=TRUE;
    }
  }
  else if(event->keyval == GDK_Delete) {
    if((modifier&(GDK_CONTROL_MASK|GDK_SHIFT_MASK))==(GDK_CONTROL_MASK|GDK_SHIFT_MASK)) {
      // delete full row
      BtWirePattern *wire_pattern;
      guint i=0;

      g_object_get(G_OBJECT(self->priv->pattern_table), "cursor-row", &self->priv->cursor_row, NULL);
      GST_INFO("ctrl-shift-delete pressed, row %lu",self->priv->cursor_row);
      while(self->priv->param_groups[i].type==0) {
        if((wire_pattern = bt_wire_get_pattern(self->priv->param_groups[i].user_data,self->priv->pattern))) {
          bt_wire_pattern_delete_full_row(wire_pattern,self->priv->cursor_row);
          g_object_unref(wire_pattern);
        }
        i++;
      }
      bt_pattern_delete_full_row(self->priv->pattern,self->priv->cursor_row);
      gtk_widget_queue_draw(GTK_WIDGET(self->priv->pattern_table));
      res=TRUE;
    }
    else if(modifier&GDK_SHIFT_MASK) {
      // delete group
      g_object_get(G_OBJECT(self->priv->pattern_table), "cursor-row", &self->priv->cursor_row, "cursor-group", &self->priv->cursor_group, NULL);
      GST_INFO("delete pressed, row %lu, group %lu",self->priv->cursor_row,self->priv->cursor_group);
      switch(self->priv->param_groups[self->priv->cursor_group].type) {
        case 0: {
          BtWirePattern *wire_pattern = bt_wire_get_pattern(self->priv->param_groups[self->priv->cursor_group].user_data,self->priv->pattern);
          if(wire_pattern) {
            bt_wire_pattern_delete_full_row(wire_pattern,self->priv->cursor_row);
            g_object_unref(wire_pattern);
          }
        } break;
        case 1: {
          guint i;
          
          for(i=0;i<self->priv->param_groups[self->priv->cursor_group].num_columns;i++) {
            bt_pattern_delete_row(self->priv->pattern,self->priv->cursor_row, i);
          }
        } break;
        case 2: {
          BtMachine *machine;
          gulong global_params, voice_params, params;
          guint i;
          
          g_object_get(G_OBJECT(self->priv->pattern),"machine",&machine,NULL);
          g_object_get(G_OBJECT(machine),"global-params",&global_params,"voice-params",&voice_params,NULL);
          params=global_params+(GPOINTER_TO_UINT(self->priv->param_groups[self->priv->cursor_group].user_data)*voice_params);
          for(i=0;i<self->priv->param_groups[self->priv->cursor_group].num_columns;i++) {
            bt_pattern_delete_row(self->priv->pattern,self->priv->cursor_row, params+i);
          }
          g_object_unref(machine);
        } break;
      }
      gtk_widget_queue_draw(GTK_WIDGET(self->priv->pattern_table));
      res=TRUE;
    }  
    else {
      // delete group
      g_object_get(G_OBJECT(self->priv->pattern_table), "cursor-row", &self->priv->cursor_row, "cursor-group", &self->priv->cursor_group, "cursor-param", &self->priv->cursor_param, NULL);
      GST_INFO("delete pressed, row %lu, group %lu, param %lu",self->priv->cursor_row,self->priv->cursor_group, self->priv->cursor_param);
      switch(self->priv->param_groups[self->priv->cursor_group].type) {
        case 0: {
          BtWirePattern *wire_pattern = bt_wire_get_pattern(self->priv->param_groups[self->priv->cursor_group].user_data,self->priv->pattern);
          if(wire_pattern) {
            bt_wire_pattern_delete_row(wire_pattern,self->priv->cursor_row, self->priv->cursor_param);
            g_object_unref(wire_pattern);
          }
        } break;
        case 1:
          bt_pattern_delete_row(self->priv->pattern,self->priv->cursor_row, self->priv->cursor_param);
          break;
        case 2: {
          BtMachine *machine;
          gulong global_params, voice_params, params;
          
          g_object_get(G_OBJECT(self->priv->pattern),"machine",&machine,NULL);
          g_object_get(G_OBJECT(machine),"global-params",&global_params,"voice-params",&voice_params,NULL);
          params=global_params+(GPOINTER_TO_UINT(self->priv->param_groups[self->priv->cursor_group].user_data)*voice_params);
          bt_pattern_delete_row(self->priv->pattern,self->priv->cursor_row, params+self->priv->cursor_param);
          g_object_unref(machine);
        } break;
      }
      gtk_widget_queue_draw(GTK_WIDGET(self->priv->pattern_table));
      res=TRUE;
    }
  }
  else if(event->keyval == GDK_i) {
    if(modifier&GDK_CONTROL_MASK) {
      gint beg,end,group,param;
      if(bt_pattern_editor_get_selection(self->priv->pattern_table,&beg,&end,&group,&param)) {
        GST_INFO("blending : %d %d , %d %d",beg,end,group,param);
        if(group==-1 && param==-1) {
          // blend full pattern
          BtWirePattern *wire_pattern;
          guint i=0;
    
          while(self->priv->param_groups[i].type==0) {
            wire_pattern = bt_wire_get_pattern(self->priv->param_groups[i].user_data,self->priv->pattern);
            if(!wire_pattern) {
              BtSong *song;
      
              g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
              wire_pattern=bt_wire_pattern_new(song,self->priv->param_groups[i].user_data,self->priv->pattern);
              g_object_unref(song);
            }
            if(wire_pattern) {
              bt_wire_pattern_blend_columns(wire_pattern,beg,end);
              g_object_unref(wire_pattern);
            }
            i++;
          }
          bt_pattern_blend_columns(self->priv->pattern,beg,end);
          gtk_widget_queue_draw(GTK_WIDGET(self->priv->pattern_table));
          res=TRUE;
        }
        if(group!=-1 && param==-1) {
          // blend whole group
          switch(self->priv->param_groups[group].type) {
            case 0: {
              BtWirePattern *wire_pattern = bt_wire_get_pattern(self->priv->param_groups[group].user_data,self->priv->pattern);
              if(!wire_pattern) {
                BtSong *song;
        
                g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
                wire_pattern=bt_wire_pattern_new(song,self->priv->param_groups[group].user_data,self->priv->pattern);
                g_object_unref(song);
              }
              if(wire_pattern) {
                bt_wire_pattern_blend_columns(wire_pattern,beg,end);
                g_object_unref(wire_pattern);
              }
            } break;
            case 1: {
              guint i;
              
              for(i=0;i<self->priv->param_groups[self->priv->cursor_group].num_columns;i++) {
                bt_pattern_blend_column(self->priv->pattern,beg,end,i);
              }
            } break;
            case 2: {
              BtMachine *machine;
              gulong global_params, voice_params, params;
              guint i;
              
              g_object_get(G_OBJECT(self->priv->pattern),"machine",&machine,NULL);
              g_object_get(G_OBJECT(machine),"global-params",&global_params,"voice-params",&voice_params,NULL);
              params=global_params+(GPOINTER_TO_UINT(self->priv->param_groups[self->priv->cursor_group].user_data)*voice_params);
              for(i=0;i<self->priv->param_groups[self->priv->cursor_group].num_columns;i++) {
                bt_pattern_blend_column(self->priv->pattern,beg,end,params+i);
              }
              g_object_unref(machine);
            } break;
          }
          gtk_widget_queue_draw(GTK_WIDGET(self->priv->pattern_table));
          res=TRUE;
        }
        if(group!=-1 && param!=-1) {
          // blend one param in one group
          switch(self->priv->param_groups[group].type) {
            case 0: {
              BtWirePattern *wire_pattern = bt_wire_get_pattern(self->priv->param_groups[group].user_data,self->priv->pattern);
              if(!wire_pattern) {
                BtSong *song;
        
                g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
                wire_pattern=bt_wire_pattern_new(song,self->priv->param_groups[group].user_data,self->priv->pattern);
                g_object_unref(song);
              }
              if(wire_pattern) {
                bt_wire_pattern_blend_column(wire_pattern,beg,end,param);
                g_object_unref(wire_pattern);
              }
            } break;
            case 1:
              bt_pattern_blend_column(self->priv->pattern,beg,end,param);
              break;
            case 2: {
              BtMachine *machine;
              gulong global_params, voice_params, params;
              
              g_object_get(G_OBJECT(self->priv->pattern),"machine",&machine,NULL);
              g_object_get(G_OBJECT(machine),"global-params",&global_params,"voice-params",&voice_params,NULL);
              params=global_params+(GPOINTER_TO_UINT(self->priv->param_groups[self->priv->cursor_group].user_data)*voice_params);
              bt_pattern_blend_column(self->priv->pattern,beg,end,params+param);
              g_object_unref(machine);
            } break;
          }
          gtk_widget_queue_draw(GTK_WIDGET(self->priv->pattern_table));
          res=TRUE;
        }
      }
    }
  }
  else if(event->keyval == GDK_r) {
    if(modifier&GDK_CONTROL_MASK) {
      gint beg,end,group,param;
      if(bt_pattern_editor_get_selection(self->priv->pattern_table,&beg,&end,&group,&param)) {
        GST_INFO("randomizing : %d %d , %d %d",beg,end,group,param);
        if(group==-1 && param==-1) {
          // randomize full pattern
          BtWirePattern *wire_pattern;
          guint i=0;
    
          while(self->priv->param_groups[i].type==0) {
            wire_pattern = bt_wire_get_pattern(self->priv->param_groups[i].user_data,self->priv->pattern);
            if(!wire_pattern) {
              BtSong *song;
      
              g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
              wire_pattern=bt_wire_pattern_new(song,self->priv->param_groups[i].user_data,self->priv->pattern);
              g_object_unref(song);
            }
            if(wire_pattern) {
              bt_wire_pattern_randomize_columns(wire_pattern,beg,end);
              g_object_unref(wire_pattern);
            }
            i++;
          }
          bt_pattern_randomize_columns(self->priv->pattern,beg,end);
          gtk_widget_queue_draw(GTK_WIDGET(self->priv->pattern_table));
          res=TRUE;
        }
        if(group!=-1 && param==-1) {
          // randomize whole group
          switch(self->priv->param_groups[group].type) {
            case 0: {
              BtWirePattern *wire_pattern = bt_wire_get_pattern(self->priv->param_groups[group].user_data,self->priv->pattern);
              if(!wire_pattern) {
                BtSong *song;
        
                g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
                wire_pattern=bt_wire_pattern_new(song,self->priv->param_groups[group].user_data,self->priv->pattern);
                g_object_unref(song);
              }
              if(wire_pattern) {
                bt_wire_pattern_randomize_columns(wire_pattern,beg,end);
                g_object_unref(wire_pattern);
              }
            } break;
            case 1: {
              guint i;
              
              for(i=0;i<self->priv->param_groups[self->priv->cursor_group].num_columns;i++) {
                bt_pattern_randomize_column(self->priv->pattern,beg,end,i);
              }
            } break;
            case 2: {
              BtMachine *machine;
              gulong global_params, voice_params, params;
              guint i;
              
              g_object_get(G_OBJECT(self->priv->pattern),"machine",&machine,NULL);
              g_object_get(G_OBJECT(machine),"global-params",&global_params,"voice-params",&voice_params,NULL);
              params=global_params+(GPOINTER_TO_UINT(self->priv->param_groups[self->priv->cursor_group].user_data)*voice_params);
              for(i=0;i<self->priv->param_groups[self->priv->cursor_group].num_columns;i++) {
                bt_pattern_randomize_column(self->priv->pattern,beg,end,params+i);
              }
              g_object_unref(machine);
            } break;
          }
          gtk_widget_queue_draw(GTK_WIDGET(self->priv->pattern_table));
          res=TRUE;
        }
        if(group!=-1 && param!=-1) {
          // randomize one param in one group
          switch(self->priv->param_groups[group].type) {
            case 0: {
              BtWirePattern *wire_pattern = bt_wire_get_pattern(self->priv->param_groups[group].user_data,self->priv->pattern);
              if(!wire_pattern) {
                BtSong *song;
        
                g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
                wire_pattern=bt_wire_pattern_new(song,self->priv->param_groups[group].user_data,self->priv->pattern);
                g_object_unref(song);
              }
              if(wire_pattern) {
                bt_wire_pattern_randomize_column(wire_pattern,beg,end,param);
                g_object_unref(wire_pattern);
              }
            } break;
            case 1:
              bt_pattern_randomize_column(self->priv->pattern,beg,end,param);
              break;
            case 2: {
              BtMachine *machine;
              gulong global_params, voice_params, params;
              
              g_object_get(G_OBJECT(self->priv->pattern),"machine",&machine,NULL);
              g_object_get(G_OBJECT(machine),"global-params",&global_params,"voice-params",&voice_params,NULL);
              params=global_params+(GPOINTER_TO_UINT(self->priv->param_groups[self->priv->cursor_group].user_data)*voice_params);
              bt_pattern_randomize_column(self->priv->pattern,beg,end,params+param);
              g_object_unref(machine);
            } break;
          }
          gtk_widget_queue_draw(GTK_WIDGET(self->priv->pattern_table));
          res=TRUE;
        }
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
  if(event->button==3) {
    gtk_menu_popup(self->priv->context_menu,NULL,NULL,NULL,NULL,3,gtk_get_current_event_time());
    res=TRUE;
  }
  return(res);
}

static void on_pattern_table_cursor_group_changed(const BtPatternEditor *editor,GParamSpec *arg,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  
  g_object_get(G_OBJECT(editor), "cursor-group", &self->priv->cursor_group, "cursor-param", &self->priv->cursor_param, NULL);
  pattern_view_update_column_description(self,UPDATE_COLUMN_UPDATE);
}

static void on_pattern_table_cursor_param_changed(const BtPatternEditor *editor,GParamSpec *arg,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  
  g_object_get(G_OBJECT(editor), "cursor-param", &self->priv->cursor_param, NULL);
  pattern_view_update_column_description(self,UPDATE_COLUMN_UPDATE);
}

static void on_pattern_table_cursor_row_changed(const BtPatternEditor *editor,GParamSpec *arg,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  
  g_object_get(G_OBJECT(editor), "cursor-row", &self->priv->cursor_row, NULL);
  pattern_view_update_column_description(self,UPDATE_COLUMN_UPDATE);
}

//-- event handler helper

static void machine_menu_add(const BtMainPagePatterns *self,BtMachine *machine,GtkListStore *store) {
  gchar *str;
  GtkTreeIter menu_iter;
  GdkPixbuf *pixbuf=bt_ui_resources_get_icon_pixbuf_by_machine(machine);

  g_object_get(G_OBJECT(machine),"id",&str,NULL);
  GST_INFO("  adding %p, \"%s\"  (machine-refs: %d)",machine,str,(G_OBJECT(machine))->ref_count);

  gtk_list_store_append(store,&menu_iter);
  gtk_list_store_set(store,&menu_iter,
    MACHINE_MENU_ICON,pixbuf,
    MACHINE_MENU_LABEL,str,
    MACHINE_MENU_MACHINE,machine,
    -1);
  g_signal_connect(G_OBJECT(machine),"notify::id",G_CALLBACK(on_machine_id_changed),(gpointer)self);

  GST_DEBUG("  (machine-refs: %d)",(G_OBJECT(machine))->ref_count);
  g_free(str);
  g_object_unref(pixbuf);
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

static void wavetable_menu_refresh(const BtMainPagePatterns *self,BtWavetable *wavetable) {
  BtWave *wave;
  gchar *str,hstr[3];
  GtkListStore *store;
  GtkTreeIter menu_iter;
  gint i,index=-1;

  // update pattern menu
  store=gtk_list_store_new(2,G_TYPE_STRING,G_TYPE_STRING);

  //-- append waves rows (buzz numbers them from 0x01 to 0xC8=200)
  for(i=0;i<200;i++) {
    if((wave=bt_wavetable_get_wave_by_index(wavetable,i))) {
      gtk_list_store_append(store, &menu_iter);
      g_object_get(G_OBJECT(wave),"name",&str,NULL);
      GST_INFO("  adding [%3d] \"%s\"",i,str);
      // buzz shows index as hex, because trackers needs it this way
      sprintf(hstr,"%02x",i);
      gtk_list_store_set(store,&menu_iter,0,hstr,1,str,-1);
      g_free(str);
      g_object_unref(wave);
      if(index==-1) index=i;
    }
  }

  gtk_widget_set_sensitive(GTK_WIDGET(self->priv->wavetable_menu),(index!=-1));
  gtk_combo_box_set_model(self->priv->wavetable_menu,GTK_TREE_MODEL(store));
  gtk_combo_box_set_active(self->priv->wavetable_menu,index);
  g_object_unref(store); // drop with comboxbox
}


typedef struct {
  gfloat (*str_to_float)(gchar *in, gpointer user_data);
  const gchar *(*float_to_str)(gfloat in, gpointer user_data);
} PatternColumnConvertersCallbacks;

typedef struct {
  PatternColumnConvertersCallbacks callbacks;
  float min,max;
} PatternColumnConvertersFloatCallbacks;

typedef struct {
  union {
    PatternColumnConvertersCallbacks any_cb;
    PatternColumnConvertersFloatCallbacks float_cb;
  };
} PatternColumnConverters;

static float pattern_edit_get_data_at(gpointer pattern_data, gpointer column_data, int row, int track, int param) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(pattern_data);
  gchar *str = NULL;
  PatternColumnGroup *group = &self->priv->param_groups[track];

  switch (group->type) {
    case 0: {
      BtWirePattern *wire_pattern = bt_wire_get_pattern(group->user_data,self->priv->pattern);
      if(wire_pattern) {
        str=bt_wire_pattern_get_event(wire_pattern,row,param);
        g_object_unref(wire_pattern);
      }
    } break;
    case 1:
      str=bt_pattern_get_global_event(self->priv->pattern,row,param);
      //if(param==0 && row<2) GST_WARNING("get global event: %s",str);
      break;
    case 2:
      str=bt_pattern_get_voice_event(self->priv->pattern,row,GPOINTER_TO_UINT(group->user_data),param);
      break;
    default:
      GST_WARNING("invalid column group type");
  }

  if(str) {
    if(column_data) {
      return ((PatternColumnConvertersCallbacks *)column_data)->str_to_float(str,column_data);
    }
    else
      return g_ascii_strtod(str,NULL);
  }
  return group->columns[param].def;
}

static void pattern_edit_set_data_at(gpointer pattern_data, gpointer column_data, int row, int track, int param, float value) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(pattern_data);
  const gchar *str = NULL;
  PatternColumnGroup *group = &self->priv->param_groups[track];
  
  if(column_data)
    str=((PatternColumnConvertersCallbacks *)column_data)->float_to_str(value,column_data);
  else
    if(value!=group->columns[param].def)
      str=bt_persistence_strfmt_double(value);

  switch (group->type) {
    case 0: {
      BtWirePattern *wire_pattern = bt_wire_get_pattern(group->user_data,self->priv->pattern);
      if(!wire_pattern) {
        BtSong *song;
        
        g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
        wire_pattern=bt_wire_pattern_new(song,group->user_data,self->priv->pattern);
        g_object_unref(song);
      }
      bt_wire_pattern_set_event(wire_pattern,row,param,str);
      g_object_unref(wire_pattern);
    } break;
    case 1:
      //if(param==0 && row<2) GST_WARNING("set global event: %s",str);
      bt_pattern_set_global_event(self->priv->pattern,row,param,str);
      break;
    case 2:
      bt_pattern_set_voice_event(self->priv->pattern,row,GPOINTER_TO_UINT(group->user_data),param,str);
      break;
    default:
      GST_WARNING("invalid column group type");
  }
}

static float note_str_to_float(gchar *note, gpointer user_data) {
  return (float)gstbt_tone_conversion_note_string_2_number(note);
}

static const gchar * note_float_to_str(gfloat in, gpointer user_data) {
  return(gstbt_tone_conversion_note_number_2_string((guint)in));
}

static float float_str_to_float(gchar *str, gpointer user_data) {
  // scale value into 0...65535 range
  PatternColumnConvertersFloatCallbacks *pcc=(PatternColumnConvertersFloatCallbacks *)user_data;
  gdouble val=g_ascii_strtod(str,NULL);
  gdouble factor=65535.0/(pcc->max-pcc->min);
  
  //GST_DEBUG("> val %lf(%s), factor %lf, result %lf",val,str,factor,(val-pcc->min)*factor); 
  
  return (val-pcc->min)*factor;
}

static const gchar * float_float_to_str(gfloat in, gpointer user_data) {
  // scale value from 0...65535 range
  PatternColumnConvertersFloatCallbacks *pcc=(PatternColumnConvertersFloatCallbacks *)user_data;
  gdouble factor=65535.0/(pcc->max-pcc->min);
  gdouble val=pcc->min+(in/factor);

  //GST_DEBUG("< val %lf, factor %lf, result %lf(%s)",in,factor,val,bt_persistence_strfmt_double(val)); 
  
  return bt_persistence_strfmt_double(val);
}

static void pattern_edit_fill_column_type(PatternColumn *col,GParamSpec *property, GValue *min_val, GValue *max_val) { 
  GType type=bt_g_type_get_base_type(property->value_type);

  GST_DEBUG("trying param type: %d,'%s'/'%s' for parameter '%s'",
    type, g_type_name(type),g_type_name(property->value_type),property->name);

  switch(type) {
    case G_TYPE_STRING: {
      PatternColumnConvertersCallbacks *pcc;

      col->type=PCT_NOTE;
      col->min=0;
      col->max=((16*9)+12);
      col->def=0;
      col->user_data=g_new(PatternColumnConverters,1);
      pcc=(PatternColumnConvertersCallbacks *)col->user_data;
      pcc->str_to_float=note_str_to_float;
      pcc->float_to_str=note_float_to_str;
    } break;
    case G_TYPE_BOOLEAN:
      col->type=PCT_SWITCH;
      col->min=0;
      col->max=1;
      col->def=col->max+1;
      col->user_data=NULL;
      break;
    case G_TYPE_ENUM:
      if(property->value_type==GSTBT_TYPE_TRIGGER_SWITCH) {
        col->type=PCT_SWITCH;
        col->min=0;
        col->max=1;
      }
      else {
        col->type=PCT_BYTE;
        col->min=g_value_get_enum(min_val);
        col->max=g_value_get_enum(max_val);
      }
      col->def=col->max+1;
      col->user_data=NULL;
      break;
    case G_TYPE_INT:
      col->type=PCT_WORD;
      col->min=g_value_get_int(min_val);
      col->max=g_value_get_int(max_val);
      col->def=col->max+1;
      if(col->min>=0 && col->max<256) {
        col->type=PCT_BYTE;
      }
      col->user_data=NULL;
      break;
    case G_TYPE_UINT:
      col->type=PCT_WORD;
      col->min=g_value_get_uint(min_val);
      col->max=g_value_get_uint(max_val);
      col->def=col->max+1;
      if(col->min>=0 && col->max<256) {
        col->type=PCT_BYTE;
      }
      col->user_data=NULL;
      break;
    case G_TYPE_FLOAT: {
      PatternColumnConvertersFloatCallbacks *pcc;

      col->type=PCT_WORD;
      col->min=0.0;
      col->max=65535.0;
      col->def=col->max+1;
      col->user_data=g_new(PatternColumnConverters,1);
      pcc=(PatternColumnConvertersFloatCallbacks *)col->user_data;
      ((PatternColumnConvertersCallbacks*)pcc)->str_to_float=float_str_to_float;
      ((PatternColumnConvertersCallbacks*)pcc)->float_to_str=float_float_to_str;
      pcc->min=g_value_get_float(min_val);
      pcc->max=g_value_get_float(max_val);
      /* @todo: need scaling
       * in case of
       *   wire.volume: 0.0->0x0000, 1.0->0x4000, 2.0->0x8000, 4.0->0xFFFF+1
       *     col->user_data=&pcc[1]; // const scale
       *     col->scaling factor =  4096.0
       *   song.master_volume: 0db->0.0->0x0000, -80db->1/100.000.000->0x4000
       *     scaling_factor is not enough
       *     col->user_data=&pcc[2]; // log-map
       *
       * - we might need to put the scaling factor into the user_data
       * - how can we detect master-volume (log mapping)
       */
    } break;
    case G_TYPE_DOUBLE: {
      PatternColumnConvertersFloatCallbacks *pcc;

      col->type=PCT_WORD;
      col->min=0.0;
      col->max=65535.0;
      col->def=col->max+1;
      col->user_data=g_new(PatternColumnConverters,1);
      pcc=(PatternColumnConvertersFloatCallbacks *)col->user_data;
      ((PatternColumnConvertersCallbacks*)pcc)->str_to_float=float_str_to_float;
      ((PatternColumnConvertersCallbacks*)pcc)->float_to_str=float_float_to_str;
      pcc->min=g_value_get_double(min_val);
      pcc->max=g_value_get_double(max_val);
    } break;
    default:
      GST_WARNING("unhandled param type: '%s' for parameter '%s'",g_type_name(type),property->name);
      col->type=0;
      col->min=col->max=col->def=0;
      col->user_data=NULL;
  }
  GST_INFO("%s parameter '%s' min/max/def : %6.4lf/%6.4lf/%6.4lf",g_type_name(type), property->name, col->min,col->max,col->def);
  g_free(min_val);
  g_free(max_val);
}

static void pattern_table_clear(const BtMainPagePatterns *self) {
  gulong i,j;

  for(i=0;i<self->priv->number_of_groups;i++) {
    for(j=0;j<self->priv->param_groups[i].num_columns;j++) {
      // voice parameter groups are copies
      if(!((self->priv->param_groups[i].type==2) && (self->priv->param_groups[i].user_data!=NULL)))
        g_free(self->priv->param_groups[i].columns[j].user_data);
    }
    g_free(self->priv->param_groups[i].name);
    g_free(self->priv->param_groups[i].columns);
  }
  g_free(self->priv->param_groups);
  self->priv->param_groups=NULL;
  //self->priv->number_of_groups=0;
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
    PatternColumnGroup *group;
    GValue *min_val,*max_val;
    GParamSpec *property;

    g_object_get(G_OBJECT(pattern),"length",&number_of_ticks,"voices",&voices,"machine",&machine,NULL);
    g_object_get(G_OBJECT(machine),"global-params",&global_params,"voice-params",&voice_params,NULL);
    GST_DEBUG("  size is %2d,%2d",number_of_ticks,global_params);

    pattern_table_clear(self);
    
    // wire pattern data
    self->priv->number_of_groups=(global_params>0?1:0)+voices;

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
      
      GST_INFO("wire parameters");
      for(node=wires;node;node=g_list_next(node)) {
        BtMachine *src=NULL;

        wire=BT_WIRE(node->data);
        // check wire config
        g_object_get(G_OBJECT(wire),"num-params",&wire_params,"src",&src,NULL);
        group->type=0;
        g_object_get(G_OBJECT(src),"id",&group->name,NULL), 
        group->user_data=wire;
        group->num_columns=wire_params;
        group->columns=g_new(PatternColumn,wire_params);
        for(i=0;i<wire_params;i++) {
          bt_wire_get_param_details(wire,i,&property,&min_val,&max_val);
          pattern_edit_fill_column_type(&group->columns[i],property,min_val,max_val);
        }
        g_object_unref(src);
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
      group->name=g_strdup("Globals");
      group->user_data=NULL;
      group->num_columns=global_params;
      group->columns=g_new(PatternColumn,global_params);
      GST_INFO("global parameters");
      for(i=0;i<global_params;i++) {
        bt_machine_get_global_param_details(machine,i,&property,&min_val,&max_val);
        pattern_edit_fill_column_type(&group->columns[i],property,min_val,max_val);
      }
      group++;
    }
    if(voices) {
      PatternColumnGroup *stamp=group;
      // create mapping for voice params
      group->type=2;
      group->name=g_strdup("Voice 1");
      group->user_data=GUINT_TO_POINTER(0);
      group->num_columns=voice_params;
      group->columns=g_new(PatternColumn,voice_params);
      GST_INFO("voice parameters");
      for(i=0;i<voice_params;i++) {
        bt_machine_get_voice_param_details(machine,i,&property,&min_val,&max_val);
        pattern_edit_fill_column_type(&group->columns[i],property,min_val,max_val);
      }
      group++;
      for(i=1;i<voices;i++) {
        group->type=2;
        group->name=g_strdup_printf("Voice %u",(guint)(i+1));
        group->user_data=GUINT_TO_POINTER(i);
        group->num_columns=voice_params;
        group->columns=g_memdup(stamp->columns,sizeof(PatternColumn)*voice_params);
        group++;
      }
    }

    bt_pattern_editor_set_pattern(self->priv->pattern_table, (gpointer)self,
      number_of_ticks, self->priv->number_of_groups, self->priv->param_groups, &callbacks);

    g_object_unref(machine);
  }
  else {
    bt_pattern_editor_set_pattern(self->priv->pattern_table, (gpointer)self, 0, 0, NULL, &callbacks);
  }
}

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

static BtPattern * add_new_pattern(const BtMainPagePatterns *self,BtMachine *machine) {
  BtSong *song;
  BtSongInfo *song_info;
  BtPattern *pattern;
  gchar *mid,*id,*name;
  gulong bars;

  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  g_object_get(G_OBJECT(song),"song-info",&song_info,NULL);
  g_object_get(G_OBJECT(song_info),"bars",&bars,NULL);
  g_object_get(G_OBJECT(machine),"id",&mid,NULL);

  name=bt_machine_get_unique_pattern_name(machine);
  id=g_strdup_printf("%s %s",mid,name);
  // new_pattern
  pattern=bt_pattern_new(song, id, name, bars, machine);
  
  // free ressources
  g_free(mid);
  g_free(id);
  g_free(name);
  g_object_unref(song_info);
  g_object_unref(song);

  return(pattern);
}

//-- event handler

static gboolean on_page_switched_idle(gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);

  gtk_widget_grab_focus_savely(GTK_WIDGET(self->priv->pattern_table));
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
#if !GTK_CHECK_VERSION(2,12,0)
        // workaround for http://bugzilla.gnome.org/show_bug.cgi?id=469374
        g_signal_emit_by_name (main_window, "keys-changed", 0);
#endif
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
    self->priv->pattern_length_changed=self->priv->pattern_voices_changed=0;
  }

  // refresh pattern view
  self->priv->pattern=bt_main_page_patterns_get_current_pattern(self);
  GST_INFO("ref'ed new pattern: %p,refs=%d",
    self->priv->pattern,(self->priv->pattern?(G_OBJECT(self->priv->pattern))->ref_count:-1));

  GST_INFO("new pattern selected : %p",self->priv->pattern);
  pattern_table_refresh(self,self->priv->pattern);
  pattern_view_update_column_description(self,UPDATE_COLUMN_UPDATE);
  gtk_widget_grab_focus_savely(GTK_WIDGET(self->priv->pattern_table));
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
  g_object_set(self->priv->pattern_table,"octave",self->priv->base_octave,NULL);
  gtk_widget_grab_focus_savely(GTK_WIDGET(self->priv->pattern_table));
}

static void on_machine_added(BtSetup *setup,BtMachine *machine,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  GtkTreeModel *store;
  gint index;

  g_assert(user_data);

  GST_INFO("new machine %p,ref_count=%d has been added",machine,G_OBJECT(machine)->ref_count);
  
  if(BT_IS_SOURCE_MACHINE(machine)) {
    BtPattern *pattern=add_new_pattern(self, machine);
    g_object_unref(pattern);
  }
  
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

static void on_wave_added_or_removed(BtWavetable *wavetable,BtWave *wave,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);

  wavetable_menu_refresh(self,wavetable);
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
    g_object_unref(machine);
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
              g_object_set(self->priv->pattern_table,"play-position",play_pos,NULL);
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
    g_object_set(self->priv->pattern_table,"play-position",-1.0,NULL);
  }
  // release the references
  g_object_unref(sequence);
  g_object_unref(cur_machine);
}

static void on_song_changed(const BtEditApplication *app,GParamSpec *arg,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  BtSong *song;
  BtSetup *setup;
  BtWavetable *wavetable;

  g_assert(user_data);

  GST_INFO("song has changed : app=%p, self=%p",app,self);
  // get song from app and then setup from song
  g_object_get(G_OBJECT(self->priv->app),"song",&song,NULL);
  if(!song) return;
  GST_INFO("song->ref_ct=%d",G_OBJECT(song)->ref_count);

  g_object_get(G_OBJECT(song),"setup",&setup,"wavetable",&wavetable,NULL);
  // update page
  machine_menu_refresh(self,setup);
  //pattern_menu_refresh(self); // should be triggered by machine_menu_refresh()
  wavetable_menu_refresh(self,wavetable);
  g_signal_connect(G_OBJECT(setup),"machine-added",G_CALLBACK(on_machine_added),(gpointer)self);
  g_signal_connect(G_OBJECT(setup),"machine-removed",G_CALLBACK(on_machine_removed),(gpointer)self);
  g_signal_connect(G_OBJECT(setup),"wire-added",G_CALLBACK(on_wire_added_or_removed),(gpointer)self);
  g_signal_connect(G_OBJECT(setup),"wire-removed",G_CALLBACK(on_wire_added_or_removed),(gpointer)self);
  g_signal_connect(G_OBJECT(wavetable),"wave-added",G_CALLBACK(on_wave_added_or_removed),(gpointer)self);
  g_signal_connect(G_OBJECT(wavetable),"wave-removed",G_CALLBACK(on_wave_added_or_removed),(gpointer)self);
  // subscribe to play-pos changes of song->sequence
  g_signal_connect(G_OBJECT(song), "notify::play-pos", G_CALLBACK(on_sequence_tick), (gpointer)self);
  // release the references
  g_object_unref(wavetable);
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
  BtMachine *machine;
  BtPattern *pattern;
  GtkWidget *dialog;

  g_assert(user_data);

  machine=bt_main_page_patterns_get_current_machine(self);
  g_return_if_fail(machine);

  g_object_get(G_OBJECT(self->priv->app),"main-window",&main_window,NULL);
  // new_pattern
  pattern=add_new_pattern(self, machine);

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
  g_object_unref(pattern);
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

  // @todo: don't ask if pattern is empty
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

  GST_DEBUG("!!!! self=%p",self);

  gtk_widget_set_name(GTK_WIDGET(self),_("pattern view"));

  // add toolbar
  toolbar=gtk_toolbar_new();
  gtk_widget_set_name(toolbar,_("pattern view toolbar"));
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
  g_object_set(G_OBJECT(renderer), "width", 22, NULL);
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(self->priv->wavetable_menu),renderer,FALSE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(self->priv->wavetable_menu),renderer,"text", 0,NULL);
  renderer=gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(self->priv->wavetable_menu),renderer,TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(self->priv->wavetable_menu),renderer,"text", 1,NULL);
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
  self->priv->cursor_bg=bt_ui_resources_get_gdk_color(BT_UI_RES_COLOR_CURSOR);
  self->priv->selection_bg1=bt_ui_resources_get_gdk_color(BT_UI_RES_COLOR_SELECTION1);
  self->priv->selection_bg2=bt_ui_resources_get_gdk_color(BT_UI_RES_COLOR_SELECTION2);

  /* @idea what about adding one control for global params and one for each voice,
   * - then these controls can be folded (hidden)
   */
  // add pattern list-view
  scrolled_window=gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_NONE);

  self->priv->pattern_table=BT_PATTERN_EDITOR(bt_pattern_editor_new());
  g_object_set(self->priv->pattern_table,"octave",self->priv->base_octave,"play-position",-1.0,NULL);
  //gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window),GTK_WIDGET(self->priv->pattern_table));
  g_signal_connect(G_OBJECT(self->priv->pattern_table), "key-release-event", G_CALLBACK(on_pattern_table_key_release_event), (gpointer)self);
  g_signal_connect(G_OBJECT(self->priv->pattern_table), "button-press-event", G_CALLBACK(on_pattern_table_button_press_event), (gpointer)self);
  g_signal_connect(G_OBJECT(self->priv->pattern_table), "notify::cursor-group", G_CALLBACK(on_pattern_table_cursor_group_changed), (gpointer)self);
  g_signal_connect(G_OBJECT(self->priv->pattern_table), "notify::cursor-param", G_CALLBACK(on_pattern_table_cursor_param_changed), (gpointer)self);
  g_signal_connect(G_OBJECT(self->priv->pattern_table), "notify::cursor-row", G_CALLBACK(on_pattern_table_cursor_row_changed), (gpointer)self);
  gtk_container_add(GTK_CONTAINER(self), GTK_WIDGET(scrolled_window));

  gtk_container_add(GTK_CONTAINER(scrolled_window),GTK_WIDGET(self->priv->pattern_table));
  gtk_widget_set_name(GTK_WIDGET(self->priv->pattern_table),_("pattern editor"));

  GST_DEBUG("  before context menu",self);
  // generate the context menu
  self->priv->accel_group=gtk_accel_group_new();
  self->priv->context_menu=GTK_MENU(g_object_ref_sink(G_OBJECT(gtk_menu_new())));
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

  menu_item=gtk_image_menu_item_new_with_label(_("New pattern ..."));
  image=gtk_image_new_from_stock(GTK_STOCK_NEW,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_menu_item_set_accel_path (GTK_MENU_ITEM (menu_item), "<Buzztard-Main>/PatternView/PatternContext/NewPattern");
  gtk_accel_map_add_entry ("<Buzztard-Main>/PatternView/PatternContext/NewPattern", GDK_Return, GDK_CONTROL_MASK);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_context_menu_pattern_new_activate),(gpointer)self);

  self->priv->context_menu_pattern_properties=menu_item=gtk_image_menu_item_new_with_label(_("Pattern properties..."));
  image=gtk_image_new_from_stock(GTK_STOCK_PROPERTIES,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_menu_item_set_accel_path (GTK_MENU_ITEM (menu_item), "<Buzztard-Main>/PatternView/PatternContext/PatternProperties");
  gtk_accel_map_add_entry ("<Buzztard-Main>/PatternView/PatternContext/PatternProperties", GDK_BackSpace, GDK_CONTROL_MASK);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_context_menu_pattern_properties_activate),(gpointer)self);

  self->priv->context_menu_pattern_remove=menu_item=gtk_image_menu_item_new_with_label(_("Remove pattern..."));
  image=gtk_image_new_from_stock(GTK_STOCK_DELETE,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_menu_item_set_accel_path (GTK_MENU_ITEM (menu_item), "<Buzztard-Main>/PatternView/PatternContext/RemovePattern");
  gtk_accel_map_add_entry ("<Buzztard-Main>/PatternView/PatternContext/RemovePattern", GDK_Delete, GDK_CONTROL_MASK);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(G_OBJECT(menu_item),"activate",G_CALLBACK(on_context_menu_pattern_remove_activate),(gpointer)self);

  self->priv->context_menu_pattern_copy=menu_item=gtk_image_menu_item_new_with_label(_("Copy pattern..."));
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
  if(self) gtk_object_destroy(GTK_OBJECT(self));
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
  gtk_widget_grab_focus_savely(GTK_WIDGET(self->priv->pattern_table));
  // release the references
  g_object_unref(machine);
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
  gtk_widget_grab_focus_savely(GTK_WIDGET(self->priv->pattern_table));
}

//-- cut/copy/paste

/**
 * bt_main_page_pattern_delete_selection:
 * @self: the pattern subpage
 *
 * Delete (clear) the selected area.
 */
void bt_main_page_pattern_delete_selection(const BtMainPagePatterns *self) {
  /* @todo implement me */
#if 0
  /* this can be used for blend/randomize,
   * - would be good to generalize to use it also for insert/delete.
   *   -> need to ignore row_end
   * - would be good to use this for copy/cut/delete(clear)
   *   -> copy/cut want to return a new pattern
   * ideally we'd like to avoid separate column/columns functions
   * - _column(BtPattern * self, gulong start_tick, gulong end_tick, gulong param)
   * - _columns(BtPattern * self, gulong start_tick, gulong end_tick)
   * - the groups would trigger the update several times as we need to use the column one a few times
   * better would be:
   * - _columns(BtPattern * self, gulong start_tick, gulong end_tick, gulong start_param, gulong end_param)
   */
  //pattern_selection_apply(self,
  //  bt_pattern_delete_columns,bt_pattern_delete_column,
  //  bt_wire_pattern_delete_columns,bt_wire_pattern_delete_column);

  gint beg,end,group,param;
  if(bt_pattern_editor_get_selection(self->priv->pattern_table,&beg,&end,&group,&param)) {
    GST_INFO("blending : %d %d , %d %d",beg,end,group,param);
    if(group==-1 && param==-1) {
      // full pattern
      bt_pattern_delete_columns(self->priv->pattern,beg,end);
      gtk_widget_queue_draw(GTK_WIDGET(self->priv->pattern_table));
    }
    if(group!=-1 && param==-1) {
      // whole group
    }
    if(group!=-1 && param!=-1) {
      // one param in one group
      bt_pattern_delete_column(self->priv->pattern,start_tick,end_tick,param);
    }
  }
#endif
}

/**
 * bt_main_page_pattern_cut_selection:
 * @self: the pattern subpage
 *
 * Cut selected area.
 * <note>not yet working</note>
 */
void bt_main_page_pattern_cut_selection(const BtMainPagePatterns *self) {
  /* @todo implement me */
#if 0
- like copy, but clear pattern cells afterwards
#endif
  bt_main_page_pattern_delete_selection(self);
}

/**
 * bt_main_page_pattern_copy_selection:
 * @self: the sequence subpage
 *
 * Copy selected area.
 * <note>not yet working</note>
 */
void bt_main_page_pattern_copy_selection(const BtMainPagePatterns *self) {
#if 0
  /* @todo implement me
  - store pattern + related wire patterns
    - whats the data
      - groups like in PatternColumnGroup
        - type (wire_params, global_params, voice_params)
        - start_param, end_param
        - data (wire_params => wirepattern, voice_params => voice)
      - gint ticks, types
      - gtype-array[types]
      - gvalue-array[types x ticks]
    - problems
      - can we still paste if a wires were added/removed?
        - num_wire_params has changes
        - wire-patterns can have different number of params
      - can we still paste if voices are added/removed?
        - num_pattern_params has changed
        - skip the last ?
    - ideas:
      - just store the above
      - serialize and deserialize on paste ?
        - wire-patterns are serialized with the wires :/
      - make a BtPatternFragment and BtWirePatternFragment
        - pattern_copy = bt_pattern_copy_selection(self->priv->pattern))
          for(,,) bt_wire_pattern_copy_selection(wire_pattern,pattern_copy)
            - where to store those :(
  - remember selection (parameter start/end and time start/end)
  */
#endif
}

/**
 * bt_main_page_pattern_paste_selection:
 * @self: the pattern subpage
 *
 * Paste at the top of the selected area.
 * <note>not yet working</note>
 */
void bt_main_page_pattern_paste_selection(const BtMainPagePatterns *self) {
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
  GST_DEBUG("unref pattern: %p,refs=%d",
    self->priv->pattern,(self->priv->pattern?(G_OBJECT(self->priv->pattern))->ref_count:-1));
  g_object_try_unref(self->priv->pattern);

  gtk_widget_destroy(GTK_WIDGET(self->priv->context_menu));
  g_object_unref(G_OBJECT(self->priv->context_menu));

  g_object_try_unref(self->priv->accel_group);

  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_main_page_patterns_finalize(GObject *object) {
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS(object);

  g_free(self->priv->column_keymode);
  
  pattern_table_clear(self);

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
