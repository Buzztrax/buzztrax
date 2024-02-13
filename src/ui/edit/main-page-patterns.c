/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
/**
 * SECTION:btmainpagepatterns
 * @short_description: the editor main pattern page
 * @see_also: #BtPattern, #BtPatternEditor
 *
 * Provides an editor for #BtPattern instances.
 */

/* TODO(ensonic): shortcuts
 * - on_pattern_table_key_press_event()
 * - used keys:
 *   a: select all
 *   b: begin selection
 *   c: copy
 *   d:
 *   e: end selection
 *   f: flip
 *   g: transpose fine/coarse down
 *   h:
 *   i: interpolate
 *   j:
 *   k: select group
 *   l: select column
 *   m:
 *   n:
 *   o:
 *   p:
 *   q:
 *   r: randomize, randomize range
 *   s:
 *   t: transpose fine/coarse up
 *   u: unselect
 *   v: paste
 *   w:
 *   x: cut
 *   y:
 *   z:
 *
 * - Ctrl-S : Smooth
 *   - low pass median filter over values
 *   - when hitting empty tick, we can leave them blank for triggers and
 *     interpolate for others (maybe, default: skip, Ctrl-Shift-S: fill)
 * - Ctrl-M : Mirror
 *   - value = max - (value-min))
 * - Ctrl-Shift-M : Mirror-Range
 *   - like invert, but take min, max from the values in the selection
 * - prev/next for combobox entries
 *   - trigger "move-active" action signal with GTK_SCROLL_STEP_UP/GTK_SCROLL_STEP_DOWN
 *   - what mechanism to use:
 *     - gtk_binding_entry_add_signal (do bindings work when not focused?)
 *     - gtk_widget_add_accelerator (can't specify signal params)
 */
/* TODO(ensonic): use gray text color for disconnected machines in the machine
 * combobox (or machine without patterns) and unused waves
 * (like for unused patterns)
 * - sensitve property of renderer does not work in comboboxes
 *   (unlike in treeviews)
 *   -> we could use italic text in all of them
 *     ("style", PANGO_STYLE_ITALIC and "style-set")
 */
/* TODO(ensonic): play live note entry
 * - support midi keyboard for entering notes and triggers
 * - have poly-input mode
 *   - if there is a keydown, enter the note
 *   - if there is another keydown before a keyup, go to next voice and
 *     enter the note there.
 *   - on keyup, return 'cursor' to that column
 * - what keys to use for trigger columns?
 * - should we use shift+note for secondary note in same voice (e.g. slide)
 */
/* TODO(ensonic): add the same context menu entries as the machines have in
 * machine view for current machine
 * - e.g. allow to open machine settings/preferences
 *   see machine-canvas-item::show_machine_properties_dialog()
 *   bt_main_page_machines_show_properties_dialog(page,machine);
 *   bt_main_page_machines_show_preferences_dialog(page,machine);
 *   .. rename/about/help
 * - also do controller-assignments like in machine-property window
 */

#define BT_EDIT
#define BT_MAIN_PAGE_PATTERNS_C

#include "bt-edit.h"
#include "gst/toneconversion.h"

#define MAX_WAVETABLE_ITEMS 200
#define DEFAULT_BASE_OCTAVE 4

struct _BtMainPagePatterns
{
  GtkBox parent;
  
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* machine selection menu */
  GtkDropDown *machine_menu;
  /* pattern selection menu */
  GtkDropDown *pattern_menu;
  /* wavetable selection menu */
  GtkDropDown *wavetable_menu;
  /* base octave selection menu */
  GtkSpinButton *base_octave_menu;

  /* the pattern table */
  BtPatternEditor *pattern_table;

  /* local commands */
  /// GTK4 GtkAccelGroup *accel_group;

  /* pattern context_menu */
  GMenu *context_menu;
  GtkWidget *context_menu_voice_add, *context_menu_voice_remove;
  GtkWidget *context_menu_pattern_properties, *context_menu_pattern_remove,
      *context_menu_pattern_copy, *context_menu_machine_preferences;

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
  gulong number_of_groups, global_params, voice_params;
  BtPatternEditorColumnGroup *param_groups;
  guint *column_keymode;

  /* editor change log */
  BtChangeLog *change_log;

  /* signal handler ids */
  gint pattern_length_changed, pattern_voices_changed;

  /* cached setup properties */
  GHashTable *properties;
};

/* - TODO(ensonic): we're converting GValue <-> str <-> float
 * - we use bt_persistence to convert between GValue* and char*
 * - we have bits and pieces here to convert between char* and float
 * - bt_value_group can return strings or pointers to the GValue
 */
typedef struct
{
  gfloat (*val_to_float) (gchar * in, gpointer user_data);
  const gchar *(*float_to_str) (gfloat in, gpointer user_data);
  gfloat min, max;
} BtPatternEditorColumnConverters;

//-- the class

static void bt_main_page_patterns_change_logger_interface_init (gpointer const
    g_iface, gconstpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtMainPagePatterns, bt_main_page_patterns,
    GTK_TYPE_BOX,
    G_IMPLEMENT_INTERFACE (BT_TYPE_CHANGE_LOGGER,
        bt_main_page_patterns_change_logger_interface_init));


/* we need this in machine-properties-dialog.c too */
/// GTK4 GdkAtom pattern_atom;

enum
{
  PATTERN_KEYMODE_NOTE = 0,
  PATTERN_KEYMODE_BOOL,
  PATTERN_KEYMODE_NUMBER
};

typedef enum
{
  UPDATE_COLUMN_POP = 1,
  UPDATE_COLUMN_PUSH,
  UPDATE_COLUMN_UPDATE
} BtPatternViewUpdateColumn;

#define PATTERN_CELL_WIDTH 70
#define PATTERN_CELL_HEIGHT 28

enum
{
  METHOD_SET_GLOBAL_EVENTS = 0,
  METHOD_SET_VOICE_EVENTS,
  METHOD_SET_WIRE_EVENTS,
  METHOD_SET_PROPERTY,
  METHOD_ADD_PATTERN,
  METHOD_REM_PATTERN,
  METHOD_SET_VOICES,
};

static BtChangeLoggerMethods change_logger_methods[] = {
  BT_CHANGE_LOGGER_METHOD ("set_global_events", 18,
      "\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9 ]+)\",([0-9]+),([0-9]+),([0-9]+),(.*)$"),
  BT_CHANGE_LOGGER_METHOD ("set_voice_events", 17,
      "\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9 ]+)\",([0-9]+),([0-9]+),([0-9]+),([0-9]+),(.*)$"),
  BT_CHANGE_LOGGER_METHOD ("set_wire_events", 16,
      "\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9 ]+)\",([0-9]+),([0-9]+),([0-9]+),(.*)$"),
  BT_CHANGE_LOGGER_METHOD ("set_pattern_property", 21,
      "\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9 ]+)\"$"),
  BT_CHANGE_LOGGER_METHOD ("add_pattern", 12,
      "\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9 ]+)\",([0-9]+)$"),
  BT_CHANGE_LOGGER_METHOD ("rem_pattern", 12,
      "\"([-_a-zA-Z0-9 ]+)\",\"([-_a-zA-Z0-9 ]+)\"$"),
  BT_CHANGE_LOGGER_METHOD ("set_voices", 10, "\"([-_a-zA-Z0-9 ]+)\",([0-9]+)$"),
  {NULL,}
};

#define MAX_CHANGE_LOGGER_METHOD_LEN (22+1000)


static void on_pattern_size_changed (BtPattern * pattern, GParamSpec * arg,
    gpointer user_data);

static gboolean change_current_pattern (BtMainPagePatterns * self,
    BtPattern * new_pattern);

static BtMachine *get_current_machine (BtMainPagePatterns * self);

static void pattern_edit_set_data_at (gpointer pattern_data,
    gpointer column_data, guint row, guint track, guint param, guint digit,
    gfloat value);

//-- tree model helper

static guint
machine_menu_model_get_idx_by_machine (GListModel * model, BtMachine * that_machine)
{
  GST_INFO ("look up iter for machine : %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (that_machine));

  for (guint i = 0; i < g_list_model_get_n_items (model); ++i) {
    BtMachine *this_machine = BT_MACHINE (g_list_model_get_item (model, i));
    if (this_machine == that_machine) {
      GST_INFO ("found iter for machine : %" G_OBJECT_REF_COUNT_FMT,
                G_OBJECT_LOG_REF_COUNT (that_machine));
      
      return i;
    }
  }

  return GTK_INVALID_LIST_POSITION;
}

static gint
pattern_menu_model_get_idx_by_pattern (GListModel * model, BtPattern * that_pattern)
{
  for (guint i = 0; i < g_list_model_get_n_items (model); ++i) {
    BtPattern *this_pattern = g_list_model_get_item (model, i);
    if (this_pattern == that_pattern) {
      return i;
    }
  }

  return GTK_INVALID_LIST_POSITION;
}

//-- status bar helpers

static gchar *
pattern_view_get_cell_description (BtMainPagePatterns * self, gint row,
    gint group, gint param)
{
  GParamSpec *prop = NULL;
  BtValueGroup *vg;
  BtParameterGroup *pg;
  gchar *str = NULL, *desc = NULL;
  GValue *v;

  vg = self->param_groups[group].vg;
  g_object_get (vg, "parameter-group", &pg, NULL);
  if (row >= 0 && (v = bt_value_group_get_event_data (vg, row, param)) &&
      G_IS_VALUE (v)) {
    desc = bt_parameter_group_describe_param_value (pg, param, v);
  }
  // get parameter description
  if ((prop = bt_parameter_group_get_param_spec (pg, param))) {
    const gchar *blurb = g_param_spec_get_blurb (prop);
    if (desc && *desc) {
      str = g_strdup_printf ("%s: %s: %s", prop->name, blurb, desc);
    } else {
      str = g_strdup_printf ("%s: %s", prop->name, blurb);
    }
  }
  g_free (desc);
  g_object_unref (pg);
  return str;
}

static void
pattern_view_update_cell_description (BtMainPagePatterns * self,
    BtPatternViewUpdateColumn mode)
{
  BtMainWindow *main_window;

  g_object_get (self->app, "main-window", &main_window, NULL);
  // called too early
  if (!main_window)
    return;

  // our page is not the current
  if (mode & UPDATE_COLUMN_UPDATE) {
    gint page_num;

    bt_child_proxy_get (main_window, "pages::page", &page_num, NULL);
    if (page_num != BT_MAIN_PAGES_PATTERNS_PAGE) {
      g_object_unref (main_window);
      return;
    }
  }
  // pop previous text by passing str=NULL;
  if (mode & UPDATE_COLUMN_POP)
    bt_child_proxy_set (main_window, "statusbar::status", NULL, NULL);

  if (mode & UPDATE_COLUMN_PUSH) {
    if (self->pattern && self->number_of_groups) {
      gchar *str;

      g_object_get (self->pattern_table, "cursor-row", &self->cursor_row,
          "cursor-param", &self->cursor_param,
          "cursor-group", &self->cursor_group, NULL);

      str = pattern_view_get_cell_description (self, self->cursor_row,
          self->cursor_group, self->cursor_param);
      bt_child_proxy_set (main_window, "statusbar::status",
          (str ? str : BT_MAIN_STATUSBAR_DEFAULT), NULL);
      g_free (str);
    } else {
      GST_DEBUG ("no or empty pattern");
      bt_child_proxy_set (main_window, "statusbar::status",
          _("Add new patterns from right click context menu."), NULL);
    }
  }

  g_object_unref (main_window);
}

//-- undo/redo helpers

/* pattern_range_copy:
 *
 * Serialize the content of the range into @data.
 */
static void
pattern_range_copy (BtMainPagePatterns * self, gint beg, gint end,
    gint group, gint param, GString * data)
{
  BtPatternEditorColumnGroup *pc_group;

  GST_INFO ("copying : %d %d , %d %d", beg, end, group, param);

  // process full pattern
  if (group == -1 && param == -1) {
    bt_pattern_serialize_columns (self->pattern, beg, end, data);
  }
  // process whole group
  if (group != -1 && param == -1) {
    pc_group = &self->param_groups[group];
    bt_value_group_serialize_columns (pc_group->vg, beg, end, data);
  }
  // process one param in one group
  if (group != -1 && param != -1) {
    pc_group = &self->param_groups[group];
    bt_value_group_serialize_column (pc_group->vg, beg, end, param, data);
  }
}

static void
pattern_column_log_undo_redo (BtMainPagePatterns * self,
    const gchar * fmt, gint param, gchar ** old_str, gchar ** new_str)
{
  gchar *p, *undo_str, *redo_str;

  p = strchr (*old_str, '\n');
  *p = '\0';
  undo_str = g_strdup_printf ("%s,%u,%s", fmt, param, *old_str);
  *old_str = &p[1];
  if (new_str) {
    p = strchr (*new_str, '\n');
    *p = '\0';
    redo_str = g_strdup_printf ("%s,%u,%s", fmt, param, *new_str);
    *new_str = &p[1];
  } else {
    redo_str = g_strdup (undo_str);
  }
  bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self), undo_str,
      redo_str);
}

static void
pattern_columns_log_undo_redo (BtMainPagePatterns * self,
    const gchar * fmt, guint num_columns, gchar ** old_str, gchar ** new_str)
{
  guint i;
  gchar *p, *undo_str, *redo_str;

  for (i = 0; i < num_columns; i++) {
    p = strchr (*old_str, '\n');
    *p = '\0';
    undo_str = g_strdup_printf ("%s,%u,%s", fmt, i, *old_str);
    *old_str = &p[1];
    if (new_str) {
      p = strchr (*new_str, '\n');
      *p = '\0';
      redo_str = g_strdup_printf ("%s,%u,%s", fmt, i, *new_str);
      *new_str = &p[1];
    } else {
      redo_str = g_strdup (undo_str);
    }
    bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
        undo_str, redo_str);
  }
}

/* pattern_range_log_undo_redo:
 *
 * Add undo/redo events for the given data to the log.
 */
static void
pattern_range_log_undo_redo (BtMainPagePatterns * self, gint beg,
    gint end, gint group, gint param, gchar * old_str, gchar * new_str)
{
  BtPatternEditorColumnGroup *pc_group;
  BtMachine *machine;
  gchar fmt[MAX_CHANGE_LOGGER_METHOD_LEN];

  g_object_get (self->pattern, "machine", &machine, NULL);

  GST_INFO ("logging : %d %d , %d %d", beg, end, group, param);
  // process full pattern
  if (group == -1 && param == -1) {
    gulong wire_groups, voices;
    guint g, v;

    bt_change_log_start_group (self->change_log);

    g_object_get (machine, "voices", &voices, NULL);
    wire_groups = g_list_length (machine->dst_wires);

    for (g = 0; g < wire_groups; g++) {
      pc_group = &self->param_groups[g];
      g_snprintf (fmt, MAX_CHANGE_LOGGER_METHOD_LEN, pc_group->fmt, beg, end);
      pattern_columns_log_undo_redo (self, fmt, pc_group->num_columns, &old_str,
          &new_str);
    }

    if (self->global_params) {
      pc_group = &self->param_groups[g];
      g_snprintf (fmt, MAX_CHANGE_LOGGER_METHOD_LEN, pc_group->fmt, beg, end);
      pattern_columns_log_undo_redo (self, fmt, pc_group->num_columns, &old_str,
          &new_str);
      g++;
    }
    for (v = 0; v < voices; v++, g++) {
      pc_group = &self->param_groups[g];
      g_snprintf (fmt, MAX_CHANGE_LOGGER_METHOD_LEN, pc_group->fmt, beg, end);
      pattern_columns_log_undo_redo (self, fmt, pc_group->num_columns, &old_str,
          &new_str);
    }
    bt_change_log_end_group (self->change_log);
  }
  // process whole group
  if (group != -1 && param == -1) {
    bt_change_log_start_group (self->change_log);

    pc_group = &self->param_groups[group];
    g_snprintf (fmt, MAX_CHANGE_LOGGER_METHOD_LEN, pc_group->fmt, beg, end);
    pattern_columns_log_undo_redo (self, fmt, pc_group->num_columns, &old_str,
        &new_str);
    bt_change_log_end_group (self->change_log);
  }
  // process one param in one group
  if (group != -1 && param != -1) {
    pc_group = &self->param_groups[group];
    g_snprintf (fmt, MAX_CHANGE_LOGGER_METHOD_LEN, pc_group->fmt, beg, end);
    pattern_column_log_undo_redo (self, fmt, param, &old_str, &new_str);
  }
  g_object_unref (machine);
}

