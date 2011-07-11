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
 * @see_also: #BtPattern, #BtPatternEditor
 *
 * Provides an editor for #BtPattern instances.
 */

/* @todo: main-page-patterns tasks
 * - shortcuts
 *   - on_pattern_table_key_press_event()
 *   - Ctrl-S : Smooth
 *     - low pass median filter over values
 *     - need to figure what to do with empty slots
 *   - Ctrl-Shift-R: Randomize-Range
 *     - randomize between first and last value
 *   - Ctrl-T : Invert (I is Interpolate already)
 *     - value = max - (value-min))
 *   - Ctrl-Shift-T : Invert-Range
 *     - like invert, but take min, max from the values in the selection
 *   - prev/next for combobox entries
 *     - trigger "move-active" action signal with GTK_SCROLL_STEP_UP/GTK_SCROLL_STEP_DOWN
 *     - what mechanism to use:
 *       - gtk_binding_entry_add_signal (do bindings work when not focused?)
 *       - gtk_widget_add_accelerator (can't specify signal params)
 *
 * - also do controller-assignments like in machine-property window
 *
 * - use gray text color for disconnected machines in the machine
 *   combobox (or machine without patterns) and unused waves
 *   (like for unused patterns)
 *   - sensitve property of renderer does not work in comboboxes
 *     (unlike in treeviews)
 *     -> we could use italic text in all of them
 *       ("style", PANGO_STYLE_ITALIC and "style-set")
 *
 * - play live note entry
 *   - support midi keyboard for entering notes and triggers
 *   - have poly-input mode
 *     - if there is a keydown, enter the note
 *     - if there is another keydown before a keyup, go to next track and
 *       enter the note there.
 *     - on keyup, return 'cursor' to that column
 *   - what keys to use for trigger columns?
 *   - should we use shift+note for secondary note in same track (e.g. slide)
 *
 * - add the same context menu entries as the machines have in machine view for
 *   current machine
 *   - e.g. allow to open machine settings/preferences
 *     see machine-canvas-item::show_machine_properties_dialog()
 *     bt_main_page_machines_show_properties_dialog(page,machine);
 *     bt_main_page_machines_show_preferences_dialog(page,machine);
 *     .. rename/about/help
 */
/* @todo: undo/redo - machine voices
 * - handle undo/redo for add/rem track
 * - handle undo/redo for randomize/blend/flip
 */

#define BT_EDIT
#define BT_MAIN_PAGE_PATTERNS_C

#include "bt-edit.h"

#define MAX_WAVETABLE_ITEMS 200

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

  /* play live flag */
  gboolean play_live;

  /* the machine that is currently active */
  BtMachine *machine;

  /* the pattern that is currently shown */
  BtPattern *pattern;
  gulong number_of_groups;
  BtPatternEditorColumnGroup *param_groups;
  guint *column_keymode;

  /* editor change log */
  BtChangeLog *change_log;

  /* signal handler ids */
  gint pattern_length_changed,pattern_voices_changed;
  gint pattern_menu_changed;

  gint wave_to_combopos[MAX_WAVETABLE_ITEMS + 2], combopos_to_wave[MAX_WAVETABLE_ITEMS + 2];
};

//-- the class

static void bt_main_page_patterns_change_logger_interface_init(gpointer const g_iface, gconstpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtMainPagePatterns, bt_main_page_patterns, GTK_TYPE_VBOX,
  G_IMPLEMENT_INTERFACE (BT_TYPE_CHANGE_LOGGER,
    bt_main_page_patterns_change_logger_interface_init));


/* we need this in machine-properties-dialog.c too */
GdkAtom pattern_atom;

enum {
  WAVE_MENU_NUMBER=0,
  WAVE_MENU_LABEL,
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

enum {
  METHOD_SET_GLOBAL_EVENTS=0,
  METHOD_SET_VOICE_EVENTS,
  METHOD_SET_WIRE_EVENTS,
  METHOD_SET_PROPERTY,
  METHOD_ADD_PATTERN,
  METHOD_REM_PATTERN
};

static BtChangeLoggerMethods change_logger_methods[] = {
  BT_CHANGE_LOGGER_METHOD("set_global_events",18,"\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9 ]+)\",([0-9]+),([0-9]+),([0-9]+),(.*)$"),
  BT_CHANGE_LOGGER_METHOD("set_voice_events",17,"\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9 ]+)\",([0-9]+),([0-9]+),([0-9]+),([0-9]+),(.*)$"),
  BT_CHANGE_LOGGER_METHOD("set_wire_events",16,"\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9 ]+)\",([0-9]+),([0-9]+),([0-9]+),(.*)$"),
  BT_CHANGE_LOGGER_METHOD("set_pattern_property",21,"\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9 ]+)\"$"),
  BT_CHANGE_LOGGER_METHOD("add_pattern",12,"\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9 ]+)\",([0-9]+)$"),
  BT_CHANGE_LOGGER_METHOD("rem_pattern",12,"\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9 ]+)\"$"),
  { NULL, }
};


static void on_context_menu_pattern_new_activate(GtkMenuItem *menuitem,gpointer user_data);
static void on_context_menu_pattern_remove_activate(GtkMenuItem *menuitem,gpointer user_data);

static void on_pattern_size_changed(BtPattern *pattern,GParamSpec *arg,gpointer user_data);

static void change_current_pattern(const BtMainPagePatterns *self, BtPattern *new_pattern);

//-- tree model helper

static void machine_menu_model_get_iter_by_machine(GtkTreeModel *store,GtkTreeIter *iter,BtMachine *that_machine) {
  BtMachine *this_machine;

  GST_INFO("look up iter for machine : %p,ref_count=%d",that_machine,G_OBJECT_REF_COUNT(that_machine));

  if(gtk_tree_model_get_iter_first(store,iter)) {
    do {
      this_machine=bt_machine_list_model_get_object((BtMachineListModel *)store,iter);
      if(this_machine==that_machine) {
        GST_INFO("found iter for machine : %p,ref_count=%d",that_machine,G_OBJECT_REF_COUNT(that_machine));
        break;
      }
    } while(gtk_tree_model_iter_next(store,iter));
  }
}

static void pattern_menu_model_get_iter_by_pattern(GtkTreeModel *store,GtkTreeIter *iter,BtPattern *that_pattern) {
  BtPattern *this_pattern;

  if(gtk_tree_model_get_iter_first(store,iter)) {
    do {
      this_pattern=bt_pattern_list_model_get_object((BtPatternListModel *)store,iter);
      if(this_pattern==that_pattern) {
        break;
      }
    } while(gtk_tree_model_iter_next(store,iter));
  }
}

//-- status bar helpers

static void pattern_view_update_column_description(const BtMainPagePatterns *self, BtPatternViewUpdateColumn mode) {
  BtMainWindow *main_window;

  g_object_get(self->priv->app,"main-window",&main_window,NULL);
  // called too early
  if(!main_window) return;

  // our page is not the current
  if(mode&UPDATE_COLUMN_UPDATE) {
    gint page_num;

    bt_child_proxy_get(main_window,"pages::page",&page_num,NULL);
    if(page_num!=BT_MAIN_PAGES_PATTERNS_PAGE) {
      g_object_unref(main_window);
      return;
    }
  }

  // pop previous text by passing str=NULL;
  if(mode&UPDATE_COLUMN_POP)
    bt_child_proxy_set(main_window,"statusbar::status",NULL,NULL);

  if(mode&UPDATE_COLUMN_PUSH) {
    if(self->priv->pattern && self->priv->number_of_groups) {
      GParamSpec *property=NULL;
      BtPatternEditorColumnGroup *group;
      gchar *str=NULL,*desc=NULL;
      const gchar *blurb="";

      g_object_get(self->priv->pattern_table, "cursor-row", &self->priv->cursor_row, "cursor-param", &self->priv->cursor_param, "cursor-group", &self->priv->cursor_group, NULL);

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
            desc=bt_machine_describe_global_param_value(machine,self->priv->cursor_param,gval);
          }
          g_object_unref(machine);
        } break;
        case 2: {
          BtMachine *machine;
          GValue *gval;

          g_object_get(self->priv->pattern,"machine",&machine,NULL);
          property=bt_machine_get_voice_param_spec(machine,self->priv->cursor_param);
          if((gval=bt_pattern_get_voice_event_data(self->priv->pattern,self->priv->cursor_row,GPOINTER_TO_UINT(group->user_data),self->priv->cursor_param)) && G_IS_VALUE(gval)) {
            desc=bt_machine_describe_voice_param_value(machine,self->priv->cursor_param,gval);
          }
          g_object_unref(machine);
        } break;
        default:
          GST_WARNING("invalid column group type");
      }
      // get parameter description
      if(property) {
        blurb=g_param_spec_get_blurb(property);
        if(desc) {
          str=g_strdup_printf("%s%s%s",blurb,(*desc?": ":""),desc);
          g_free(desc);
        }
        else {
          str=g_strdup(blurb);
        }
      }
      bt_child_proxy_set(main_window,"statusbar::status",(str?str:BT_MAIN_STATUSBAR_DEFAULT),NULL);
      g_free(str);
    }
    else {
      GST_DEBUG("no or empty pattern");
      bt_child_proxy_set(main_window,"statusbar::status",_("Add new patterns from right click context menu."),NULL);
    }
  }

  g_object_unref(main_window);
}

//-- undo/redo helpers

/* pattern_range_copy:
 *
 * Serialize the content of the range into @data.
 */
static void pattern_range_copy(const BtMainPagePatterns *self,gint beg,gint end,gint group,gint param,GString *data) {
  BtPatternEditorColumnGroup *pc_group;
  guint i;

  GST_INFO("copying : %d %d , %d %d",beg,end,group,param);

  // process full pattern
  if(group==-1 && param==-1) {
    BtWirePattern *wire_pattern;

    i=0;
    pc_group=&self->priv->param_groups[0];
    while(pc_group->type==PGT_WIRE) {
      wire_pattern=bt_wire_get_pattern(pc_group->user_data,self->priv->pattern);
      if(wire_pattern) {
        bt_wire_pattern_serialize_columns(wire_pattern,beg,end,data);
        g_object_unref(wire_pattern);
      }
      i++;
      if(i==self->priv->number_of_groups)
        break;
      pc_group=&self->priv->param_groups[i];
    }
    bt_pattern_serialize_columns(self->priv->pattern,beg,end,data);
  }
  // process whole group
  if(group!=-1 && param==-1) {
    pc_group=&self->priv->param_groups[group];
    switch(pc_group->type) {
      case PGT_WIRE: {
        BtWirePattern *wire_pattern=bt_wire_get_pattern(pc_group->user_data,self->priv->pattern);
        if(wire_pattern) {
          for(i=0;i<pc_group->num_columns;i++) {
            bt_wire_pattern_serialize_column(wire_pattern,beg,end,i,data);
          }
          g_object_unref(wire_pattern);
        }
      } break;
      case PGT_GLOBAL: {
        for(i=0;i<pc_group->num_columns;i++) {
          bt_pattern_serialize_column(self->priv->pattern,beg,end,i,data);
        }
      } break;
      case PGT_VOICE: {
        BtMachine *machine;
        gulong global_params, voice_params, params;
        guint voice = GPOINTER_TO_UINT(pc_group->user_data);

        g_object_get(self->priv->pattern,"machine",&machine,NULL);
        g_object_get(machine,"global-params",&global_params,"voice-params",&voice_params,NULL);
        params=global_params+(voice*voice_params);
        for(i=0;i<pc_group->num_columns;i++) {
          bt_pattern_serialize_column(self->priv->pattern,beg,end,params+i,data);
        }
        g_object_unref(machine);
      } break;
    }
  }
  // process one param in one group
  if(group!=-1 && param!=-1) {
    pc_group=&self->priv->param_groups[group];
    switch(pc_group->type) {
      case PGT_WIRE: {
        BtWirePattern *wire_pattern=bt_wire_get_pattern(pc_group->user_data,self->priv->pattern);
        if(wire_pattern) {
          bt_wire_pattern_serialize_column(wire_pattern,beg,end,param,data);
          g_object_unref(wire_pattern);
        }
      } break;
      case PGT_GLOBAL:
        bt_pattern_serialize_column(self->priv->pattern,beg,end,param,data);
        break;
      case PGT_VOICE: {
        BtMachine *machine;
        gulong global_params, voice_params, params;

        g_object_get(self->priv->pattern,"machine",&machine,NULL);
        g_object_get(machine,"global-params",&global_params,"voice-params",&voice_params,NULL);
        params=global_params+(GPOINTER_TO_UINT(pc_group->user_data)*voice_params);
        bt_pattern_serialize_column(self->priv->pattern,beg,end,params+param,data);
        g_object_unref(machine);
      } break;
    }
  }
}

/* pattern_range_log_undo_redo:
 *
 * Add undo/redo events for the given data to the log.
 */
static void pattern_range_log_undo_redo(const BtMainPagePatterns *self,gint beg,gint end,gint group,gint param,gchar *old_str,gchar *new_str) {
  BtPatternEditorColumnGroup *pc_group;
  BtMachine *machine;
  gchar *mid,*pid;
  guint i;
  gchar *p;

  g_object_get(self->priv->pattern,"id",&pid,"machine",&machine,NULL);
  g_object_get(machine,"id",&mid,NULL);

  GST_INFO("logging : %d %d , %d %d",beg,end,group,param);
  // process full pattern
  if(group==-1 && param==-1) {
    gchar *undo_str,*redo_str;
    BtWire *wire;
    BtWirePattern *wire_pattern;
    gulong wire_groups, global_params, voices;
    guint g,v;

    bt_change_log_start_group(self->priv->change_log);

    g_object_get(machine,"global-params",&global_params,"voices",&voices,NULL);
    wire_groups=g_list_length(machine->dst_wires);

    for(g=0;g<wire_groups;g++) {
      pc_group=&self->priv->param_groups[g];
      wire = pc_group->user_data;
      wire_pattern=bt_wire_get_pattern(wire,self->priv->pattern);
      if(wire_pattern) {
        BtMachine *smachine;
        gchar *smid;

        g_object_get(wire,"src",&smachine,NULL);
        g_object_get(smachine,"id",&smid,NULL);

        for(i=0;i<pc_group->num_columns;i++) {
          p=strchr(old_str,'\n');*p='\0';
          undo_str = g_strdup_printf("set_wire_events \"%s\",\"%s\",\"%s\",%u,%u,%u,%s",smid,mid,pid,beg,end,i,old_str);
          old_str=&p[1];
          p=strchr(new_str,'\n');*p='\0';
          redo_str = g_strdup_printf("set_wire_events \"%s\",\"%s\",\"%s\",%u,%u,%u,%s",smid,mid,pid,beg,end,i,new_str);
          new_str=&p[1];
          bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,redo_str);
        }

        g_free(smid);
        g_object_unref(smachine);
        g_object_unref(wire_pattern);
      }
    }

    if(global_params) {
      pc_group=&self->priv->param_groups[g];
      for(i=0;i<pc_group->num_columns;i++) {
        p=strchr(old_str,'\n');*p='\0';
        undo_str = g_strdup_printf("set_global_events \"%s\",\"%s\",%u,%u,%u,%s",mid,pid,beg,end,i,old_str);
        old_str=&p[1];
        p=strchr(new_str,'\n');*p='\0';
        redo_str = g_strdup_printf("set_global_events \"%s\",\"%s\",%u,%u,%u,%s",mid,pid,beg,end,i,new_str);
        new_str=&p[1];
        bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,redo_str);
      }
      g++;
    }
    if(voices) {
      for(v=0;v<voices;v++,g++) {
        pc_group=&self->priv->param_groups[g];
        for(i=0;i<pc_group->num_columns;i++) {
          p=strchr(old_str,'\n');*p='\0';
          undo_str = g_strdup_printf("set_voice_events \"%s\",\"%s\",%u,%u,%u,%u,%s",mid,pid,beg,end,v,i,old_str);
          old_str=&p[1];
          p=strchr(new_str,'\n');*p='\0';
          redo_str = g_strdup_printf("set_voice_events \"%s\",\"%s\",%u,%u,%u,%u,%s",mid,pid,beg,end,v,i,new_str);
          new_str=&p[1];
          bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,redo_str);
        }
      }
    }
    bt_change_log_end_group(self->priv->change_log);
  }
  // process whole group
  if(group!=-1 && param==-1) {
    gchar *undo_str,*redo_str;

    bt_change_log_start_group(self->priv->change_log);

    pc_group=&self->priv->param_groups[group];
    switch(pc_group->type) {
      case PGT_WIRE: {
        BtWirePattern *wire_pattern=bt_wire_get_pattern(pc_group->user_data,self->priv->pattern);
        if(wire_pattern) {
          BtWire *wire = pc_group->user_data;
          BtMachine *smachine;
          gchar *smid;

          g_object_get(wire,"src",&smachine,NULL);
          g_object_get(smachine,"id",&smid,NULL);

          for(i=0;i<pc_group->num_columns;i++) {
            p=strchr(old_str,'\n');*p='\0';
            undo_str = g_strdup_printf("set_wire_events \"%s\",\"%s\",\"%s\",%u,%u,%u,%s",smid,mid,pid,beg,end,i,old_str);
            old_str=&p[1];
            p=strchr(new_str,'\n');*p='\0';
            redo_str = g_strdup_printf("set_wire_events \"%s\",\"%s\",\"%s\",%u,%u,%u,%s",smid,mid,pid,beg,end,i,new_str);
            new_str=&p[1];
            bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,redo_str);
          }

          g_free(smid);
          g_object_unref(smachine);
          g_object_unref(wire_pattern);
        }
      } break;
      case PGT_GLOBAL: {
        for(i=0;i<pc_group->num_columns;i++) {
          p=strchr(old_str,'\n');*p='\0';
          undo_str = g_strdup_printf("set_global_events \"%s\",\"%s\",%u,%u,%u,%s",mid,pid,beg,end,i,old_str);
          old_str=&p[1];
          p=strchr(new_str,'\n');*p='\0';
          redo_str = g_strdup_printf("set_global_events \"%s\",\"%s\",%u,%u,%u,%s",mid,pid,beg,end,i,new_str);
          new_str=&p[1];
          bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,redo_str);
        }
      } break;
      case PGT_VOICE: {
        guint voice = GPOINTER_TO_UINT(pc_group->user_data);

        for(i=0;i<pc_group->num_columns;i++) {
          p=strchr(old_str,'\n');*p='\0';
          undo_str = g_strdup_printf("set_voice_events \"%s\",\"%s\",%u,%u,%u,%u,%s",mid,pid,beg,end,voice,i,old_str);
          old_str=&p[1];
          p=strchr(new_str,'\n');*p='\0';
          redo_str = g_strdup_printf("set_voice_events \"%s\",\"%s\",%u,%u,%u,%u,%s",mid,pid,beg,end,voice,i,new_str);
          new_str=&p[1];
          bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,redo_str);
        }
      } break;
    }
    bt_change_log_end_group(self->priv->change_log);
  }
  // process one param in one group
  if(group!=-1 && param!=-1) {
    gchar *undo_str,*redo_str;

    pc_group=&self->priv->param_groups[group];
    switch(pc_group->type) {
      case PGT_WIRE: {
        BtWirePattern *wire_pattern=bt_wire_get_pattern(pc_group->user_data,self->priv->pattern);
        if(wire_pattern) {
          BtWire *wire = pc_group->user_data;
          BtMachine *smachine;
          gchar *smid;

          g_object_get(wire,"src",&smachine,NULL);
          g_object_get(smachine,"id",&smid,NULL);

          undo_str = g_strdup_printf("set_wire_events \"%s\",\"%s\",\"%s\",%u,%u,%u,%s",smid,mid,pid,beg,end,param,g_strchomp(old_str));
          redo_str = g_strdup_printf("set_wire_events \"%s\",\"%s\",\"%s\",%u,%u,%u,%s",smid,mid,pid,beg,end,param,g_strchomp(new_str));
          bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,redo_str);

          g_free(smid);
          g_object_unref(smachine);
          g_object_unref(wire_pattern);
        }
      } break;
      case PGT_GLOBAL:
        undo_str = g_strdup_printf("set_global_events \"%s\",\"%s\",%u,%u,%u,%s",mid,pid,beg,end,param,g_strchomp(old_str));
        redo_str = g_strdup_printf("set_global_events \"%s\",\"%s\",%u,%u,%u,%s",mid,pid,beg,end,param,g_strchomp(new_str));
        bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,redo_str);
        break;
      case PGT_VOICE: {
        guint voice = GPOINTER_TO_UINT(pc_group->user_data);

        undo_str = g_strdup_printf("set_voice_events \"%s\",\"%s\",%u,%u,%u,%u,%s",mid,pid,beg,end,voice,param,g_strchomp(old_str));
        redo_str = g_strdup_printf("set_voice_events \"%s\",\"%s\",%u,%u,%u,%u,%s",mid,pid,beg,end,voice,param,g_strchomp(new_str));
        bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,redo_str);
      } break;
    }
  }
  g_free(mid);g_free(pid);
  g_object_unref(machine);
}

//-- selection helpers

/* this is used for delete/blend/randomize,
 * - would be good to generalize to use it also for insert/delete-row.
 *   -> need to ignore row_end
 * - would be good to use this for cut/copy/paste
 *   -> copy/cut want to return the data from the selection
 *   -> paste needs to receive the data from the selection
 * ideally we'd like to avoid separate column/columns functions
 * - _column(BtPattern * self, gulong start_tick, gulong end_tick, gulong param)
 * - _columns(BtPattern * self, gulong start_tick, gulong end_tick)
 * - the groups would trigger the update several times as we need to use the column one a few times
 * better would be:
 * - _columns(BtPattern * self, gulong start_tick, gulong end_tick, gulong start_param, gulong end_param)
 */

typedef void (*DoBtPatternEditorColumn)(const BtPattern * const self, const gulong start_tick, const gulong end_tick, const gulong param);
typedef void (*DoBtPatternEditorColumns)(const BtPattern * const self, const gulong start_tick, const gulong end_tick);
typedef void (*DoWireBtPatternEditorColumn)(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick, const gulong param);
typedef void (*DoWireBtPatternEditorColumns)(const BtWirePattern * const self, const gulong start_tick, const gulong end_tick);

static gboolean pattern_selection_apply(const BtMainPagePatterns *self,DoBtPatternEditorColumn do_pattern_column,DoBtPatternEditorColumns do_pattern_columns,DoWireBtPatternEditorColumn do_wire_pattern_column,DoWireBtPatternEditorColumns do_wire_pattern_columns) {
  gboolean res=FALSE;
  gint beg,end,group,param;

  if(bt_pattern_editor_get_selection(self->priv->pattern_table,&beg,&end,&group,&param)) {
    GString *old_data=g_string_new(NULL),*new_data=g_string_new(NULL);
    BtPatternEditorColumnGroup *pc_group;
    guint i;

    pattern_range_copy(self,beg,end,group,param,old_data);

    GST_INFO("applying : %d %d , %d %d",beg,end,group,param);
    // process full pattern
    if(group==-1 && param==-1) {
      BtWirePattern *wire_pattern;

      i=0;
      pc_group=&self->priv->param_groups[0];
      while(pc_group->type==PGT_WIRE) {
        wire_pattern=bt_wire_get_pattern(pc_group->user_data,self->priv->pattern);
        if(wire_pattern) {
          do_wire_pattern_columns(wire_pattern,beg,end);
          g_object_unref(wire_pattern);
        }
        i++;
        if(i==self->priv->number_of_groups)
          break;
        pc_group=&self->priv->param_groups[i];
      }
      do_pattern_columns(self->priv->pattern,beg,end);
      res=TRUE;
    }
    // process whole group
    if(group!=-1 && param==-1) {
      pc_group=&self->priv->param_groups[group];
      switch(pc_group->type) {
        case PGT_WIRE: {
          BtWirePattern *wire_pattern=bt_wire_get_pattern(pc_group->user_data,self->priv->pattern);
          if(wire_pattern) {
            do_wire_pattern_columns(wire_pattern,beg,end);
            g_object_unref(wire_pattern);
          }
        } break;
        case PGT_GLOBAL: {
          for(i=0;i<pc_group->num_columns;i++) {
            do_pattern_column(self->priv->pattern,beg,end,i);
          }
        } break;
        case PGT_VOICE: {
          BtMachine *machine;
          gulong global_params, voice_params, params;
          guint voice = GPOINTER_TO_UINT(pc_group->user_data);

          g_object_get(self->priv->pattern,"machine",&machine,NULL);
          g_object_get(machine,"global-params",&global_params,"voice-params",&voice_params,NULL);
          params=global_params+(voice*voice_params);

          for(i=0;i<pc_group->num_columns;i++) {
            do_pattern_column(self->priv->pattern,beg,end,params+i);
          }
          g_object_unref(machine);
        } break;
      }
      res=TRUE;
    }
    // process one param in one group
    if(group!=-1 && param!=-1) {
      pc_group=&self->priv->param_groups[group];
      switch(pc_group->type) {
        case PGT_WIRE: {
          BtWirePattern *wire_pattern=bt_wire_get_pattern(pc_group->user_data,self->priv->pattern);
          if(wire_pattern) {
            do_wire_pattern_column(wire_pattern,beg,end,param);
            g_object_unref(wire_pattern);
          }
        } break;
        case PGT_GLOBAL:
          do_pattern_column(self->priv->pattern,beg,end,param);
          break;
        case PGT_VOICE: {
          BtMachine *machine;
          gulong global_params, voice_params, params;
          guint voice = GPOINTER_TO_UINT(pc_group->user_data);

          g_object_get(self->priv->pattern,"machine",&machine,NULL);
          g_object_get(machine,"global-params",&global_params,"voice-params",&voice_params,NULL);
          params=global_params+(voice*voice_params);
          do_pattern_column(self->priv->pattern,beg,end,params+param);
          g_object_unref(machine);
        } break;
      }
      res=TRUE;
    }

    if(res) {
      gtk_widget_queue_draw(GTK_WIDGET(self->priv->pattern_table));
      pattern_range_copy(self,beg,end,group,param,new_data);
      pattern_range_log_undo_redo(self,beg,end,group,param,old_data->str,new_data->str);
    }
    g_string_free(old_data,TRUE);
    g_string_free(new_data,TRUE);
  }
  return res;
}

//-- event handlers

#if 0
static void on_pattern_added(BtMachine *machine,BtPattern *pattern,gpointer user_data) {
/* this would be too early, the pattern properties are not set :/
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);

  // add undo/redo details
  if(bt_change_log_is_active(self->priv->change_log)) {
    gchar *undo_str,*redo_str;
    gchar *mid,*pid,*pname;
    gulong length;

    g_object_get(machine,"id",&mid,NULL);
    g_object_get(pattern,"id",&pid,"name",&pname,"length",&length,NULL);

    undo_str = g_strdup_printf("rem_pattern \"%s\",\"%s\"",mid,pid);
    redo_str = g_strdup_printf("add_pattern \"%s\",\"%s\",\"%s\",%lu",mid,pid,pname,length);
    bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,redo_str);
    g_free(mid);g_free(pid);g_free(pname);
  }
*/
}
#endif

static void on_pattern_removed(BtMachine *machine,BtPattern *pattern,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);

  // add undo/redo details
  if(bt_change_log_is_active(self->priv->change_log)) {
    BtWire *wire;
    BtWirePattern *wire_pattern;
    gchar *undo_str,*redo_str;
    gchar *mid,*pid,*pname;
    gulong length;
    gulong wire_params,global_params,voice_params,voices;
    guint end;
    GList *node;
    GString *data=g_string_new(NULL);
    gchar *str,*p;
    guint i,v;

    g_object_get(pattern,"id",&pid,"name",&pname,"length",&length,NULL);
    g_object_get(machine,"id",&mid,"global-params",&global_params,"voice-params",&voice_params,"voices",&voices,NULL);

    bt_change_log_start_group(self->priv->change_log);

    undo_str = g_strdup_printf("add_pattern \"%s\",\"%s\",\"%s\",%lu",mid,pid,pname,length);
    redo_str = g_strdup_printf("rem_pattern \"%s\",\"%s\"",mid,pid);
    bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,redo_str);

    /* we can't use the usual helpers here
     * - we don't need redo data
     * - we don't have the BtPatternEditorColumnGroup
     */
    end=length-1;
    for(node=machine->dst_wires;node;node=g_list_next(node)) {
      if((wire_pattern=bt_wire_get_pattern((BtWire *)node->data,pattern))) {
        bt_wire_pattern_serialize_columns(wire_pattern,0,end,data);
        g_object_unref(wire_pattern);
      }
    }
    bt_pattern_serialize_columns(pattern,0,end,data);
    str=data->str;
    // log events
    for(node=machine->dst_wires;node;node=g_list_next(node)) {
      wire=(BtWire *)node->data;
      wire_pattern=bt_wire_get_pattern(wire,pattern);
      if(wire_pattern) {
        BtMachine *smachine;
        gchar *smid;

        g_object_get(wire,"src",&smachine,"num-params",&wire_params,NULL);
        g_object_get(smachine,"id",&smid,NULL);

        for(i=0;i<wire_params;i++) {
          p=strchr(str,'\n');*p='\0';
          undo_str = g_strdup_printf("set_wire_events \"%s\",\"%s\",\"%s\",0,%u,%u,%s",smid,mid,pid,end,i,str);
          str=&p[1];
          bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,g_strdup(undo_str));
        }

        g_free(smid);
        g_object_unref(smachine);
        g_object_unref(wire_pattern);
      }
    }
    if(global_params) {
      for(i=0;i<global_params;i++) {
        p=strchr(str,'\n');*p='\0';
        undo_str = g_strdup_printf("set_global_events \"%s\",\"%s\",0,%u,%u,%s",mid,pid,end,i,str);
        str=&p[1];
        bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,g_strdup(undo_str));
      }
    }
    if(voices) {
      for(v=0;v<voices;v++) {
        for(i=0;i<voice_params;i++) {
          p=strchr(str,'\n');*p='\0';
          undo_str = g_strdup_printf("set_voice_events \"%s\",\"%s\",0,%u,%u,%u,%s",mid,pid,end,v,i,str);
          str=&p[1];
          bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,g_strdup(undo_str));
        }
      }
    }
    g_string_free(data,TRUE);

    bt_change_log_end_group(self->priv->change_log);

    g_free(mid);g_free(pid);g_free(pname);
  }
  GST_WARNING("removed pattern: %p,pattern->ref_ct=%d",pattern,G_OBJECT_REF_COUNT(pattern));
}