//-- selection helpers

/* this is used for delete/blend/randomize,
 * - would be good to generalize to use it also for insert/delete-row.
 *   -> need to ignore row_end
 * - would be good to use this for cut/copy/paste
 *   -> copy/cut want to return the data from the selection
 *   -> paste needs to receive the data from the selection
 */

static gboolean
pattern_selection_apply (BtMainPagePatterns * self, BtValueGroupOp op)
{
  gboolean res = FALSE;
  gint beg, end, group, param;

  if (bt_pattern_editor_get_selection (self->pattern_table, &beg, &end,
          &group, &param)) {
    GString *old_data = g_string_new (NULL), *new_data = g_string_new (NULL);
    BtPatternEditorColumnGroup *pc_group;

    pattern_range_copy (self, beg, end, group, param, old_data);

    GST_INFO ("applying : %d %d , %d %d", beg, end, group, param);
    // process full pattern
    if (group == -1 && param == -1) {
      bt_pattern_transform_colums (self->pattern, op, beg, end);
      res = TRUE;
    }
    // process whole group
    if (group != -1 && param == -1) {
      pc_group = &self->param_groups[group];
      bt_value_group_transform_colums (pc_group->vg, op, beg, end);
      res = TRUE;
    }
    // process one param in one group
    if (group != -1 && param != -1) {
      pc_group = &self->param_groups[group];
      bt_value_group_transform_colum (pc_group->vg, op, beg, end, param);
      res = TRUE;
    }

    if (res) {
      gtk_widget_queue_draw (GTK_WIDGET (self->pattern_table));
      pattern_range_copy (self, beg, end, group, param, new_data);
      pattern_range_log_undo_redo (self, beg, end, group, param, old_data->str,
          new_data->str);
    }
    g_string_free (old_data, TRUE);
    g_string_free (new_data, TRUE);
  }
  return res;
}

//-- event handlers

#if 0
static void
on_pattern_added (BtMachine * machine, BtPattern * pattern, gpointer user_data)
{
/* this would be too early, the pattern properties are not set :/
  BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);

  // add undo/redo details
  if(bt_change_log_is_active(self->change_log)) {
    gchar *undo_str,*redo_str;
    gchar *mid,*pid,*pname;
    gulong length;

    g_object_get(machine,"id",&mid,NULL);
    g_object_get(pattern,"id",&pid,"name",&pname,"length",&length,NULL);

    undo_str = g_strdup_printf("rem_pattern \"%s\",\"%s\"",mid,pid);
    redo_str = g_strdup_printf("add_pattern \"%s\",\"%s\",\"%s\",%lu",mid,pid,pname,length);
    bt_change_log_add(self->change_log,BT_CHANGE_LOGGER(self),undo_str,redo_str);
    g_free(mid);g_free(pid);g_free(pname);
  }
*/
}
#endif

static void
on_pattern_removed (BtMachine * machine, BtPattern * pattern,
    gpointer user_data)
{
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS (user_data);

  // add undo/redo details
  if (bt_change_log_is_active (self->change_log)) {
    BtWire *wire;
    gchar fmt[MAX_CHANGE_LOGGER_METHOD_LEN];
    gchar *mid, *pid, *str, *undo_str, *redo_str;
    gulong length;
    gulong wire_params, voices, global_params, voice_params;
    guint end;
    GList *node;
    GString *data = g_string_new (NULL);
    guint v;

    g_object_get (pattern, "name", &pid, "length", &length, NULL);
    g_object_get (machine, "id", &mid, "voices", &voices, "global-params",
        &global_params, "voice-params", &voice_params, NULL);

    bt_change_log_start_group (self->change_log);

    undo_str =
        g_strdup_printf ("add_pattern \"%s\",\"%s\",%lu", mid, pid, length);
    redo_str = g_strdup_printf ("rem_pattern \"%s\",\"%s\"", mid, pid);
    bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
        undo_str, redo_str);

    /* we can't use the usual helpers here
     * - we don't need redo data
     * - we don't have the BtPatternEditorColumnGroup
     */
    end = length - 1;
    bt_pattern_serialize_columns (pattern, 0, end, data);
    GST_DEBUG ("serialized size: %" G_GSIZE_FORMAT, data->len);
    g_string_append_c (data, '\n');
    str = data->str;
    // log events
    for (node = machine->dst_wires; node; node = g_list_next (node)) {
      BtMachine *smachine;
      gchar *smid;

      wire = (BtWire *) node->data;
      g_object_get (wire, "src", &smachine, "num-params", &wire_params, NULL);
      g_object_get (smachine, "id", &smid, NULL);
      g_snprintf (fmt, MAX_CHANGE_LOGGER_METHOD_LEN,
          "set_wire_events \"%s\",\"%s\",\"%s\",0,%u", smid, mid, pid, end);
      pattern_columns_log_undo_redo (self, fmt, wire_params, &str, NULL);
      g_free (smid);
      g_object_unref (smachine);
    }
    if (global_params) {
      g_snprintf (fmt, MAX_CHANGE_LOGGER_METHOD_LEN,
          "set_global_events \"%s\",\"%s\",0,%u", mid, pid, end);
      pattern_columns_log_undo_redo (self, fmt, global_params, &str, NULL);
    }
    for (v = 0; v < voices; v++) {
      g_snprintf (fmt, MAX_CHANGE_LOGGER_METHOD_LEN,
          "set_voice_events \"%s\",\"%s\",0,%u,%u", mid, pid, end, v);
      pattern_columns_log_undo_redo (self, fmt, voice_params, &str, NULL);
    }
    g_string_free (data, TRUE);

    bt_change_log_end_group (self->change_log);

    g_free (mid);
    g_free (pid);
  }
  GST_DEBUG ("removed pattern: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (pattern));
}