// use key-press-event, as then we get key repeats
static gboolean on_pattern_table_key_press_event(GtkWidget *widget,GdkEventKey *event,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  gboolean res=FALSE;
  gulong modifier=(gulong)event->state&gtk_accelerator_get_default_mod_mask();
  //gulong modifier=(gulong)event->state&(GDK_SHIFT_MASK|GDK_CONTROL_MASK|GDK_MOD4_MASK);

  if(!gtk_widget_get_realized(GTK_WIDGET(self->priv->pattern_table))) return(FALSE);

  GST_INFO("pattern_table key : state 0x%x, keyval 0x%x, hw-code 0x%x, name %s",
    event->state,event->keyval,event->hardware_keycode,gdk_keyval_name(event->keyval));
  if(event->keyval==GDK_Return) {  /* GDK_KP_Enter */
    BtMainWindow *main_window;
    //BtMainPageSequence *sequence_page;

    g_object_get(self->priv->app,"main-window",&main_window,NULL);
    //bt_child_proxy_get(main_window,"pages::sequence-page",&sequence_page,NULL);
    bt_child_proxy_set(main_window,"pages::page",BT_MAIN_PAGES_SEQUENCE_PAGE,NULL);
    /* if we came from sequesnce page via Enter we could go back
     * if the machine or pattern has been changed here, we could go to first
     * track and first pos where the new pattern is used.
     */
    //bt_main_page_sequence_goto_???(sequence_page,pattern);
    //g_object_unref(sequence_page);
    g_object_unref(main_window);

    res=TRUE;
  }
  else if(event->keyval==GDK_Menu) {
    gtk_menu_popup(self->priv->context_menu,NULL,NULL,NULL,NULL,3,gtk_get_current_event_time());
    res=TRUE;
  }
  else if (event->keyval == GDK_Insert) {
    GString *old_data=g_string_new(NULL),*new_data=g_string_new(NULL);
    gulong number_of_ticks;
    gint beg,end,group,param;

    g_object_get(self->priv->pattern_table, "cursor-row", &self->priv->cursor_row, "cursor-group", &self->priv->cursor_group, "cursor-param", &self->priv->cursor_param, NULL);
    g_object_get(self->priv->pattern,"length",&number_of_ticks,NULL);
    beg=self->priv->cursor_row;
    end=number_of_ticks-1;
    if((modifier&(GDK_CONTROL_MASK|GDK_SHIFT_MASK))==(GDK_CONTROL_MASK|GDK_SHIFT_MASK)) {
      group=-1;
      param=-1;
    }
    else if(modifier&GDK_SHIFT_MASK) {
      group=self->priv->cursor_group;
      param=-1;
    }
    else {
      group=self->priv->cursor_group;
      param=self->priv->cursor_param;
    }
    pattern_range_copy(self,beg,end,group,param,old_data);

    if((modifier&(GDK_CONTROL_MASK|GDK_SHIFT_MASK))==(GDK_CONTROL_MASK|GDK_SHIFT_MASK)) {
      // insert full row
      BtWirePattern *wire_pattern;
      guint i=0;

      GST_INFO("ctrl-shift-insert pressed, row %lu",self->priv->cursor_row);
      while(i<self->priv->number_of_groups && self->priv->param_groups[i].type==PGT_WIRE) {
        if((wire_pattern = bt_wire_get_pattern(self->priv->param_groups[i].user_data,self->priv->pattern))) {
          bt_wire_pattern_insert_full_row(wire_pattern,self->priv->cursor_row);
          g_object_unref(wire_pattern);
        }
        i++;
      }
      bt_pattern_insert_full_row(self->priv->pattern,self->priv->cursor_row);
      res=TRUE;
    }
    else if(modifier&GDK_SHIFT_MASK) {
      // insert group
      GST_INFO("shift-insert pressed, row %ld, group %u",self->priv->cursor_row,self->priv->cursor_group);
      switch(self->priv->param_groups[self->priv->cursor_group].type) {
        case PGT_WIRE: {
          BtWirePattern *wire_pattern = bt_wire_get_pattern(self->priv->param_groups[self->priv->cursor_group].user_data,self->priv->pattern);
          if(wire_pattern) {
            bt_wire_pattern_insert_full_row(wire_pattern,self->priv->cursor_row);
            g_object_unref(wire_pattern);
          }
        } break;
        case PGT_GLOBAL: {
          guint i;

          for(i=0;i<self->priv->param_groups[self->priv->cursor_group].num_columns;i++) {
            bt_pattern_insert_row(self->priv->pattern,self->priv->cursor_row, i);
          }
        } break;
        case PGT_VOICE: {
          BtMachine *machine;
          gulong global_params, voice_params, params;
          guint i;

          g_object_get(self->priv->pattern,"machine",&machine,NULL);
          g_object_get(machine,"global-params",&global_params,"voice-params",&voice_params,NULL);
          params=global_params+(GPOINTER_TO_UINT(self->priv->param_groups[self->priv->cursor_group].user_data)*voice_params);
          for(i=0;i<self->priv->param_groups[self->priv->cursor_group].num_columns;i++) {
            bt_pattern_insert_row(self->priv->pattern,self->priv->cursor_row, params+i);
          }
          g_object_unref(machine);
        } break;
      }
      res=TRUE;
    }
    else {
      // insert column
      GST_INFO("insert pressed, row %ld, group %u, param %u",self->priv->cursor_row,self->priv->cursor_group, self->priv->cursor_param);
      switch(self->priv->param_groups[self->priv->cursor_group].type) {
        case PGT_WIRE: {
          BtWirePattern *wire_pattern = bt_wire_get_pattern(self->priv->param_groups[self->priv->cursor_group].user_data,self->priv->pattern);
          if(wire_pattern) {
            bt_wire_pattern_insert_row(wire_pattern,self->priv->cursor_row, self->priv->cursor_param);
            g_object_unref(wire_pattern);
          }
        } break;
        case PGT_GLOBAL:
          bt_pattern_insert_row(self->priv->pattern,self->priv->cursor_row, self->priv->cursor_param);
          break;
        case PGT_VOICE: {
          BtMachine *machine;
          gulong global_params, voice_params, params;

          g_object_get(self->priv->pattern,"machine",&machine,NULL);
          g_object_get(machine,"global-params",&global_params,"voice-params",&voice_params,NULL);
          params=global_params+(GPOINTER_TO_UINT(self->priv->param_groups[self->priv->cursor_group].user_data)*voice_params);
          bt_pattern_insert_row(self->priv->pattern,self->priv->cursor_row, params+self->priv->cursor_param);
          g_object_unref(machine);
        } break;
      }
      res=TRUE;
    }

    if(res) {
      gtk_widget_queue_draw(GTK_WIDGET(self->priv->pattern_table));
      pattern_range_copy(self,beg,end,group,param,new_data);
      pattern_range_log_undo_redo(self,beg,end,group,param,old_data->str,new_data->str);
    }
    g_string_free(old_data,TRUE);
    g_string_free(new_data,TRUE);
  }
  else if(event->keyval == GDK_Delete) {
    GString *old_data=g_string_new(NULL),*new_data=g_string_new(NULL);
    gulong number_of_ticks;
    gint beg,end,group,param;

    g_object_get(self->priv->pattern_table, "cursor-row", &self->priv->cursor_row, "cursor-group", &self->priv->cursor_group, "cursor-param", &self->priv->cursor_param, NULL);
    g_object_get(self->priv->pattern,"length",&number_of_ticks,NULL);
    beg=self->priv->cursor_row;
    end=number_of_ticks-1;
    if((modifier&(GDK_CONTROL_MASK|GDK_SHIFT_MASK))==(GDK_CONTROL_MASK|GDK_SHIFT_MASK)) {
      group=-1;
      param=-1;
    }
    else if(modifier&GDK_SHIFT_MASK) {
      group=self->priv->cursor_group;
      param=-1;
    }
    else {
      group=self->priv->cursor_group;
      param=self->priv->cursor_param;
    }

    pattern_range_copy(self,beg,end,group,param,old_data);

    if((modifier&(GDK_CONTROL_MASK|GDK_SHIFT_MASK))==(GDK_CONTROL_MASK|GDK_SHIFT_MASK)) {
      // delete full row
      BtWirePattern *wire_pattern;
      guint i=0;

      g_object_get(self->priv->pattern_table, "cursor-row", &self->priv->cursor_row, NULL);
      GST_INFO("ctrl-shift-delete pressed, row %lu",self->priv->cursor_row);
      while(i<self->priv->number_of_groups && self->priv->param_groups[i].type==PGT_WIRE) {
        if((wire_pattern = bt_wire_get_pattern(self->priv->param_groups[i].user_data,self->priv->pattern))) {
          bt_wire_pattern_delete_full_row(wire_pattern,self->priv->cursor_row);
          g_object_unref(wire_pattern);
        }
        i++;
      }
      bt_pattern_delete_full_row(self->priv->pattern,self->priv->cursor_row);
      res=TRUE;
    }
    else if(modifier&GDK_SHIFT_MASK) {
      // delete group
      g_object_get(self->priv->pattern_table, "cursor-row", &self->priv->cursor_row, "cursor-group", &self->priv->cursor_group, NULL);
      GST_INFO("delete pressed, row %ld, group %u",self->priv->cursor_row,self->priv->cursor_group);
      switch(self->priv->param_groups[self->priv->cursor_group].type) {
        case PGT_WIRE: {
          BtWirePattern *wire_pattern = bt_wire_get_pattern(self->priv->param_groups[self->priv->cursor_group].user_data,self->priv->pattern);
          if(wire_pattern) {
            bt_wire_pattern_delete_full_row(wire_pattern,self->priv->cursor_row);
            g_object_unref(wire_pattern);
          }
        } break;
        case PGT_GLOBAL: {
          guint i;

          for(i=0;i<self->priv->param_groups[self->priv->cursor_group].num_columns;i++) {
            bt_pattern_delete_row(self->priv->pattern,self->priv->cursor_row, i);
          }
        } break;
        case PGT_VOICE: {
          BtMachine *machine;
          gulong global_params, voice_params, params;
          guint i;

          g_object_get(self->priv->pattern,"machine",&machine,NULL);
          g_object_get(machine,"global-params",&global_params,"voice-params",&voice_params,NULL);
          params=global_params+(GPOINTER_TO_UINT(self->priv->param_groups[self->priv->cursor_group].user_data)*voice_params);
          for(i=0;i<self->priv->param_groups[self->priv->cursor_group].num_columns;i++) {
            bt_pattern_delete_row(self->priv->pattern,self->priv->cursor_row, params+i);
          }
          g_object_unref(machine);
        } break;
      }
      res=TRUE;
    }
    else {
      // delete column
      g_object_get(self->priv->pattern_table, "cursor-row", &self->priv->cursor_row, "cursor-group", &self->priv->cursor_group, "cursor-param", &self->priv->cursor_param, NULL);
      GST_INFO("delete pressed, row %ld, group %u, param %u",self->priv->cursor_row,self->priv->cursor_group, self->priv->cursor_param);
      switch(self->priv->param_groups[self->priv->cursor_group].type) {
        case PGT_WIRE: {
          BtWirePattern *wire_pattern = bt_wire_get_pattern(self->priv->param_groups[self->priv->cursor_group].user_data,self->priv->pattern);
          if(wire_pattern) {
            bt_wire_pattern_delete_row(wire_pattern,self->priv->cursor_row, self->priv->cursor_param);
            g_object_unref(wire_pattern);
          }
        } break;
        case PGT_GLOBAL:
          bt_pattern_delete_row(self->priv->pattern,self->priv->cursor_row, self->priv->cursor_param);
          break;
        case PGT_VOICE: {
          BtMachine *machine;
          gulong global_params, voice_params, params;

          g_object_get(self->priv->pattern,"machine",&machine,NULL);
          g_object_get(machine,"global-params",&global_params,"voice-params",&voice_params,NULL);
          params=global_params+(GPOINTER_TO_UINT(self->priv->param_groups[self->priv->cursor_group].user_data)*voice_params);
          bt_pattern_delete_row(self->priv->pattern,self->priv->cursor_row, params+self->priv->cursor_param);
          g_object_unref(machine);
        } break;
      }
      res=TRUE;
    }

    if(res) {
      gtk_widget_queue_draw(GTK_WIDGET(self->priv->pattern_table));
      pattern_range_copy(self,beg,end,group,param,new_data);
      pattern_range_log_undo_redo(self,beg,end,group,param,old_data->str,new_data->str);
    }
    g_string_free(old_data,TRUE);
    g_string_free(new_data,TRUE);
  }
  else if(event->keyval == GDK_f) {
    if(modifier&GDK_CONTROL_MASK) {
      res=pattern_selection_apply(self,
        bt_pattern_flip_column,bt_pattern_flip_columns,
        bt_wire_pattern_flip_column,bt_wire_pattern_flip_columns);
    }
  }
  else if(event->keyval == GDK_i) {
    if(modifier&GDK_CONTROL_MASK) {
      res=pattern_selection_apply(self,
        bt_pattern_blend_column,bt_pattern_blend_columns,
        bt_wire_pattern_blend_column,bt_wire_pattern_blend_columns);
    }
  }
  else if(event->keyval == GDK_r) {
    if(modifier&GDK_CONTROL_MASK) {
      res=pattern_selection_apply(self,
        bt_pattern_randomize_column,bt_pattern_randomize_columns,
        bt_wire_pattern_randomize_column,bt_wire_pattern_randomize_columns);
    }
  }
  else if((event->keyval == GDK_Up) && (modifier==GDK_CONTROL_MASK)) {
    g_signal_emit_by_name(self->priv->machine_menu,"move-active",GTK_SCROLL_STEP_BACKWARD,NULL);
    res=TRUE;
  }
  else if((event->keyval == GDK_Down) && (modifier==GDK_CONTROL_MASK)) {
    g_signal_emit_by_name(self->priv->machine_menu,"move-active",GTK_SCROLL_STEP_FORWARD,NULL);
    res=TRUE;
  }
  else if(event->keyval == GDK_KP_Subtract) {
    g_signal_emit_by_name(self->priv->pattern_menu,"move-active",GTK_SCROLL_STEP_BACKWARD,NULL);
    res=TRUE;
  }
  else if(event->keyval == GDK_KP_Add) {
    g_signal_emit_by_name(self->priv->pattern_menu,"move-active",GTK_SCROLL_STEP_FORWARD,NULL);
    res=TRUE;
  }
  else if(event->keyval == GDK_KP_Divide) {
    g_signal_emit_by_name(self->priv->base_octave_menu,"move-active",GTK_SCROLL_STEP_BACKWARD,NULL);
    res=TRUE;
  }
  else if(event->keyval == GDK_KP_Multiply) {
    g_signal_emit_by_name(self->priv->base_octave_menu,"move-active",GTK_SCROLL_STEP_FORWARD,NULL);
    res=TRUE;
  }
  else if(event->keyval == GDK_less) {
    g_signal_emit_by_name(self->priv->wavetable_menu,"move-active",GTK_SCROLL_STEP_BACKWARD,NULL);
    res=TRUE;
  }
  else if(event->keyval == GDK_greater) {
    g_signal_emit_by_name(self->priv->wavetable_menu,"move-active",GTK_SCROLL_STEP_FORWARD,NULL);
    res=TRUE;
  }
  return(res);
}

static gboolean on_pattern_table_button_press_event(GtkWidget *widget,GdkEventButton *event,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  gboolean res=FALSE;

  GST_INFO("pattern_table button_press : button 0x%x, type 0x%d",event->button,event->type);
  if(event->button==3) {
    gtk_menu_popup(self->priv->context_menu,NULL,NULL,NULL,NULL,3,gtk_get_current_event_time());
    res=TRUE;
  }
  return(res);
}

static void on_pattern_table_cursor_group_changed(const BtPatternEditor *editor,GParamSpec *arg,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);

  g_object_get((gpointer)editor, "cursor-group", &self->priv->cursor_group, "cursor-param", &self->priv->cursor_param, NULL);
  pattern_view_update_column_description(self,UPDATE_COLUMN_UPDATE);
}

static void on_pattern_table_cursor_param_changed(const BtPatternEditor *editor,GParamSpec *arg,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);

  g_object_get((gpointer)editor, "cursor-param", &self->priv->cursor_param, NULL);
  pattern_view_update_column_description(self,UPDATE_COLUMN_UPDATE);
}

static void on_pattern_table_cursor_row_changed(const BtPatternEditor *editor,GParamSpec *arg,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);

  g_object_get((gpointer)editor, "cursor-row", &self->priv->cursor_row, NULL);
  pattern_view_update_column_description(self,UPDATE_COLUMN_UPDATE);
}

static void on_machine_model_row_inserted(GtkTreeModel *tree_model, GtkTreePath *path,GtkTreeIter *iter,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);

  //GST_WARNING("-- added index %s", gtk_tree_path_to_string(path));

  gtk_combo_box_set_active_iter(self->priv->machine_menu,iter);
}

static void on_machine_model_row_deleted(GtkTreeModel *tree_model,GtkTreePath *path,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  GtkTreeIter iter;

  //GST_WARNING("-- removed index %s", gtk_tree_path_to_string(path));

  // the last machine is master which we actually cannot remove
  if(!gtk_tree_model_get_iter(tree_model, &iter, path)) {
    GtkTreePath *p=gtk_tree_path_copy(path);
    if(!gtk_tree_path_prev(p)) {
    	gtk_tree_path_free(p);
      return;
    }
    if(!gtk_tree_model_get_iter(tree_model, &iter, p)) {
    	gtk_tree_path_free(p);
      return;
    }
    gtk_tree_path_free(p);
  }

  //GST_WARNING("-- activate index %s", gtk_tree_path_to_string(path));
  gtk_combo_box_set_active_iter(self->priv->machine_menu,&iter);
}

//-- event handler helper

static void machine_menu_refresh(const BtMainPagePatterns *self,const BtSetup *setup) {
  BtMachine *machine=NULL;
  GList *node,*list;
  BtMachineListModel *store;
  gint index;

  // create machine menu
  store=bt_machine_list_model_new((BtSetup *)setup);
  index=gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store),NULL)-1;
  // connect signal handlers for pattern undo/redo
  g_object_get((gpointer)setup,"machines",&list,NULL);
  for(node=list;node;node=g_list_next(node)) {
    machine=BT_MACHINE(node->data);
    //g_signal_connect(machine,"pattern-added",G_CALLBACK(on_pattern_added),(gpointer)self);
    g_signal_connect(machine,"pattern-removed",G_CALLBACK(on_pattern_removed),(gpointer)self);
  }
  g_list_free(list);
  g_signal_connect(store,"row-inserted",G_CALLBACK(on_machine_model_row_inserted),(gpointer)self);
  g_signal_connect(store,"row-deleted",G_CALLBACK(on_machine_model_row_deleted),(gpointer)self);

  GST_INFO("machine menu refreshed, active item %d",index);
  gtk_widget_set_sensitive(GTK_WIDGET(self->priv->machine_menu),(index!=-1));
  gtk_combo_box_set_model(self->priv->machine_menu,GTK_TREE_MODEL(store));
  gtk_combo_box_set_active(self->priv->machine_menu,index);
  g_object_unref(store); // drop with comboxbox
}

static void pattern_menu_refresh(const BtMainPagePatterns *self,BtMachine *machine) {
  BtPatternListModel *store;
  BtPattern *pattern=NULL;
  GtkTreeIter iter;
  gint index=-1;
  gint active=-1;
  BtSong *song;
  BtSequence *sequence;

  g_assert(machine);

  // update pattern menu
  g_object_get(self->priv->app,"song",&song,NULL);
  g_object_get(song,"sequence",&sequence,NULL);
  store=bt_pattern_list_model_new(machine,sequence,TRUE);

  if(gtk_tree_model_get_iter_first((GtkTreeModel *)store,&iter)) {
    do {
      pattern=bt_pattern_list_model_get_object((BtPatternListModel *)store,&iter);
      index++;  // count pattern index, so that we can activate one in the combobox
      if(pattern==self->priv->pattern) {
        active=index;
        break;
      }
    } while(gtk_tree_model_iter_next((GtkTreeModel *)store,&iter));
  }
  g_object_unref(sequence);
  g_object_unref(song);
  GST_INFO("pattern menu refreshed, active entry=%d",active);

  if(active==-1) {
    // use the last one, if there is no active one
    active=index;
  }

  gtk_widget_set_sensitive(GTK_WIDGET(self->priv->pattern_menu),(index!=-1));
  gtk_combo_box_set_model(self->priv->pattern_menu,GTK_TREE_MODEL(store));
  gtk_combo_box_set_active(self->priv->pattern_menu,active);
  g_object_unref(store); // drop with comboxbox

  // unfortunately we need to do this, gtk+ swallows the first changed signal here
  // as nothing was selected and we don't select anything
  if(active==-1) {
    change_current_pattern(self,NULL);
  }
}

static void wavetable_menu_refresh(const BtMainPagePatterns *self,BtWavetable *wavetable) {
  BtWave *wave;
  gchar *str,hstr[3];
  GtkListStore *store;
  GtkTreeIter menu_iter;
  gint i,index=-1, count = 0;

  // update pattern menu
  store=gtk_list_store_new(2,G_TYPE_STRING,G_TYPE_STRING);

  //-- append waves rows (buzz numbers them from 0x01 to 0xC8=200)
  for(i=1;i<=200;i++)
    self->priv->wave_to_combopos[i] = self->priv->combopos_to_wave[i] = -1;
  for(i=1;i<=200;i++) {
    if((wave=bt_wavetable_get_wave_by_index(wavetable,i))) {
      self->priv->wave_to_combopos[i] = count;
      self->priv->combopos_to_wave[count] = i;
      gtk_list_store_append(store, &menu_iter);
      g_object_get(wave,"name",&str,NULL);
      GST_INFO("  adding [%3d] \"%s\"",i,str);
      // buzz shows index as hex, because trackers needs it this way
      sprintf(hstr,"%02x",i);
      gtk_list_store_set(store,&menu_iter,
        WAVE_MENU_NUMBER,hstr,
        WAVE_MENU_LABEL,str,
        -1);
      g_free(str);
      g_object_unref(wave);
      if(index==-1) index=i-1;
      count++;
    }
  }
  GST_INFO("  index=%d",index);

  gtk_widget_set_sensitive(GTK_WIDGET(self->priv->wavetable_menu),(index!=-1));
  gtk_combo_box_set_model(self->priv->wavetable_menu,GTK_TREE_MODEL(store));
  gtk_combo_box_set_active(self->priv->wavetable_menu,index);
  g_object_unref(store); // drop with comboxbox
}

/* - we are using GValue directly for reading out the patterns
 * - @todo: for changes from the pattern editor we still convert the float
 *   values to strings and then to GValues.
 */
typedef struct {
  gfloat (*val_to_float)(GValue *in, gpointer user_data);
  const gchar *(*float_to_str)(gfloat in, gpointer user_data);
} BtPatternEditorColumnConvertersCallbacks;

typedef struct {
  BtPatternEditorColumnConvertersCallbacks callbacks;
  gfloat min,max;
} BtPatternEditorColumnConvertersFloatCallbacks;

typedef struct {
  union {
    BtPatternEditorColumnConvertersCallbacks any_cb;
    BtPatternEditorColumnConvertersFloatCallbacks float_cb;
  };
} BtPatternEditorColumnConverters;

static gfloat pattern_edit_get_data_at(gpointer pattern_data, gpointer column_data, guint row, guint track, guint param) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(pattern_data);
  GValue *val = NULL;
  BtPatternEditorColumnGroup *group = &self->priv->param_groups[track];

  switch (group->type) {
    case PGT_WIRE: {
      BtWirePattern *wire_pattern = bt_wire_get_pattern(group->user_data,self->priv->pattern);
      if(wire_pattern) {
        val=bt_wire_pattern_get_event_data(wire_pattern,row,param);
        g_object_unref(wire_pattern);
      }
    } break;
    case PGT_GLOBAL:
      val=bt_pattern_get_global_event_data(self->priv->pattern,row,param);
      break;
    case PGT_VOICE:
      val=bt_pattern_get_voice_event_data(self->priv->pattern,row,GPOINTER_TO_UINT(group->user_data),param);
      break;
    default:
      GST_WARNING("invalid column group type");
  }

  if(val &&  BT_IS_GVALUE(val) && column_data) {
    return ((BtPatternEditorColumnConvertersCallbacks *)column_data)->val_to_float(val,column_data);
  }
  return group->columns[param].def;
}

static void pattern_edit_set_data_at(gpointer pattern_data, gpointer column_data, guint row, guint track, guint param, guint digit, gfloat value) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(pattern_data);
  const gchar *str = NULL;
  BtPatternEditorColumnGroup *group = &self->priv->param_groups[track];
  BtMachine *machine;

  if(column_data)
    str=((BtPatternEditorColumnConvertersCallbacks *)column_data)->float_to_str(value,column_data);
  else
    if(value!=group->columns[param].def)
      str=bt_persistence_strfmt_double(value);

  g_object_get(self->priv->pattern,"machine",&machine,NULL);

  if(group->type==PGT_GLOBAL || group->type==PGT_VOICE) {
    gboolean is_trigger;

    if(group->type==PGT_GLOBAL)
      is_trigger=bt_machine_is_global_param_trigger(machine,param);
    else
      is_trigger=bt_machine_is_voice_param_trigger(machine,param);

    if(is_trigger) {
      gboolean update_wave=FALSE;
			BtPatternEditorColumn *col=&group->columns[param];

      // play live (notes, triggers)
      if(BT_IS_STRING(str) && self->priv->play_live) {
        GstObject *element,*voice;

        g_object_get(machine,"machine",&element,NULL);

        /* @todo: buzz machines need set, tick, unset */
        if(group->type==PGT_GLOBAL) {
          GST_INFO("play global trigger: %f,'%s'",value,str);
          switch(col->type) {
            case PCT_NOTE:
              g_object_set(element,bt_machine_get_global_param_name(machine,param),str,NULL);
              break;
            case PCT_SWITCH:
            case PCT_BYTE:
            case PCT_WORD: {
              gint val=atoi(str);
              if(val==col->def) {
              	g_object_set(element,bt_machine_get_global_param_name(machine,param),val,NULL);
              }
            } break;
            case PCT_FLOAT: {
              gfloat val=atof(str);
              if(val==col->def) {
              	g_object_set(element,bt_machine_get_global_param_name(machine,param),val,NULL);
              }
            } break;
          }
        }
        else {
          GST_INFO("play voice %u trigger: %f,'%s'",GPOINTER_TO_UINT(group->user_data),value,str);
          voice=gst_child_proxy_get_child_by_index(GST_CHILD_PROXY(element),GPOINTER_TO_UINT(group->user_data));
          switch(col->type) {
            case PCT_NOTE:
              g_object_set(voice,bt_machine_get_voice_param_name(machine,param),str,NULL);
              break;
            case PCT_SWITCH:
            case PCT_BYTE:
            case PCT_WORD: {
              gint val=atoi(str);
              if(val==col->def) {
              	g_object_set(voice,bt_machine_get_voice_param_name(machine,param),val,NULL);
              }
            } break;
            case PCT_FLOAT: {
              gfloat val=atof(str);
              if(val==col->def) {
              	g_object_set(voice,bt_machine_get_voice_param_name(machine,param),val,NULL);
              }
            } break;
          }

          gst_object_unref(voice);
        }
        gst_object_unref(element);
      }

      if(col->type == PCT_NOTE) {
        // do not update the wave if it's an octave column or if the new value is 'off'
        if(digit == 0 && value != 255 && value != 0) {
          update_wave=TRUE;
        }
      }
      // if machine can play wave, lookup wave column and enter wave index
      if(update_wave) {
        const gchar *wave_str = "";
        glong wave_param=-1;

        if(BT_IS_STRING(str)) {
          gint wave_ix=gtk_combo_box_get_active(self->priv->wavetable_menu);
          if (wave_ix >= 0) {
            wave_ix = self->priv->combopos_to_wave[wave_ix];
            wave_str=bt_persistence_strfmt_ulong(wave_ix);
          }
          GST_DEBUG("wav index: %d, %s",wave_ix,wave_str);
        }

        switch (group->type) {
          case PGT_GLOBAL:
            // search for param that has flags&GSTBT_PROPERTY_META_WAVE in global machine params
            wave_param=bt_machine_get_global_wave_param_index(machine);
            GST_DEBUG("global wav param: %ld",wave_param);
            if(wave_param>-1) {
              bt_pattern_set_global_event(self->priv->pattern,row,wave_param,wave_str);
            }
            break;
          case PGT_VOICE:
            // search for param that has flags&GSTBT_PROPERTY_META_WAVE in voice machine params
            wave_param=bt_machine_get_voice_wave_param_index(machine);
            GST_DEBUG("voice wav param: %ld",wave_param);
            if(wave_param>-1) {
              bt_pattern_set_voice_event(self->priv->pattern,row,GPOINTER_TO_UINT(group->user_data),wave_param,wave_str);
            }
            break;
          default:
            break;
        }
        if(wave_param>-1) {
          gtk_widget_queue_draw(GTK_WIDGET(self->priv->pattern_table));
        }
      }
    }
    if(group->columns[param].type == PCT_BYTE) {
      glong wave_param = -1;
      switch (group->type) {
        case PGT_GLOBAL:
          wave_param = bt_machine_get_global_wave_param_index(machine);
          break;
        case PGT_VOICE:
          wave_param=bt_machine_get_voice_wave_param_index(machine);
          break;
        default:
          break;
      }
      if (wave_param == param) {
        int v = (int)value;
        if (value >= 0 && v < MAX_WAVETABLE_ITEMS + 2 && self->priv->wave_to_combopos[v] != -1)
          gtk_combo_box_set_active(self->priv->wavetable_menu, self->priv->wave_to_combopos[v]);
      }
    }
  }

  switch (group->type) {
    case PGT_WIRE: {
      BtWire *wire = group->user_data;
      BtWirePattern *wire_pattern = bt_wire_get_pattern(wire,self->priv->pattern);
      BtMachine *smachine;
      GString *old_data=g_string_new(NULL),*new_data=g_string_new(NULL);
      gchar *undo_str,*redo_str;
      gchar *smid,*dmid,*pid;

      if(!wire_pattern) {
        BtSong *song;

        g_object_get(self->priv->app,"song",&song,NULL);
        wire_pattern=bt_wire_pattern_new(song,wire,self->priv->pattern);
        g_object_unref(song);
      }

      g_object_get(wire,"src",&smachine,NULL);
      g_object_get(smachine,"id",&smid,NULL);
      g_object_get(machine,"id",&dmid,NULL);
      g_object_get(self->priv->pattern,"id",&pid,NULL);

      bt_wire_pattern_serialize_column(wire_pattern,row,row,param,old_data);
      bt_wire_pattern_set_event(wire_pattern,row,param,str);
      bt_wire_pattern_serialize_column(wire_pattern,row,row,param,new_data);
      undo_str = g_strdup_printf("set_wire_events \"%s\",\"%s\",\"%s\",%u,%u,%u,%s",smid,dmid,pid,row,row,param,g_strchomp(g_string_free(old_data,FALSE)));
      redo_str = g_strdup_printf("set_wire_events \"%s\",\"%s\",\"%s\",%u,%u,%u,%s",smid,dmid,pid,row,row,param,g_strchomp(g_string_free(new_data,FALSE)));
      bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,redo_str);
      g_free(smid);g_free(dmid);g_free(pid);
      g_object_unref(smachine);
      g_object_unref(wire_pattern);
    } break;
    case PGT_GLOBAL: {
      // serialize action
      GString *old_data=g_string_new(NULL),*new_data=g_string_new(NULL);
      gchar *undo_str,*redo_str;
      gchar *mid,*pid;

      g_object_get(machine,"id",&mid,NULL);
      g_object_get(self->priv->pattern,"id",&pid,NULL);

      bt_pattern_serialize_column(self->priv->pattern,row,row,param,old_data);
      bt_pattern_set_global_event(self->priv->pattern,row,param,str);
      bt_pattern_serialize_column(self->priv->pattern,row,row,param,new_data);
      undo_str = g_strdup_printf("set_global_events \"%s\",\"%s\",%u,%u,%u,%s",mid,pid,row,row,param,g_strchomp(g_string_free(old_data,FALSE)));
      redo_str = g_strdup_printf("set_global_events \"%s\",\"%s\",%u,%u,%u,%s",mid,pid,row,row,param,g_strchomp(g_string_free(new_data,FALSE)));
      bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,redo_str);
      g_free(mid);g_free(pid);
    } break;
    case PGT_VOICE: {
      // serialize action
      GString *old_data=g_string_new(NULL),*new_data=g_string_new(NULL);
      guint voice = GPOINTER_TO_UINT(group->user_data);
      gchar *undo_str,*redo_str;
      gchar *mid,*pid;
      gulong global_params,voice_params,param_offset;

      g_object_get(machine,"id",&mid,"global-params",&global_params,"voice-params",&voice_params,NULL);
      g_object_get(self->priv->pattern,"id",&pid,NULL);
      param_offset=global_params+(voice*voice_params);

      bt_pattern_serialize_column(self->priv->pattern,row,row,param_offset+param,old_data);
      bt_pattern_set_voice_event(self->priv->pattern,row,voice,param,str);
      bt_pattern_serialize_column(self->priv->pattern,row,row,param_offset+param,new_data);
      undo_str = g_strdup_printf("set_voice_events \"%s\",\"%s\",%u,%u,%u,%u,%s",mid,pid,row,row,voice,param,g_strchomp(g_string_free(old_data,FALSE)));
      redo_str = g_strdup_printf("set_voice_events \"%s\",\"%s\",%u,%u,%u,%u,%s",mid,pid,row,row,voice,param,g_strchomp(g_string_free(new_data,FALSE)));
      bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,redo_str);
      g_free(mid);g_free(pid);
    } break;
    default:
      GST_WARNING("invalid column group type");
  }
  pattern_view_update_column_description(self,UPDATE_COLUMN_UPDATE);
  g_object_unref(machine);
}

static const gchar * any_float_to_str(gfloat in, gpointer user_data) {
  return(bt_persistence_strfmt_double(in));
}

static gfloat note_val_to_float(GValue *v, gpointer user_data) {
  const gchar *note=g_value_get_string(v);
  if(note)
    return (gfloat)gstbt_tone_conversion_note_string_2_number(note);
  return 0.0;
}

static const gchar * note_float_to_str(gfloat in, gpointer user_data) {
  return(gstbt_tone_conversion_note_number_2_string((guint)in));
}

static gfloat float_val_to_float(GValue *v, gpointer user_data) {
  // scale value into 0...65535 range
  BtPatternEditorColumnConvertersFloatCallbacks *pcc=(BtPatternEditorColumnConvertersFloatCallbacks *)user_data;
  gdouble val=g_value_get_float(v);
  gdouble factor=65535.0/(pcc->max-pcc->min);

  //GST_DEBUG("> val %lf, factor %lf, result %lf",val,factor,(val-pcc->min)*factor);

  return (val-pcc->min)*factor;
}

static gfloat double_val_to_float(GValue *v, gpointer user_data) {
  // scale value into 0...65535 range
  BtPatternEditorColumnConvertersFloatCallbacks *pcc=(BtPatternEditorColumnConvertersFloatCallbacks *)user_data;
  gdouble val=g_value_get_double(v);
  gdouble factor=65535.0/(pcc->max-pcc->min);

  //GST_DEBUG("> val %lf, factor %lf, result %lf",val,factor,(val-pcc->min)*factor);

  return (val-pcc->min)*factor;
}

static const gchar * float_float_to_str(gfloat in, gpointer user_data) {
  // scale value from 0...65535 range
  BtPatternEditorColumnConvertersFloatCallbacks *pcc=(BtPatternEditorColumnConvertersFloatCallbacks *)user_data;
  gdouble factor=65535.0/(pcc->max-pcc->min);
  gdouble val=pcc->min+(in/factor);

  //GST_DEBUG("< val %lf, factor %lf, result %lf(%s)",in,factor,val,bt_persistence_strfmt_double(val));

  return bt_persistence_strfmt_double(val);
}

static gfloat boolean_val_to_float(GValue *v, gpointer user_data) {
  return (gfloat)g_value_get_boolean(v);
}

static gfloat enum_val_to_float(GValue *v, gpointer user_data) {
  return (gfloat)g_value_get_enum(v);
}

static gfloat int_val_to_float(GValue *v, gpointer user_data) {
  return (gfloat)g_value_get_int(v);
}

static gfloat uint_val_to_float(GValue *v, gpointer user_data) {
  return (gfloat)g_value_get_uint(v);
}