// use key-press-event, as then we get key repeats
static gboolean
on_pattern_table_key_press_event (GtkEventControllerKey* controller, guint keyval, guint keycode,
    GdkModifierType state, gpointer user_data)
{
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS (user_data);
  gboolean res = FALSE;
  gulong modifier =
      (gulong) state & gtk_accelerator_get_default_mod_mask ();

  if (!gtk_widget_get_realized (GTK_WIDGET (self->pattern_table)))
    return FALSE;

  GST_INFO
      ("pattern_table key : state 0x%x, keyval 0x%x, name %s",
      state, keyval, gdk_keyval_name (keyval));
  if (keyval == GDK_KEY_Return) {        /* GDK_KP_Enter */
    bt_child_proxy_set (self->app, "main-window::pages::page",
        BT_MAIN_PAGES_SEQUENCE_PAGE, NULL);
    /* TODO(ensonic): if we came from sequence page via Enter we could go back
     * to where we came from, but if the machine or pattern has been changed
     * here, we could go to first track and first pos where the new pattern is
     * used.
     */
    //BtMainPageSequence *sequence_page;
    //bt_child_proxy_get(self->app,"main-window::pages::sequence-page",&sequence_page,NULL);
    //bt_main_page_sequence_goto_???(sequence_page,pattern);
    //g_object_unref(sequence_page);

    res = TRUE;
  } else if (keyval == GDK_KEY_Menu) {
    GtkPopover* popover = GTK_POPOVER (gtk_popover_menu_new_from_model (G_MENU_MODEL (self->context_menu)));
    gtk_widget_set_parent (GTK_WIDGET (popover), GTK_WIDGET (self));
    gtk_popover_popup (popover);
    res = TRUE;
  } else if (keyval == ',') {
    BtPatternEditorColumnGroup *group;
    BtParameterGroup *pg;
    guint param;

    // copy the current value from the machine to the pattern
    g_object_get (self->pattern_table, "cursor-row",
        &self->cursor_row, "cursor-group", &self->cursor_group,
        "cursor-param", &self->cursor_param, NULL);
    group = &self->param_groups[self->cursor_group];
    param = self->cursor_param;
    g_object_get (group->vg, "parameter-group", &pg, NULL);

    // don't do this for trigger params !!!!! (not readable) =======================
    if (!bt_parameter_group_is_param_trigger (pg, param)) {
      GObject *parent;
      const gchar *prop_name;
      GValue value = { 0, };
      gchar *str;
      gulong number_of_ticks;
      BtPatternEditorColumnConverters *pcc = (BtPatternEditorColumnConverters *)
          group->columns[param].user_data;

      parent = bt_parameter_group_get_param_parent (pg, param);
      prop_name = bt_parameter_group_get_param_name (pg, param);
      g_value_init (&value, bt_parameter_group_get_param_type (pg, param));
      g_object_get_property (parent, prop_name, &value);
      str = bt_str_format_gvalue (&value);
      GST_DEBUG ("get property %s: %s", prop_name, str);

      pattern_edit_set_data_at (self, pcc, self->cursor_row,
          self->cursor_group, param, 0, pcc->val_to_float (str, pcc));

      g_object_get (self->pattern, "length", &number_of_ticks, NULL);
      if (self->cursor_row + 1 < number_of_ticks) {
        g_object_set (self->pattern_table, "cursor-row", self->cursor_row + 1, NULL);
      }
      gtk_widget_queue_draw (GTK_WIDGET (self->pattern_table));

      g_free (str);
      g_value_unset (&value);
    }
    g_object_unref (pg);
  } else if (keyval == GDK_KEY_Insert) {
    GString *old_data = g_string_new (NULL), *new_data = g_string_new (NULL);
    gulong number_of_ticks;
    gint beg, end, group, param;

    g_object_get (self->pattern_table, "cursor-row",
        &self->cursor_row, "cursor-group", &self->cursor_group,
        "cursor-param", &self->cursor_param, NULL);
    g_object_get (self->pattern, "length", &number_of_ticks, NULL);
    beg = self->cursor_row;
    end = number_of_ticks - 1;
    if ((modifier & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) ==
        (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) {
      group = -1;
      param = -1;
    } else if (modifier & GDK_SHIFT_MASK) {
      group = self->cursor_group;
      param = -1;
    } else {
      group = self->cursor_group;
      param = self->cursor_param;
    }
    pattern_range_copy (self, beg, end, group, param, old_data);

    if ((modifier & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) ==
        (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) {
      // insert full row
      GST_INFO ("ctrl-shift-insert pressed, row %lu", self->cursor_row);
      bt_pattern_insert_row (self->pattern, self->cursor_row);
      res = TRUE;
    } else if (modifier & GDK_SHIFT_MASK) {
      BtPatternEditorColumnGroup *group = &self->param_groups[self->cursor_group];
      // insert group
      GST_INFO ("shift-insert pressed, row %ld, group %u",
          self->cursor_row, self->cursor_group);
      bt_value_group_insert_full_row (group->vg, self->cursor_row);
      res = TRUE;
    } else {
      BtPatternEditorColumnGroup *group = &self->param_groups[self->cursor_group];
      // insert column
      GST_INFO ("insert pressed, row %ld, group %u, param %u",
          self->cursor_row, self->cursor_group, self->cursor_param);
      bt_value_group_insert_row (group->vg, self->cursor_row, self->cursor_param);
      res = TRUE;
    }

    if (res) {
      gtk_widget_queue_draw (GTK_WIDGET (self->pattern_table));
      pattern_range_copy (self, beg, end, group, param, new_data);
      pattern_range_log_undo_redo (self, beg, end, group, param, old_data->str,
          new_data->str);
    }
    g_string_free (old_data, TRUE);
    g_string_free (new_data, TRUE);
  } else if (keyval == GDK_KEY_Delete) {
    GString *old_data = g_string_new (NULL), *new_data = g_string_new (NULL);
    gulong number_of_ticks;
    gint beg, end, group, param;

    g_object_get (self->pattern_table, "cursor-row",
        &self->cursor_row, "cursor-group", &self->cursor_group,
        "cursor-param", &self->cursor_param, NULL);
    g_object_get (self->pattern, "length", &number_of_ticks, NULL);
    beg = self->cursor_row;
    end = number_of_ticks - 1;
    if ((modifier & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) ==
        (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) {
      group = -1;
      param = -1;
    } else if (modifier & GDK_SHIFT_MASK) {
      group = self->cursor_group;
      param = -1;
    } else {
      group = self->cursor_group;
      param = self->cursor_param;
    }

    pattern_range_copy (self, beg, end, group, param, old_data);

    if ((modifier & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) ==
        (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) {
      // delete full row
      GST_INFO ("ctrl-shift-delete pressed, row %lu", self->cursor_row);
      bt_pattern_delete_row (self->pattern, self->cursor_row);
      res = TRUE;
    } else if (modifier & GDK_SHIFT_MASK) {
      BtPatternEditorColumnGroup *group = &self->param_groups[self->cursor_group];
      // delete group
      GST_INFO ("delete pressed, row %ld, group %u", self->cursor_row,
          self->cursor_group);
      bt_value_group_delete_full_row (group->vg, self->cursor_row);
      res = TRUE;
    } else {
      BtPatternEditorColumnGroup *group = &self->param_groups[self->cursor_group];
      // delete column
      GST_INFO ("delete pressed, row %ld, group %u, param %u",
          self->cursor_row, self->cursor_group, self->cursor_param);
      bt_value_group_delete_row (group->vg, self->cursor_row, self->cursor_param);
      res = TRUE;
    }

    if (res) {
      gtk_widget_queue_draw (GTK_WIDGET (self->pattern_table));
      pattern_range_copy (self, beg, end, group, param, new_data);
      pattern_range_log_undo_redo (self, beg, end, group, param, old_data->str,
          new_data->str);
    }
    g_string_free (old_data, TRUE);
    g_string_free (new_data, TRUE);
  } else if (keyval == GDK_KEY_f) {
    if (modifier & GDK_CONTROL_MASK) {
      res = pattern_selection_apply (self, BT_VALUE_GROUP_OP_FLIP);
    }
  } else if (keyval == GDK_KEY_i) {
    if (modifier & GDK_CONTROL_MASK) {
      res = pattern_selection_apply (self, BT_VALUE_GROUP_OP_BLEND);
    }
  } else if (keyval == GDK_KEY_r) {
    if (modifier & GDK_CONTROL_MASK) {
      res = pattern_selection_apply (self, BT_VALUE_GROUP_OP_RANDOMIZE);
    }
  } else if (keyval == GDK_KEY_R) {
    if (modifier & GDK_CONTROL_MASK) {
      res = pattern_selection_apply (self, BT_VALUE_GROUP_OP_RANGE_RANDOMIZE);
    }
  } else if (keyval == GDK_KEY_t) {
    if (modifier & GDK_CONTROL_MASK) {
      res = pattern_selection_apply (self, BT_VALUE_GROUP_OP_TRANSPOSE_FINE_UP);
    }
  } else if (keyval == GDK_KEY_T) {
    if (modifier & GDK_CONTROL_MASK) {
      res = pattern_selection_apply (self,
          BT_VALUE_GROUP_OP_TRANSPOSE_COARSE_UP);
    }
  } else if (keyval == GDK_KEY_g) {
    if (modifier & GDK_CONTROL_MASK) {
      res = pattern_selection_apply (self,
          BT_VALUE_GROUP_OP_TRANSPOSE_FINE_DOWN);
    }
  } else if (keyval == GDK_KEY_G) {
    if (modifier & GDK_CONTROL_MASK) {
      res = pattern_selection_apply (self,
          BT_VALUE_GROUP_OP_TRANSPOSE_COARSE_DOWN);
    }
  } else if ((keyval == GDK_KEY_Up) && (modifier == GDK_CONTROL_MASK)) {
    g_signal_emit_by_name (self->machine_menu, "move-active",
        GTK_SCROLL_STEP_BACKWARD, NULL);
    res = TRUE;
  } else if ((keyval == GDK_KEY_Down) && (modifier == GDK_CONTROL_MASK)) {
    g_signal_emit_by_name (self->machine_menu, "move-active",
        GTK_SCROLL_STEP_FORWARD, NULL);
    res = TRUE;
  } else if (keyval == GDK_KEY_KP_Subtract) {
    g_signal_emit_by_name (self->pattern_menu, "move-active",
        GTK_SCROLL_STEP_BACKWARD, NULL);
    res = TRUE;
  } else if (keyval == GDK_KEY_KP_Add) {
    g_signal_emit_by_name (self->pattern_menu, "move-active",
        GTK_SCROLL_STEP_FORWARD, NULL);
    res = TRUE;
  } else if (keyval == GDK_KEY_KP_Divide) {
    g_signal_emit_by_name (self->base_octave_menu, "change-value",
        GTK_SCROLL_STEP_BACKWARD, NULL);
    res = TRUE;
  } else if (keyval == GDK_KEY_KP_Multiply) {
    g_signal_emit_by_name (self->base_octave_menu, "change-value",
        GTK_SCROLL_STEP_FORWARD, NULL);
    res = TRUE;
  } else if (keyval == GDK_KEY_less) {
    g_signal_emit_by_name (self->wavetable_menu, "move-active",
        GTK_SCROLL_STEP_BACKWARD, NULL);
    res = TRUE;
  } else if (keyval == GDK_KEY_greater) {
    g_signal_emit_by_name (self->wavetable_menu, "move-active",
        GTK_SCROLL_STEP_FORWARD, NULL);
    res = TRUE;
  }
  return res;
}

static gboolean
on_pattern_table_button_press_event (GtkGestureClick* gesture, gint n_press, gdouble x, gdouble y, gpointer user_data)
{
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS (user_data);
  gboolean res = FALSE;
  guint button = gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE (gesture));

  GST_INFO ("pattern_table button_press : button 0x%x", button);
  if (button == 2) {
    GtkPopover* popover = GTK_POPOVER (gtk_popover_menu_new_from_model (G_MENU_MODEL (self->context_menu)));
    gtk_widget_set_parent (GTK_WIDGET (popover), GTK_WIDGET (self));
    GdkRectangle rect = { x, y, 1, 1 };
    gtk_popover_set_pointing_to (popover, &rect);
    gtk_popover_popup (popover);
  }
  return res;
}

static void
on_pattern_table_cursor_group_changed (const BtPatternEditor * editor,
    GParamSpec * arg, gpointer user_data)
{
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS (user_data);

  g_object_get ((gpointer) editor, "cursor-group", &self->cursor_group,
      "cursor-param", &self->cursor_param, NULL);
  pattern_view_update_cell_description (self, UPDATE_COLUMN_UPDATE);
}

static void
on_pattern_table_cursor_param_changed (const BtPatternEditor * editor,
    GParamSpec * arg, gpointer user_data)
{
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS (user_data);

  g_object_get ((gpointer) editor, "cursor-param", &self->cursor_param,
      NULL);
  pattern_view_update_cell_description (self, UPDATE_COLUMN_UPDATE);
}

static void
on_pattern_table_cursor_row_changed (const BtPatternEditor * editor,
    GParamSpec * arg, gpointer user_data)
{
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS (user_data);

  g_object_get ((gpointer) editor, "cursor-row", &self->cursor_row, NULL);
  pattern_view_update_cell_description (self, UPDATE_COLUMN_UPDATE);
}

static gboolean
on_pattern_table_query_tooltip (GtkWidget * widget, gint x, gint y,
    gboolean keyboard_mode, GtkTooltip * tooltip, gpointer user_data)
{
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS (user_data);
  gint row, group, param, digit;
  gchar *str = NULL;

  if (!bt_pattern_editor_position_to_coords (self->pattern_table, x, y,
          &row, &group, &param, &digit) || (group == -1)) {
    return FALSE;
  }
  if ((str = pattern_view_get_cell_description (self, row, group, param))) {
    gtk_tooltip_set_text (tooltip, str);
    g_free (str);
    return TRUE;
  }
  return FALSE;
}

static void
on_machine_model_items_changed (GListModel* model, guint position, guint removed, guint added, gpointer user_data)
{
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS (user_data);

  if (added != 0) {
    gtk_drop_down_set_selected (self->machine_menu, position);
  }

  if (removed != 0) {
    /// GTK4 still needed? gtk_combo_box_set_active_iter (self->machine_menu, &iter);
  }
}

static void
on_pattern_model_items_changed (GListModel* model, guint position, guint removed, guint added, gpointer user_data)
{
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS (user_data);

  if (added != 0) {
    gtk_drop_down_set_selected (self->pattern_menu, position);
  }

  if (removed != 0) {
    /// GTK4 still needed? gtk_combo_box_set_active_iter (self->pattern_menu, &iter);
  }
}

static void
on_wave_model_items_changed (GListModel* model, guint position, guint removed, guint added, gpointer user_data)
{
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS (user_data);

  if (added != 0) {
    gtk_drop_down_set_selected (self->wavetable_menu, position);
    
    BtMachine *machine = get_current_machine (self);
    if (machine) {
      gtk_widget_set_sensitive (GTK_WIDGET (self->wavetable_menu),
          bt_machine_handles_waves (machine));
      g_object_unref (machine);
    }
  }

  gtk_widget_set_sensitive (GTK_WIDGET (self->wavetable_menu),
      gtk_drop_down_get_selected (self->wavetable_menu) != GTK_INVALID_LIST_POSITION) ;
}

//-- event handler helper

// call from on_song_changed
static void
machine_menu_refresh (BtMainPagePatterns * self, const BtSetup * setup)
{
  BtMachine *machine = NULL;
  GList *node, *list;
  BtMachineListModel *store;
  gint index = -1;
  gint active = -1;

  // create machine menu
  store = bt_machine_list_model_new ((BtSetup *) setup);
  // connect signal handlers for pattern undo/redo
  g_object_get ((gpointer) setup, "machines", &list, NULL);
  for (node = list; node; node = g_list_next (node)) {
    machine = BT_MACHINE (node->data);
    GST_INFO ("refresh menu for machine: %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (machine));
    index++;
    g_signal_connect (machine, "pattern-removed",
        G_CALLBACK (on_pattern_removed), (gpointer) self);
    if (machine == self->machine)
      active = index;
  }
  g_list_free (list);
  g_signal_connect (store, "items-changed",
      G_CALLBACK (on_machine_model_items_changed), (gpointer) self);
  GST_INFO ("machine menu refreshed, active item %d", active);

  if (active == -1) {
    // use the last one, if there is no active one
    active = index;
  }

  gtk_widget_set_sensitive (GTK_WIDGET (self->machine_menu),
      (index != -1));
  gtk_drop_down_set_model (self->machine_menu, G_LIST_MODEL (store));
  gtk_drop_down_set_selected (self->machine_menu, active);
  g_object_unref (store);       // drop with comboxbox
}

static void
pattern_menu_refresh (BtMainPagePatterns * self, BtMachine * machine)
{
  BtSong *song;
  BtSequence *sequence;

  g_assert (machine);

  // update pattern menu
  g_object_get (self->app, "song", &song, NULL);
  g_object_get (song, "sequence", &sequence, NULL);
  BtPatternListModel* pattern_list = bt_pattern_list_model_new (machine, sequence, TRUE);
  gtk_drop_down_set_model (self->pattern_menu, G_LIST_MODEL (pattern_list));

  g_signal_connect (pattern_list, "items-changed",
      G_CALLBACK (on_pattern_model_items_changed), (gpointer) self);
  
  g_object_unref (pattern_list);
  g_object_unref (sequence);
  g_object_unref (song);
}

static void
wavetable_menu_refresh (BtMainPagePatterns * self,
    BtWavetable * wavetable)
{
  gtk_drop_down_set_model (self->wavetable_menu, G_LIST_MODEL (wavetable));
}

static gfloat
pattern_edit_get_data_at (gpointer pattern_data, gpointer column_data,
    guint row, guint track, guint param)
{
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS (pattern_data);
  BtPatternEditorColumnGroup *group = &self->param_groups[track];

  if (column_data) {
    gchar *val = bt_value_group_get_event (group->vg, row, param);
    if (BT_IS_STRING (val)) {
      gfloat res = ((BtPatternEditorColumnConverters *)
          column_data)->val_to_float (val, column_data);
      g_free (val);
      return res;
    }
    g_free (val);
  }
  return group->columns[param].def;
}

static void
pattern_edit_set_data_at (gpointer pattern_data, gpointer column_data,
    guint row, guint track, guint param, guint digit, gfloat value)
{
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS (pattern_data);
  BtMachine *machine;
  const gchar *str = NULL;
  BtPatternEditorColumnGroup *group = &self->param_groups[track];
  gboolean cancel_edit = FALSE;
  gboolean is_trigger;
  BtParameterGroup *pg;
  glong wave_param;

  GST_INFO ("row=%u, track=%u, param=%u, digit=%u: %f",
      row, track, param, digit, value);

  g_object_get (self->pattern, "machine", &machine, NULL);

  if (value != group->columns[param].def) {
    if (column_data)
      str = ((BtPatternEditorColumnConverters *)
          column_data)->float_to_str (value, column_data);
    else
      str = bt_str_format_double (value);
  }

  g_object_get (group->vg, "parameter-group", &pg, NULL);
  is_trigger = bt_parameter_group_is_param_trigger (pg, param);
  wave_param = bt_parameter_group_get_wave_param_index (pg);

  if (is_trigger) {
    gboolean update_wave = FALSE;
    BtPatternEditorColumn *col = &group->columns[param];

    // play live (notes, triggers)
    if (BT_IS_STRING (str) && self->play_live) {
      GObject *param_parent;

      /* TODO(ensonic): buzz machines need set, tick, unset */
      GST_INFO ("play trigger: %f,'%s'", value, str);
      param_parent = bt_parameter_group_get_param_parent (pg, param);
      switch (col->type) {
        case PCT_NOTE:{
          GEnumClass *enum_class = g_type_class_peek_static (GSTBT_TYPE_NOTE);
          GEnumValue *enum_value;
          gint val = 0;

          if ((enum_value = g_enum_get_value_by_nick (enum_class, str))) {
            val = enum_value->value;
          }
          g_object_set (param_parent, bt_parameter_group_get_param_name (pg,
                  param), val, NULL);
        }
          break;
        case PCT_SWITCH:
        case PCT_BYTE:
        case PCT_WORD:{
          gint val = atoi (str);
          if (val == col->def) {
            g_object_set (param_parent, bt_parameter_group_get_param_name (pg,
                    param), val, NULL);
          }
        }
          break;
        case PCT_FLOAT:{
          gfloat val = atof (str);
          if (val == col->def) {
            g_object_set (param_parent, bt_parameter_group_get_param_name (pg,
                    param), val, NULL);
          }
        }
          break;
      }
    }

    if (col->type == PCT_NOTE) {
      // do not update the wave if it's an octave column or if the new value is 'off'
      if (digit == 0 && value != 255 && value != 0) {
        update_wave = TRUE;
      }
    }
    // if machine can play waves, enter wave index
    if (update_wave && (wave_param > -1)) {
      const gchar *wave_str = "";

      if (BT_IS_STRING (str)) {
        GListModel *model = gtk_drop_down_get_model (self->wavetable_menu);
        g_assert (model);

        gulong wave_ix = gtk_drop_down_get_selected (self->wavetable_menu);
        if (wave_ix != GTK_INVALID_LIST_POSITION) {
          wave_str = bt_str_format_ulong (wave_ix);
          // FIXME(ensonic): e.g. Matilde Tracker expects to get a wave, even if
          // wave table is empty
          GST_DEBUG ("wave index: %lu, %s", wave_ix, wave_str);
        }
      }
      GST_DEBUG ("set wave for: %u, %ld to %s", row, wave_param, wave_str);
      bt_value_group_set_event (group->vg, row, wave_param, wave_str);
      gtk_widget_queue_draw (GTK_WIDGET (self->pattern_table));
    }
  }
  if (group->columns[param].type == PCT_BYTE) {
    if (wave_param == param) {
      GST_DEBUG ("edit wave column: %f,'%s'", value, str);
      // don't clear wave column, if we have a note in any trigger column
      if (!BT_IS_STRING (str)) {
        guint i;
        for (i = 0; i < group->num_columns; i++) {
          if ((i != wave_param) &&
              bt_parameter_group_is_param_trigger (pg, i) &&
              bt_value_group_test_event (group->vg, row, i)) {
            GST_DEBUG ("have note set in col: %d, row: %d", i, row);
            cancel_edit = TRUE;
            break;
          }
        }
      } else {
        // make wavetable combo follow wave value
#if 0 /// GTK 4
        gint v = (gint) value;
        GtkTreeModel *model = gtk_drop_down_get_model (self->wavetable_menu);
        g_assert (model);

        cancel_edit = TRUE;

        if ((filter_model =
                gtk_combo_box_get_model (self->wavetable_menu))) {
          model =
              gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER
              (filter_model));
          if (gtk_tree_model_iter_nth_child (model, &iter, NULL, (v - 1))) {
            if (gtk_tree_model_filter_convert_child_iter_to_iter
                (GTK_TREE_MODEL_FILTER (filter_model), &filter_iter, &iter)) {
              gtk_combo_box_set_active_iter (self->wavetable_menu,
                  &filter_iter);
              cancel_edit = FALSE;
            }
          }
        }
        #endif
      }
    }
  }
  g_object_unref (pg);

  if (cancel_edit) {
    gtk_widget_queue_draw (GTK_WIDGET (self->pattern_table));
  } else {
    GString *old_data = g_string_new (NULL), *new_data = g_string_new (NULL);

    bt_value_group_serialize_column (group->vg, row, row, param, old_data);
    if (bt_value_group_set_event (group->vg, row, param, str)) {
      gchar fmt[MAX_CHANGE_LOGGER_METHOD_LEN];
      gchar *old_str, *new_str;

      bt_value_group_serialize_column (group->vg, row, row, param, new_data);
      old_str = old_data->str;
      new_str = new_data->str;
      g_snprintf (fmt, MAX_CHANGE_LOGGER_METHOD_LEN, group->fmt, row, row);
      pattern_column_log_undo_redo (self, fmt, param, &old_str, &new_str);
      g_string_free (new_data, TRUE);
    } else {
      // reject edit (e.g. value out of range)
      gtk_widget_queue_draw (GTK_WIDGET (self->pattern_table));
    }
    g_string_free (old_data, TRUE);
  }

  pattern_view_update_cell_description (self, UPDATE_COLUMN_UPDATE);
  g_object_unref (machine);
}

/* val_to_float: read data from pattern and convert to float for pattern-editor
 * used in pattern_edit_get_data_at()
 */

static gfloat
note_val_to_float (gchar * in, gpointer user_data)
{
  if (in)
    return (gfloat) gstbt_tone_conversion_note_string_2_number (in);
  return 0.0;
}

static gfloat
float_val_to_float (gchar * in, gpointer user_data)
{
  // scale value into 0...65535 range
  BtPatternEditorColumnConverters *pcc =
      (BtPatternEditorColumnConverters *) user_data;
  gdouble factor = 65535.0 / (pcc->max - pcc->min);

  return (g_ascii_strtod (in, NULL) - pcc->min) * factor;
}

static gfloat
int_val_to_float (gchar * in, gpointer user_data)
{
  return (gfloat) atoi (in);
}

/* float_to_str: convert the float value to a deserializable string
 * used in pattern_edit_set_data_at()
 */

static const gchar *
any_float_to_str (gfloat in, gpointer user_data)
{
  return bt_str_format_double (in);
}

static const gchar *
note_float_to_str (gfloat in, gpointer user_data)
{
  return gstbt_tone_conversion_note_number_2_string ((guint) in);
}

static const gchar *
float_float_to_str (gfloat in, gpointer user_data)
{
  // scale value from 0...65535 range
  BtPatternEditorColumnConverters *pcc =
      (BtPatternEditorColumnConverters *) user_data;
  gdouble factor = 65535.0 / (pcc->max - pcc->min);
  gdouble val = pcc->min + (in / factor);

  //GST_DEBUG("< val %lf, factor %lf, result %lf(%s)",in,factor,val,bt_str_format_double(val));

  return bt_str_format_double (val);
}


static void
pattern_edit_fill_column_type (BtPatternEditorColumn * col,
    GParamSpec * property, GValue * min_val, GValue * max_val, GValue * no_val)
{
  GType type = bt_g_type_get_base_type (property->value_type);
  BtPatternEditorColumnConverters *pcc;

  GST_LOG ("filling param type: '%s'::'%s'/'%s' for parameter '%s'",
      g_type_name (property->owner_type), g_type_name (type),
      g_type_name (property->value_type), property->name);

  pcc = col->user_data = g_new (BtPatternEditorColumnConverters, 1);
  switch (type) {
    case G_TYPE_STRING:
      col->type = PCT_NOTE;
      col->min = GSTBT_NOTE_NONE;
      col->max = GSTBT_NOTE_LAST;
      col->def = GSTBT_NOTE_NONE;
      pcc->val_to_float = note_val_to_float;
      pcc->float_to_str = note_float_to_str;
      break;
    case G_TYPE_BOOLEAN:
      col->type = PCT_SWITCH;
      col->min = 0;
      col->max = 1;
      col->def =
          BT_IS_GVALUE (no_val) ? g_value_get_boolean (no_val) : col->max + 1;
      pcc->val_to_float = int_val_to_float;
      pcc->float_to_str = any_float_to_str;
      break;
    case G_TYPE_ENUM:
      pcc->val_to_float = int_val_to_float;
      pcc->float_to_str = any_float_to_str;
      if (property->value_type == GSTBT_TYPE_TRIGGER_SWITCH) {
        col->type = PCT_SWITCH;
        col->min = GSTBT_TRIGGER_SWITCH_OFF;
        col->max = GSTBT_TRIGGER_SWITCH_ON;
        col->def = GSTBT_TRIGGER_SWITCH_EMPTY;
      } else if (property->value_type == GSTBT_TYPE_NOTE) {
        col->type = PCT_NOTE;
        col->min = GSTBT_NOTE_NONE;
        col->max = GSTBT_NOTE_LAST;
        col->def = GSTBT_NOTE_NONE;
        // we are using buzz like note values
        pcc->val_to_float = note_val_to_float;
        pcc->float_to_str = note_float_to_str;
      } else {
        col->type = PCT_BYTE;
        col->min = g_value_get_enum (min_val);
        col->max = g_value_get_enum (max_val);
        col->def =
            BT_IS_GVALUE (no_val) ? g_value_get_enum (no_val) : col->max + 1;
      }
      break;
    case G_TYPE_INT:
      col->type = PCT_WORD;
      col->min = g_value_get_int (min_val);
      col->max = g_value_get_int (max_val);
      col->def =
          BT_IS_GVALUE (no_val) ? g_value_get_int (no_val) : col->max + 1;
      if (col->min >= 0 && col->max < 256) {
        col->type = PCT_BYTE;
      }
      pcc->val_to_float = int_val_to_float;
      pcc->float_to_str = any_float_to_str;
      break;
    case G_TYPE_UINT:
      col->type = PCT_WORD;
      col->min = g_value_get_uint (min_val);
      col->max = g_value_get_uint (max_val);
      col->def =
          BT_IS_GVALUE (no_val) ? g_value_get_uint (no_val) : col->max + 1;
      if (col->min >= 0 && col->max < 256) {
        col->type = PCT_BYTE;
      }
      pcc->val_to_float = int_val_to_float;
      pcc->float_to_str = any_float_to_str;
      break;
    case G_TYPE_LONG:
      col->type = PCT_WORD;
      col->min = g_value_get_long (min_val);
      col->max = g_value_get_long (max_val);
      col->def =
          BT_IS_GVALUE (no_val) ? g_value_get_long (no_val) : col->max + 1;
      if (col->min >= 0 && col->max < 256) {
        col->type = PCT_BYTE;
      }
      pcc->val_to_float = int_val_to_float;
      pcc->float_to_str = any_float_to_str;
      break;
    case G_TYPE_ULONG:
      col->type = PCT_WORD;
      col->min = g_value_get_ulong (min_val);
      col->max = g_value_get_ulong (max_val);
      col->def =
          BT_IS_GVALUE (no_val) ? g_value_get_ulong (no_val) : col->max + 1;
      if (col->min >= 0 && col->max < 256) {
        col->type = PCT_BYTE;
      }
      pcc->val_to_float = int_val_to_float;
      pcc->float_to_str = any_float_to_str;
      break;
    case G_TYPE_FLOAT:
      col->type = PCT_WORD;
      col->min = 0.0;
      col->max = 65535.0;
      col->def = col->max + 1;
      pcc->val_to_float = float_val_to_float;
      pcc->float_to_str = float_float_to_str;
      pcc->min = g_value_get_float (min_val);
      pcc->max = g_value_get_float (max_val);
      /* TODO(ensonic): need scaling
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
      break;
    case G_TYPE_DOUBLE:
      col->type = PCT_WORD;
      col->min = 0.0;
      col->max = 65535.0;
      col->def = col->max + 1;
      pcc->val_to_float = float_val_to_float;
      pcc->float_to_str = float_float_to_str;
      // identify wire-elements
      // TODO(ensonic): this is not the best way to identify a volume element
      // gst_registry_find_feature() is better but too slow here
      if (!strcmp (g_type_name (property->owner_type), "GstVolume")) {
        pcc->min = 0.0;
        pcc->max = 4.0;
      } else {
        pcc->min = g_value_get_double (min_val);
        pcc->max = g_value_get_double (max_val);
      }
      break;
    default:
      GST_WARNING ("unhandled param type: '%s' for parameter '%s'",
          g_type_name (type), property->name);
      col->type = 0;
      col->min = col->max = col->def = 0;
      g_free (col->user_data);
      col->user_data = NULL;
  }
  GST_DEBUG ("%s parameter '%s' min/max/def : %6.4lf/%6.4lf/%6.4lf",
      g_type_name (type), property->name, col->min, col->max, col->def);
  g_value_unset (min_val);
  g_value_unset (max_val);
  g_free (min_val);
  g_free (max_val);
}

static void
pattern_table_clear (BtMainPagePatterns * self)
{
  BtPatternEditorColumnGroup *group;
  gulong i, j;

  for (i = 0; i < self->number_of_groups; i++) {
    group = &self->param_groups[i];
    for (j = 0; j < group->num_columns; j++) {
      g_free (group->columns[j].user_data);
    }
    g_free (group->name);
    g_free (group->columns);
    g_free (group->fmt);
  }
  g_free (self->param_groups);
  self->param_groups = NULL;
  //self->number_of_groups=0;
}

static void
pattern_table_refresh (BtMainPagePatterns * self)
{
  static BtPatternEditorCallbacks callbacks = {
    pattern_edit_get_data_at,
    pattern_edit_set_data_at
  };

  if (self->pattern) {
    gulong i, j;
    gulong number_of_ticks, voices;
    BtMachine *machine;
    BtPatternEditorColumnGroup *group;
    BtParameterGroup *pg;
    GParamSpec *property;
    GValue *min_val, *max_val, *no_val;
    gchar *mid, *pid;

    pattern_table_clear (self);

    g_object_get (self->pattern, "name", &pid, "length", &number_of_ticks,
        "voices", &voices, "machine", &machine, NULL);
    g_object_get (machine, "id", &mid, "global-params",
        &self->global_params, "voice-params", &self->voice_params,
        NULL);

    self->number_of_groups =
        (self->global_params > 0 ? 1 : 0) + voices;

    if (!BT_IS_SOURCE_MACHINE (machine) && machine->dst_wires) {
      GList *node;
      BtWire *wire;
      gulong wire_params;

      // need to iterate over all inputs
      node = machine->dst_wires;
      self->number_of_groups += g_list_length (node);
      group = self->param_groups =
          g_new (BtPatternEditorColumnGroup, self->number_of_groups);
      GST_INFO ("refresh pattern %p, size is ticks = %2lu, groups = %2lu",
          self->pattern, number_of_ticks, self->number_of_groups);

      GST_DEBUG ("wire parameters (%d)", g_list_length (node));
      for (; node; node = g_list_next (node)) {
        BtMachine *src = NULL;

        wire = BT_WIRE (node->data);
        // check wire config
        g_object_get (wire, "num-params", &wire_params, "src", &src, NULL);
        g_object_get (src, "id", &group->name, NULL);
        group->vg = bt_pattern_get_wire_group (self->pattern, wire);
        group->fmt =
            g_strdup_printf ("set_wire_events \"%s\",\"%s\",\"%s\",%%u,%%u",
            group->name, mid, pid);
        group->num_columns = wire_params;
        group->columns = g_new (BtPatternEditorColumn, wire_params);
        group->width = 0;
        pg = bt_wire_get_param_group (wire);
        for (i = 0; i < wire_params; i++) {
          bt_parameter_group_get_param_details (pg, i, &property, &min_val,
              &max_val);
          pattern_edit_fill_column_type (&group->columns[i], property, min_val,
              max_val, NULL);
        }
        g_object_unref (src);
        group++;
      }
    } else {
      group = self->param_groups =
          g_new (BtPatternEditorColumnGroup, self->number_of_groups);
      GST_INFO ("refresh pattern %p, size is ticks = %2lu, groups = %2lu",
          self->pattern, number_of_ticks, self->number_of_groups);
    }
    if (self->global_params) {
      // create mapping for global params
      /* label for first global parameter column in a pattern */
      group->name = g_strdup (_("Globals"));
      group->vg = bt_pattern_get_global_group (self->pattern);
      group->fmt =
          g_strdup_printf ("set_global_events \"%s\",\"%s\",%%u,%%u", mid, pid);
      group->num_columns = self->global_params;
      group->columns = g_new (BtPatternEditorColumn, group->num_columns);
      group->width = 0;
      GST_DEBUG ("global parameters, vg = %p", group->vg);
      pg = bt_machine_get_global_param_group (machine);
      for (i = 0; i < group->num_columns; i++) {
        bt_parameter_group_get_param_details (pg, i, &property, &min_val,
            &max_val);
        no_val = bt_parameter_group_get_param_no_value (pg, i);
        pattern_edit_fill_column_type (&group->columns[i], property, min_val,
            max_val, no_val);
      }
      group++;
    }
    if (voices) {
      BtPatternEditorColumnGroup *stamp = group;
      // create mapping for voice params
      /* label for parameters of first voice column in a pattern */
      group->name = g_strdup (_("Voice 1"));
      group->vg = bt_pattern_get_voice_group (self->pattern, 0);
      group->fmt =
          g_strdup_printf ("set_voice_events \"%s\",\"%s\",%%u,%%u,%u", mid,
          pid, 0);
      group->num_columns = self->voice_params;
      group->columns = g_new (BtPatternEditorColumn, group->num_columns);
      group->width = 0;
      GST_DEBUG ("voice parameters (%lu) , vg = %p", voices, group->vg);
      pg = bt_machine_get_voice_param_group (machine, 0);
      for (i = 0; i < group->num_columns; i++) {
        bt_parameter_group_get_param_details (pg, i, &property, &min_val,
            &max_val);
        no_val = bt_parameter_group_get_param_no_value (pg, i);
        pattern_edit_fill_column_type (&group->columns[i], property, min_val,
            max_val, no_val);
      }
      group++;
      for (j = 1; j < voices; j++) {
        /* label for parameters of voice columns in a pattern */
        group->name = g_strdup_printf (_("Voice %u"), (guint) (j + 1));
        group->vg = bt_pattern_get_voice_group (self->pattern, j);
        group->fmt =
            g_strdup_printf ("set_voice_events \"%s\",\"%s\",%%u,%%u,%lu", mid,
            pid, j);
        group->num_columns = self->voice_params;
        group->columns =
            g_memdup2 (stamp->columns,
                sizeof (BtPatternEditorColumn) * group->num_columns);
        for (i = 0; i < group->num_columns; i++) {
          group->columns[i].user_data =
              g_memdup2 (group->columns[i].user_data,
                  sizeof (BtPatternEditorColumnConverters));
        }
        group->width = 0;
        group++;
      }
    }

    bt_pattern_editor_set_pattern (self->pattern_table, (gpointer) self,
        number_of_ticks, self->number_of_groups, self->param_groups,
        &callbacks);

    g_free (mid);
    g_free (pid);
    g_object_unref (machine);
  } else {
    self->global_params = self->voice_params = 0;
    bt_pattern_editor_set_pattern (self->pattern_table, (gpointer) self,
        0, 0, NULL, &callbacks);
  }
  g_object_set (self->pattern_table, "play-position", -1.0, NULL);
}

/*
 * context_menu_refresh:
 * @self:  the pattern page
 * @machine: the currently selected machine
 *
 * enable/disable context menu items
 */
static void
context_menu_refresh (BtMainPagePatterns * self, BtMachine * machine)
{
  GST_INFO ("refresh context menu for machine: %p", machine);

  if (machine) {
    gboolean has_patterns = bt_machine_has_patterns (machine);
    gulong num_property_params;

    //gtk_widget_set_sensitive(GTK_WIDGET(self->context_menu),TRUE);
    if (has_patterns) {
      if (bt_machine_is_polyphonic (machine)) {
        GstElement *elem;
        GParamSpecULong *pspec;
        gulong min_voices = 0, max_voices = G_MAXULONG, voices;

        g_object_get (machine, "voices", &voices, "machine", &elem, NULL);
        if ((pspec = (GParamSpecULong *)
                g_object_class_find_property (G_OBJECT_GET_CLASS (elem),
                    "children"))) {
          min_voices = pspec->minimum;
          max_voices = pspec->maximum;
        }

        gtk_widget_set_sensitive (GTK_WIDGET (self->
                context_menu_voice_add), (voices < max_voices));
        gtk_widget_set_sensitive (GTK_WIDGET (self->
                context_menu_voice_remove), (voices > min_voices));
        gst_object_unref (elem);
      } else {
        gtk_widget_set_sensitive (GTK_WIDGET (self->
                context_menu_voice_add), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (self->
                context_menu_voice_remove), FALSE);
      }
      gtk_widget_set_sensitive (GTK_WIDGET (self->
              context_menu_pattern_properties), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (self->
              context_menu_pattern_remove), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (self->
              context_menu_pattern_copy), TRUE);
    } else {
      GST_INFO_OBJECT (machine, "machine has no patterns");
      gtk_widget_set_sensitive (GTK_WIDGET (self->context_menu_voice_add),
          FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (self->
              context_menu_voice_remove), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (self->
              context_menu_pattern_properties), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (self->
              context_menu_pattern_remove), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (self->
              context_menu_pattern_copy), FALSE);
    }
    g_object_get (machine, "prefs-params", &num_property_params, NULL);
    gtk_widget_set_sensitive (GTK_WIDGET (self->
            context_menu_machine_preferences), (num_property_params != 0));
  } else {
    GST_INFO ("no machine");
    //gtk_widget_set_sensitive(GTK_WIDGET(self->context_menu),FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (self->context_menu_voice_add),
        FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (self->
            context_menu_voice_remove), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (self->
            context_menu_pattern_properties), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (self->
            context_menu_pattern_remove), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (self->
            context_menu_pattern_copy), FALSE);
  }
}

static BtPattern *
add_new_pattern (BtMainPagePatterns * self, BtMachine * machine)
{
  BtSong *song;
  BtPattern *pattern;
  gchar *name;
  gulong bars;

  g_object_get (self->app, "song", &song, NULL);
  bt_child_proxy_get (song, "song-info::bars", &bars, NULL);

  name = bt_machine_get_unique_pattern_name (machine);
  // new_pattern
  pattern = bt_pattern_new (song, name, bars, machine);

  // free ressources
  g_free (name);
  g_object_unref (song);

  return pattern;
}

static gboolean
change_current_pattern (BtMainPagePatterns * self,
    BtPattern * new_pattern)
{
  BtPattern *old_pattern = self->pattern;

  GST_DEBUG ("change_pattern: %p -> %p", old_pattern, new_pattern);

  if (new_pattern == old_pattern) {
    GST_WARNING ("new pattern is the same as previous");
    return FALSE;
  }

  self->pattern = g_object_try_ref (new_pattern);
  GST_INFO ("store new pattern : %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (new_pattern));
  if (old_pattern) {
    if (self->pattern_length_changed) {
      g_signal_handler_disconnect (old_pattern,
          self->pattern_length_changed);
      self->pattern_length_changed = 0;
    }
    if (self->pattern_voices_changed) {
      g_signal_handler_disconnect (old_pattern,
          self->pattern_voices_changed);
      self->pattern_voices_changed = 0;
    }
    GST_INFO ("unref old pattern: %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (old_pattern));
    g_object_unref (old_pattern);
  }
  // select pattern combo entry
  if (new_pattern) {
    // watch the pattern
    self->pattern_length_changed =
        g_signal_connect (new_pattern, "notify::length",
        G_CALLBACK (on_pattern_size_changed), (gpointer) self);
    self->pattern_voices_changed =
        g_signal_connect (new_pattern, "notify::voices",
        G_CALLBACK (on_pattern_size_changed), (gpointer) self);
  }
  // refresh pattern view
  pattern_table_refresh (self);
  pattern_view_update_cell_description (self, UPDATE_COLUMN_UPDATE);
  gtk_widget_grab_focus_savely (GTK_WIDGET (self->pattern_table));
  return TRUE;
}

/**
 * get_current_machine:
 *
 * Returns: (transfer full) The machine currently selected in the page dropdown.
 */
static BtMachine *
get_current_machine (BtMainPagePatterns * self)
{
  GObject* item = gtk_drop_down_get_selected_item (self->machine_menu);
  if (item) {
    return BT_MACHINE (g_object_ref (item));
  } else {
    return NULL;
  }
}

static void
change_current_machine (BtMainPagePatterns * self,
    BtMachine * new_machine)
{
  BtMachine *old_machine = self->machine;

  if (new_machine == old_machine) {
    return;
  }

  GST_INFO ("store new machine %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (new_machine));
  self->machine = g_object_try_ref (new_machine);

  GST_INFO ("unref old machine %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (old_machine));
  g_object_try_unref (old_machine);

  // show new list of pattern in pattern menu
  pattern_menu_refresh (self, new_machine);
  GST_INFO ("1st done for  machine %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (new_machine));
  // refresh context menu
  context_menu_refresh (self, new_machine);
  GST_INFO ("2nd done for  machine %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (new_machine));
}

static void
switch_machine_and_pattern (BtMainPagePatterns * self,
    BtMachine * machine, BtPattern * pattern)
{
  if (machine) {
    // update machine menu
    guint idx = machine_menu_model_get_idx_by_machine (gtk_drop_down_get_model (self->machine_menu), machine);
    if (idx != GTK_INVALID_LIST_POSITION)
      gtk_drop_down_set_selected (self->machine_menu, idx);
  }
  if (pattern) {
    // update pattern menu
    guint idx = pattern_menu_model_get_idx_by_pattern (gtk_drop_down_get_model (self->pattern_menu), pattern);
    if (idx != GTK_INVALID_LIST_POSITION)
      gtk_drop_down_set_selected (self->pattern_menu, idx);
  }
}

static void
lookup_machine_and_pattern (BtMainPagePatterns * self,
    BtMachine ** machine, BtPattern ** pattern, gchar * mid, gchar * c_mid,
    gchar * pid, gchar * c_pid)
{
  if (!c_mid || strcmp (mid, c_mid)) {
    BtSetup *setup;
    // change machine and pattern
    bt_child_proxy_get (self->app, "song::setup", &setup, NULL);
    g_object_try_unref (*machine);
    *machine = bt_setup_get_machine_by_id (setup, mid);
    if (pid) {
      g_object_try_unref (*pattern);
      *pattern = (BtPattern *) bt_machine_get_pattern_by_name (*machine, pid);
      switch_machine_and_pattern (self, *machine, *pattern);
    }
    g_object_unref (setup);
  } else if (pid && (!c_pid || strcmp (pid, c_pid))) {
    // change pattern
    g_object_try_unref (*pattern);
    *pattern = (BtPattern *) bt_machine_get_pattern_by_name (*machine, pid);
    switch_machine_and_pattern (self, NULL, *pattern);
  }
}

//-- event handler

static void
on_page_mapped (GtkWidget * widget, gpointer user_data)
{
  GTK_WIDGET_GET_CLASS (widget)->focus (widget, GTK_DIR_TAB_FORWARD);
}

static void
on_page_switched (GtkNotebook * notebook, GParamSpec * arg, gpointer user_data)
{
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS (user_data);
  BtMainWindow *main_window;
  guint page_num;
  static gint prev_page_num = -1;

  g_object_get (notebook, "page", &page_num, NULL);

  if (page_num == BT_MAIN_PAGES_PATTERNS_PAGE) {
    // only do this if the page really has changed
    if (prev_page_num != BT_MAIN_PAGES_PATTERNS_PAGE) {
      BtSong *song;

      GST_DEBUG ("enter pattern page");
      if (self->machine) {
        // refresh to update colors in the menu (as usage might have changed)
        pattern_menu_refresh (self, self->machine);
      }
      // add local commands
      g_object_get (self->app, "main-window", &main_window, "song", &song,
          NULL);
      if (main_window) {
#if 0 /// GTK4
        gtk_window_add_accel_group (GTK_WINDOW (main_window),
            self->accel_group);
#endif
        g_object_unref (main_window);
      }
      if (song) {
        g_object_set (song, "is-idle", self->play_live, NULL);
        g_object_unref (song);
      }
    }
  } else {
    // only do this if the page was BT_MAIN_PAGES_PATTERNS_PAGE
    if (prev_page_num == BT_MAIN_PAGES_PATTERNS_PAGE) {
      BtSong *song;

      // only reset old
      GST_DEBUG ("leave pattern page");
      pattern_view_update_cell_description (self, UPDATE_COLUMN_POP);
      // remove local commands
      g_object_get (self->app, "main-window", &main_window, "song", &song,
          NULL);
      if (main_window) {
#if 0 /// GTK4
        gtk_window_remove_accel_group (GTK_WINDOW (main_window),
            self->accel_group);
#endif
        g_object_unref (main_window);
      }
      if (song) {
        g_object_set (song, "is-idle", FALSE, NULL);
        g_object_unref (song);
      }
    }
  }
  prev_page_num = page_num;
}

static void
on_pattern_size_changed (BtPattern * pattern, GParamSpec * arg,
    gpointer user_data)
{
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS (user_data);

  GST_INFO ("pattern size changed (%s): %p", arg->name, self->pattern);
  // FIXME(ensonic): as length and voices are no constructor properties we seem
  // to get notifies for new patterns and thus redraw things twice :/
  pattern_table_refresh (self);
  pattern_view_update_cell_description (self, UPDATE_COLUMN_UPDATE);
  gtk_widget_grab_focus_savely (GTK_WIDGET (self->pattern_table));
}

static void
on_pattern_menu_changed (GObject* object, GParamSpec* pspec, gpointer user_data)
{
  GtkDropDown *dropdown = GTK_DROP_DOWN (object);
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS (user_data);
  BtPattern *pattern = NULL;

  // refresh pattern view
  pattern = BT_PATTERN (gtk_drop_down_get_selected_item (dropdown));
  GST_DEBUG ("  got new pattern: %" G_OBJECT_REF_COUNT_FMT,
             G_OBJECT_LOG_REF_COUNT (pattern));
  
  if (!change_current_pattern (self, pattern)) {
    return;
  }

  if (self->properties) {
    gchar *prop, *pid = NULL;
    gboolean have_val = FALSE;

    if (pattern)
      g_object_get (pattern, "name", &pid, NULL);
    if ((prop =
            (gchar *) g_hash_table_lookup (self->properties,
                "selected-pattern"))) {
      have_val = TRUE;
    }
    if (!pid) {
      g_hash_table_remove (self->properties, "selected-pattern");
      if (have_val)             // irks, this is also triggered by undo and thus keeping the song dirty
        bt_edit_application_set_song_unsaved (self->app);
    } else if ((!have_val) || strcmp (prop, pid)) {
      g_hash_table_insert (self->properties,
          g_strdup ("selected-pattern"), pid);
      if (have_val)             // irks, this is also triggered by undo and thus keeping the song dirty
        bt_edit_application_set_song_unsaved (self->app);
      pid = NULL;
    }
    g_free (pid);
  }
}

static void
on_base_octave_menu_changed (GtkSpinButton * spinbutton, gpointer user_data)
{
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS (user_data);
  GHashTable *properties;

  self->base_octave = gtk_spin_button_get_value_as_int (spinbutton);
  g_object_set (self->pattern_table, "octave", self->base_octave,
      NULL);

  // remember for machine
  g_object_get (self->machine, "properties", &properties, NULL);
  g_hash_table_insert (properties, g_strdup ("base-octave"),
      g_strdup_printf ("%d", self->base_octave));
#if !GTK_CHECK_VERSION (3, 20, 0)
  gtk_widget_grab_focus_savely (GTK_WIDGET (self->pattern_table));
#endif
}

static void
on_play_live_toggled (GtkButton * button, gpointer user_data)
{
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS (user_data);
  BtSong *song;

  self->play_live =
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

  g_object_get (self->app, "song", &song, NULL);
  g_object_set (song, "is-idle", self->play_live, NULL);
  g_object_unref (song);
}

static void
on_machine_added (BtSetup * setup, BtMachine * machine, gpointer user_data)
{
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS (user_data);

  GST_INFO ("new machine added: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));

  if (bt_change_log_is_active (self->change_log)) {
    if (BT_IS_SOURCE_MACHINE (machine)) {
      BtPattern *pattern = add_new_pattern (self, machine);
      context_menu_refresh (self, machine);
      g_object_unref (pattern);
    }
  }
  
  g_signal_connect (machine, "pattern-removed", G_CALLBACK (on_pattern_removed),
      (gpointer) self);

  GST_INFO ("... machine added: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));
}

static void
on_machine_removed (BtSetup * setup, BtMachine * machine, gpointer user_data)
{
  //BtMainPagePatterns *self=BT_MAIN_PAGE_PATTERNS(user_data);
  GList *node, *list;
  BtCmdPattern *pattern;

  g_return_if_fail (BT_IS_MACHINE (machine));

  GST_INFO ("machine removed: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));

  // TODO(ensonic): just refresh the context menu? or are we doing this to kick
  // ordered undo/redo serialisation

  // remove all patterns to ensure we emit "pattern-removed" signals
  g_object_get (machine, "patterns", &list, NULL);
  for (node = list; node; node = g_list_next (node)) {
    pattern = BT_CMD_PATTERN (node->data);
    if (BT_IS_PATTERN (pattern)) {
      GST_DEBUG ("removing pattern: %" G_OBJECT_REF_COUNT_FMT,
          G_OBJECT_LOG_REF_COUNT (pattern));
      bt_machine_remove_pattern (machine, pattern);
      GST_DEBUG ("removed pattern: %" G_OBJECT_REF_COUNT_FMT,
          G_OBJECT_LOG_REF_COUNT (pattern));
    } else {
      GST_DEBUG ("keeping pattern: %" G_OBJECT_REF_COUNT_FMT,
          G_OBJECT_LOG_REF_COUNT (pattern));
    }
    g_object_unref (pattern);
  }
  g_list_free (list);
}

static void
on_wire_added (BtSetup * setup, BtWire * wire, gpointer user_data)
{
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS (user_data);
  BtMachine *this_machine, *that_machine;

  if (!self->pattern)
    return;
  g_object_get (self->pattern, "machine", &this_machine, NULL);
  g_object_get (wire, "dst", &that_machine, NULL);
  if (this_machine == that_machine) {
    pattern_table_refresh (self);
  }
  g_object_unref (this_machine);
  g_object_unref (that_machine);
}

static void
on_wire_removed (BtSetup * setup, BtWire * wire, gpointer user_data)
{
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS (user_data);
  BtMachine *this_machine, *that_machine;

  if (!self->pattern)
    return;

  g_object_get (self->pattern, "machine", &this_machine, NULL);
  g_object_get (wire, "dst", &that_machine, NULL);
  if (this_machine == that_machine) {
    pattern_table_refresh (self);
  }

  GST_INFO_OBJECT (wire, "wire removed: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (wire));

  // add undo/redo details
  if (bt_change_log_is_active (self->change_log)) {
    GList *list, *node;
    BtMachine *smachine;
    BtPattern *pattern;
    BtValueGroup *vg;
    gulong length;
    gulong wire_params;
    gchar *smid, *dmid;

    g_object_get (wire, "src", &smachine, "num-params", &wire_params, NULL);
    g_object_get (smachine, "id", &smid, NULL);
    g_object_get (that_machine, "id", &dmid, "patterns", &list, NULL);

    for (node = list; node; node = g_list_next (node)) {
      if (BT_IS_PATTERN (node->data)) {
        pattern = BT_PATTERN (node->data);
        if ((vg = bt_pattern_get_wire_group (self->pattern, wire))) {
          gchar *undo_str;
          GString *data = g_string_new (NULL);
          guint end, i;
          gchar *str, *p, *pid;

          g_object_get (pattern, "name", &pid, "length", &length, NULL);
          end = length - 1;

          bt_value_group_serialize_columns (vg, 0, end, data);
          str = data->str;

          bt_change_log_start_group (self->change_log);
          for (i = 0; i < wire_params; i++) {
            p = strchr (str, '\n');
            *p = '\0';
            undo_str =
                g_strdup_printf
                ("set_wire_events \"%s\",\"%s\",\"%s\",0,%u,%u,%s", smid, dmid,
                pid, end, i, str);
            str = &p[1];
            bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
                undo_str, g_strdup (undo_str));
          }
          bt_change_log_end_group (self->change_log);

          g_string_free (data, TRUE);
          g_free (pid);
        }
      }
      g_object_unref (node->data);
    }
    g_list_free (list);
    g_free (smid);
    g_free (dmid);
    g_object_unref (smachine);
  }
  g_object_unref (this_machine);
  g_object_unref (that_machine);

}

static void
on_machine_menu_changed (GObject* object, GParamSpec* pspec, gpointer user_data)
{
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS (user_data);
  BtMachine *machine;
  GHashTable *properties;
  gchar *prop;

  machine = get_current_machine (self);
  GST_DEBUG ("new machine is %s: %" G_OBJECT_REF_COUNT_FMT,
      (machine ? GST_OBJECT_NAME (machine) : ""),
      G_OBJECT_LOG_REF_COUNT (machine));
  change_current_machine (self, machine);
  if (self->properties) {
    gchar *prop, *mid;
    gboolean have_val = FALSE;

    g_object_get (self->machine, "id", &mid, NULL);
    if ((prop =
            (gchar *) g_hash_table_lookup (self->properties,
                "selected-machine"))) {
      have_val = TRUE;
    }
    if ((!have_val) || (strcmp (prop, mid))) {
      g_hash_table_insert (self->properties,
          g_strdup ("selected-machine"), mid);
      if (have_val)
        bt_edit_application_set_song_unsaved (self->app);
      mid = NULL;
    }
    g_free (mid);
  }
  // switch to last used base octave of that machine
  g_object_get (self->machine, "properties", &properties, NULL);
  if ((prop = (gchar *) g_hash_table_lookup (properties, "base-octave"))) {
    self->base_octave = atoi (prop);
  } else {
    self->base_octave = DEFAULT_BASE_OCTAVE;
  }
  gtk_spin_button_set_value (self->base_octave_menu,
      self->base_octave);

  // enable disable the wave combobox, depending on machine caps
  gtk_widget_set_sensitive (GTK_WIDGET (self->wavetable_menu),
      bt_machine_handles_waves (machine));

  g_object_try_unref (machine);
}

static void
on_sequence_tick (const BtSong * song, GParamSpec * arg, gpointer user_data)
{
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS (user_data);
  BtSequence *sequence;
  BtMachine *machine, *cur_machine = self->machine;
  BtCmdPattern *pattern;
  gulong i, pos, spos;
  glong j;
  gulong tracks, length, sequence_length;
  gdouble play_pos;
  gboolean found = FALSE;

  if (!self->pattern)
    return;

  g_object_get (self->pattern, "length", &length, NULL);
  g_object_get ((gpointer) song, "sequence", &sequence, "play-pos", &pos, NULL);
  g_object_get (sequence, "tracks", &tracks, "length", &sequence_length, NULL);

  if (pos < sequence_length) {
    // check all tracks
    // TODO(ensonic): what if the pattern is played on multiple tracks ?
    // we'd need to draw several lines
    for (i = 0; ((i < tracks) && !found); i++) {
      machine = bt_sequence_get_machine (sequence, i);
      if (machine == cur_machine) {
        // find pattern start in sequence (search <length> ticks from current pos
        spos = (pos > length) ? (pos - length) : 0;
        for (j = pos; ((j > spos) && !found); j--) {
          // get pattern for current machine and current tick from sequence
          if ((pattern = bt_sequence_get_pattern (sequence, j, i))) {
            // if it is the pattern we currently show, set play-line
            if (pattern == (BtCmdPattern *) self->pattern) {
              play_pos = (gdouble) (pos - j) / (gdouble) length;
              g_object_set (self->pattern_table, "play-position",
                  play_pos, NULL);
              found = TRUE;
            }
            g_object_unref (pattern);
            /* if there was a different pattern, don't look further */
            break;
          }
        }
      }
      g_object_unref (machine);
    }
  }
  if (!found) {
    // unfortunately the 2nd widget may lag behind with redrawing itself :(
    g_object_set (self->pattern_table, "play-position", -1.0, NULL);
  }
  // release the references
  g_object_unref (sequence);
}

static void
on_song_changed (const BtEditApplication * app, GParamSpec * arg,
    gpointer user_data)
{
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS (user_data);
  BtSong *song;
  BtSetup *setup;
  BtWavetable *wavetable;
  gchar *prop;

  GST_INFO ("song has changed : app=%p, self=%p", app, self);
  // get song from app and then setup from song
  g_object_get (self->app, "song", &song, NULL);
  if (!song) {
    self->properties = NULL;
    GST_INFO ("song (null) has changed done");
    return;
  }
  GST_INFO ("song: %" G_OBJECT_REF_COUNT_FMT, G_OBJECT_LOG_REF_COUNT (song));

  g_object_get (song, "setup", &setup, "wavetable", &wavetable, NULL);
  g_object_get (setup, "properties", &self->properties, NULL);

  g_signal_connect (wavetable, "items-changed",
      G_CALLBACK (on_wave_model_items_changed), (gpointer) self); 
                    
  // get stored settings
  if ((prop =
          (gchar *) g_hash_table_lookup (self->properties,
              "selected-machine"))) {
    BtMachine *machine;
    if ((machine = bt_setup_get_machine_by_id (setup, prop))) {
      change_current_machine (self, machine);
      g_object_unref (machine);
    }
  }
  if ((prop =
          (gchar *) g_hash_table_lookup (self->properties,
              "selected-pattern"))) {
    BtPattern *pattern;
    if ((pattern =
            (BtPattern *) bt_machine_get_pattern_by_name (self->machine,
                prop))) {
      if (change_current_pattern (self, pattern)) {
        GListModel *store = gtk_drop_down_get_model (self->pattern_menu);
        for (guint i = 0; i < g_list_model_get_n_items (store); ++i) {
          if (g_list_model_get_item (store, i) == pattern) {
            gtk_drop_down_set_selected (self->pattern_menu, i);
            GST_DEBUG ("selected new pattern");
            break;
          }
        }
        g_object_unref (pattern);
      }
    }
  }
  // update page
  machine_menu_refresh (self, setup);
  //pattern_menu_refresh(self); // should be triggered by machine_menu_refresh()
  wavetable_menu_refresh (self, wavetable);
  g_signal_connect_object (setup, "machine-added",
      G_CALLBACK (on_machine_added), (gpointer) self, 0);
  g_signal_connect_object (setup, "machine-removed",
      G_CALLBACK (on_machine_removed), (gpointer) self, G_CONNECT_AFTER);
  g_signal_connect_object (setup, "wire-added", G_CALLBACK (on_wire_added),
      (gpointer) self, G_CONNECT_AFTER);
  g_signal_connect_object (setup, "wire-removed", G_CALLBACK (on_wire_removed),
      (gpointer) self, 0);
  // subscribe to play-pos changes of song->sequence
  g_signal_connect_object (song, "notify::play-pos",
      G_CALLBACK (on_sequence_tick), (gpointer) self, 0l);
  // release the references
  g_object_unref (wavetable);
  g_object_unref (setup);
  g_object_unref (song);
  GST_INFO ("song has changed done");
}

static void
on_voice_add_activate (GtkWidget* widget, const char* action_name, GVariant* parameter)
{
  BtMainPagePatterns* self = BT_MAIN_PAGE_PATTERNS (widget);
  gulong voices;
  gchar *undo_str, *redo_str;
  gchar *mid;

  g_object_get (self->machine, "voices", &voices, "id", &mid, NULL);

  undo_str = g_strdup_printf ("set_voices \"%s\",%lu", mid, voices);
  redo_str = g_strdup_printf ("set_voices \"%s\",%lu", mid, voices + 1);
  bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self), undo_str,
      redo_str);
  g_free (mid);

  voices++;
  g_object_set (self->machine, "voices", voices, NULL);

  // we adjust sensitivity of add/rem voice menu items
  context_menu_refresh (self, self->machine);
}

static void
on_voice_remove_activate (GtkWidget* widget, const char* action_name, GVariant* parameter)
{
  BtMainPagePatterns* self = BT_MAIN_PAGE_PATTERNS (widget);
  BtPattern *saved_pattern;
  BtValueGroup *vg;
  gulong voices;
  gchar *undo_str, *redo_str;
  gchar *mid;
  GList *list, *node;
  GString *old_data;
  gulong length;
  gint group;

  g_object_get (self->machine, "voices", &voices, "id", &mid, "patterns",
      &list, NULL);

  bt_change_log_start_group (self->change_log);
  undo_str = g_strdup_printf ("set_voices \"%s\",%lu", mid, voices);
  redo_str = g_strdup_printf ("set_voices \"%s\",%lu", mid, voices - 1);
  bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self), undo_str,
      redo_str);

  // find the group for the last voice
  vg = bt_pattern_get_voice_group (self->pattern, voices - 1);
  for (group = 0; group < self->number_of_groups; group++) {
    if (self->param_groups[group].vg == vg)
      break;
  }
  /* TODO(ensonic): this is hackish, we muck with self->pattern as
   * some local methods assume the current pattern */
  saved_pattern = self->pattern;
  /* save voice-data for *all* patterns of this machine */
  for (node = list; node; node = g_list_next (node)) {
    if (BT_IS_PATTERN (node->data)) {
      self->pattern = (BtPattern *) (node->data);
      g_object_get (self->pattern, "length", &length, NULL);
      old_data = g_string_new (NULL);
      pattern_range_copy (self, 0, length - 1, group, -1, old_data);
      pattern_range_log_undo_redo (self, 0, length - 1, group, -1,
          old_data->str, g_strdup (old_data->str));
      g_string_free (old_data, TRUE);
    }
    g_object_unref (node->data);
  }
  self->pattern = saved_pattern;
  g_list_free (list);
  g_free (mid);
  bt_change_log_end_group (self->change_log);

  voices--;
  g_object_set (self->machine, "voices", voices, NULL);

  // we adjust sensitivity of add/rem voice menu items
  context_menu_refresh (self, self->machine);
}

static void
on_pattern_new_activate (GtkWidget* widget, const char* action_name, GVariant* parameter)
{
  BtMainPagePatterns* self = BT_MAIN_PAGE_PATTERNS (widget);
  BtPattern *pattern = add_new_pattern (self, self->machine);
  gtk_window_present (GTK_WINDOW (bt_pattern_properties_dialog_new (pattern)));
 
#if 0 /// GTK4
  GtkWidget *dialog;

  // pattern_properties
  dialog = GTK_WIDGET (bt_pattern_properties_dialog_new (pattern));
  bt_edit_application_attach_child_window (self->app,
      GTK_WINDOW (dialog));
  gtk_widget_show_all (dialog);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
    gchar *undo_str, *redo_str;
    gchar *mid, *pid;
    gulong length;

    bt_pattern_properties_dialog_apply (BT_PATTERN_PROPERTIES_DIALOG (dialog));

    GST_INFO ("new pattern added : %" G_OBJECT_REF_COUNT_FMT,
        G_OBJECT_LOG_REF_COUNT (pattern));

    g_object_get (self->machine, "id", &mid, NULL);
    g_object_get (pattern, "name", &pid, "length", &length, NULL);

    undo_str = g_strdup_printf ("rem_pattern \"%s\",\"%s\"", mid, pid);
    redo_str =
        g_strdup_printf ("add_pattern \"%s\",\"%s\",%lu", mid, pid, length);
    bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
        undo_str, redo_str);
    g_free (mid);
    g_free (pid);

    context_menu_refresh (self, self->machine);
  } else {
    bt_machine_remove_pattern (self->machine, (BtCmdPattern *) pattern);
  }
  gtk_widget_destroy (dialog);

  // free ressources
  g_object_unref (pattern);
  GST_DEBUG ("new pattern done");
#endif
}

static void
on_pattern_properties_activate (GtkWidget* widget, const char* action_name, GVariant* parameter)
{
  BtMainPagePatterns* self = BT_MAIN_PAGE_PATTERNS (widget);
  g_return_if_fail (self->pattern);

  gtk_window_present (GTK_WINDOW (bt_pattern_properties_dialog_new (self->pattern)));

#if 0 /// GTK4
  GtkWidget *dialog;

  // pattern_properties
  dialog = GTK_WIDGET (bt_pattern_properties_dialog_new (self->pattern));
  bt_edit_application_attach_child_window (self->app,
      GTK_WINDOW (dialog));
  gtk_widget_show_all (dialog);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
    BtMachine *machine;
    gchar *new_name, *old_name;
    gulong new_length, old_length, new_voices, old_voices;
    gchar *undo_str, *redo_str;
    gchar *mid;

    /* we need to check what got changed in the properties to do the undo/redo
     * before applying the changes, we can check the settings from the dialog
     */
    g_object_get (self->pattern, "name", &old_name, "length",
        &old_length, "voices", &old_voices, "machine", &machine, NULL);
    g_object_get (dialog, "name", &new_name, "length", &new_length, "voices",
        &new_voices, NULL);
    g_object_get (machine, "id", &mid, NULL);

    bt_change_log_start_group (self->change_log);

    if (strcmp (old_name, new_name)) {
      undo_str =
          g_strdup_printf ("set_pattern_property \"%s\",\"%s\",\"name\",\"%s\"",
          mid, new_name, old_name);
      redo_str =
          g_strdup_printf ("set_pattern_property \"%s\",\"%s\",\"name\",\"%s\"",
          mid, old_name, new_name);
      bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
          undo_str, redo_str);
    }
    if (old_length != new_length) {
      undo_str =
          g_strdup_printf
          ("set_pattern_property \"%s\",\"%s\",\"length\",\"%lu\"", mid,
          old_name, old_length);
      redo_str =
          g_strdup_printf
          ("set_pattern_property \"%s\",\"%s\",\"length\",\"%lu\"", mid,
          old_name, new_length);
      bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
          undo_str, redo_str);
      if (old_length > new_length) {
        GString *old_data = g_string_new (NULL);
        pattern_range_copy (self, new_length, old_length - 1, -1, -1, old_data);
        pattern_range_log_undo_redo (self, new_length, old_length - 1, -1, -1,
            old_data->str, g_strdup (old_data->str));
        g_string_free (old_data, TRUE);
      }
    }
    if (old_voices != new_voices) {
      undo_str = g_strdup_printf ("set_voices \"%s\",%lu", mid, old_voices);
      redo_str = g_strdup_printf ("set_voices \"%s\",%lu", mid, new_voices);
      bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
          undo_str, redo_str);
      if (old_voices > new_voices) {
        BtPattern *saved_pattern;
        BtValueGroup *vg;
        GList *list, *node;
        GString *old_data;
        gulong length;
        gint group, beg_group, end_group;

        // find the group for the removed voice(s)
        vg = bt_pattern_get_voice_group (self->pattern, new_voices);
        for (group = 0; group < self->number_of_groups; group++) {
          if (self->param_groups[group].vg == vg)
            break;
        }
        beg_group = group;
        end_group = group + (old_voices - new_voices);
        /* TODO(ensonic): this is hackish, we muck with self->pattern as
         * some local methods assume the current pattern */
        saved_pattern = self->pattern;
        /* save voice-data for *all* patterns of this machine */
        g_object_get (machine, "patterns", &list, NULL);
        for (node = list; node; node = g_list_next (node)) {
          if (BT_IS_PATTERN (node->data)) {
            self->pattern = (BtPattern *) (node->data);
            g_object_get (self->pattern, "length", &length, NULL);
            for (group = beg_group; group < end_group; group++) {
              old_data = g_string_new (NULL);
              pattern_range_copy (self, 0, length - 1, group, -1, old_data);
              pattern_range_log_undo_redo (self, 0, length - 1, group, -1,
                  old_data->str, g_strdup (old_data->str));
              g_string_free (old_data, TRUE);
            }
          }
          g_object_unref (node->data);
        }
        self->pattern = saved_pattern;
        g_list_free (list);
      }
    }

    bt_change_log_end_group (self->change_log);

    bt_pattern_properties_dialog_apply (BT_PATTERN_PROPERTIES_DIALOG (dialog));
    g_free (mid);
    g_free (old_name);
    g_free (new_name);
  }
  gtk_widget_destroy (dialog);
#endif
}