static void pattern_edit_fill_column_type(BtPatternEditorColumn *col,GParamSpec *property, GValue *min_val, GValue *max_val, GValue *no_val) {
  GType type=bt_g_type_get_base_type(property->value_type);

  GST_LOG("filling param type: '%s'::'%s'/'%s' for parameter '%s'",
    g_type_name(property->owner_type),g_type_name(type),
    g_type_name(property->value_type),property->name);

  switch(type) {
    case G_TYPE_STRING: {
      BtPatternEditorColumnConvertersCallbacks *pcc;

      col->type=PCT_NOTE;
      col->min=0;
      col->max=((16*9)+12);
      col->def=GSTBT_TONE_CONVERSION_NOTE_NO;
      col->user_data=g_new(BtPatternEditorColumnConverters,1);
      pcc=(BtPatternEditorColumnConvertersCallbacks *)col->user_data;
      pcc->val_to_float=note_val_to_float;
      pcc->float_to_str=note_float_to_str;
    } break;
    case G_TYPE_BOOLEAN: {
      BtPatternEditorColumnConvertersCallbacks *pcc;

      col->type=PCT_SWITCH;
      col->min=0;
      col->max=1;
      col->def=BT_IS_GVALUE(no_val)?g_value_get_boolean(no_val):col->max+1;
      col->user_data=g_new(BtPatternEditorColumnConverters,1);
      pcc=(BtPatternEditorColumnConvertersCallbacks *)col->user_data;
      pcc->val_to_float=boolean_val_to_float;
      pcc->float_to_str=any_float_to_str;
    } break;
    case G_TYPE_ENUM: {
      BtPatternEditorColumnConvertersCallbacks *pcc;

      if(property->value_type==GSTBT_TYPE_TRIGGER_SWITCH) {
        col->type=PCT_SWITCH;
        col->min=GSTBT_TRIGGER_SWITCH_OFF;
        col->max=GSTBT_TRIGGER_SWITCH_ON;
        col->def=GSTBT_TRIGGER_SWITCH_EMPTY;
      }
      else {
        col->type=PCT_BYTE;
        col->min=g_value_get_enum(min_val);
        col->max=g_value_get_enum(max_val);
        col->def=BT_IS_GVALUE(no_val)?g_value_get_enum(no_val):col->max+1;
      }
      col->user_data=g_new(BtPatternEditorColumnConverters,1);
      pcc=(BtPatternEditorColumnConvertersCallbacks *)col->user_data;
      pcc->val_to_float=enum_val_to_float;
      pcc->float_to_str=any_float_to_str;
    } break;
    case G_TYPE_INT: {
      BtPatternEditorColumnConvertersCallbacks *pcc;

      col->type=PCT_WORD;
      col->min=g_value_get_int(min_val);
      col->max=g_value_get_int(max_val);
      col->def=BT_IS_GVALUE(no_val)?g_value_get_int(no_val):col->max+1;
      if(col->min>=0 && col->max<256) {
        col->type=PCT_BYTE;
      }
      col->user_data=g_new(BtPatternEditorColumnConverters,1);
      pcc=(BtPatternEditorColumnConvertersCallbacks *)col->user_data;
      pcc->val_to_float=int_val_to_float;
      pcc->float_to_str=any_float_to_str;
    } break;
    case G_TYPE_UINT: {
      BtPatternEditorColumnConvertersCallbacks *pcc;

      col->type=PCT_WORD;
      col->min=g_value_get_uint(min_val);
      col->max=g_value_get_uint(max_val);
      col->def=BT_IS_GVALUE(no_val)?g_value_get_uint(no_val):col->max+1;
      if(col->min>=0 && col->max<256) {
        col->type=PCT_BYTE;
      }
      col->user_data=g_new(BtPatternEditorColumnConverters,1);
      pcc=(BtPatternEditorColumnConvertersCallbacks *)col->user_data;
      pcc->val_to_float=uint_val_to_float;
      pcc->float_to_str=any_float_to_str;
    } break;
    case G_TYPE_FLOAT: {
      BtPatternEditorColumnConvertersFloatCallbacks *pcc;

      col->type=PCT_WORD;
      col->min=0.0;
      col->max=65535.0;
      col->def=col->max+1;
      col->user_data=g_new(BtPatternEditorColumnConverters,1);
      pcc=(BtPatternEditorColumnConvertersFloatCallbacks *)col->user_data;
      ((BtPatternEditorColumnConvertersCallbacks*)pcc)->val_to_float=float_val_to_float;
      ((BtPatternEditorColumnConvertersCallbacks*)pcc)->float_to_str=float_float_to_str;
      pcc->min=g_value_get_float(min_val);
      pcc->max=g_value_get_float(max_val);
      /* @todo: need scaling
       * - in case of
       *   wire.volume: 0.0->0x0000, 1.0->0x4000, 2.0->0x8000, 4.0->0xFFFF+1
       *    (see case G_TYPE_DOUBLE:)
       *   wire.panorama: -1.0->0x0000, 0.0->0x4000, 1.0->0x4000
       *   song.master_volume: 0db->0.0->0x0000, -80db->1/100.000.000->0x4000
       *     scaling_factor is not enough
       *     col->user_data=&pcc[2]; // log-map
       * - also showing gstreamer long desc for these elements is not so useful
       *
       * - we might need to put the scaling factor into the user_data
       * - how can we detect master-volume (log mapping)
       */
    } break;
    case G_TYPE_DOUBLE: {
      BtPatternEditorColumnConvertersFloatCallbacks *pcc;

      col->type=PCT_WORD;
      col->min=0.0;
      col->max=65535.0;
      col->def=col->max+1;
      col->user_data=g_new(BtPatternEditorColumnConverters,1);
      pcc=(BtPatternEditorColumnConvertersFloatCallbacks *)col->user_data;
      ((BtPatternEditorColumnConvertersCallbacks*)pcc)->val_to_float=double_val_to_float;
      ((BtPatternEditorColumnConvertersCallbacks*)pcc)->float_to_str=float_float_to_str;
      // identify wire-elements
      // this is not the best way to identify a volume element
      // gst_registry_find_feature() is better but too slow here
      if(!strcmp(g_type_name(property->owner_type),"GstVolume")) {
        pcc->min=0.0;
        pcc->max=4.0;
      }
      else {
        pcc->min=g_value_get_double(min_val);
        pcc->max=g_value_get_double(max_val);
      }
    } break;
    default:
      GST_WARNING("unhandled param type: '%s' for parameter '%s'",g_type_name(type),property->name);
      col->type=0;
      col->min=col->max=col->def=0;
      col->user_data=NULL;
  }
  GST_INFO("%s parameter '%s' min/max/def : %6.4lf/%6.4lf/%6.4lf",g_type_name(type), property->name, col->min,col->max,col->def);
  g_value_unset(min_val);
  g_value_unset(max_val);
  g_free(min_val);
  g_free(max_val);
}

static void pattern_table_clear(const BtMainPagePatterns *self) {
  gulong i,j;

  for(i=0;i<self->priv->number_of_groups;i++) {
    for(j=0;j<self->priv->param_groups[i].num_columns;j++) {
      // voice parameter groups are copies
      if(!((self->priv->param_groups[i].type==PGT_VOICE) && (self->priv->param_groups[i].user_data!=NULL)))
        g_free(self->priv->param_groups[i].columns[j].user_data);
    }
    g_free(self->priv->param_groups[i].name);
    g_free(self->priv->param_groups[i].columns);
  }
  g_free(self->priv->param_groups);
  self->priv->param_groups=NULL;
  //self->priv->number_of_groups=0;
}

static void pattern_table_refresh(const BtMainPagePatterns *self) {
  static BtPatternEditorCallbacks callbacks = {
    pattern_edit_get_data_at,
    pattern_edit_set_data_at
  };

  if(self->priv->pattern) {
    gulong i;
    gulong number_of_ticks,voices,global_params,voice_params;
    BtMachine *machine;
    BtPatternEditorColumnGroup *group;
    GParamSpec *property;
    GValue *min_val,*max_val,*no_val;

    g_object_get(self->priv->pattern,"length",&number_of_ticks,"voices",&voices,"machine",&machine,NULL);
    g_object_get(machine,"global-params",&global_params,"voice-params",&voice_params,NULL);
    GST_DEBUG("  size is %2lu,%2lu",number_of_ticks,global_params);

    pattern_table_clear(self);

    // wire pattern data
    self->priv->number_of_groups=(global_params>0?1:0)+voices;

    if(!BT_IS_SOURCE_MACHINE(machine)) {
      GList *node;
      BtWire *wire;
      gulong wire_params;

      // need to iterate over all inputs
      node=machine->dst_wires;
      self->priv->number_of_groups+=g_list_length(node);
      group=self->priv->param_groups=g_new(BtPatternEditorColumnGroup,self->priv->number_of_groups);

      GST_INFO("wire parameters");
      for(;node;node=g_list_next(node)) {
        BtMachine *src=NULL;

        wire=BT_WIRE(node->data);
        // check wire config
        g_object_get(wire,"num-params",&wire_params,"src",&src,NULL);
        group->type=PGT_WIRE;
        g_object_get(src,"id",&group->name,NULL),
        group->user_data=wire;
        group->num_columns=wire_params;
        group->columns=g_new(BtPatternEditorColumn,wire_params);
        group->width=0;
        for(i=0;i<wire_params;i++) {
          bt_wire_get_param_details(wire,i,&property,&min_val,&max_val);
          pattern_edit_fill_column_type(&group->columns[i],property,min_val,max_val,NULL);
        }
        g_object_unref(src);
        group++;
      }
    }
    else {
      group=self->priv->param_groups=g_new(BtPatternEditorColumnGroup,self->priv->number_of_groups);
    }
    if(global_params) {
      // create mapping for global params
      group->type=PGT_GLOBAL;
      /* label for first global parameter column in a pattern */
      group->name=g_strdup(_("Globals"));
      group->user_data=NULL;
      group->num_columns=global_params;
      group->columns=g_new(BtPatternEditorColumn,global_params);
      group->width=0;
      GST_INFO("global parameters");
      for(i=0;i<global_params;i++) {
        bt_machine_get_global_param_details(machine,i,&property,&min_val,&max_val);
      	no_val=bt_machine_get_global_param_no_value(machine,i);
        pattern_edit_fill_column_type(&group->columns[i],property,min_val,max_val,no_val);
        //g_value_unset(&no_val);
      }
      group++;
    }
    if(voices) {
      BtPatternEditorColumnGroup *stamp=group;
      // create mapping for voice params
      group->type=PGT_VOICE;
      /* label for parameters of first voice column in a pattern */
      group->name=g_strdup(_("Voice 1"));
      group->user_data=GUINT_TO_POINTER(0);
      group->num_columns=voice_params;
      group->columns=g_new(BtPatternEditorColumn,voice_params);
      group->width=0;
      GST_INFO("voice parameters");
      for(i=0;i<voice_params;i++) {
        bt_machine_get_voice_param_details(machine,i,&property,&min_val,&max_val);
      	no_val=bt_machine_get_voice_param_no_value(machine,i);
        pattern_edit_fill_column_type(&group->columns[i],property,min_val,max_val,no_val);
      }
      group++;
      for(i=1;i<voices;i++) {
        group->type=PGT_VOICE;
        /* label for parameters of voice columns in a pattern */
        group->name=g_strdup_printf(_("Voice %u"),(guint)(i+1));
        group->user_data=GUINT_TO_POINTER(i);
        group->num_columns=voice_params;
        group->columns=g_memdup(stamp->columns,sizeof(BtPatternEditorColumn)*voice_params);
        group->width=0;
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
  g_object_set(self->priv->pattern_table,"play-position",-1.0,NULL);
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
        gulong voices;

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
    GST_INFO("no machine");
    //gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu),FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_track_add),FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_track_remove),FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_pattern_properties),FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_pattern_remove),FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(self->priv->context_menu_pattern_copy),FALSE);
  }
}

static BtPattern *add_new_pattern(const BtMainPagePatterns *self,BtMachine *machine) {
  BtSong *song;
  BtSongInfo *song_info;
  BtPattern *pattern;
  gchar *mid,*id,*name;
  gulong bars;

  g_object_get(self->priv->app,"song",&song,NULL);
  g_object_get(song,"song-info",&song_info,NULL);
  g_object_get(song_info,"bars",&bars,NULL);
  g_object_get(machine,"id",&mid,NULL);

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

static BtPattern *get_current_pattern(const BtMainPagePatterns *self) {
  BtPattern *pattern;
  GtkTreeIter iter;
  GtkTreeModel *store;

  GST_INFO("get current pattern");

  if(self->priv->machine) {
    if(gtk_combo_box_get_active_iter(self->priv->pattern_menu,&iter)) {
      store=gtk_combo_box_get_model(self->priv->pattern_menu);
      if((pattern=bt_pattern_list_model_get_object(BT_PATTERN_LIST_MODEL(store),&iter))) {
        GST_DEBUG("  got pattern: %p,pattern-refs: %d",pattern,G_OBJECT_REF_COUNT(pattern));
        return(g_object_ref(pattern));
      }
    }
  }
  return(NULL);
}

static void change_current_pattern(const BtMainPagePatterns *self, BtPattern *new_pattern) {
  BtPattern *old_pattern=self->priv->pattern;

  if(new_pattern==old_pattern) {
    return;
  }

  self->priv->pattern=g_object_try_ref(new_pattern);
  if(old_pattern) {
    if(self->priv->pattern_length_changed) {
      g_signal_handler_disconnect(old_pattern,self->priv->pattern_length_changed);
      self->priv->pattern_length_changed=0;
    }
    if(self->priv->pattern_voices_changed) {
      g_signal_handler_disconnect(old_pattern,self->priv->pattern_voices_changed);
      self->priv->pattern_voices_changed=0;
    }
    GST_INFO("unref old pattern: %p,ref_count=%d",old_pattern,G_OBJECT_REF_COUNT(old_pattern));
    g_object_unref(old_pattern);
  }

  // select pattern combo entry
  if(new_pattern) {
    GtkTreeModel *store;
    GtkTreeIter iter;

    store=gtk_combo_box_get_model(self->priv->pattern_menu);
    // get the row where row.pattern==pattern
    pattern_menu_model_get_iter_by_pattern(store,&iter,new_pattern);
    gtk_combo_box_set_active_iter(self->priv->pattern_menu,&iter);

    GST_DEBUG("selecting new pattern");
  }

  // refresh pattern view
  GST_INFO("store new pattern : %p,ref_count=%d",new_pattern,G_OBJECT_REF_COUNT(new_pattern));
  pattern_table_refresh(self);
  pattern_view_update_column_description(self,UPDATE_COLUMN_UPDATE);
  gtk_widget_grab_focus_savely(GTK_WIDGET(self->priv->pattern_table));
  if(new_pattern) {
    // watch the pattern
    self->priv->pattern_length_changed=g_signal_connect(new_pattern,"notify::length",G_CALLBACK(on_pattern_size_changed),(gpointer)self);
    self->priv->pattern_voices_changed=g_signal_connect(new_pattern,"notify::voices",G_CALLBACK(on_pattern_size_changed),(gpointer)self);
  }
}

static BtMachine *get_current_machine(const BtMainPagePatterns *self) {
  BtMachine *machine;
  GtkTreeIter iter;
  GtkTreeModel *store;

  GST_INFO("get current machine");

  if(gtk_combo_box_get_active_iter(self->priv->machine_menu,&iter)) {
    store=gtk_combo_box_get_model(self->priv->machine_menu);
    if((machine=bt_machine_list_model_get_object(BT_MACHINE_LIST_MODEL(store),&iter))) {
      GST_DEBUG("  got machine: %p,machine-refs: %d",machine,G_OBJECT_REF_COUNT(machine));
      return(g_object_ref(machine));
    }
  }
  return(NULL);
}

static void change_current_machine(const BtMainPagePatterns *self, BtMachine *new_machine) {
  BtMachine *old_machine=self->priv->machine;

  if(new_machine==old_machine) {
    return;
  }

  GST_INFO("store new machine %p,ref_count=%d",new_machine,G_OBJECT_REF_COUNT(new_machine));
  self->priv->machine=g_object_try_ref(new_machine);

  GST_INFO("unref old machine %p,ref_count=%d",old_machine,G_OBJECT_REF_COUNT(old_machine));
  g_object_try_unref(old_machine);

  // show new list of pattern in pattern menu
  pattern_menu_refresh(self,new_machine);
  GST_INFO("1st done for  machine %p,ref_count=%d",new_machine,G_OBJECT_REF_COUNT(new_machine));
  // refresh context menu
  context_menu_refresh(self,new_machine);
  GST_INFO("2nd done for  machine %p,ref_count=%d",new_machine,G_OBJECT_REF_COUNT(new_machine));
}

static void switch_machine_and_pattern(const BtMainPagePatterns *self,BtMachine *machine, BtPattern *pattern) {
  GtkTreeIter iter;
  GtkTreeModel *store;

  if(machine) {
    // update machine menu
    store=gtk_combo_box_get_model(self->priv->machine_menu);
    machine_menu_model_get_iter_by_machine(store,&iter,machine);
    gtk_combo_box_set_active_iter(self->priv->machine_menu,&iter);
  }
  if(pattern) {
    // update pattern menu
    store=gtk_combo_box_get_model(self->priv->pattern_menu);
    pattern_menu_model_get_iter_by_pattern(store,&iter,pattern);
    gtk_combo_box_set_active_iter(self->priv->pattern_menu,&iter);
  }
}

static void lookup_machine_and_pattern(const BtMainPagePatterns *self,BtMachine **machine, BtPattern **pattern,gchar *mid, gchar *c_mid,gchar *pid, gchar *c_pid) {
  if(!c_mid || strcmp(mid,c_mid)) {
    BtSong *song;
    BtSetup *setup;
    // change machine and pattern
    g_object_get(self->priv->app,"song",&song,NULL);
    g_object_get(song,"setup",&setup,NULL);
    g_object_try_unref(*machine);
    *machine=bt_setup_get_machine_by_id(setup,mid);
    if (pid) {
      g_object_try_unref(*pattern);
      *pattern=bt_machine_get_pattern_by_id(*machine,pid);
      switch_machine_and_pattern(self,*machine,*pattern);
    }
    g_object_unref(setup);
    g_object_unref(song);
  } else if(pid && (!c_pid || strcmp(pid,c_pid))) {
    // change pattern
    g_object_try_unref(*pattern);
    *pattern=bt_machine_get_pattern_by_id(*machine,pid);
    switch_machine_and_pattern(self,NULL,*pattern);
  }
}

//-- event handler

static void on_page_switched(GtkNotebook *notebook, GParamSpec *arg, gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  BtMainWindow *main_window;
  guint page_num;
  static gint prev_page_num=-1;

  g_object_get(notebook,"page",&page_num,NULL);

  if(page_num==BT_MAIN_PAGES_PATTERNS_PAGE) {
    // only do this if the page really has changed
    if(prev_page_num != BT_MAIN_PAGES_PATTERNS_PAGE) {
      BtSong *song;

      GST_DEBUG("enter pattern page");
      if(self->priv->machine) {
        // refresh to update colors in the menu (as usage might have changed)
        pattern_menu_refresh(self,self->priv->machine);
      }
      // add local commands
      g_object_get(self->priv->app,"main-window",&main_window,"song",&song,NULL);
      if(main_window) {
        gtk_window_add_accel_group(GTK_WINDOW(main_window),self->priv->accel_group);
        g_object_unref(main_window);
      }
      if(song) {
        g_object_set(song,"is-idle",self->priv->play_live,NULL);
        g_object_unref(song);
      }
    }
  }
  else {
    // only do this if the page was BT_MAIN_PAGES_PATTERNS_PAGE
    if(prev_page_num == BT_MAIN_PAGES_PATTERNS_PAGE) {
      BtSong *song;

      // only reset old
      GST_DEBUG("leave pattern page");
      pattern_view_update_column_description(self,UPDATE_COLUMN_POP);
      // remove local commands
      g_object_get(self->priv->app,"main-window",&main_window,"song",&song,NULL);
      if(main_window) {
        gtk_window_remove_accel_group(GTK_WINDOW(main_window),self->priv->accel_group);
        g_object_unref(main_window);
      }
      if(song) {
        g_object_set(song,"is-idle",FALSE,NULL);
        g_object_unref(song);
      }
    }
  }
  prev_page_num = page_num;
}

static void on_pattern_size_changed(BtPattern *pattern,GParamSpec *arg,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);

  GST_INFO("pattern size changed : %p",self->priv->pattern);
  pattern_table_refresh(self);
  pattern_view_update_column_description(self,UPDATE_COLUMN_UPDATE);
  gtk_widget_grab_focus_savely(GTK_WIDGET(self->priv->pattern_table));
}

static void on_pattern_menu_changed(GtkComboBox *menu, gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  BtPattern *pattern;

  // refresh pattern view
  GST_INFO("unref'ed old pattern: %p,refs=%d",
    self->priv->pattern,G_OBJECT_REF_COUNT(self->priv->pattern));
  pattern=get_current_pattern(self);
  change_current_pattern(self,pattern);
  g_object_try_unref(pattern);
  GST_INFO("ref'ed new pattern: %p,refs=%d",
    self->priv->pattern,G_OBJECT_REF_COUNT(self->priv->pattern));
}

/*
static void on_wavetable_menu_changed(GtkComboBox *menu, gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  guint wave_ix;

  wave_ix=gtk_combo_box_get_active(self->priv->wavetable_menu);
}
*/

static void on_base_octave_menu_changed(GtkComboBox *menu, gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);

  self->priv->base_octave=gtk_combo_box_get_active(self->priv->base_octave_menu);
  g_object_set(self->priv->pattern_table,"octave",self->priv->base_octave,NULL);
}

static void on_play_live_toggled(GtkButton *button, gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  BtSong *song;

  self->priv->play_live=gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(button));

  g_object_get(self->priv->app,"song",&song,NULL);
  g_object_set(song,"is-idle",self->priv->play_live,NULL);
  g_object_unref(song);
}