static void
on_pattern_remove_activate (GtkWidget* widget, const char* action_name, GVariant* parameter)
{
  BtMainPagePatterns* self = BT_MAIN_PAGE_PATTERNS (widget);
  BtCmdPattern *pattern = (BtCmdPattern *) self->pattern;
  BtMachine *machine;

  g_return_if_fail (pattern);

  g_object_get (pattern, "machine", &machine, NULL);

  bt_change_log_start_group (self->change_log);
  bt_machine_remove_pattern (machine, pattern);
  bt_change_log_end_group (self->change_log);

  context_menu_refresh (self, machine);

  g_object_unref (machine);
}

static void
on_pattern_copy_activate (GtkWidget* widget, const char* action_name, GVariant* parameter)
{
  BtMainPagePatterns* self = BT_MAIN_PAGE_PATTERNS (widget);
  BtMachine *machine;
  BtPattern *pattern;

  g_return_if_fail (self->pattern);

  // copy pattern
  pattern = bt_pattern_copy (self->pattern);
  g_return_if_fail (pattern);
  g_object_get (pattern, "machine", &machine, NULL);

  // pattern_properties
  g_return_if_fail (self->pattern);

  gtk_window_present (GTK_WINDOW (bt_pattern_properties_dialog_new (self->pattern)));

#if 0 /// GTK4
  guint current_pattern_ix = gtk_drop_down_get_selected (self->pattern_menu);
  GtkWidget *dialog;
  dialog = GTK_WIDGET (bt_pattern_properties_dialog_new (pattern));
  bt_edit_application_attach_child_window (self->app,
      GTK_WINDOW (dialog));
  gtk_widget_show_all (dialog);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
    gchar *undo_str, *redo_str;
    gchar *mid, *pid;
    gulong length;

    bt_pattern_properties_dialog_apply (BT_PATTERN_PROPERTIES_DIALOG (dialog));

    GST_INFO ("new pattern added : %p", pattern);

    g_object_get (machine, "id", &mid, NULL);
    g_object_get (pattern, "name", &pid, "length", &length, NULL);

    undo_str = g_strdup_printf ("rem_pattern \"%s\",\"%s\"", mid, pid);
    redo_str =
        g_strdup_printf ("add_pattern \"%s\",\"%s\",%lu", mid, pid, length);
    bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
        undo_str, redo_str);
    g_free (mid);
    g_free (pid);

    context_menu_refresh (self, machine);
  } else {
    bt_machine_remove_pattern (machine, (BtCmdPattern *) pattern);
    gtk_combo_box_set_active (self->pattern_menu, current_pattern_ix);
  }
  gtk_widget_destroy (dialog);