static void on_toolbar_menu_clicked(GtkButton *button, gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);

  gtk_menu_popup(self->priv->context_menu,NULL,NULL,NULL,NULL,1,gtk_get_current_event_time());
}

static void on_machine_added(BtSetup *setup,BtMachine *machine,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);

  GST_INFO("new machine %p,ref_count=%d has been added",machine,G_OBJECT_REF_COUNT(machine));

  if(bt_change_log_is_active(self->priv->change_log)) {
    if(BT_IS_SOURCE_MACHINE(machine)) {
      BtPattern *pattern=add_new_pattern(self, machine);
      context_menu_refresh(self,machine);
      g_object_unref(pattern);
    }
  }

  //g_signal_connect(machine,"pattern-added",G_CALLBACK(on_pattern_added),(gpointer)self);
  g_signal_connect(machine,"pattern-removed",G_CALLBACK(on_pattern_removed),(gpointer)self);

  GST_INFO("... machine %p,ref_count=%d has been added",machine,G_OBJECT_REF_COUNT(machine));
}

static void on_machine_removed(BtSetup *setup,BtMachine *machine,gpointer user_data) {
  //BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  BtPattern *pattern=NULL;
  GList *node,*list;
  gboolean is_internal;

  g_return_if_fail(BT_IS_MACHINE(machine));

  GST_INFO("machine %p,ref_count=%d has been removed",machine,G_OBJECT_REF_COUNT(machine));

  // remove all patterns to ensure we emit "pattern-removed" signals
  g_object_get(machine,"patterns",&list,NULL);
  for(node=list;node;node=g_list_next(node)) {
    pattern=BT_PATTERN(node->data);
    g_object_get(pattern,"is-internal",&is_internal,NULL);
    if(!is_internal) {
      GST_DEBUG("removing pattern: %p,ref_count=%d",pattern,G_OBJECT_REF_COUNT(pattern));
      bt_machine_remove_pattern(machine,pattern);
      GST_DEBUG("removed pattern: %p,ref_count=%d",pattern,G_OBJECT_REF_COUNT(pattern));
    } else {
      GST_DEBUG("keeping pattern: %p,ref_count=%d",pattern,G_OBJECT_REF_COUNT(pattern));
    }
    g_object_unref(pattern);
  }
  g_list_free(list);
}

static void on_wire_added_or_removed(BtSetup *setup,BtWire *wire,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  BtMachine *this_machine,*that_machine;

  if(!self->priv->pattern) return;

  g_object_get(self->priv->pattern,"machine",&this_machine,NULL);
  g_object_get(wire,"dst",&that_machine,NULL);
  if(this_machine==that_machine) {
    pattern_table_refresh(self);
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

  machine=get_current_machine(self);
  GST_DEBUG("machine_menu changed, new machine is %s",(machine?GST_OBJECT_NAME(machine):""));
  change_current_machine(self,machine);
  g_object_try_unref(machine);
}

static void on_sequence_tick(const BtSong *song,GParamSpec *arg,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  BtSequence *sequence;
  BtMachine *machine,*cur_machine=self->priv->machine;
  BtPattern *pattern;
  gulong i,pos,spos;
  glong j;
  gulong tracks,length,sequence_length;
  gdouble play_pos;
  gboolean found=FALSE;

  if(!self->priv->pattern) return;

  g_object_get(self->priv->pattern,"length",&length,NULL);
  g_object_get((gpointer)song,"sequence",&sequence,"play-pos",&pos,NULL);
  g_object_get(sequence,"tracks",&tracks,"length",&sequence_length,NULL);

  if(pos<sequence_length) {
    // check all tracks
    // @todo: what if the pattern is played on multiple tracks ?
    // we'd need to draw several lines
    for(i=0;((i<tracks) && !found);i++) {
      machine=bt_sequence_get_machine(sequence,i);
      if(machine==cur_machine) {
        // find pattern start in sequence (search <length> ticks from current pos
        spos=(pos>length)?(pos-length):0;
        for(j=pos;((j>spos) && !found);j--) {
          // get pattern for current machine and current tick from sequence
          if((pattern=bt_sequence_get_pattern(sequence,j,i))) {
            // if it is the pattern we currently show, set play-line
            if(pattern==self->priv->pattern) {
              play_pos=(gdouble)(pos-j)/(gdouble)length;
              g_object_set(self->priv->pattern_table,"play-position",play_pos,NULL);
              found=TRUE;
            }
            g_object_unref(pattern);
            /* if there was a different pattern, don't look further */
            break;
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
}

static void on_song_changed(const BtEditApplication *app,GParamSpec *arg,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  BtSong *song;
  BtSetup *setup;
  BtWavetable *wavetable;

  GST_INFO("song has changed : app=%p, self=%p",app,self);
  // get song from app and then setup from song
  g_object_get(self->priv->app,"song",&song,NULL);
  if(!song) return;
  GST_INFO("song->ref_ct=%d",G_OBJECT_REF_COUNT(song));

  g_object_get(song,"setup",&setup,"wavetable",&wavetable,NULL);
  // update page
  machine_menu_refresh(self,setup);
  //pattern_menu_refresh(self); // should be triggered by machine_menu_refresh()
  wavetable_menu_refresh(self,wavetable);
  g_signal_connect(setup,"machine-added",G_CALLBACK(on_machine_added),(gpointer)self);
  g_signal_connect(setup,"machine-removed",G_CALLBACK(on_machine_removed),(gpointer)self);
  g_signal_connect(setup,"wire-added",G_CALLBACK(on_wire_added_or_removed),(gpointer)self);
  g_signal_connect(setup,"wire-removed",G_CALLBACK(on_wire_added_or_removed),(gpointer)self);
  g_signal_connect(wavetable,"wave-added",G_CALLBACK(on_wave_added_or_removed),(gpointer)self);
  g_signal_connect(wavetable,"wave-removed",G_CALLBACK(on_wave_added_or_removed),(gpointer)self);
  // subscribe to play-pos changes of song->sequence
  g_signal_connect(song, "notify::play-pos", G_CALLBACK(on_sequence_tick), (gpointer)self);
  // release the references
  g_object_unref(wavetable);
  g_object_unref(setup);
  g_object_unref(song);
  GST_INFO("song has changed done");
}

static void on_context_menu_track_add_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  gint voices;

  g_object_get(self->priv->machine,"voices",&voices,NULL);
  voices++;
  g_object_set(self->priv->machine,"voices",voices,NULL);
  // we adjust sensitivity of add/rem track menu items
  context_menu_refresh(self,self->priv->machine);

  /* FIXME: undo/redo
   * undo: remove-track
   */
}

static void on_context_menu_track_remove_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  gint voices;

  g_object_get(self->priv->machine,"voices",&voices,NULL);
  voices--;
  g_object_set(self->priv->machine,"voices",voices,NULL);

  // we adjust sensitivity of add/rem track menu items
  context_menu_refresh(self,self->priv->machine);

  /* FIXME: undo/redo
   * undo: save voice-data for *all* patterns of this machine
   * -> pattern_range_copy() and pattern_range_log_undo_redo()
   */

}

static void on_context_menu_pattern_new_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  BtPattern *pattern;
  GtkWidget *dialog;

  // new_pattern
  pattern=add_new_pattern(self,self->priv->machine);

  // pattern_properties
  dialog=GTK_WIDGET(bt_pattern_properties_dialog_new(pattern));
  bt_edit_application_attach_child_window(self->priv->app,GTK_WINDOW(dialog));
  gtk_widget_show_all(dialog);

  if(gtk_dialog_run(GTK_DIALOG(dialog))==GTK_RESPONSE_ACCEPT) {
    gchar *undo_str,*redo_str;
    gchar *mid,*pid,*pname;
    gulong length;

    bt_pattern_properties_dialog_apply(BT_PATTERN_PROPERTIES_DIALOG(dialog));

    GST_INFO("new pattern added : %p,ref_count=%d",pattern,G_OBJECT_REF_COUNT(pattern));

    g_object_get(self->priv->machine,"id",&mid,NULL);
    g_object_get(pattern,"id",&pid,"name",&pname,"length",&length,NULL);

    undo_str = g_strdup_printf("rem_pattern \"%s\",\"%s\"",mid,pid);
    redo_str = g_strdup_printf("add_pattern \"%s\",\"%s\",\"%s\",%lu",mid,pid,pname,length);
    bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,redo_str);
    g_free(mid);g_free(pid);g_free(pname);

    change_current_pattern(self,pattern);
    context_menu_refresh(self,self->priv->machine);
  }
  else {
    bt_machine_remove_pattern(self->priv->machine,pattern);
  }
  gtk_widget_destroy(dialog);

  // free ressources
  g_object_unref(pattern);
  GST_DEBUG("new pattern done");
}

static void on_context_menu_pattern_properties_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  GtkWidget *dialog;

  g_return_if_fail(self->priv->pattern);

  // pattern_properties
  dialog=GTK_WIDGET(bt_pattern_properties_dialog_new(self->priv->pattern));
  bt_edit_application_attach_child_window(self->priv->app,GTK_WINDOW(dialog));
  gtk_widget_show_all(dialog);

  if(gtk_dialog_run(GTK_DIALOG(dialog))==GTK_RESPONSE_ACCEPT) {
  	BtMachine *machine;
  	gchar *new_name, *old_name;
  	gulong new_length, old_length, new_voices, old_voices;
  	gchar *undo_str,*redo_str;
  	gchar *mid,*pid;

    /* FIXME: undo/redo
     * here, we don't know what is going to be changed :/
     *   a) add method here and set it this way
     *   b) catch g_object_set? - won't work, as we need it before
     *   c) make name/length/voices properties on the dialog
     *      and move the bt_pattern_properties_dialog_apply() here.
     *   d) make a tmp copy of the pattern (need to pick an 'internal' name)
     *      and take the old data from it
     *   E) make name/length/voices properties on the dialog
     *      and check them from here
     */
    g_object_get(self->priv->pattern,"id",&pid,"name",&old_name,"length",&old_length,"voices",&old_voices,"machine",&machine,NULL);
    g_object_get(dialog,"name",&new_name,"length",&new_length,"voices",&new_voices,NULL);
    g_object_get(machine,"id",&mid,NULL);
    
		bt_change_log_start_group(self->priv->change_log);

    if(strcmp(old_name,new_name)) {
			undo_str = g_strdup_printf("set_pattern_property \"%s\",\"%s\",\"name\",\"%s\"",mid,pid,old_name);
			redo_str = g_strdup_printf("set_pattern_property \"%s\",\"%s\",\"name\",\"%s\"",mid,pid,new_name);
			bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,redo_str);
    }
    if(old_length!=new_length) {
			undo_str = g_strdup_printf("set_pattern_property \"%s\",\"%s\",\"length\",\"%lu\"",mid,pid,old_length);
			redo_str = g_strdup_printf("set_pattern_property \"%s\",\"%s\",\"length\",\"%lu\"",mid,pid,new_length);
			bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,redo_str);
      if(old_length>new_length) {
      	GString *old_data=g_string_new(NULL);
      	pattern_range_copy(self,new_length,old_length-1,-1,-1,old_data);
        pattern_range_log_undo_redo(self,new_length,old_length-1,-1,-1,old_data->str,g_strdup(old_data->str));
				g_string_free(old_data,TRUE);
      }
    }
    /* if old_voices<voices
     *   set the property for machine
     * else
     *   save regions for *all* patterns
     *   set the property for machine
     * // we might need to do that from the machine-page
     * // maybe the voices parameter needs to be moved
     */

    bt_change_log_end_group(self->priv->change_log);

    bt_pattern_properties_dialog_apply(BT_PATTERN_PROPERTIES_DIALOG(dialog));
    g_free(mid);g_free(pid);g_free(old_name);g_free(new_name);
  }
  gtk_widget_destroy(dialog);
}

static void on_context_menu_pattern_remove_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  BtPattern *pattern=self->priv->pattern;
	BtMachine *machine;

  g_return_if_fail(pattern);

	g_object_get(pattern,"machine",&machine,NULL);

	bt_change_log_start_group(self->priv->change_log);
	bt_machine_remove_pattern(machine,pattern);
	bt_change_log_end_group(self->priv->change_log);

	change_current_pattern(self,NULL);
	context_menu_refresh(self,machine);

	g_object_unref(machine);
}

static void on_context_menu_pattern_copy_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  BtMachine *machine;
  BtPattern *pattern;
  GtkWidget *dialog;

  g_return_if_fail(self->priv->pattern);

  // copy pattern
  pattern=bt_pattern_copy(self->priv->pattern);
  g_return_if_fail(pattern);
  g_object_get(pattern,"machine",&machine,NULL);

  // pattern_properties
  dialog=GTK_WIDGET(bt_pattern_properties_dialog_new(pattern));
  bt_edit_application_attach_child_window(self->priv->app,GTK_WINDOW(dialog));
  gtk_widget_show_all(dialog);

  if(gtk_dialog_run(GTK_DIALOG(dialog))==GTK_RESPONSE_ACCEPT) {
    gchar *undo_str,*redo_str;
    gchar *mid,*pid,*pname;
    gulong length;

    bt_pattern_properties_dialog_apply(BT_PATTERN_PROPERTIES_DIALOG(dialog));

    GST_INFO("new pattern added : %p",pattern);

    g_object_get(machine,"id",&mid,NULL);
    g_object_get(pattern,"id",&pid,"name",&pname,"length",&length,NULL);

    undo_str = g_strdup_printf("rem_pattern \"%s\",\"%s\"",mid,pid);
    redo_str = g_strdup_printf("add_pattern \"%s\",\"%s\",\"%s\",%lu",mid,pid,pname,length);
    bt_change_log_add(self->priv->change_log,BT_CHANGE_LOGGER(self),undo_str,redo_str);
    g_free(mid);g_free(pid);g_free(pname);

    change_current_pattern(self,pattern);
    context_menu_refresh(self,machine);
  }
  else {
    bt_machine_remove_pattern(machine,pattern);
  }
  gtk_widget_destroy(dialog);

  g_object_unref(pattern);
  g_object_unref(machine);
}

static void on_context_menu_machine_properties_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);

  bt_machine_show_properties_dialog(self->priv->machine);
}

static void on_context_menu_machine_preferences_activate(GtkMenuItem *menuitem,gpointer user_data) {
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);

  bt_machine_show_preferences_dialog(self->priv->machine);
}

static void on_toolbar_style_changed(const BtSettings *settings,GParamSpec *arg,gpointer user_data) {
  GtkToolbar *toolbar=GTK_TOOLBAR(user_data);
  gchar *toolbar_style;

  g_object_get((gpointer)settings,"toolbar-style",&toolbar_style,NULL);
  if(!BT_IS_STRING(toolbar_style)) return;

  GST_INFO("!!!  toolbar style has changed '%s'", toolbar_style);
  gtk_toolbar_set_style(toolbar,gtk_toolbar_get_style_from_string(toolbar_style));
  g_free(toolbar_style);
}

//-- helper methods