#endif

  g_object_unref (pattern);
  g_object_unref (machine);
}

static void
on_machine_properties_activate (GtkWidget* widget, const char* action_name, GVariant* parameter)
{
  BtMainPagePatterns* self = BT_MAIN_PAGE_PATTERNS (widget);
  bt_machine_show_properties_dialog (self->machine);
}

static void
on_machine_preferences_activate (GtkWidget* widget, const char* action_name, GVariant* parameter)
{
  BtMainPagePatterns* self = BT_MAIN_PAGE_PATTERNS (widget);
  bt_machine_show_preferences_dialog (self->machine);
}

//-- helper methods

static void
bt_main_page_patterns_init_ui (BtMainPagePatterns * self,
    const BtMainPages * pages)
{
  GST_DEBUG ("!!!! self=%p", self);

  gtk_widget_set_name (GTK_WIDGET (self), "pattern view");              
                                                                        
  g_signal_connect (self->pattern_menu, "notify::selected",
      G_CALLBACK (on_pattern_menu_changed), (gpointer) self);
  
  g_object_set (self->pattern_table, "octave", self->base_octave, 
      "play-position", -1.0, "has-tooltip", TRUE, NULL);                
  //gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window),GTK_WIDGET(self->pattern_table));

  g_signal_connect (self->machine_menu, "notify::selected",  
      G_CALLBACK (on_machine_menu_changed), (gpointer) self); 

  
  GtkEventController* evt = gtk_event_controller_key_new ();
  gtk_widget_add_controller (GTK_WIDGET (self->pattern_table), evt);
  g_signal_connect (evt, "key-pressed",       
      G_CALLBACK (on_pattern_table_key_press_event), (gpointer) self);
  
  GtkGesture* gesture = gtk_gesture_click_new ();
  gtk_widget_add_controller (GTK_WIDGET (self->pattern_table), GTK_EVENT_CONTROLLER (gesture));
  g_signal_connect (gesture, "pressed",    
      G_CALLBACK (on_pattern_table_button_press_event), (gpointer) self);
  
  g_signal_connect (self->pattern_table, "notify::cursor-group",  
      G_CALLBACK (on_pattern_table_cursor_group_changed), (gpointer) self); 
  g_signal_connect (self->pattern_table, "notify::cursor-param",  
      G_CALLBACK (on_pattern_table_cursor_param_changed), (gpointer) self); 
  g_signal_connect (self->pattern_table, "notify::cursor-row",    
      G_CALLBACK (on_pattern_table_cursor_row_changed), (gpointer) self); 
  g_signal_connect (self->pattern_table, "query-tooltip",         
      G_CALLBACK (on_pattern_table_query_tooltip), (gpointer) self);    
                                                                        
  gtk_widget_set_name (GTK_WIDGET (self->pattern_table),          
      "pattern editor");                                                
                                                                        
  // generate the context menu                                          
                                                                        
  GtkBuilder *builder = gtk_builder_new_from_resource(
      "/org/buzztrax/ui/machine-rename-dialog-context-menu.ui");
                                                                        
  self->context_menu = G_MENU (gtk_builder_get_object (builder, "menu")); 
  g_object_ref (self->context_menu);                              
                                                                        
  g_object_unref (builder);                                             
                                                                        
  // register event handlers                                            
  g_signal_connect_object ((gpointer) (self->app), "notify::song", 
      G_CALLBACK (on_song_changed), (gpointer) self, 0);                
  // listen to page changes                                             
  g_signal_connect ((gpointer) pages, "notify::page",                   
      G_CALLBACK (on_page_switched), (gpointer) self);                  
  g_signal_connect ((gpointer) self, "map",                             
      G_CALLBACK (on_page_mapped), (gpointer) self);                    

  // let settings control toolbar style                                 
#if 0 /// GTK4                                                          
  g_object_get (self->app, "settings", &settings, NULL);          
  g_object_bind_property_full (settings, "toolbar-style", toolbar,      
      "toolbar-style", G_BINDING_SYNC_CREATE, bt_toolbar_style_changed, NULL, 
      NULL, NULL);                                                      
#endif                                                                  
  GST_DEBUG ("  done");                                                 
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
BtMainPagePatterns *                                                    
bt_main_page_patterns_new (const BtMainPages * pages)                   
{                                                                       
  BtMainPagePatterns *self;                                             
                                                                        
  self =                                                                
      BT_MAIN_PAGE_PATTERNS (g_object_new (BT_TYPE_MAIN_PAGE_PATTERNS, NULL)); 
  bt_main_page_patterns_init_ui (self, pages);                          
  return self;                                                          
}                                                                       
                                                                        
//-- methods                                                            
                                                                        
/**                                                                     
 * bt_main_page_patterns_show_pattern:                                  
 * @self: the pattern subpage                                           
 * @pattern: the pattern to show                                        
 *                                                                      
 * Show the given @pattern. Will update machine and pattern menu.       
 */                                                                     
void                                                                    
bt_main_page_patterns_show_pattern (BtMainPagePatterns * self,    
    BtPattern * pattern)                                                
{                                                                       
  BtMachine *machine;                                                   
                                                                        
  g_object_get (pattern, "machine", &machine, NULL);                    
  switch_machine_and_pattern (self, machine, pattern);                  
  // focus pattern editor                                               
  gtk_widget_grab_focus_savely (GTK_WIDGET (self->pattern_table)); 
  // release the references                                             
  g_object_unref (machine);                                             
}                                                                       
                                                                        
/**                                                                     
 * bt_main_page_patterns_show_machine:                                  
 * @self: the pattern subpage                                           
 * @machine: the machine to show                                        
 *                                                                      
 * Show the given @machine. Will update machine menu.                   
 */                                                                     
void                                                                    
bt_main_page_patterns_show_machine (BtMainPagePatterns * self,    
    BtMachine * machine)                                                
{                                                                       
  GListModel* store = gtk_drop_down_get_model (self->machine_menu);
  
  // update machine menu
  for (guint i = 0; i < g_list_model_get_n_items (store); ++i) {
    if (g_list_model_get_item (store, i) == machine) {
      gtk_drop_down_set_selected (self->machine_menu, i);
      break;
    }
  }
  
  // focus pattern editor                                               
  gtk_widget_grab_focus_savely (GTK_WIDGET (self->pattern_table)); 
}                                                                       
                                                                        
//-- cut/copy/paste                                                     
                                                                        
#if 0 /// GTK 4                                                         
static void                                                             
pattern_clipboard_get_func (GtkClipboard * clipboard,                   
    GtkSelectionData * selection_data, guint info, gpointer data)       
{                                                                       
  GST_INFO ("get clipboard data, info=%d, data=%p", info, data);        
  GST_INFO ("sending : [%s]", (gchar *) data);                          
  // TODO(ensonic): do we need to format differently depending on info? 
  if (gtk_selection_data_get_target (selection_data) == pattern_atom) { 
    gtk_selection_data_set (selection_data, pattern_atom, 8, (guchar *) data, 
        strlen (data));                                                 
  } else {                                                              
    // allow pasting into a test editor for debugging                   
    // its only active if we register the formats in _copy_selection() below 
    gtk_selection_data_set_text (selection_data, data, -1);
  }
}

static void
pattern_clipboard_clear_func (GtkClipboard * clipboard, gpointer data)
{
  GST_INFO ("freeing clipboard data, data=%p", data);
  g_free (data);
}
#endif

/**
 * bt_main_page_patterns_delete_selection:
 * @self: the pattern subpage
 *
 * Delete (clear) the selected area.
 */
void
bt_main_page_patterns_delete_selection (BtMainPagePatterns * self)
{
  pattern_selection_apply (self, BT_VALUE_GROUP_OP_CLEAR);
}

/**
 * bt_main_page_patterns_cut_selection:
 * @self: the pattern subpage
 *
 * Cut selected area.
 */
void
bt_main_page_patterns_cut_selection (BtMainPagePatterns * self)
{
  /// GTK4 bt_main_page_patterns_copy_selection (self);
  /// GTK4 bt_main_page_patterns_delete_selection (self);
}

/**
 * bt_main_page_patterns_copy_selection:
 * @self: the sequence subpage
 *
 * Copy selected area.
 */
#if 0 /// GTK 4                                                         
void
bt_main_page_patterns_copy_selection (BtMainPagePatterns * self)
{
  gint beg, end, group, param;
  if (bt_pattern_editor_get_selection (self->pattern_table, &beg, &end,
          &group, &param)) {
    //GtkClipboard *cb=gtk_clipboard_get_for_display(gdk_display_get_default(),GDK_SELECTION_CLIPBOARD);
    //GtkClipboard *cb=gtk_widget_get_clipboard(GTK_WIDGET(self->pattern_table),GDK_SELECTION_SECONDARY);
    GtkClipboard *cb =
        gtk_widget_get_clipboard (GTK_WIDGET (self->pattern_table),
        GDK_SELECTION_CLIPBOARD);
    GtkTargetEntry *targets;
    gint n_targets;
    GString *data = g_string_new (NULL);

    targets = gtk_target_table_make (pattern_atom, &n_targets);

    /* the number of ticks */
    g_string_append_printf (data, "%d\n", end - beg);
    pattern_range_copy (self, beg, end, group, param, data);

    GST_INFO ("copying : [%s]", data->str);

    /* put to clipboard */
    if (gtk_clipboard_set_with_data (cb, targets, n_targets,
            pattern_clipboard_get_func, pattern_clipboard_clear_func,
            g_string_free (data, FALSE))
        ) {
      gtk_clipboard_set_can_store (cb, NULL, 0);
    } else {
      GST_INFO ("copy failed");
    }

    gtk_target_table_free (targets, n_targets);
    GST_INFO ("copy done");
  }
}

static void
pattern_clipboard_received_func (GtkClipboard * clipboard,
    GtkSelectionData * selection_data, gpointer user_data)
{
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS (user_data);
  gchar **lines;
  gchar *data;

  GST_INFO ("receiving clipboard data");

  data = (gchar *) gtk_selection_data_get_data (selection_data);
  GST_INFO ("pasting : [%s]", data);

  if (!data)
    return;

  lines = g_strsplit_set (data, "\n", 0);
  if (lines[0]) {
    guint ticks = atol (lines[0]);
    gint i = 1, g, p;
    gint beg, end;
    gulong pattern_length;
    BtPatternEditorColumnGroup *pc_group;
    gboolean res = TRUE;

    g_object_get (self->pattern, "length", &pattern_length, NULL);
    pattern_length--;
    // paste from self->cursor_row to MIN(self->cursor_row+ticks,pattern_length)
    beg = self->cursor_row;
    end = beg + ticks;
    end = MIN (end, pattern_length);
    GST_INFO ("pasting from row %d to %d", beg, end);

    g = self->cursor_group;
    p = self->cursor_param;
    pc_group = &self->param_groups[g];
    // process each line (= pattern column)
    while (lines[i] && *lines[i] && res) {
      if (*lines[i] != '\n') {
        res =
            bt_value_group_deserialize_column (pc_group->vg, beg, end, p,
            lines[i]);
      } else {
        GST_INFO ("skip blank line");
      }
      i++;
      p++;
      if (p == pc_group->num_columns) {
        // switch to next group or stop
        if (g < self->number_of_groups) {
          g++;
          p = 0;
          pc_group = &self->param_groups[g];
        } else {
          break;
        }
      }
    }
    gtk_widget_queue_draw (GTK_WIDGET (self->pattern_table));
  }
  g_strfreev (lines);
}

/**
 * bt_main_page_patterns_paste_selection:
 * @self: the pattern subpage
 *
 * Paste at the top of the selected area.
 */
void
bt_main_page_patterns_paste_selection (BtMainPagePatterns * self)
{
  //GtkClipboard *cb=gtk_clipboard_get_for_display(gdk_display_get_default(),GDK_SELECTION_CLIPBOARD);
  //GtkClipboard *cb=gtk_widget_get_clipboard(GTK_WIDGET(self->pattern_table),GDK_SELECTION_SECONDARY);
  GtkClipboard *cb =
      gtk_widget_get_clipboard (GTK_WIDGET (self->pattern_table),
      GDK_SELECTION_CLIPBOARD);

  gtk_clipboard_request_contents (cb, pattern_atom,
      pattern_clipboard_received_func, (gpointer) self);
}
#endif

//-- change logger interface

gboolean
bt_main_page_patterns_change_logger_change (BtChangeLogger * owner,
    const gchar * data)
{
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS (owner);
  gboolean res = FALSE;
  BtMachine *machine;
  BtPattern *pattern = g_object_try_ref (self->pattern);
  gchar *c_mid, *c_pid;
  GMatchInfo *match_info;
  guint s_row = 0, e_row = 0, group = 0, param = 0;
  gchar *s;

  if (pattern) {
    g_object_get (pattern, "machine", &machine, "name", &c_pid, NULL);
    g_object_get (machine, "id", &c_mid, NULL);
  } else {
    machine = NULL;
    c_pid = NULL;
    c_mid = NULL;
  }

  GST_INFO ("undo/redo: [%s]", data);
  // parse data and apply action
  switch (bt_change_logger_match_method (change_logger_methods, data,
          &match_info)) {
    case METHOD_SET_GLOBAL_EVENTS:{
      BtValueGroup *vg;
      gchar *str, *mid, *pid;

      mid = g_match_info_fetch (match_info, 1);
      pid = g_match_info_fetch (match_info, 2);
      s = g_match_info_fetch (match_info, 3);
      s_row = atoi (s);
      g_free (s);
      s = g_match_info_fetch (match_info, 4);
      e_row = atoi (s);
      g_free (s);
      s = g_match_info_fetch (match_info, 5);
      param = atoi (s);
      g_free (s);
      str = g_match_info_fetch (match_info, 6);
      g_match_info_free (match_info);

      GST_DEBUG ("-> [%s|%s|%u|%u|%u|%s]", mid, pid, s_row, e_row, param, str);
      lookup_machine_and_pattern (self, &machine, &pattern, mid, c_mid, pid,
          c_pid);
      vg = bt_pattern_get_global_group (pattern);
      res = bt_value_group_deserialize_column (vg, s_row, e_row, param, str);
      g_free (str);
      g_free (mid);
      g_free (pid);

      if (res) {
        // move cursor
        for (group = 0; group < self->number_of_groups; group++) {
          if (self->param_groups[group].vg == vg)
            break;
        }
        g_object_set (self->pattern_table, "cursor-row", s_row,
            "cursor-group", group, "cursor-param", param, NULL);
        pattern_table_refresh (self);
      }
      break;
    }
    case METHOD_SET_VOICE_EVENTS:{
      BtValueGroup *vg;
      guint voice;
      gchar *str, *mid, *pid;

      mid = g_match_info_fetch (match_info, 1);
      pid = g_match_info_fetch (match_info, 2);
      s = g_match_info_fetch (match_info, 3);
      s_row = atoi (s);
      g_free (s);
      s = g_match_info_fetch (match_info, 4);
      e_row = atoi (s);
      g_free (s);
      s = g_match_info_fetch (match_info, 5);
      voice = atoi (s);
      g_free (s);
      s = g_match_info_fetch (match_info, 6);
      param = atoi (s);
      g_free (s);
      str = g_match_info_fetch (match_info, 7);
      g_match_info_free (match_info);

      GST_DEBUG ("-> [%s|%s|%u|%u|%u|%u|%s]", mid, pid, s_row, e_row, voice,
          param, str);
      lookup_machine_and_pattern (self, &machine, &pattern, mid, c_mid, pid,
          c_pid);
      vg = bt_pattern_get_voice_group (pattern, voice);
      res = bt_value_group_deserialize_column (vg, s_row, e_row, param, str);
      g_free (str);
      g_free (mid);
      g_free (pid);

      if (res) {
        // move cursor
        for (group = 0; group < self->number_of_groups; group++) {
          if (self->param_groups[group].vg == vg)
            break;
        }
        g_object_set (self->pattern_table, "cursor-row", s_row,
            "cursor-group", group, "cursor-param", param, NULL);
        pattern_table_refresh (self);
      }
      break;
    }
    case METHOD_SET_WIRE_EVENTS:{
      gchar *str, *smid, *dmid, *pid;
      BtSetup *setup;
      BtMachine *smachine;
      BtValueGroup *vg;
      BtWire *wire;

      smid = g_match_info_fetch (match_info, 1);
      dmid = g_match_info_fetch (match_info, 2);
      pid = g_match_info_fetch (match_info, 3);
      s = g_match_info_fetch (match_info, 4);
      s_row = atoi (s);
      g_free (s);
      s = g_match_info_fetch (match_info, 5);
      e_row = atoi (s);
      g_free (s);
      s = g_match_info_fetch (match_info, 6);
      param = atoi (s);
      g_free (s);
      str = g_match_info_fetch (match_info, 7);
      g_match_info_free (match_info);

      GST_DEBUG ("-> [%s|%s|%s|%u|%u|%u|%s]", smid, dmid, pid, s_row, e_row,
          param, str);
      lookup_machine_and_pattern (self, &machine, &pattern, dmid, c_mid, pid,
          c_pid);

      bt_child_proxy_get (self->app, "song::setup", &setup, NULL);
      smachine = bt_setup_get_machine_by_id (setup, smid);
      wire = bt_setup_get_wire_by_machines (setup, smachine, machine);
      vg = bt_pattern_get_wire_group (pattern, wire);
      res = bt_value_group_deserialize_column (vg, s_row, e_row, param, str);
      g_free (str);
      g_free (smid);
      g_free (dmid);
      g_free (pid);
      g_object_unref (wire);
      g_object_unref (smachine);
      g_object_unref (setup);

      if (res) {
        // move cursor
        for (group = 0; group < self->number_of_groups; group++) {
          if (self->param_groups[group].vg == vg)
            break;
        }
        g_object_set (self->pattern_table, "cursor-row", s_row,
            "cursor-group", group, "cursor-param", param, NULL);
        pattern_table_refresh (self);
      }
      break;
    }
    case METHOD_SET_PROPERTY:{
      gchar *mid, *pid, *key, *val;

      mid = g_match_info_fetch (match_info, 1);
      pid = g_match_info_fetch (match_info, 2);
      key = g_match_info_fetch (match_info, 3);
      val = g_match_info_fetch (match_info, 4);
      g_match_info_free (match_info);

      GST_DEBUG ("-> [%s|%s|%s|%s]", mid, pid, key, val);

      lookup_machine_and_pattern (self, &machine, &pattern, mid, c_mid, pid,
          c_pid);

      res = TRUE;
      if (!strcmp (key, "name")) {
        g_object_set (pattern, "name", val, NULL);
      } else if (!strcmp (key, "length")) {
        g_object_set (pattern, "length", atol (val), NULL);
      } else {
        GST_WARNING ("unhandled property '%s'", key);
        res = FALSE;
      }

      g_free (mid);
      g_free (pid);
      g_free (key);
      g_free (val);
      break;
    }
    case METHOD_ADD_PATTERN:{
      gchar *mid, *pid;
      gulong length;
      BtSong *song;

      mid = g_match_info_fetch (match_info, 1);
      pid = g_match_info_fetch (match_info, 2);
      s = g_match_info_fetch (match_info, 3);
      length = atol (s);
      g_free (s);
      g_match_info_free (match_info);

      GST_DEBUG ("-> [%s|%s|%lu]", mid, pid, length);
      lookup_machine_and_pattern (self, &machine, NULL, mid, c_mid, NULL, NULL);
      g_object_get (self->app, "song", &song, NULL);
      pattern = bt_pattern_new (song, pid, length, machine);
      g_object_unref (song);
      res = TRUE;
      g_free (mid);
      g_free (pid);

      context_menu_refresh (self, machine);
      break;
    }
    case METHOD_REM_PATTERN:{
      gchar *mid, *pid;

      mid = g_match_info_fetch (match_info, 1);
      pid = g_match_info_fetch (match_info, 2);
      g_match_info_free (match_info);

      GST_DEBUG ("-> [%s|%s]", mid, pid);
      lookup_machine_and_pattern (self, &machine, &pattern, mid, c_mid, pid,
          c_pid);
      bt_machine_remove_pattern (machine, (BtCmdPattern *) pattern);
      res = TRUE;

      context_menu_refresh (self, machine);
      break;
    }
    case METHOD_SET_VOICES:{
      gchar *mid;
      gulong voices;

      mid = g_match_info_fetch (match_info, 1);
      s = g_match_info_fetch (match_info, 2);
      voices = atol (s);
      g_free (s);
      g_match_info_free (match_info);

      GST_DEBUG ("-> [%s]", mid);
      lookup_machine_and_pattern (self, &machine, NULL, mid, c_mid, NULL, NULL);
      g_object_set (machine, "voices", voices, NULL);

      context_menu_refresh (self, machine);
      break;
    }
    default:
      GST_WARNING ("unhandled undo/redo method: [%s]", data);
  }

  g_object_try_unref (machine);
  g_object_try_unref (pattern);
  g_free (c_mid);
  g_free (c_pid);

  GST_INFO ("undo/redo: %s : [%s]", (res ? "okay" : "failed"), data);
  return res;
}

void
bt_main_page_patterns_change_logger_interface_init (gpointer const g_iface,
    gconstpointer const iface_data)
{
  BtChangeLoggerInterface *const iface = g_iface;

  iface->change = bt_main_page_patterns_change_logger_change;
}

//-- wrapper

//-- class internals

gboolean
bt_main_page_patterns_focus (GtkWidget * widget, GtkDirectionType direction)
{
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS (widget);

  GST_DEBUG ("focusing default widget");
  gtk_widget_grab_focus_savely (GTK_WIDGET (self->pattern_table));
  // only set new text
  pattern_view_update_cell_description (self, UPDATE_COLUMN_PUSH);
  return FALSE;
}

void
bt_main_page_patterns_dispose (GObject * object)
{
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS (object);

  return_if_disposed_self ();
  self->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  g_object_unref (self->change_log);
  g_object_unref (self->app);
  GST_DEBUG ("unref pattern: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (self->pattern));
  g_object_try_unref (self->pattern);
  GST_DEBUG ("unref machine: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (self->machine));
  g_object_try_unref (self->machine);

  g_object_unref (self->context_menu);

  /// GTK4 g_object_try_unref (self->accel_group);

  gtk_widget_dispose_template (GTK_WIDGET (self), BT_TYPE_MAIN_PAGE_PATTERNS);
  
  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (bt_main_page_patterns_parent_class)->dispose (object);
}

void
bt_main_page_patterns_finalize (GObject * object)
{
  BtMainPagePatterns *self = BT_MAIN_PAGE_PATTERNS (object);

  g_free (self->column_keymode);

  pattern_table_clear (self);

  G_OBJECT_CLASS (bt_main_page_patterns_parent_class)->finalize (object);
}

void
bt_main_page_patterns_init (BtMainPagePatterns * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  
  self = bt_main_page_patterns_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  self->app = bt_edit_application_new ();

  //self->cursor_column=0;
  //self->cursor_row=0;
  self->selection_start_column = -1;
  self->selection_start_row = -1;
  self->selection_end_column = -1;
  self->selection_end_row = -1;

  self->base_octave = DEFAULT_BASE_OCTAVE;

  // the undo/redo changelogger
  self->change_log = bt_change_log_new ();
  bt_change_log_register (self->change_log, BT_CHANGE_LOGGER (self));

  gtk_orientable_set_orientation (GTK_ORIENTABLE (self),
      GTK_ORIENTATION_VERTICAL);
}

void
bt_main_page_patterns_class_init (BtMainPagePatternsClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
#if 0 /// GTK4
  pattern_atom =
      gdk_atom_intern_static_string ("application/buzztrax::pattern");
#endif
  
  gobject_class->dispose = bt_main_page_patterns_dispose;
  gobject_class->finalize = bt_main_page_patterns_finalize;

  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  
  widget_class->focus = bt_main_page_patterns_focus;

  gtk_widget_class_set_template_from_resource (widget_class,
      "/org/buzztrax/ui/main-page-patterns.ui");
  
  gtk_widget_class_bind_template_child (widget_class, BtMainPagePatterns, machine_menu);
  gtk_widget_class_bind_template_child (widget_class, BtMainPagePatterns, pattern_menu);
  gtk_widget_class_bind_template_child (widget_class, BtMainPagePatterns, wavetable_menu);
  gtk_widget_class_bind_template_child (widget_class, BtMainPagePatterns, base_octave_menu);
  gtk_widget_class_bind_template_child (widget_class, BtMainPagePatterns, pattern_table);
  gtk_widget_class_bind_template_callback (widget_class, on_base_octave_menu_changed);
  gtk_widget_class_bind_template_callback (widget_class, on_play_live_toggled);

  gtk_widget_class_install_action (widget_class, "machine.voice.add", NULL, on_voice_add_activate);
  gtk_widget_class_install_action (widget_class, "machine.voice.remove_last", NULL, on_voice_remove_activate);
  gtk_widget_class_install_action (widget_class, "machine.pattern.new", NULL, on_pattern_new_activate);
  gtk_widget_class_install_action (widget_class, "machine.pattern.properties", NULL, on_pattern_properties_activate);
  gtk_widget_class_install_action (widget_class, "machine.pattern.remove", NULL, on_pattern_remove_activate);
  gtk_widget_class_install_action (widget_class, "machine.pattern.copy", NULL, on_pattern_copy_activate);
  gtk_widget_class_install_action (widget_class, "machine.properties.show", NULL, on_machine_properties_activate);
  gtk_widget_class_install_action (widget_class, "machine.preferences.show", NULL, on_machine_preferences_activate);
}