static void bt_main_page_patterns_init_ui(const BtMainPagePatterns *self,const BtMainPages *pages) {
  GtkWidget *toolbar,*tool_item,*box;
  GtkWidget *scrolled_window;
  GtkWidget *menu_item,*image;
  GtkCellRenderer *renderer;
  BtSettings *settings;
  gint i;
  gchar oct_str[2];

  GST_DEBUG("!!!! self=%p",self);

  gtk_widget_set_name(GTK_WIDGET(self),"pattern view");

  // add toolbar
  toolbar=gtk_toolbar_new();
  gtk_widget_set_name(toolbar,"pattern view toolbar");
  gtk_box_pack_start(GTK_BOX(self),toolbar,FALSE,FALSE,0);
  gtk_toolbar_set_style(GTK_TOOLBAR(toolbar),GTK_TOOLBAR_BOTH);

  // add toolbar widgets
  // machine select
  box=gtk_hbox_new(FALSE,2);
  gtk_container_set_border_width(GTK_CONTAINER(box),4);
  self->priv->machine_menu=GTK_COMBO_BOX(gtk_combo_box_new());
  gtk_combo_box_set_focus_on_click(self->priv->machine_menu,FALSE);
  renderer=gtk_cell_renderer_pixbuf_new ();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(self->priv->machine_menu),renderer,FALSE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(self->priv->machine_menu),renderer,"pixbuf",BT_MACHINE_MODEL_ICON,NULL);
  renderer=gtk_cell_renderer_text_new();
  //gtk_cell_renderer_set_fixed_size(renderer, 1, -1);
  gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer), 1);
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(self->priv->machine_menu),renderer,TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(self->priv->machine_menu),renderer,"text",BT_MACHINE_MODEL_LABEL,NULL);
  g_signal_connect(self->priv->machine_menu, "changed", G_CALLBACK(on_machine_menu_changed), (gpointer)self);
  /* this won't work, as we can't pass anything to the event handler
   * gtk_widget_add_accelerator(self->priv->machine_menu, "key-press-event", accel_group, GDK_Cursor_Up, GDK_CONTROL_MASK, 0);
   * so, we need to subclass the combobox and add two signals: select-next, select-prev
   */

  gtk_box_pack_start(GTK_BOX(box),gtk_label_new(_("Machine")),FALSE,FALSE,2);
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->priv->machine_menu),TRUE,TRUE,2);

  tool_item=GTK_WIDGET(gtk_tool_item_new());
  gtk_widget_set_name(tool_item,"Machine");
  gtk_container_add(GTK_CONTAINER(tool_item),box);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar),GTK_TOOL_ITEM(tool_item),-1);

#ifndef USE_HILDON
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar),gtk_separator_tool_item_new(),-1);
#endif

  // pattern select
  box=gtk_hbox_new(FALSE,2);
  gtk_container_set_border_width(GTK_CONTAINER(box),4);
  self->priv->pattern_menu=GTK_COMBO_BOX(gtk_combo_box_new());
  gtk_combo_box_set_focus_on_click(self->priv->pattern_menu,FALSE);
  renderer=gtk_cell_renderer_text_new();
  //gtk_cell_renderer_set_fixed_size(renderer, 1, -1);
  gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer), 1);
  g_object_set(renderer,
    "foreground","gray",
    NULL);
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(self->priv->pattern_menu),renderer,TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(self->priv->pattern_menu),renderer,
    "text",BT_PATTERN_MODEL_LABEL,
    "foreground-set",BT_PATTERN_MODEL_IS_UNUSED,
    NULL);
  gtk_box_pack_start(GTK_BOX(box),gtk_label_new(_("Pattern")),FALSE,FALSE,2);
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->priv->pattern_menu),TRUE,TRUE,2);
  self->priv->pattern_menu_changed=g_signal_connect(self->priv->pattern_menu, "changed", G_CALLBACK(on_pattern_menu_changed), (gpointer)self);

  tool_item=GTK_WIDGET(gtk_tool_item_new());
  gtk_widget_set_name(tool_item,"Pattern");
  gtk_container_add(GTK_CONTAINER(tool_item),box);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar),GTK_TOOL_ITEM(tool_item),-1);

#ifndef USE_HILDON
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar),gtk_separator_tool_item_new(),-1);
#endif

  // add wavetable entry select
  box=gtk_hbox_new(FALSE,2);
  gtk_container_set_border_width(GTK_CONTAINER(box),4);
  self->priv->wavetable_menu=GTK_COMBO_BOX(gtk_combo_box_new());
  gtk_combo_box_set_focus_on_click(self->priv->wavetable_menu,FALSE);
  renderer=gtk_cell_renderer_text_new();
  //gtk_cell_renderer_set_fixed_size(renderer, 1, -1);
  gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer), 1);
  g_object_set(renderer, "width", 22, NULL);
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(self->priv->wavetable_menu),renderer,FALSE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(self->priv->wavetable_menu),renderer,"text",WAVE_MENU_NUMBER,NULL);
  renderer=gtk_cell_renderer_text_new();
  //gtk_cell_renderer_set_fixed_size(renderer, 1, -1);
  gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer), 1);
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(self->priv->wavetable_menu),renderer,TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(self->priv->wavetable_menu),renderer,"text",WAVE_MENU_LABEL,NULL);
  gtk_box_pack_start(GTK_BOX(box),gtk_label_new(_("Wave")),FALSE,FALSE,2);
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->priv->wavetable_menu),TRUE,TRUE,2);
  //g_signal_connect(self->priv->wavetable_menu, "changed", G_CALLBACK(on_wavetable_menu_changed), (gpointer)self);

  tool_item=GTK_WIDGET(gtk_tool_item_new());
  gtk_widget_set_name(tool_item,"Wave");
  gtk_container_add(GTK_CONTAINER(tool_item),box);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar),GTK_TOOL_ITEM(tool_item),-1);

#ifndef USE_HILDON
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar),gtk_separator_tool_item_new(),-1);
#endif

  // add base octave (0-8)
  box=gtk_hbox_new(FALSE,2);
  gtk_container_set_border_width(GTK_CONTAINER(box),4);
  self->priv->base_octave_menu=GTK_COMBO_BOX(gtk_combo_box_new_text());
  gtk_combo_box_set_focus_on_click(self->priv->base_octave_menu,FALSE);
  for(i=0;i<8;i++) {
    sprintf(oct_str,"%1d",i);
    gtk_combo_box_append_text(self->priv->base_octave_menu,oct_str);
  }
  gtk_combo_box_set_active(self->priv->base_octave_menu,self->priv->base_octave);
  gtk_box_pack_start(GTK_BOX(box),gtk_label_new(_("Octave")),FALSE,FALSE,2);
  gtk_box_pack_start(GTK_BOX(box),GTK_WIDGET(self->priv->base_octave_menu),TRUE,TRUE,2);
  g_signal_connect(self->priv->base_octave_menu, "changed", G_CALLBACK(on_base_octave_menu_changed), (gpointer)self);

  tool_item=GTK_WIDGET(gtk_tool_item_new());
  gtk_widget_set_name(tool_item,"Octave");
  gtk_container_add(GTK_CONTAINER(tool_item),box);
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar),GTK_TOOL_ITEM(tool_item),-1);

#ifndef USE_HILDON
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar),gtk_separator_tool_item_new(),-1);
#endif

  // add play live toggle tool button
  tool_item=GTK_WIDGET(gtk_toggle_tool_button_new());
  gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(tool_item),gtk_image_new_from_filename("stock_volume.png"));
  gtk_tool_button_set_label(GTK_TOOL_BUTTON(tool_item),_("Play live"));
  gtk_widget_set_name(tool_item,"Play live");
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM(tool_item),_("Play notes and triggers while editing the pattern"));
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(tool_item,"toggled",G_CALLBACK(on_play_live_toggled),(gpointer)self);

#ifndef USE_HILDON
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar),gtk_separator_tool_item_new(),-1);
#endif

  // popup menu button
  image=gtk_image_new_from_filename("popup-menu.png");
  tool_item=GTK_WIDGET(gtk_tool_button_new(image,_("Pattern view menu")));
  gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM(tool_item),_("Menu actions for pattern view below"));
  gtk_toolbar_insert(GTK_TOOLBAR(toolbar),GTK_TOOL_ITEM(tool_item),-1);
  g_signal_connect(tool_item,"clicked",G_CALLBACK(on_toolbar_menu_clicked),(gpointer)self);


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
  g_signal_connect(self->priv->pattern_table, "key-press-event", G_CALLBACK(on_pattern_table_key_press_event), (gpointer)self);
  g_signal_connect(self->priv->pattern_table, "button-press-event", G_CALLBACK(on_pattern_table_button_press_event), (gpointer)self);
  g_signal_connect(self->priv->pattern_table, "notify::cursor-group", G_CALLBACK(on_pattern_table_cursor_group_changed), (gpointer)self);
  g_signal_connect(self->priv->pattern_table, "notify::cursor-param", G_CALLBACK(on_pattern_table_cursor_param_changed), (gpointer)self);
  g_signal_connect(self->priv->pattern_table, "notify::cursor-row", G_CALLBACK(on_pattern_table_cursor_row_changed), (gpointer)self);
  gtk_container_add(GTK_CONTAINER(self), GTK_WIDGET(scrolled_window));

  gtk_container_add(GTK_CONTAINER(scrolled_window),GTK_WIDGET(self->priv->pattern_table));
  gtk_widget_set_name(GTK_WIDGET(self->priv->pattern_table),"pattern editor");

  // generate the context menu
  self->priv->accel_group=gtk_accel_group_new();
  self->priv->context_menu=GTK_MENU(g_object_ref_sink(gtk_menu_new()));
  gtk_menu_set_accel_group(GTK_MENU(self->priv->context_menu), self->priv->accel_group);
  gtk_menu_set_accel_path(GTK_MENU(self->priv->context_menu),"<Buzztard-Main>/PatternView/PatternContext");

  self->priv->context_menu_track_add=menu_item=gtk_image_menu_item_new_with_label(_("New track"));
  image=gtk_image_new_from_stock(GTK_STOCK_ADD,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_menu_item_set_accel_path (GTK_MENU_ITEM (menu_item), "<Buzztard-Main>/PatternView/PatternContext/AddTrack");
  gtk_accel_map_add_entry ("<Buzztard-Main>/PatternView/PatternContext/AddTrack", GDK_plus, GDK_CONTROL_MASK);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(menu_item,"activate",G_CALLBACK(on_context_menu_track_add_activate),(gpointer)self);

  self->priv->context_menu_track_remove=menu_item=gtk_image_menu_item_new_with_label(_("Remove last track"));
  image=gtk_image_new_from_stock(GTK_STOCK_REMOVE,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_menu_item_set_accel_path (GTK_MENU_ITEM (menu_item), "<Buzztard-Main>/PatternView/PatternContext/RemoveTrack");
  gtk_accel_map_add_entry ("<Buzztard-Main>/PatternView/PatternContext/RemoveTrack", GDK_minus, GDK_CONTROL_MASK);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(menu_item,"activate",G_CALLBACK(on_context_menu_track_remove_activate),(gpointer)self);

  menu_item=gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);

  menu_item=gtk_image_menu_item_new_with_label(_("New pattern ..."));
  image=gtk_image_new_from_stock(GTK_STOCK_NEW,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_menu_item_set_accel_path (GTK_MENU_ITEM (menu_item), "<Buzztard-Main>/PatternView/PatternContext/NewPattern");
  gtk_accel_map_add_entry ("<Buzztard-Main>/PatternView/PatternContext/NewPattern", GDK_Return, GDK_CONTROL_MASK);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(menu_item,"activate",G_CALLBACK(on_context_menu_pattern_new_activate),(gpointer)self);

  self->priv->context_menu_pattern_properties=menu_item=gtk_image_menu_item_new_with_label(_("Pattern properties..."));
  image=gtk_image_new_from_stock(GTK_STOCK_PROPERTIES,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_menu_item_set_accel_path (GTK_MENU_ITEM (menu_item), "<Buzztard-Main>/PatternView/PatternContext/PatternProperties");
  gtk_accel_map_add_entry ("<Buzztard-Main>/PatternView/PatternContext/PatternProperties", GDK_BackSpace, GDK_CONTROL_MASK);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(menu_item,"activate",G_CALLBACK(on_context_menu_pattern_properties_activate),(gpointer)self);

  self->priv->context_menu_pattern_remove=menu_item=gtk_image_menu_item_new_with_label(_("Remove pattern..."));
  image=gtk_image_new_from_stock(GTK_STOCK_DELETE,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_menu_item_set_accel_path (GTK_MENU_ITEM (menu_item), "<Buzztard-Main>/PatternView/PatternContext/RemovePattern");
  gtk_accel_map_add_entry ("<Buzztard-Main>/PatternView/PatternContext/RemovePattern", GDK_Delete, GDK_CONTROL_MASK);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(menu_item,"activate",G_CALLBACK(on_context_menu_pattern_remove_activate),(gpointer)self);

  self->priv->context_menu_pattern_copy=menu_item=gtk_image_menu_item_new_with_label(_("Copy pattern..."));
  image=gtk_image_new_from_stock(GTK_STOCK_COPY,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_menu_item_set_accel_path (GTK_MENU_ITEM (menu_item), "<Buzztard-Main>/PatternView/PatternContext/CopyPattern");
  gtk_accel_map_add_entry ("<Buzztard-Main>/PatternView/PatternContext/CopyPattern", GDK_Return, GDK_CONTROL_MASK|GDK_SHIFT_MASK);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(menu_item,"activate",G_CALLBACK(on_context_menu_pattern_copy_activate),(gpointer)self);

  menu_item=gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);

  menu_item=gtk_image_menu_item_new_with_label(_("Machine properties"));  // dynamic part
  image=gtk_image_new_from_stock(GTK_STOCK_PROPERTIES,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(menu_item,"activate",G_CALLBACK(on_context_menu_machine_properties_activate),(gpointer)self);

  menu_item=gtk_image_menu_item_new_with_label(_("Machine preferences"));  // static part
  image=gtk_image_new_from_stock(GTK_STOCK_PREFERENCES,GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),image);
  gtk_menu_shell_append(GTK_MENU_SHELL(self->priv->context_menu),menu_item);
  gtk_widget_show(menu_item);
  g_signal_connect(menu_item,"activate",G_CALLBACK(on_context_menu_machine_preferences_activate),(gpointer)self);

  // --
  // @todo solo, mute, bypass
  // --
  // @todo cut, copy, paste

  // register event handlers
  g_signal_connect((gpointer)(self->priv->app), "notify::song", G_CALLBACK(on_song_changed), (gpointer)self);
  // listen to page changes
  g_signal_connect((gpointer)pages,"notify::page",G_CALLBACK(on_page_switched),(gpointer)self);

  // let settings control toolbar style
  g_object_get(self->priv->app,"settings",&settings,NULL);
  on_toolbar_style_changed(settings,NULL,(gpointer)toolbar);
  g_signal_connect(settings,"notify::toolbar-style",G_CALLBACK(on_toolbar_style_changed),(gpointer)toolbar);
  g_object_unref(settings);

  GST_DEBUG("  done");
}

//-- constructor methods

/**
 * bt_main_page_patterns_new:
 * @pages: the page collection
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtMainPagePatterns *bt_main_page_patterns_new(const BtMainPages *pages) {
  BtMainPagePatterns *self;

  self=BT_MAIN_PAGE_PATTERNS(g_object_new(BT_TYPE_MAIN_PAGE_PATTERNS,NULL));
  bt_main_page_patterns_init_ui(self,pages);
  return(self);
}

//-- methods

/**
 * bt_main_page_patterns_show_pattern:
 * @self: the pattern subpage
 * @pattern: the pattern to show
 *
 * Show the given @pattern. Will update machine and pattern menu.
 */
void bt_main_page_patterns_show_pattern(const BtMainPagePatterns *self,BtPattern *pattern) {
  BtMachine *machine;

  g_object_get(pattern,"machine",&machine,NULL);
  switch_machine_and_pattern(self,machine,pattern);
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
  machine_menu_model_get_iter_by_machine(store,&iter,machine);
  gtk_combo_box_set_active_iter(self->priv->machine_menu,&iter);
  // focus pattern editor
  gtk_widget_grab_focus_savely(GTK_WIDGET(self->priv->pattern_table));
}

//-- cut/copy/paste

static void pattern_clipboard_get_func(GtkClipboard *clipboard,GtkSelectionData *selection_data,guint info,gpointer data) {
  GST_INFO("get clipboard data, info=%d, data=%p",info,data);
  GST_INFO("sending : [%s]",data);
  // @todo: do we need to format differently depending on info?
  if(gtk_selection_data_get_target(selection_data)==pattern_atom) {
    gtk_selection_data_set(selection_data,pattern_atom,8,(guchar *)data,strlen(data));
  }
  else {
    // allow pasting into a test editor for debugging
    // its only active if we register the formats in _copy_selection() below
    gtk_selection_data_set_text(selection_data,data,-1);
  }
}

static void pattern_clipboard_clear_func(GtkClipboard *clipboard,gpointer data) {
  GST_INFO("freeing clipboard data, data=%p",data);
  g_free(data);
}

/**
 * bt_main_page_patterns_delete_selection:
 * @self: the pattern subpage
 *
 * Delete (clear) the selected area.
 */
void bt_main_page_patterns_delete_selection(const BtMainPagePatterns *self) {
  pattern_selection_apply(self,
    bt_pattern_delete_column,bt_pattern_delete_columns,
    bt_wire_pattern_delete_column,bt_wire_pattern_delete_columns);
}

/**
 * bt_main_page_patterns_cut_selection:
 * @self: the pattern subpage
 *
 * Cut selected area.
 */
void bt_main_page_patterns_cut_selection(const BtMainPagePatterns *self) {
  bt_main_page_patterns_copy_selection(self);
  bt_main_page_patterns_delete_selection(self);
}

/**
 * bt_main_page_patterns_copy_selection:
 * @self: the sequence subpage
 *
 * Copy selected area.
 */
void bt_main_page_patterns_copy_selection(const BtMainPagePatterns *self) {
  gint beg,end,group,param;
  if(bt_pattern_editor_get_selection(self->priv->pattern_table,&beg,&end,&group,&param)) {
    //GtkClipboard *cb=gtk_clipboard_get_for_display(gdk_display_get_default(),GDK_SELECTION_CLIPBOARD);
    //GtkClipboard *cb=gtk_widget_get_clipboard(GTK_WIDGET(self->priv->pattern_table),GDK_SELECTION_SECONDARY);
    GtkClipboard *cb=gtk_widget_get_clipboard(GTK_WIDGET(self->priv->pattern_table),GDK_SELECTION_CLIPBOARD);
    GtkTargetEntry *targets;
    gint n_targets;
    GString *data=g_string_new(NULL);

    targets = gtk_target_table_make(pattern_atom, &n_targets);

    /* the number of ticks */
    g_string_append_printf(data,"%d\n",end-beg);
    pattern_range_copy(self,beg,end,group,param,data);

    GST_INFO("copying : [%s]",data->str);

    /* put to clipboard */
    if(gtk_clipboard_set_with_data(cb,targets,n_targets,
       pattern_clipboard_get_func,pattern_clipboard_clear_func,
       g_string_free(data,FALSE))
    ) {
      gtk_clipboard_set_can_store (cb, NULL, 0);
    }
    else {
      GST_INFO("copy failed");
    }

    gtk_target_table_free (targets, n_targets);
    GST_INFO("copy done");
  }
}

static void pattern_clipboard_received_func(GtkClipboard *clipboard,GtkSelectionData *selection_data,gpointer user_data) {
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS(user_data);
  gchar **lines;
  gchar *data;

  GST_INFO("receiving clipboard data");

  data=(gchar *)gtk_selection_data_get_data(selection_data);
  GST_INFO("pasting : [%s]",data);

  if(!data)
    return;

  lines=g_strsplit_set(data,"\n",0);
  if(lines[0]) {
    guint ticks=atol(lines[0]);
    gint i=1,g,p;
    gint beg,end;
    gulong pattern_length;
    BtPatternEditorColumnGroup *pc_group;
    BtMachine *machine;
    gboolean res=TRUE;

    g_object_get(self->priv->pattern,"length",&pattern_length,"machine",&machine,NULL);
    pattern_length--;
    // paste from self->priv->cursor_row to MIN(self->priv->cursor_row+ticks,pattern_length)
    beg=self->priv->cursor_row;
    end=beg+ticks;
    end=MIN(end,pattern_length);
    GST_INFO("pasting from row %d to %d",beg,end);

    g=self->priv->cursor_group;
    p=self->priv->cursor_param;
    pc_group=&self->priv->param_groups[g];
    // process each line (= pattern column)
    while(lines[i] && *lines[i] && res) {
      if(*lines[i]!='\n') {
        switch (pc_group->type) {
          case PGT_WIRE: {
            BtWirePattern *wire_pattern=bt_wire_get_pattern(pc_group->user_data,self->priv->pattern);
            if(wire_pattern) {
              res=bt_wire_pattern_deserialize_column(wire_pattern,beg,end,p,lines[i]);
              g_object_unref(wire_pattern);
            }
          } break;
          case PGT_GLOBAL:
            res=bt_pattern_deserialize_column(self->priv->pattern,beg,end,p,lines[i]);
            break;
          case PGT_VOICE: {
            gulong global_params, voice_params, params;

            g_object_get(machine,"global-params",&global_params,"voice-params",&voice_params,NULL);
            params=global_params+(GPOINTER_TO_UINT(pc_group->user_data)*voice_params);
            res=bt_pattern_deserialize_column(self->priv->pattern,beg,end,params+p,lines[i]);
          } break;
        }
      }
      else {
        GST_INFO("skip blank line");
      }
      i++;p++;
      if(p==pc_group->num_columns) {
        // switch to next group or stop
        if(g<self->priv->number_of_groups) {
          g++;p=0;
          pc_group=&self->priv->param_groups[g];
        }
        else {
          break;
        }
      }
    }
    g_object_unref(machine);
    gtk_widget_queue_draw(GTK_WIDGET(self->priv->pattern_table));
  }
  g_strfreev(lines);
}

/**
 * bt_main_page_patterns_paste_selection:
 * @self: the pattern subpage
 *
 * Paste at the top of the selected area.
 */
void bt_main_page_patterns_paste_selection(const BtMainPagePatterns *self) {
  //GtkClipboard *cb=gtk_clipboard_get_for_display(gdk_display_get_default(),GDK_SELECTION_CLIPBOARD);
  //GtkClipboard *cb=gtk_widget_get_clipboard(GTK_WIDGET(self->priv->pattern_table),GDK_SELECTION_SECONDARY);
  GtkClipboard *cb=gtk_widget_get_clipboard(GTK_WIDGET(self->priv->pattern_table),GDK_SELECTION_CLIPBOARD);

  gtk_clipboard_request_contents(cb,pattern_atom,pattern_clipboard_received_func,(gpointer)self);
}

//-- change logger interface

static gboolean bt_main_page_patterns_change_logger_change(const BtChangeLogger *owner,const gchar *data) {
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS(owner);
  gboolean res=FALSE;
  BtMachine *machine;
  BtPattern *pattern=g_object_try_ref(self->priv->pattern);
  gchar *c_mid,*c_pid;
  GMatchInfo *match_info;
  guint s_row=0,e_row=0,group=0,param=0;
  gchar *s;

  if(pattern) {
    g_object_get(pattern,"machine",&machine,"id",&c_pid,NULL);
    g_object_get(machine,"id",&c_mid,NULL);
  } else {
    machine=NULL;
    c_pid=NULL;
    c_mid=NULL;
  }

  GST_INFO("undo/redo: [%s]",data);
  // parse data and apply action
  switch (bt_change_logger_match_method(change_logger_methods, data, &match_info)) {
    case METHOD_SET_GLOBAL_EVENTS: {
      gchar *str,*mid,*pid;

      mid=g_match_info_fetch(match_info,1);
      pid=g_match_info_fetch(match_info,2);
      s=g_match_info_fetch(match_info,3);s_row=atoi(s);g_free(s);
      s=g_match_info_fetch(match_info,4);e_row=atoi(s);g_free(s);
      s=g_match_info_fetch(match_info,5);param=atoi(s);g_free(s);
      str=g_match_info_fetch(match_info,6);
      g_match_info_free(match_info);

      GST_DEBUG("-> [%s|%s|%u|%u|%u|%s]",mid,pid,s_row,e_row,param,str);
      lookup_machine_and_pattern(self,&machine,&pattern,mid,c_mid,pid,c_pid);
      res=bt_pattern_deserialize_column(pattern,s_row,e_row,param,str);
      g_free(str);
      g_free(mid);
      g_free(pid);

      if(res) {
        // move cursor
        for(group=0;group<self->priv->number_of_groups;group++) {
          if(self->priv->param_groups[group].type==PGT_GLOBAL) break;
        }
        g_object_set(self->priv->pattern_table,"cursor-row",s_row,"cursor-group",group,"cursor-param",param,NULL);
        pattern_table_refresh(self);
      }
      break;
    }
    case METHOD_SET_VOICE_EVENTS: {
      guint voice;
      gchar *str,*mid,*pid;
      gulong global_params,voice_params,param_offset;

      mid=g_match_info_fetch(match_info,1);
      pid=g_match_info_fetch(match_info,2);
      s=g_match_info_fetch(match_info,3);s_row=atoi(s);g_free(s);
      s=g_match_info_fetch(match_info,4);e_row=atoi(s);g_free(s);
      s=g_match_info_fetch(match_info,5);voice=atoi(s);g_free(s);
      s=g_match_info_fetch(match_info,6);param=atoi(s);g_free(s);
      str=g_match_info_fetch(match_info,7);
      g_match_info_free(match_info);

      GST_DEBUG("-> [%s|%s|%u|%u|%u|%u|%s]",mid,pid,s_row,e_row,voice,param,str);
      lookup_machine_and_pattern(self,&machine,&pattern,mid,c_mid,pid,c_pid);
      g_object_get(machine,"global-params",&global_params,"voice-params",&voice_params,NULL);
      param_offset=global_params+(voice*voice_params);
      res=bt_pattern_deserialize_column(pattern,s_row,e_row,param_offset+param,str);
      g_free(str);
      g_free(mid);
      g_free(pid);

      if(res) {
        // move cursor
        for(group=0;group<self->priv->number_of_groups;group++) {
          if((self->priv->param_groups[group].type==PGT_VOICE) && (GPOINTER_TO_UINT(self->priv->param_groups[group].user_data)==voice)) break;
        }
        g_object_set(self->priv->pattern_table,"cursor-row",s_row,"cursor-group",group,"cursor-param",param,NULL);
        pattern_table_refresh(self);
      }
      break;
    }
    case METHOD_SET_WIRE_EVENTS: {
      gchar *str,*smid,*dmid,*pid;
      BtSong *song;
      BtSetup *setup;
      BtMachine *smachine;
      BtWire *wire;
      BtWirePattern *wire_pattern;

      smid=g_match_info_fetch(match_info,1);
      dmid=g_match_info_fetch(match_info,2);
      pid=g_match_info_fetch(match_info,3);
      s=g_match_info_fetch(match_info,4);s_row=atoi(s);g_free(s);
      s=g_match_info_fetch(match_info,5);e_row=atoi(s);g_free(s);
      s=g_match_info_fetch(match_info,6);param=atoi(s);g_free(s);
      str=g_match_info_fetch(match_info,7);
      g_match_info_free(match_info);

      GST_DEBUG("-> [%s|%s|%s|%u|%u|%u|%s]",smid,dmid,pid,s_row,e_row,param,str);
      lookup_machine_and_pattern(self,&machine,&pattern,dmid,c_mid,pid,c_pid);

      g_object_get(self->priv->app,"song",&song,NULL);
      g_object_get(song,"setup",&setup,NULL);
      smachine=bt_setup_get_machine_by_id(setup,smid);
      wire=bt_setup_get_wire_by_machines(setup,smachine,machine);
      if(!(wire_pattern=bt_wire_get_pattern(wire,pattern))) {
        wire_pattern=bt_wire_pattern_new(song,wire,pattern);
      }
      res=bt_wire_pattern_deserialize_column(wire_pattern,s_row,e_row,param,str);
      //res=bt_wire_pattern_set_event(wire_pattern,row,param,str);
      g_free(str);
      g_free(smid);
      g_free(dmid);
      g_free(pid);
      g_object_unref(wire_pattern);
      g_object_unref(wire);
      g_object_unref(smachine);
      g_object_unref(setup);
      g_object_unref(song);

      if(res) {
        // move cursor
        for(group=0;group<self->priv->number_of_groups;group++) {
          if((self->priv->param_groups[group].type==PGT_WIRE) && (self->priv->param_groups[group].user_data==wire_pattern)) break;
        }
        g_object_set(self->priv->pattern_table,"cursor-row",s_row,"cursor-group",group,"cursor-param",param,NULL);
        pattern_table_refresh(self);
      }
      break;
    }
    case METHOD_SET_PROPERTY: {
      gchar *mid,*pid,*key,*val;

      mid=g_match_info_fetch(match_info,1);
      pid=g_match_info_fetch(match_info,2);
      key=g_match_info_fetch(match_info,3);
      val=g_match_info_fetch(match_info,4);
      g_match_info_free(match_info);

		  GST_DEBUG("-> [%s|%s|%s|%s]",mid,pid,key,val);

      lookup_machine_and_pattern(self,&machine,&pattern,mid,c_mid,pid,c_pid);

      res=TRUE;
      if(!strcmp(key,"name")) {
        g_object_set(pattern,"name",val,NULL);
      } else if(!strcmp(key,"length")) {
      	g_object_set(pattern,"length",atol(val),NULL);
      }
			else {
				GST_WARNING("unhandled property '%s'",key);
				res=FALSE;
			}

      g_free(mid);
      g_free(pid);
      g_free(key);
      g_free(val);
      break;
    }
    case METHOD_ADD_PATTERN: {
      gchar *mid,*pid,*pname;
      gulong length;
      BtSong *song;

      mid=g_match_info_fetch(match_info,1);
      pid=g_match_info_fetch(match_info,2);
      pname=g_match_info_fetch(match_info,3);
      s=g_match_info_fetch(match_info,4);length=atol(s);g_free(s);

      GST_DEBUG("-> [%s|%s|%s|%lu]",mid,pid,pname,length);
      lookup_machine_and_pattern(self,&machine,NULL,mid,c_mid,NULL,NULL);
      g_object_get(self->priv->app,"song",&song,NULL);
      pattern=bt_pattern_new(song, pid, pname, length, machine);
      g_object_unref(song);
      res=TRUE;
      g_free(pname);
      g_free(mid);
      g_free(pid);

      change_current_pattern(self,pattern);
      context_menu_refresh(self,machine);
      break;
    }
    case METHOD_REM_PATTERN: {
      gchar *mid,*pid;

      mid=g_match_info_fetch(match_info,1);
      pid=g_match_info_fetch(match_info,2);

      GST_DEBUG("-> [%s|%s]",mid,pid);
      lookup_machine_and_pattern(self,&machine,&pattern,mid,c_mid,pid,c_pid);
      bt_machine_remove_pattern(machine,pattern);
      res=TRUE;

      change_current_pattern(self,NULL);
      context_menu_refresh(self,machine);
      break;
    }
    default:
      GST_WARNING("unhandled undo/redo method: [%s]",data);
  }

  g_object_try_unref(machine);
  g_object_try_unref(pattern);
  g_free(c_mid);g_free(c_pid);

  GST_INFO("undo/redo: %s : [%s]",(res?"okay":"failed"),data);
  return res;
}

static void bt_main_page_patterns_change_logger_interface_init(gpointer const g_iface, gconstpointer const iface_data) {
  BtChangeLoggerInterface * const iface = g_iface;

  iface->change = bt_main_page_patterns_change_logger_change;
}

//-- wrapper

//-- class internals

static gboolean bt_main_page_patterns_focus(GtkWidget *widget, GtkDirectionType direction) {
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS(widget);

  GST_DEBUG("focusing default widget");
  gtk_widget_grab_focus_savely(GTK_WIDGET(self->priv->pattern_table));
  // only set new text
  pattern_view_update_column_description(self,UPDATE_COLUMN_PUSH);
  return FALSE;
}

static void bt_main_page_patterns_dispose(GObject *object) {
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS(object);
  BtSong *song;

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);

  g_object_get(self->priv->app,"song",&song,NULL);
  if(song) {
    BtSetup *setup;
    BtWavetable *wavetable;

    GST_DEBUG("disconnect handlers from song=%p, song->ref_ct=%d",song,G_OBJECT_REF_COUNT(song));
    g_object_get(song,"setup",&setup,"wavetable",&wavetable,NULL);

    g_signal_handlers_disconnect_matched(setup,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_machine_added,NULL);
    g_signal_handlers_disconnect_matched(setup,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_machine_removed,NULL);
    g_signal_handlers_disconnect_matched(setup,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_wire_added_or_removed,NULL);
    g_signal_handlers_disconnect_matched(wavetable,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_wave_added_or_removed,NULL);
    g_signal_handlers_disconnect_matched(song,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_sequence_tick,NULL);

    // release the references
    g_object_unref(wavetable);
    g_object_unref(setup);
    g_object_unref(song);
  }

  g_signal_handlers_disconnect_matched(self->priv->app,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_song_changed,NULL);

  g_object_unref(self->priv->change_log);
  g_object_unref(self->priv->app);
  GST_DEBUG("unref pattern: %p,refs=%d",self->priv->pattern,G_OBJECT_REF_COUNT(self->priv->pattern));
  g_object_try_unref(self->priv->pattern);
  GST_DEBUG("unref machine: %p,refs=%d",self->priv->machine,G_OBJECT_REF_COUNT(self->priv->machine));
  g_object_try_unref(self->priv->machine);

  gtk_widget_destroy(GTK_WIDGET(self->priv->context_menu));
  g_object_unref(self->priv->context_menu);

  g_object_try_unref(self->priv->accel_group);

  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(bt_main_page_patterns_parent_class)->dispose(object);
}

static void bt_main_page_patterns_finalize(GObject *object) {
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS(object);

  g_free(self->priv->column_keymode);

  pattern_table_clear(self);

  G_OBJECT_CLASS(bt_main_page_patterns_parent_class)->finalize(object);
}

static void bt_main_page_patterns_init(BtMainPagePatterns *self) {
  guint i;

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_MAIN_PAGE_PATTERNS, BtMainPagePatternsPrivate);
  GST_DEBUG("!!!! self=%p",self);
  self->priv->app = bt_edit_application_new();

  //self->priv->cursor_column=0;
  //self->priv->cursor_row=0;
  self->priv->selection_start_column=-1;
  self->priv->selection_start_row=-1;
  self->priv->selection_end_column=-1;
  self->priv->selection_end_row=-1;

  self->priv->base_octave=4;
  for (i = 0; i < MAX_WAVETABLE_ITEMS + 2; i++)
    self->priv->wave_to_combopos[i] = self->priv->combopos_to_wave[i] = -1;

  // the undo/redo changelogger
  self->priv->change_log=bt_change_log_new();
  bt_change_log_register(self->priv->change_log,BT_CHANGE_LOGGER(self));
}

static void bt_main_page_patterns_class_init(BtMainPagePatternsClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GtkWidgetClass *gtkwidget_class = GTK_WIDGET_CLASS(klass);

  pattern_atom=gdk_atom_intern_static_string("application/buzztard::pattern");

  g_type_class_add_private(klass,sizeof(BtMainPagePatternsPrivate));

  gobject_class->dispose      = bt_main_page_patterns_dispose;
  gobject_class->finalize     = bt_main_page_patterns_finalize;

  gtkwidget_class->focus      = bt_main_page_patterns_focus;
}

