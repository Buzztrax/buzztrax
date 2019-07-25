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
 * SECTION:btmachinepropertiesdialog
 * @short_description: machine realtime parameters
 * @see_also: #BtMachine
 *
 * A dialog to configure dynamic settings of a #BtMachine. The dialog also
 * allows to editing and manage presets for machines that support them.
 *
 * The song remembers the open/close state of the window and its preset pane. It
 * also remembers the last loaded preset if any.
 */

/* TODO(ensonic): play demo-song
 * - add a play demo-song button to tool-bar
 * - we play that using playbin on the same sink as the wave-preview
 * - the songs are under: $(datadir)/buzztrax/songs/Gear/
 * - maybe we need a meta-data key, like we use for the docs
 *   gst_element_class_add_metadata(klass, "bt::demo-song", ...)
 */
/* TODO(ensonic): play machines
 * - we want to assign a note-controller to a machines note-trigger property
 *   and boolean-trigger controller to machines trigger properties
 * - we should also create widgets (or just drawables) for triggers:
 *   - note: small keyboard
 *   - int-triggers: a button where the x-pos is mapped to value
 *   - bool-triggers: a plain button
 *   - then we can play then with the mouse (or keyboard when focused) and
 *     assign controllers
 *   - these should probably be all in one expander that is sticky at the bottom
 *     - it needs to combine voice triggers into a poly trigger
 *   - this needs to have idle-playback running
 *     - we could run idle only if the machine-properties window is focused (and
 *       if there is no controller assigned) (and if the machine is connected)
 */
/* TODO(ensonic): mute/solo/bypass
 * - have a row with mute/solo/bypass check-boxes in the UI
 *   - then we can assign controller buttons
 *   - alternatively allow assigning the buttons in the sequence view
 *   - another option would be to allow this in the keyboard shortcut editor
 *   - in any case we'll need code in bt-machine to deal with flags
 *     and in interaction-controller-menu to attach on BtMachine properties
 *     instead of the GstElement properties
 */
/* TODO(ensonic): more details in title
 * - a machine has:
 *   - a canonical name <Author>-<Machine>,
 *   - an 'id' - the short name used in the song and unique for it
 *   - a selected preset - if any
 * - it would be nice to show more info in the title or in a line right below
 *   the title?
 * - we might also offer the machine-rename from the properties window
 */
/* TODO(ensonic): sync the preset selection with the parameters
 * - we store the selected preset with the song
 * - once we change the parameters, it would be nice to add e.g. a '*' to
 *   show that it is not the original preset
 * - we'd also need a method like:
 *     gfloat gst_preset_distance(GST_PRESET (machine), name))
 *   that would return: 0.0 for machine settings are equal to the preset and
 *   1.0 for machine settings are completely different from preset
 *   - a simple implementation would just count the params that differ and return
 *     params_changed / params_total
 * - this method would allow us to:
 *   - select the preset in older songs (where we did not store it)
 *   - dynamically add/remove the preset changed marker
 *
 */
#define BT_EDIT
#define BT_MACHINE_PROPERTIES_DIALOG_C

#include "bt-edit.h"
#include "core/persistence.h"
#include "gst/childbin.h"
#include <glib/gprintf.h>

#define PRESET_BOX_WIDTH 135

//-- property ids

enum
{
  MACHINE_PROPERTIES_DIALOG_MACHINE = 1
};

struct _BtMachinePropertiesDialogPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* the underlying machine */
  BtMachine *machine;
  GHashTable *properties;
  gulong voices;
  const gchar *help_uri;

  GtkWidget *main_toolbar, *preset_toolbar;
  GtkWidget *preset_box;
  GtkWidget *preset_list;
  BtPresetListModel *preset_model;

  GtkWidget *param_group_box;
  /* need this to remove right expander when wire is removed */
  GHashTable *group_to_object;
  /* num_global={0,1}, num_voices is voices above */
  guint num_global, num_wires;

  /* expander to param group */
  GHashTable *param_groups;

  /* context menus */
  GtkMenu *group_menu;
  GtkMenu *param_menu[4];       // we have two controller types + 2 variants

  /* first real child for setting a sane focus */
  GtkWidget *first_widget;
};

static GQuark widget_peer_quark = 0;
static GQuark widget_parent_quark = 0;
static GQuark widget_child_quark = 0;

static GQuark widget_param_group_quark = 0;     /* points to ParamGroup */
static GQuark widget_param_num_quark = 0;       /* which parameter inside the group */

//-- the class

G_DEFINE_TYPE (BtMachinePropertiesDialog, bt_machine_properties_dialog,
    GTK_TYPE_WINDOW);

//-- event handler helper

static gboolean
preset_list_edit_preset_meta (const BtMachinePropertiesDialog * self,
    GstElement * machine, gchar ** name, gchar ** comment)
{
  gboolean result = FALSE;
  GtkWidget *dialog;

  GST_INFO ("create preset edit dialog");

  dialog =
      GTK_WIDGET (bt_machine_preset_properties_dialog_new (machine, name,
          comment));
  bt_edit_application_attach_child_window (self->priv->app,
      GTK_WINDOW (dialog));
  GST_INFO ("run preset edit dialog");
  gtk_widget_show_all (GTK_WIDGET (dialog));

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
    bt_machine_preset_properties_dialog_apply
        (BT_MACHINE_PRESET_PROPERTIES_DIALOG (dialog));
    result = TRUE;
  }

  gtk_widget_destroy (dialog);
  return result;
}

static BtParameterGroup *
get_param_group (const BtMachinePropertiesDialog * self, GObject * param_parent)
{
  if (bt_machine_is_polyphonic (self->priv->machine)) {
    if (GSTBT_IS_CHILD_BIN (param_parent)) {
      return bt_machine_get_global_param_group (self->priv->machine);
    } else {
      return bt_machine_get_voice_param_group (self->priv->machine, 0);
    }
  } else {
    return bt_machine_get_global_param_group (self->priv->machine);
  }
}

// interaction control helper

static void
on_parameter_reset (GtkMenuItem * menuitem, gpointer user_data)
{
  //BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(user_data);
  GtkWidget *menu;
  GObject *object;
  gchar *property_name;
  GParamSpec *pspec;

  menu = gtk_widget_get_parent (GTK_WIDGET (menuitem));

  g_object_get ((gpointer) menu, "selected-object", &object,
      "selected-property-name", &property_name, NULL);

  if ((pspec =
          g_object_class_find_property (G_OBJECT_GET_CLASS (object),
              property_name))) {
    GValue gvalue = { 0, };

    g_value_init (&gvalue, pspec->value_type);
    g_param_value_set_default (pspec, &gvalue);
    g_object_set_property (object, property_name, &gvalue);
    g_value_unset (&gvalue);
  }

  g_object_unref (object);
  g_free (property_name);
}

static void
on_parameter_reset_all (GtkMenuItem * menuitem, gpointer user_data)
{
  const BtMachinePropertiesDialog *self =
      BT_MACHINE_PROPERTIES_DIALOG (user_data);

  bt_machine_reset_parameters (self->priv->machine);
}

//-- cut/copy/paste

extern GdkAtom pattern_atom;

static void
pattern_clipboard_get_func (GtkClipboard * clipboard,
    GtkSelectionData * selection_data, guint info, gpointer data)
{
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

static void
on_parameters_copy_single (GtkMenuItem * menuitem, gpointer user_data)
{
  //const BtMachinePropertiesDialog *self=BT_MACHINE_PROPERTIES_DIALOG(user_data);
  GtkWidget *menu;
  GObject *object;
  gchar *property_name;
  GParamSpec *pspec;
  GtkClipboard *cb =
      gtk_widget_get_clipboard (GTK_WIDGET (menuitem), GDK_SELECTION_CLIPBOARD);
  GtkTargetEntry *targets;
  gint n_targets;
  GString *data = g_string_new (NULL);
  GValue value = { 0, };
  gchar *val_str;

  menu = gtk_widget_get_parent (GTK_WIDGET (menuitem));

  g_object_get ((gpointer) menu, "selected-object", &object,
      "selected-property-name", &property_name, NULL);
  pspec =
      g_object_class_find_property (G_OBJECT_GET_CLASS (object), property_name);

  targets = gtk_target_table_make (pattern_atom, &n_targets);

  g_string_append_printf (data, "1\n");

  g_string_append (data, g_type_name (pspec->value_type));
  g_value_init (&value, pspec->value_type);
  g_object_get_property (object, property_name, &value);
  if ((val_str = bt_str_format_gvalue (&value))) {
    g_string_append_c (data, ',');
    g_string_append (data, val_str);
    g_free (val_str);
  }
  g_value_unset (&value);
  g_string_append_c (data, '\n');

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
  g_object_unref (object);
  g_free (property_name);
}

static void
on_parameters_copy_group (GtkMenuItem * menuitem, gpointer user_data)
{
  GtkWidget *menu;
  BtParameterGroup *pg;
  GtkClipboard *cb =
      gtk_widget_get_clipboard (GTK_WIDGET (menuitem), GDK_SELECTION_CLIPBOARD);
  GtkTargetEntry *targets;
  gint n_targets;
  GString *data = g_string_new (NULL);
  GValue value = { 0, };
  gchar *val_str;
  gulong i, num_params;
  GObject **parent;
  GParamSpec **property;

  menu = gtk_widget_get_parent (GTK_WIDGET (menuitem));

  pg = g_object_get_qdata (G_OBJECT (menu), widget_param_group_quark);
  g_object_get (pg, "num-params", &num_params, "parents", &parent, "params",
      &property, NULL);

  targets = gtk_target_table_make (pattern_atom, &n_targets);

  g_string_append_printf (data, "1\n");

  for (i = 0; i < num_params; i++) {
    if (parent[i] && property[i]) {
      g_string_append (data, g_type_name (property[i]->value_type));
      if (property[i]->flags & G_PARAM_READABLE) {
        g_value_init (&value, property[i]->value_type);
        g_object_get_property (parent[i], property[i]->name, &value);
        if ((val_str = bt_str_format_gvalue (&value))) {
          g_string_append_c (data, ',');
          g_string_append (data, val_str);
          g_free (val_str);
        }
        g_value_unset (&value);
      } else {
        g_string_append_c (data, ',');
      }
    }
    g_string_append_c (data, '\n');
  }

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
}

typedef struct
{
  const BtMachinePropertiesDialog *self;
  BtParameterGroup *pg;
  gint pi;
} PasteData;

static void
pattern_clipboard_received_func (GtkClipboard * clipboard,
    GtkSelectionData * selection_data, gpointer user_data)
{
  PasteData *pd = (PasteData *) user_data;
  const BtMachinePropertiesDialog *self = pd->self;
  BtParameterGroup *pg = pd->pg;
  gint p = pd->pi != -1 ? pd->pi : 0, g = 0;
  gint i;
  gint number_of_groups = g_hash_table_size (self->priv->param_groups);
  GValue value = { 0, };
  gchar **lines;
  gchar **fields;
  gchar *data;
#if 0
  BtParameterGroup **groups;
  GList *node;
#endif
  gulong num_params;
  GObject **parent;
  GParamSpec **property;

  GST_INFO ("receiving clipboard data");
  g_slice_free (PasteData, pd);

  data = (gchar *) gtk_selection_data_get_data (selection_data);
  GST_INFO ("pasting : [%s] @ %p,%d", data, pg, p);

  if (!data)
    return;

  g_object_get (pg, "num-params", &num_params, "parents", &parent, "params",
      &property, NULL);

  /* FIXME(ensonic): need to build an array[number_of_groups]
   * wire-groups (machine->dst_wires), global-group, voices-groups (0...)
   */
#if 0
  groups = g_slice_alloc0 (sizeof (gpointer) * number_of_groups);
  i = 0;
  // wires
  node = self->priv->machine->dst_wires;
  for (; node; node = g_list_next (node)) {
    // search param_groups for entry where entry->parent=node->data
    groups[i++] =
        g_hash_table_find (self->priv->param_groups,
        (GHRFunc) find_param_group_by_parent, node->data);
  }
  // global
  groups[i++] =
      g_hash_table_find (self->priv->param_groups,
      (GHRFunc) find_param_group_by_parent, XXXX);
  // voices
#endif

  lines = g_strsplit_set (data, "\n", 0);
  if (lines[0]) {
    GType stype;
    //guint ticks=atol(lines[0]);
    gboolean res = TRUE;

    /* we paste from position as long as types match */
    i = 1;
    while (lines[i] && *lines[i] && res) {
      if (*lines[i] != '\n') {
        fields = g_strsplit_set (lines[i], ",", 0);
        stype = g_type_from_name (fields[0]);
        if (stype == property[p]->value_type) {
          g_value_init (&value, property[p]->value_type);
          if (bt_str_parse_gvalue (&value, fields[1])) {
            g_object_set_property (parent[p], property[p]->name, &value);
          }
          g_value_unset (&value);
        } else {
          GST_INFO ("types don't match in %s <-> %s", fields[0],
              g_type_name (property[p]->value_type));
          res = FALSE;
        }
        g_strfreev (fields);
      } else {
        GST_INFO ("skip blank line");
      }
      i++;
      p++;
      if (p == num_params) {
        // switch to next group or stop
        if (g < number_of_groups) {
          g++;
          p = 0;
          // FIXME(ensonic): need array with group order
          //pg=&self->priv->param_groups[g];
        } else {
          break;
        }
      }
    }
  }
  g_strfreev (lines);

#if 0
  g_slice_free1 (sizeof (gpointer) * number_of_groups, groups);
#endif
}

static void
on_parameters_paste (GtkMenuItem * menuitem, gpointer user_data)
{
  const BtMachinePropertiesDialog *self =
      BT_MACHINE_PROPERTIES_DIALOG (user_data);
  GtkClipboard *cb =
      gtk_widget_get_clipboard (GTK_WIDGET (menuitem), GDK_SELECTION_CLIPBOARD);
  GObject *menu = (GObject *) gtk_widget_get_parent (GTK_WIDGET (menuitem));
  PasteData *pd = g_slice_new (PasteData);

  pd->self = self;
  pd->pg = g_object_get_qdata (menu, widget_param_group_quark);
  pd->pi = GPOINTER_TO_INT (g_object_get_qdata (menu, widget_param_num_quark));

  gtk_clipboard_request_contents (cb, pattern_atom,
      pattern_clipboard_received_func, (gpointer) pd);
}

//-- event handler

static void on_double_range_property_changed (GtkRange * range,
    gpointer user_data);
static void on_float_range_property_changed (GtkRange * range,
    gpointer user_data);
static void on_int_range_property_changed (GtkRange * range,
    gpointer user_data);
static void on_uint_range_property_changed (GtkRange * range,
    gpointer user_data);
static void on_uint64_range_property_changed (GtkRange * range,
    gpointer user_data);
static void on_uint64_entry_property_changed (GtkEditable * editable,
    gpointer user_data);
static void on_checkbox_property_toggled (GtkToggleButton * togglebutton,
    gpointer user_data);
static void on_combobox_property_changed (GtkComboBox * combobox,
    gpointer user_data);

static gchar *
on_int_range_property_format_value (GtkScale * scale, gdouble value,
    gpointer user_data)
{
  BtParameterGroup *pg = BT_PARAMETER_GROUP (user_data);
  const gchar *name = gtk_widget_get_name (GTK_WIDGET (scale));
  glong index = bt_parameter_group_get_param_index (pg, name);
  GValue int_value = { 0, };
  gchar *str = NULL;
  static gchar _str[20];

  g_value_init (&int_value, G_TYPE_INT);
  g_value_set_int (&int_value, (gint) value);
  if (!(str = bt_parameter_group_describe_param_value (pg, index, &int_value))) {
    g_snprintf (_str, sizeof(_str), "%d", (gint) value);
    str = _str;
  } else {
    strncpy (_str, str, sizeof(_str));
    _str[sizeof(_str)-1] = '\0';
    g_free (str);
    str = _str;
  }
  return str;
}

static gchar *
on_uint_range_property_format_value (GtkScale * scale, gdouble value,
    gpointer user_data)
{
  BtParameterGroup *pg = BT_PARAMETER_GROUP (user_data);
  const gchar *name = gtk_widget_get_name (GTK_WIDGET (scale));
  glong index = bt_parameter_group_get_param_index (pg, name);
  GValue uint_value = { 0, };
  gchar *str = NULL;
  static gchar _str[20];

  g_value_init (&uint_value, G_TYPE_UINT);
  g_value_set_uint (&uint_value, (guint) value);
  if (!(str = bt_parameter_group_describe_param_value (pg, index, &uint_value))) {
    g_snprintf (_str, sizeof(_str), "%u", (guint) value);
    str = _str;
  } else {
    strncpy (_str, str, sizeof(_str));
    _str[sizeof(_str)-1] = '\0';
    g_free (str);
    str = _str;
  }
  return str;
}

static gchar *
on_uint64_range_property_format_value (GtkScale * scale, gdouble value,
    gpointer user_data)
{
  BtParameterGroup *pg = BT_PARAMETER_GROUP (user_data);
  const gchar *name = gtk_widget_get_name (GTK_WIDGET (scale));
  glong index = bt_parameter_group_get_param_index (pg, name);
  GValue uint64_value = { 0, };
  gchar *str = NULL;
  static gchar _str[30];

  g_value_init (&uint64_value, G_TYPE_UINT64);
  g_value_set_uint64 (&uint64_value, (guint64) value);
  if (!(str =
          bt_parameter_group_describe_param_value (pg, index, &uint64_value))) {
    g_snprintf (_str, sizeof(_str), "%" G_GUINT64_FORMAT, (guint64) value);
    str = _str;
  } else {
    strncpy (_str, str, sizeof(_str));
    _str[sizeof(_str)-1] = '\0';
    g_free (str);
    str = _str;
  }
  return str;
}

static void
update_param_after_interaction (GtkWidget * widget, gpointer user_data)
{
  BtMachinePropertiesDialog *self =
      BT_MACHINE_PROPERTIES_DIALOG (g_object_get_qdata (G_OBJECT (widget),
          widget_parent_quark));

  bt_machine_update_default_param_value (self->priv->machine,
      gtk_widget_get_name (GTK_WIDGET (widget)),
      g_object_get_qdata (G_OBJECT (widget), widget_param_group_quark));
}

static gboolean
on_button_press_event (GtkWidget * widget, GdkEventButton * event,
    gpointer user_data, BtInteractionControllerMenuType type,
    gboolean add_copy_paste)
{
  GstObject *param_parent = GST_OBJECT (user_data);
  const gchar *property_name = gtk_widget_get_name (widget);
  GObject *w = (GObject *) widget;
  BtMachinePropertiesDialog *self =
      BT_MACHINE_PROPERTIES_DIALOG (g_object_get_qdata (w,
          widget_parent_quark));
  gboolean res = FALSE;

  GST_INFO ("button_press : button 0x%x, type 0x%d", event->button,
      event->type);
  if (event->type == GDK_BUTTON_PRESS) {
    if (event->button == GDK_BUTTON_SECONDARY) {
      GObject *m;
      BtParameterGroup *pg = g_object_get_qdata (w, widget_param_group_quark);
      gint pi =
          GPOINTER_TO_INT (g_object_get_qdata (w, widget_param_num_quark));
      gint ix = (type << 1) | add_copy_paste;

      // create context menu
      if (!self->priv->param_menu[ix]) {
        GtkWidget *menu_item;
        GtkMenuShell *menu;

        self->priv->param_menu[ix] =
            GTK_MENU (g_object_ref_sink (bt_interaction_controller_menu_new
                (type, self->priv->machine)));
        menu = GTK_MENU_SHELL (self->priv->param_menu[ix]);

        // add extra items
        menu_item = gtk_separator_menu_item_new ();
        gtk_menu_shell_append (menu, menu_item);
        gtk_widget_show (menu_item);

        menu_item = gtk_menu_item_new_with_label (_("Reset parameter"));
        g_signal_connect (menu_item, "activate",
            G_CALLBACK (on_parameter_reset), (gpointer) self);
        gtk_menu_shell_append (menu, menu_item);
        gtk_widget_show (menu_item);

        menu_item = gtk_menu_item_new_with_label (_("Reset all parameters"));
        g_signal_connect (menu_item, "activate",
            G_CALLBACK (on_parameter_reset_all), (gpointer) self);
        gtk_menu_shell_append (menu, menu_item);
        gtk_widget_show (menu_item);

        if (add_copy_paste) {
          // add copy/paste item
          menu_item = gtk_separator_menu_item_new ();
          gtk_menu_shell_append (menu, menu_item);
          gtk_widget_show (menu_item);

          menu_item = gtk_menu_item_new_with_label (_("Copy parameter"));
          g_signal_connect (menu_item, "activate",
              G_CALLBACK (on_parameters_copy_single), (gpointer) self);
          gtk_menu_shell_append (menu, menu_item);
          gtk_widget_show (menu_item);

          menu_item = gtk_menu_item_new_with_label (_("Paste"));
          g_signal_connect (menu_item, "activate",
              G_CALLBACK (on_parameters_paste), (gpointer) self);
          gtk_menu_shell_append (menu, menu_item);
          gtk_widget_show (menu_item);
        }
      }
      m = (GObject *) self->priv->param_menu[ix];
      g_object_set_qdata (m, widget_param_group_quark, (gpointer) pg);
      g_object_set_qdata (m, widget_param_num_quark, GINT_TO_POINTER (pi));

      g_object_set (m, "selected-object", param_parent,
          "selected-parameter-group", pg, "selected-property-name",
          property_name, NULL);

      gtk_menu_popup_at_pointer (GTK_MENU (m), NULL);
      res = TRUE;
    } else if (event->button == GDK_BUTTON_PRIMARY) {
      gst_object_set_control_binding_disabled (param_parent, property_name,
          TRUE);
      // keep res = FALSE to let the original handler run
    }
  }
  return res;
}

static gboolean
on_button_release_event (GtkWidget * widget, GdkEventButton * event,
    gpointer user_data)
{
  if (event->button == GDK_BUTTON_PRIMARY && event->type == GDK_BUTTON_RELEASE) {
    update_param_after_interaction (widget, user_data);
  }
  return FALSE;
}

static gboolean
on_key_release_event (GtkWidget * widget, GdkEventKey * event,
    gpointer user_data)
{
  if (event->type == GDK_KEY_RELEASE) {
    update_param_after_interaction (widget, user_data);
  }
  return FALSE;
}

static gboolean
on_boolean_button_press_event (GtkWidget * widget, GdkEventButton * event,
    gpointer user_data)
{
  return on_button_press_event (widget, event, user_data,
      BT_INTERACTION_CONTROLLER_TRIGGER_MENU, TRUE);
}

static gboolean
on_range_button_press_event (GtkWidget * widget, GdkEventButton * event,
    gpointer user_data)
{
  return on_button_press_event (widget, event, user_data,
      BT_INTERACTION_CONTROLLER_RANGE_MENU, TRUE);
}

static gboolean
on_trigger_button_press_event (GtkWidget * widget, GdkEventButton * event,
    gpointer user_data)
{
  return on_button_press_event (widget, event, user_data,
      BT_INTERACTION_CONTROLLER_RANGE_MENU, FALSE);
}

static gboolean
on_label_button_press_event (GtkWidget * widget, GdkEventButton * event,
    gpointer user_data)
{
  if (event->button == GDK_BUTTON_PRIMARY) {
    gtk_widget_grab_focus ((GtkWidget *) user_data);
  }
  return FALSE;
}

static gboolean
on_group_button_press_event (GtkWidget * widget, GdkEventButton * event,
    gpointer user_data)
{
  BtMachinePropertiesDialog *self = BT_MACHINE_PROPERTIES_DIALOG (user_data);
  gboolean res = FALSE;

  GST_INFO ("button_press : button 0x%x, type 0x%d", event->button,
      event->type);
  if (event->type == GDK_BUTTON_PRESS) {
    if (event->button == GDK_BUTTON_SECONDARY) {
      GtkMenu *menu;
      GtkWidget *menu_item;
      BtParameterGroup *pg =
          g_hash_table_lookup (self->priv->param_groups, widget);

      // create context menu
      if (!self->priv->group_menu) {
        self->priv->group_menu = menu =
            GTK_MENU (g_object_ref_sink (gtk_menu_new ()));

        // add copy/paste item
        menu_item = gtk_menu_item_new_with_label (_("Copy group"));
        g_signal_connect (menu_item, "activate",
            G_CALLBACK (on_parameters_copy_group), (gpointer) self);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
        gtk_widget_show (menu_item);

        menu_item = gtk_menu_item_new_with_label (_("Paste"));
        g_signal_connect (menu_item, "activate",
            G_CALLBACK (on_parameters_paste), (gpointer) self);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
        gtk_widget_show (menu_item);
      } else {
        menu = self->priv->group_menu;
      }
      g_object_set_qdata (G_OBJECT (menu), widget_param_group_quark,
          (gpointer) pg);
      g_object_set_qdata (G_OBJECT (menu), widget_param_num_quark,
          GINT_TO_POINTER (-1));
      gtk_menu_popup_at_pointer (menu, NULL);
      res = TRUE;
    }
  }
  return res;
}


static void
update_double_range_label (GtkLabel * label, gdouble value)
{
  gchar str[100];

  g_snprintf (str, sizeof(str), "%lf", value);
  gtk_label_set_text (label, str);
}

static void
on_double_range_property_notify_idle (GObject * machine, GParamSpec * property,
    gpointer user_data)
{
  GtkWidget *widget = GTK_WIDGET (user_data);
  GtkLabel *label =
      GTK_LABEL (g_object_get_qdata (G_OBJECT (widget), widget_peer_quark));
  gdouble value;

  g_object_get (machine, property->name, &value, NULL);
  //GST_INFO("property value notify received : %s => : %lf",property->name,value);

  g_signal_handlers_block_matched (widget,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_double_range_property_changed, (gpointer) machine);
  gtk_range_set_value (GTK_RANGE (widget), value);
  g_signal_handlers_unblock_matched (widget,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_double_range_property_changed, (gpointer) machine);
  update_double_range_label (label, value);
}

static void
on_double_range_property_notify (GObject * object, GParamSpec * property,
    gpointer user_data)
{
  bt_notify_idle_dispatch (object, property, user_data,
      on_double_range_property_notify_idle);
}

static void
on_double_range_property_changed (GtkRange * range, gpointer user_data)
{
  GstObject *param_parent = GST_OBJECT (user_data);
  const gchar *name = gtk_widget_get_name (GTK_WIDGET (range));
  GtkLabel *label =
      GTK_LABEL (g_object_get_qdata (G_OBJECT (range), widget_peer_quark));
  const BtMachinePropertiesDialog *self =
      BT_MACHINE_PROPERTIES_DIALOG (g_object_get_qdata (G_OBJECT (range),
          widget_parent_quark));
  gdouble value = gtk_range_get_value (range);

  //GST_INFO("property value change received : %lf",value);

  g_signal_handlers_block_matched (param_parent,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_double_range_property_notify, (gpointer) range);
  g_object_set (param_parent, name, value, NULL);
  g_signal_handlers_unblock_matched (param_parent,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_double_range_property_notify, (gpointer) range);
  update_double_range_label (label, value);
  bt_edit_application_set_song_unsaved (self->priv->app);
}


static void
update_float_range_label (GtkLabel * label, gfloat value)
{
  gchar str[100];

  g_snprintf (str, sizeof(str), "%f", value);
  gtk_label_set_text (label, str);
}

static void
on_float_range_property_notify_idle (GObject * machine, GParamSpec * property,
    gpointer user_data)
{
  GtkWidget *widget = GTK_WIDGET (user_data);
  GtkLabel *label =
      GTK_LABEL (g_object_get_qdata (G_OBJECT (widget), widget_peer_quark));
  gfloat value;

  g_object_get (machine, property->name, &value, NULL);
  //GST_INFO("property value notify received : %s => : %f",property->name,value);

  g_signal_handlers_block_matched (widget,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_float_range_property_changed, (gpointer) machine);
  gtk_range_set_value (GTK_RANGE (widget), value);
  g_signal_handlers_unblock_matched (widget,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_float_range_property_changed, (gpointer) machine);
  update_float_range_label (label, value);
}

static void
on_float_range_property_notify (GObject * object, GParamSpec * property,
    gpointer user_data)
{
  bt_notify_idle_dispatch (object, property, user_data,
      on_float_range_property_notify_idle);
}

static void
on_float_range_property_changed (GtkRange * range, gpointer user_data)
{
  GstObject *param_parent = GST_OBJECT (user_data);
  const gchar *name = gtk_widget_get_name (GTK_WIDGET (range));
  GtkLabel *label =
      GTK_LABEL (g_object_get_qdata (G_OBJECT (range), widget_peer_quark));
  const BtMachinePropertiesDialog *self =
      BT_MACHINE_PROPERTIES_DIALOG (g_object_get_qdata (G_OBJECT (range),
          widget_parent_quark));
  gfloat value = gtk_range_get_value (range);

  //GST_INFO("property value change received : %f",value);

  g_signal_handlers_block_matched (param_parent,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_float_range_property_notify, (gpointer) range);
  g_object_set (param_parent, name, value, NULL);
  g_signal_handlers_unblock_matched (param_parent,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_float_range_property_notify, (gpointer) range);
  update_float_range_label (label, value);
  bt_edit_application_set_song_unsaved (self->priv->app);
}


static void
update_int_range_label (const BtMachinePropertiesDialog * self,
    GtkRange * range, GObject * param_parent, GtkLabel * label, gdouble value)
{
  gtk_label_set_text (label,
      on_int_range_property_format_value (GTK_SCALE (range), value,
          (gpointer) get_param_group (self, param_parent)));
}

static void
on_int_range_property_notify_idle (GObject * machine, GParamSpec * property,
    gpointer user_data)
{
  GtkWidget *widget = GTK_WIDGET (user_data);
  GtkLabel *label =
      GTK_LABEL (g_object_get_qdata (G_OBJECT (widget), widget_peer_quark));
  BtMachinePropertiesDialog *self =
      BT_MACHINE_PROPERTIES_DIALOG (g_object_get_qdata (G_OBJECT (widget),
          widget_parent_quark));
  gint value;

  g_object_get (machine, property->name, &value, NULL);
  //GST_INFO("property value notify received : %s => : %d",property->name,value);

  g_signal_handlers_block_matched (widget,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_int_range_property_changed, (gpointer) machine);
  gtk_range_set_value (GTK_RANGE (widget), (gdouble) value);
  g_signal_handlers_unblock_matched (widget,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_int_range_property_changed, (gpointer) machine);
  update_int_range_label (self, GTK_RANGE (widget), G_OBJECT (machine), label,
      (gdouble) value);
}

static void
on_int_range_property_notify (GObject * object, GParamSpec * property,
    gpointer user_data)
{
  bt_notify_idle_dispatch (object, property, user_data,
      on_int_range_property_notify_idle);
}

static void
on_int_range_property_changed (GtkRange * range, gpointer user_data)
{
  GObject *param_parent = G_OBJECT (user_data);
  const gchar *name = gtk_widget_get_name (GTK_WIDGET (range));
  GtkLabel *label =
      GTK_LABEL (g_object_get_qdata (G_OBJECT (range), widget_peer_quark));
  const BtMachinePropertiesDialog *self =
      BT_MACHINE_PROPERTIES_DIALOG (g_object_get_qdata (G_OBJECT (range),
          widget_parent_quark));
  gdouble value = gtk_range_get_value (range);

  //GST_INFO("property value change received");

  g_signal_handlers_block_matched (param_parent,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_int_range_property_notify, (gpointer) range);
  g_object_set (param_parent, name, (gint) value, NULL);
  g_signal_handlers_unblock_matched (param_parent,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_int_range_property_notify, (gpointer) range);
  update_int_range_label (self, range, param_parent, label, value);
  bt_edit_application_set_song_unsaved (self->priv->app);
}


static void
update_uint_range_label (const BtMachinePropertiesDialog * self,
    GtkRange * range, GObject * param_parent, GtkLabel * label, gdouble value)
{
  gtk_label_set_text (label,
      on_uint_range_property_format_value (GTK_SCALE (range), value,
          (gpointer) get_param_group (self, param_parent)));
}

static void
on_uint_range_property_notify_idle (GObject * machine, GParamSpec * property,
    gpointer user_data)
{
  GtkWidget *widget = GTK_WIDGET (user_data);
  GtkLabel *label =
      GTK_LABEL (g_object_get_qdata (G_OBJECT (widget), widget_peer_quark));
  BtMachinePropertiesDialog *self =
      BT_MACHINE_PROPERTIES_DIALOG (g_object_get_qdata (G_OBJECT (widget),
          widget_parent_quark));
  guint value;

  g_object_get (machine, property->name, &value, NULL);
  //GST_INFO("property value notify received : %s => : %d",property->name,value);

  g_signal_handlers_block_matched (widget,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_uint_range_property_changed, (gpointer) machine);
  gtk_range_set_value (GTK_RANGE (widget), (gdouble) value);
  g_signal_handlers_unblock_matched (widget,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_uint_range_property_changed, (gpointer) machine);
  update_uint_range_label (self, GTK_RANGE (widget), G_OBJECT (machine),
      label, (gdouble) value);
}

static void
on_uint_range_property_notify (GObject * object, GParamSpec * property,
    gpointer user_data)
{
  bt_notify_idle_dispatch (object, property, user_data,
      on_uint_range_property_notify_idle);
}

static void
on_uint_range_property_changed (GtkRange * range, gpointer user_data)
{
  GObject *param_parent = G_OBJECT (user_data);
  const gchar *name = gtk_widget_get_name (GTK_WIDGET (range));
  GtkLabel *label =
      GTK_LABEL (g_object_get_qdata (G_OBJECT (range), widget_peer_quark));
  const BtMachinePropertiesDialog *self =
      BT_MACHINE_PROPERTIES_DIALOG (g_object_get_qdata (G_OBJECT (range),
          widget_parent_quark));
  gdouble value = gtk_range_get_value (range);

  GST_INFO ("property value change received");

  g_signal_handlers_block_matched (param_parent,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_uint_range_property_notify, (gpointer) range);
  g_object_set (param_parent, name, (guint) value, NULL);
  g_signal_handlers_unblock_matched (param_parent,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_uint_range_property_notify, (gpointer) range);
  update_uint_range_label (self, range, param_parent, label, value);
  bt_edit_application_set_song_unsaved (self->priv->app);
}


static void
update_uint64_range_entry (const BtMachinePropertiesDialog * self,
    GtkRange * range, GObject * param_parent, GtkEntry * entry, gdouble value)
{
  gtk_entry_set_text (entry,
      on_uint64_range_property_format_value (GTK_SCALE (range), value,
          (gpointer) get_param_group (self, param_parent)));
}

static void
on_uint64_range_property_notify_idle (GObject * machine, GParamSpec * property,
    gpointer user_data)
{
  GtkWidget *widget = GTK_WIDGET (user_data);
  GtkEntry *entry =
      GTK_ENTRY (g_object_get_qdata (G_OBJECT (widget), widget_peer_quark));
  BtMachinePropertiesDialog *self =
      BT_MACHINE_PROPERTIES_DIALOG (g_object_get_qdata (G_OBJECT (widget),
          widget_parent_quark));
  guint64 value;

  g_object_get (machine, property->name, &value, NULL);
  //GST_INFO("property value notify received : %s ",property->name);

  g_signal_handlers_block_matched (widget,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_uint64_range_property_changed, (gpointer) machine);
  gtk_range_set_value (GTK_RANGE (widget), (gdouble) value);
  g_signal_handlers_unblock_matched (widget,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_uint64_range_property_changed, (gpointer) machine);
  g_signal_handlers_block_matched (entry,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_uint64_entry_property_changed, (gpointer) machine);
  update_uint64_range_entry (self, GTK_RANGE (widget), machine,
      entry, (gdouble) value);
  g_signal_handlers_unblock_matched (entry,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_uint64_entry_property_changed, (gpointer) machine);
}

static void
on_uint64_range_property_notify (GObject * object, GParamSpec * property,
    gpointer user_data)
{
  bt_notify_idle_dispatch (object, property, user_data,
      on_uint64_range_property_notify_idle);
}

static void
on_uint64_range_property_changed (GtkRange * range, gpointer user_data)
{
  GObject *param_parent = G_OBJECT (user_data);
  const gchar *name = gtk_widget_get_name (GTK_WIDGET (range));
  GtkEntry *entry =
      GTK_ENTRY (g_object_get_qdata (G_OBJECT (range), widget_peer_quark));
  const BtMachinePropertiesDialog *self =
      BT_MACHINE_PROPERTIES_DIALOG (g_object_get_qdata (G_OBJECT (range),
          widget_parent_quark));
  gdouble value = gtk_range_get_value (range);
  guint64 value_u64 = (guint64) value;

  GST_INFO ("property '%s' value change received, value = %lf -> %"
      G_GUINT64_FORMAT, name, value, value_u64);

  // because of the gdouble <->uint64 conv. this can overflow
  if (((gdouble) value_u64) < (value - 1.0)) {
    GST_WARNING ("overflow on double -> guint64 conversion");
    return;
  }

  g_signal_handlers_block_matched (param_parent,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_uint64_range_property_notify, (gpointer) range);
  g_object_set (param_parent, name, value_u64, NULL);
  g_signal_handlers_unblock_matched (param_parent,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_uint64_range_property_notify, (gpointer) range);
  g_signal_handlers_block_matched (entry,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_uint64_entry_property_changed, (gpointer) user_data);
  update_uint64_range_entry (self, range, param_parent, entry, value);
  g_signal_handlers_unblock_matched (entry,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_uint64_entry_property_changed, (gpointer) user_data);
  bt_edit_application_set_song_unsaved (self->priv->app);
}

static void
on_uint64_entry_property_changed (GtkEditable * editable, gpointer user_data)
{
  GObject *param_parent = G_OBJECT (user_data);
  const gchar *name = gtk_widget_get_name (GTK_WIDGET (editable));
  GtkRange *range =
      GTK_RANGE (g_object_get_qdata (G_OBJECT (editable), widget_peer_quark));
  const BtMachinePropertiesDialog *self =
      BT_MACHINE_PROPERTIES_DIALOG (g_object_get_qdata (G_OBJECT (editable),
          widget_parent_quark));
  guint64 clamped_value, value =
      g_ascii_strtoull (gtk_entry_get_text (GTK_ENTRY (editable)), NULL, 10);

  GST_INFO ("property value change received");

  g_signal_handlers_block_matched (range,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_uint64_range_property_changed, (gpointer) user_data);
  gtk_range_set_value (range, (gdouble) value);
  clamped_value = gtk_range_get_value (range);
  g_signal_handlers_unblock_matched (range,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_uint64_range_property_changed, (gpointer) user_data);

  g_signal_handlers_block_matched (param_parent,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_uint64_range_property_notify, (gpointer) range);
  g_object_set (param_parent, name, clamped_value, NULL);
  g_signal_handlers_unblock_matched (param_parent,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_uint64_range_property_notify, (gpointer) range);
  bt_edit_application_set_song_unsaved (self->priv->app);
}

static void
on_combobox_property_notify_idle (GObject * machine, GParamSpec * property,
    gpointer user_data)
{
  GtkWidget *widget = GTK_WIDGET (user_data);
  gint ivalue, nvalue;
  GtkTreeModel *store;
  GtkTreeIter iter;

  //GST_DEBUG ("property value notify received : %s ", property->name);

  g_object_get (machine, property->name, &nvalue, NULL);
  store = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));
  gtk_tree_model_get_iter_first (store, &iter);
  do {
    gtk_tree_model_get (store, &iter, 0, &ivalue, -1);
    if (ivalue == nvalue)
      break;
  } while (gtk_tree_model_iter_next (store, &iter));

  g_signal_handlers_block_matched (widget,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_combobox_property_changed, (gpointer) machine);
  if (ivalue == nvalue) {
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (widget), &iter);
  } else {
    gtk_combo_box_set_active (GTK_COMBO_BOX (widget), -1);
    GST_WARNING
        ("current value (%d) for machines parameter %s is not part of the enum values",
        nvalue, property->name);
  }
  g_signal_handlers_unblock_matched (widget,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_combobox_property_changed, (gpointer) machine);
}

static void
on_combobox_property_notify (GObject * object, GParamSpec * property,
    gpointer user_data)
{
  bt_notify_idle_dispatch (object, property, user_data,
      on_combobox_property_notify_idle);
}

static void
on_combobox_property_changed (GtkComboBox * combobox, gpointer user_data)
{
  GObject *param_parent = G_OBJECT (user_data);
  const gchar *name = gtk_widget_get_name (GTK_WIDGET (combobox));
  const BtMachinePropertiesDialog *self =
      BT_MACHINE_PROPERTIES_DIALOG (g_object_get_qdata (G_OBJECT (combobox),
          widget_parent_quark));
  GtkTreeModel *store;
  GtkTreeIter iter;
  gint value;

  //GST_INFO("property value change received");

  //value=gtk_combo_box_get_active(combobox);
  store = gtk_combo_box_get_model (combobox);
  if (gtk_combo_box_get_active_iter (combobox, &iter)) {
    gtk_tree_model_get (store, &iter, 0, &value, -1);
    g_signal_handlers_block_matched (param_parent,
        G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
        on_combobox_property_notify, (gpointer) combobox);
    g_object_set (param_parent, name, value, NULL);
    g_signal_handlers_unblock_matched (param_parent,
        G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
        on_combobox_property_notify, (gpointer) combobox);
    //GST_WARNING("property value change received: %d",value);
    update_param_after_interaction (GTK_WIDGET (combobox), user_data);
    bt_edit_application_set_song_unsaved (self->priv->app);
  }
}


static void
on_checkbox_property_notify_idle (GObject * machine, GParamSpec * property,
    gpointer user_data)
{
  GtkWidget *widget = GTK_WIDGET (user_data);
  gboolean value;

  //GST_INFO("property value notify received : %s ",property->name);

  g_object_get (machine, property->name, &value, NULL);
  g_signal_handlers_block_matched (widget,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_checkbox_property_toggled, (gpointer) machine);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), value);
  g_signal_handlers_unblock_matched (widget,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_checkbox_property_toggled, (gpointer) machine);
}

static void
on_checkbox_property_notify (GObject * object, GParamSpec * property,
    gpointer user_data)
{
  bt_notify_idle_dispatch (object, property, user_data,
      on_checkbox_property_notify_idle);
}

static void
on_checkbox_property_toggled (GtkToggleButton * togglebutton,
    gpointer user_data)
{
  GObject *param_parent = G_OBJECT (user_data);
  const gchar *name = gtk_widget_get_name (GTK_WIDGET (togglebutton));
  const BtMachinePropertiesDialog *self =
      BT_MACHINE_PROPERTIES_DIALOG (g_object_get_qdata (G_OBJECT (togglebutton),
          widget_parent_quark));
  gboolean value;

  //GST_INFO("property value change received");

  value = gtk_toggle_button_get_active (togglebutton);
  g_signal_handlers_block_matched (param_parent,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_checkbox_property_notify, (gpointer) togglebutton);
  g_object_set (param_parent, name, value, NULL);
  g_signal_handlers_unblock_matched (param_parent,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_checkbox_property_notify, (gpointer) togglebutton);
  //update_param_after_interaction(GTK_WIDGET(togglebutton),user_data);
  bt_edit_application_set_song_unsaved (self->priv->app);
}


static void
on_toolbar_help_clicked (GtkButton * button, gpointer user_data)
{
  BtMachinePropertiesDialog *self = BT_MACHINE_PROPERTIES_DIALOG (user_data);

  // show help for machine
  gtk_show_uri_simple (GTK_WIDGET (self), self->priv->help_uri);
}

static void
on_toolbar_about_clicked (GtkButton * button, gpointer user_data)
{
  BtMachinePropertiesDialog *self = BT_MACHINE_PROPERTIES_DIALOG (user_data);
  BtMainWindow *main_window;

  // show info about machine
  g_object_get (self->priv->app, "main-window", &main_window, NULL);
  bt_machine_action_about (self->priv->machine, main_window);
  g_object_unref (main_window);
}

static void
on_toolbar_random_clicked (GtkButton * button, gpointer user_data)
{
  const BtMachinePropertiesDialog *self =
      BT_MACHINE_PROPERTIES_DIALOG (user_data);

  bt_machine_randomize_parameters (self->priv->machine);
}

static void
on_toolbar_reset_clicked (GtkButton * button, gpointer user_data)
{
  const BtMachinePropertiesDialog *self =
      BT_MACHINE_PROPERTIES_DIALOG (user_data);

  bt_machine_reset_parameters (self->priv->machine);
}

static void
on_toolbar_show_hide_clicked (GtkButton * button, gpointer user_data)
{
  BtMachinePropertiesDialog *self = BT_MACHINE_PROPERTIES_DIALOG (user_data);
  GtkAllocation win_alloc;
  GtkRequisition pb_req;
  GHashTable *properties;
  gint width;

  gtk_widget_get_preferred_size (self->priv->preset_list, NULL, &pb_req);
  gtk_widget_get_allocation (GTK_WIDGET (self), &win_alloc);

  GST_DEBUG ("win: %d,%d, box: %d,%d",
      win_alloc.width, win_alloc.height, pb_req.width, pb_req.height);

  g_object_get (self->priv->machine, "properties", &properties, NULL);

  width = (pb_req.width == 0) ? PRESET_BOX_WIDTH : pb_req.width;
  if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (button))) {
    // expand window
    gtk_window_resize (GTK_WINDOW (self),
        win_alloc.width + (width + 12), win_alloc.height);

    g_hash_table_insert (properties, g_strdup ("presets-shown"),
        g_strdup ("1"));
    gtk_widget_show_all (self->priv->preset_box);
  } else {
    gtk_widget_hide (self->priv->preset_box);
    g_hash_table_insert (properties, g_strdup ("presets-shown"),
        g_strdup ("0"));
    // shrink window
    gtk_window_resize (GTK_WINDOW (self),
        win_alloc.width - (width + 12), win_alloc.height);
  }
}

static void
on_toolbar_preset_add_clicked (GtkButton * button, gpointer user_data)
{
  BtMachinePropertiesDialog *self = BT_MACHINE_PROPERTIES_DIALOG (user_data);
  GstElement *machine;
  gchar *name = NULL, *comment = NULL;

  g_object_get (self->priv->machine, "machine", &machine, NULL);

  // ask for name & comment
  if (preset_list_edit_preset_meta (self, machine, &name, &comment)) {
    GST_INFO ("about to add a new preset : '%s'", name);

    gst_preset_save_preset (GST_PRESET (machine), name);
    gst_preset_set_meta (GST_PRESET (machine), name, "comment", comment);
    bt_preset_list_model_add (self->priv->preset_model, name);
  }
  gst_object_unref (machine);
}

static void
on_toolbar_preset_remove_clicked (GtkButton * button, gpointer user_data)
{
  BtMachinePropertiesDialog *self = BT_MACHINE_PROPERTIES_DIALOG (user_data);
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;

  // get current preset from list
  selection =
      gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->preset_list));
  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    gchar *name;
    GstElement *machine;

    gtk_tree_model_get (model, &iter, BT_PRESET_LIST_MODEL_LABEL, &name, -1);
    g_object_get (self->priv->machine, "machine", &machine, NULL);

    GST_INFO ("about to delete preset : '%s'", name);
    gst_preset_delete_preset (GST_PRESET (machine), name);
    bt_preset_list_model_remove (self->priv->preset_model, name);
    gst_object_unref (machine);
  }
}

static void
on_toolbar_preset_edit_clicked (GtkButton * button, gpointer user_data)
{
  const BtMachinePropertiesDialog *self =
      BT_MACHINE_PROPERTIES_DIALOG (user_data);
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;

  // get current preset from list
  selection =
      gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->preset_list));
  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    gchar *old_name, *new_name, *comment;
    GstElement *machine;

    gtk_tree_model_get (model, &iter, BT_PRESET_LIST_MODEL_LABEL, &old_name,
        -1);
    g_object_get (self->priv->machine, "machine", &machine, NULL);

    GST_INFO ("about to edit preset : '%s'", old_name);
    gst_preset_get_meta (GST_PRESET (machine), old_name, "comment", &comment);
    new_name = g_strdup (old_name);
    // change for name & comment
    if (preset_list_edit_preset_meta (self, machine, &new_name, &comment)) {
      gst_preset_rename_preset (GST_PRESET (machine), old_name, new_name);
      gst_preset_set_meta (GST_PRESET (machine), new_name, "comment", comment);
      bt_preset_list_model_rename (self->priv->preset_model, old_name,
          new_name);
    }
    g_free (old_name);
    g_free (comment);
    gst_object_unref (machine);
  }
}

static void
on_preset_list_row_activated (GtkTreeView * tree_view, GtkTreePath * path,
    GtkTreeViewColumn * column, gpointer user_data)
{
  const BtMachinePropertiesDialog *self =
      BT_MACHINE_PROPERTIES_DIALOG (user_data);
  GtkTreeModel *model;
  GtkTreeIter iter;

  // get current preset from list
  model = gtk_tree_view_get_model (tree_view);
  if (gtk_tree_model_get_iter (model, &iter, path)) {
    gchar *name;
    GstElement *machine;

    gtk_tree_model_get (model, &iter, BT_PRESET_LIST_MODEL_LABEL, &name, -1);

    // remember preset
    g_hash_table_insert (self->priv->properties, g_strdup ("preset"),
        g_strdup (name));

    g_object_get (self->priv->machine, "machine", &machine, NULL);
    GST_INFO ("about to load preset : '%s'", name);
    if (gst_preset_load_preset (GST_PRESET (machine), name)) {
      bt_edit_application_set_song_unsaved (self->priv->app);
      bt_machine_set_param_defaults (self->priv->machine);
    }
    gst_object_unref (machine);
  }
}

static gboolean
on_preset_list_query_tooltip (GtkWidget * widget, gint x, gint y,
    gboolean keyboard_mode, GtkTooltip * tooltip, gpointer user_data)
{
  GtkTreeView *tree_view = GTK_TREE_VIEW (widget);
  GtkTreePath *path;
  GtkTreeViewColumn *column;
  gboolean res = FALSE;

  if (gtk_tree_view_get_path_at_pos (tree_view, x, y, &path, &column, NULL,
          NULL)) {
    GtkTreeModel *model;
    GtkTreeIter iter;

    model = gtk_tree_view_get_model (tree_view);
    if (gtk_tree_model_get_iter (model, &iter, path)) {
      gchar *comment;

      gtk_tree_model_get (model, &iter, BT_PRESET_LIST_MODEL_COMMENT, &comment,
          -1);
      if (comment) {
        GST_LOG ("tip is '%s'", comment);
        gtk_tooltip_set_text (tooltip, comment);
        res = TRUE;
      }
    }
    gtk_tree_path_free (path);
  }
  return res;
}

static void
on_preset_list_selection_changed (GtkTreeSelection * treeselection,
    gpointer user_data)
{
  gtk_widget_set_sensitive (GTK_WIDGET (user_data),
      (gtk_tree_selection_count_selected_rows (treeselection) != 0));
}

/*
 * on_box_realize:
 *
 * we adjust the scrollable-window size to contain the whole area
 */
static void
on_box_realize (GtkWidget * widget, gpointer user_data)
{
  BtMachinePropertiesDialog *self = BT_MACHINE_PROPERTIES_DIALOG (user_data);
  GtkScrolledWindow *parent =
      GTK_SCROLLED_WINDOW (gtk_widget_get_parent (gtk_widget_get_parent
          (widget)));
  GtkRequisition minimum, natural, requisition;
  GtkAllocation tb_alloc;
  gint height, max_heigth, width, max_width, border;

  gtk_widget_get_preferred_size (widget, &minimum, &natural);
  gtk_widget_get_allocation (GTK_WIDGET (self->priv->main_toolbar), &tb_alloc);
  border = gtk_container_get_border_width (GTK_CONTAINER (widget));

  requisition.width = MAX (minimum.width, natural.width) + border;
  requisition.height = MAX (minimum.height, natural.height) + border;
  bt_gtk_workarea_size (&max_width, &max_heigth);
  max_heigth -= tb_alloc.height;

  GST_DEBUG ("#### box size req %d x %d (toolbar-height %d, max %d x %d)",
      requisition.width, requisition.height, tb_alloc.height, max_width,
      max_heigth);

  // constrain the size by screen size minus some space for panels, deco and toolbar
  height = MIN (requisition.height, max_heigth);
  width = MIN (requisition.width, max_width);

  gtk_scrolled_window_set_min_content_height (parent, height);
  gtk_scrolled_window_set_min_content_width (parent, width);
}

static void
on_window_show (GtkWidget * widget, gpointer user_data)
{
  BtMachinePropertiesDialog *self = BT_MACHINE_PROPERTIES_DIALOG (user_data);

  if (self->priv->preset_box) {
    gtk_widget_set_no_show_all (self->priv->preset_box, FALSE);
  }
  gtk_widget_grab_focus (self->priv->first_widget);
}

//-- helper methods

static GtkWidget *
make_int_range_widget (const BtMachinePropertiesDialog * self,
    GObject * machine, GParamSpec * property, GValue * range_min,
    GValue * range_max)
{
  GtkWidget *widget;
  gchar *signal_name;
  gint value;

  g_object_get (machine, property->name, &value, NULL);
  //step=(int_property->maximum-int_property->minimum)/1024.0;
  widget = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL,
      g_value_get_int (range_min), g_value_get_int (range_max), 1.0);
  gtk_scale_set_draw_value (GTK_SCALE (widget), /*TRUE*/ FALSE);
  gtk_range_set_value (GTK_RANGE (widget), value);
  // TODO(ensonic): add numerical entry as well ?

  const gint len = 9 + strlen (property->name);
  signal_name = g_alloca (len);
  g_snprintf (signal_name, len, "notify::%s", property->name);
  g_signal_connect (machine, signal_name,
      G_CALLBACK (on_int_range_property_notify), (gpointer) widget);
  g_signal_connect (widget, "value-changed",
      G_CALLBACK (on_int_range_property_changed), (gpointer) machine);
  g_signal_connect (widget, "button-press-event",
      G_CALLBACK (on_range_button_press_event), (gpointer) machine);
  g_signal_connect (widget, "button-release-event",
      G_CALLBACK (on_button_release_event), (gpointer) machine);
  g_signal_connect (widget, "key-release-event",
      G_CALLBACK (on_key_release_event), (gpointer) machine);
  return widget;
}

static GtkWidget *
make_uint_range_widget (const BtMachinePropertiesDialog * self,
    GObject * machine, GParamSpec * property, GValue * range_min,
    GValue * range_max)
{
  GtkWidget *widget;
  gchar *signal_name;
  guint value;

  g_object_get (machine, property->name, &value, NULL);
  //step=(int_property->maximum-int_property->minimum)/1024.0;
  widget = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL,
      g_value_get_uint (range_min), g_value_get_uint (range_max), 1.0);
  gtk_scale_set_draw_value (GTK_SCALE (widget), /*TRUE*/ FALSE);
  gtk_range_set_value (GTK_RANGE (widget), value);
  // TODO(ensonic): add numerical entry as well ?

  const gint len = 9 + strlen (property->name);
  signal_name = g_alloca (len);
  g_snprintf (signal_name, len, "notify::%s", property->name);
  g_signal_connect (machine, signal_name,
      G_CALLBACK (on_uint_range_property_notify), (gpointer) widget);
  g_signal_connect (widget, "value-changed",
      G_CALLBACK (on_uint_range_property_changed), (gpointer) machine);
  g_signal_connect (widget, "button-press-event",
      G_CALLBACK (on_range_button_press_event), (gpointer) machine);
  g_signal_connect (widget, "button-release-event",
      G_CALLBACK (on_button_release_event), (gpointer) machine);
  g_signal_connect (widget, "key-release-event",
      G_CALLBACK (on_key_release_event), (gpointer) machine);
  return widget;
}

static GtkWidget *
make_uint64_range_widget (const BtMachinePropertiesDialog * self,
    GObject * machine, GParamSpec * property, GValue * range_min,
    GValue * range_max, GtkWidget * entry)
{
  GtkWidget *widget;
  gchar *signal_name;
  guint64 value;

  g_object_get (machine, property->name, &value, NULL);
  //step=(int_property->maximum-int_property->minimum)/1024.0;
  widget = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL,
      (gdouble) g_value_get_uint64 (range_min),
      (gdouble) g_value_get_uint64 (range_max), 1.0);
  gtk_scale_set_draw_value (GTK_SCALE (widget), /*TRUE*/ FALSE);
  gtk_range_set_value (GTK_RANGE (widget), (gdouble) value);

  const gint len = 9 + strlen (property->name);
  signal_name = g_alloca (len);
  g_snprintf (signal_name, len, "notify::%s", property->name);
  g_signal_connect (machine, signal_name,
      G_CALLBACK (on_uint64_range_property_notify), (gpointer) widget);
  g_signal_connect (widget, "value-changed",
      G_CALLBACK (on_uint64_range_property_changed), (gpointer) machine);
  g_signal_connect (widget, "button-press-event",
      G_CALLBACK (on_range_button_press_event), (gpointer) machine);
  g_signal_connect (widget, "button-release-event",
      G_CALLBACK (on_button_release_event), (gpointer) machine);
  g_signal_connect (widget, "key-release-event",
      G_CALLBACK (on_key_release_event), (gpointer) machine);
  g_signal_connect (entry, "changed",
      G_CALLBACK (on_uint64_entry_property_changed), (gpointer) machine);
  return widget;
}

static GtkWidget *
make_float_range_widget (const BtMachinePropertiesDialog * self,
    GObject * machine, GParamSpec * property, GValue * range_min,
    GValue * range_max)
{
  GtkWidget *widget;
  gchar *signal_name;
  gfloat step, value;
  gfloat value_min = g_value_get_float (range_min);
  gfloat value_max = g_value_get_float (range_max);

  g_object_get (machine, property->name, &value, NULL);
  step = ((gdouble) value_max - (gdouble) value_min) / 1024.0;

  widget = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, value_min,
      value_max, step);
  gtk_scale_set_draw_value (GTK_SCALE (widget), /*TRUE*/ FALSE);
  gtk_range_set_value (GTK_RANGE (widget), value);
  // TODO(ensonic): add numerical entry as well ?

  const gint len = 9 + strlen (property->name);
  signal_name = g_alloca (len);
  g_snprintf (signal_name, len, "notify::%s", property->name);
  g_signal_connect (machine, signal_name,
      G_CALLBACK (on_float_range_property_notify), (gpointer) widget);
  g_signal_connect (widget, "value-changed",
      G_CALLBACK (on_float_range_property_changed), (gpointer) machine);
  g_signal_connect (widget, "button-press-event",
      G_CALLBACK (on_range_button_press_event), (gpointer) machine);
  g_signal_connect (widget, "button-release-event",
      G_CALLBACK (on_button_release_event), (gpointer) machine);
  g_signal_connect (widget, "key-release-event",
      G_CALLBACK (on_key_release_event), (gpointer) machine);
  return widget;
}

static GtkWidget *
make_double_range_widget (const BtMachinePropertiesDialog * self,
    GObject * machine, GParamSpec * property, GValue * range_min,
    GValue * range_max)
{
  GtkWidget *widget;
  gchar *signal_name;
  gdouble step, value;
  gdouble value_min = g_value_get_double (range_min);
  gdouble value_max = g_value_get_double (range_max);

  g_object_get (machine, property->name, &value, NULL);
  step = (value_max - value_min) / 1024.0;

  widget = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL,
      value_min, value_max, step);
  gtk_scale_set_draw_value (GTK_SCALE (widget), /*TRUE*/ FALSE);
  gtk_range_set_value (GTK_RANGE (widget), value);
  // TODO(ensonic): add numerical entry as well ?

  const gint len = 9 + strlen (property->name);
  signal_name = g_alloca (len);
  g_snprintf (signal_name, len, "notify::%s", property->name);
  g_signal_connect (machine, signal_name,
      G_CALLBACK (on_double_range_property_notify), (gpointer) widget);
  g_signal_connect (widget, "value-changed",
      G_CALLBACK (on_double_range_property_changed), (gpointer) machine);
  g_signal_connect (widget, "button-press-event",
      G_CALLBACK (on_range_button_press_event), (gpointer) machine);
  g_signal_connect (widget, "button-release-event",
      G_CALLBACK (on_button_release_event), (gpointer) machine);
  g_signal_connect (widget, "key-release-event",
      G_CALLBACK (on_key_release_event), (gpointer) machine);
  return widget;
}

static GtkWidget *
make_combobox_widget (const BtMachinePropertiesDialog * self, GObject * machine,
    GParamSpec * property, GValue * range_min, GValue * range_max)
{
  GtkWidget *widget;
  gchar *signal_name;
  GParamSpecEnum *enum_property = G_PARAM_SPEC_ENUM (property);
  GEnumClass *enum_class = enum_property->enum_class;
  GEnumValue *enum_value;
  GtkCellRenderer *renderer;
  GtkListStore *store;
  GtkTreeIter iter;
  gint value, ivalue;

  widget = gtk_combo_box_new ();

  // need a real model because of sparse enums
  store = gtk_list_store_new (2, G_TYPE_INT, G_TYPE_STRING);
  for (value = enum_class->minimum; value <= enum_class->maximum; value++) {
    if ((enum_value = g_enum_get_value (enum_class, value))) {
      //gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget),enum_value->value_nick);
      if (BT_IS_STRING (enum_value->value_nick)) {
        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter,
            0, enum_value->value, 1, enum_value->value_nick, -1);
      }
    }
  }
  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_set_fixed_size (renderer, 1, -1);
  gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT
      (renderer), 1);
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (widget), renderer, "text", 1,
      NULL);
  gtk_combo_box_set_model (GTK_COMBO_BOX (widget), GTK_TREE_MODEL (store));

  g_object_get (machine, property->name, &value, NULL);
  gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter);
  do {
    gtk_tree_model_get ((GTK_TREE_MODEL (store)), &iter, 0, &ivalue, -1);
    if (ivalue == value)
      break;
  } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter));

  if (ivalue == value) {
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (widget), &iter);
  } else {
    gtk_combo_box_set_active (GTK_COMBO_BOX (widget), -1);
    GST_WARNING
        ("current value (%d) for machines parameter %s is not part of the enum values",
        value, property->name);
  }

  const gint len = 9 + strlen (property->name);
  signal_name = g_alloca (len);
  g_snprintf (signal_name, len, "notify::%s", property->name);
  g_signal_connect (machine, signal_name,
      G_CALLBACK (on_combobox_property_notify), (gpointer) widget);
  g_signal_connect (widget, "changed",
      G_CALLBACK (on_combobox_property_changed), (gpointer) machine);
  g_signal_connect (widget, "button-press-event",
      G_CALLBACK (on_range_button_press_event), (gpointer) machine);
  g_signal_connect (widget, "button-release-event",
      G_CALLBACK (on_button_release_event), (gpointer) machine);
  g_signal_connect (widget, "key-release-event",
      G_CALLBACK (on_key_release_event), (gpointer) machine);
  return widget;
}

static GtkWidget *
make_checkbox_widget (const BtMachinePropertiesDialog * self, GObject * machine,
    GParamSpec * property)
{
  GtkWidget *widget;
  gchar *signal_name;
  gboolean value;

  g_object_get (machine, property->name, &value, NULL);

  widget = gtk_check_button_new ();
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), value);

  const gint len = 9 + strlen (property->name);
  signal_name = g_alloca (len);
  g_snprintf (signal_name, len, "notify::%s", property->name);
  g_signal_connect (machine, signal_name,
      G_CALLBACK (on_checkbox_property_notify), (gpointer) widget);
  g_signal_connect (widget, "toggled",
      G_CALLBACK (on_checkbox_property_toggled), (gpointer) machine);
  g_signal_connect (widget, "button-press-event",
      G_CALLBACK (on_trigger_button_press_event), (gpointer) machine);
  g_signal_connect (widget, "button-release-event",
      G_CALLBACK (on_button_release_event), (gpointer) machine);
  g_signal_connect (widget, "key-release-event",
      G_CALLBACK (on_key_release_event), (gpointer) machine);
  return widget;
}

static void
make_param_control (const BtMachinePropertiesDialog * self, GObject * object,
    GParamSpec * property, GValue * range_min, GValue * range_max,
    GtkWidget * table, gulong row, BtParameterGroup * pg)
{
  GtkWidget *label, *widget1, *widget2, *evb;
  GType base_type;
  gboolean is_boolean = FALSE;
  const gchar *tool_tip_text = g_param_spec_get_blurb (property);

  // label for parameter name
  evb = gtk_event_box_new ();
  g_object_set (evb, "visible-window", FALSE, NULL);
  label = gtk_label_new (g_param_spec_get_nick (property));
  g_object_set (label, "single-line-mode", TRUE, "xalign", 1.0, "tooltip-text",
      tool_tip_text, NULL);
  gtk_container_add (GTK_CONTAINER (evb), label);
  gtk_grid_attach (GTK_GRID (table), evb, 0, row, 1, 1);

  base_type = bt_g_type_get_base_type (property->value_type);
  GST_INFO ("... base type is : %s", g_type_name (base_type));

#ifdef USE_DEBUG
  if (range_min && range_max) {
    gchar *str_min = g_strdup_value_contents (range_min);
    gchar *str_max = g_strdup_value_contents (range_max);
    GST_DEBUG ("... has range : %s ... %s", str_min, str_max);
    g_free (str_min);
    g_free (str_max);
  }
#endif

  // implement widget types
  switch (base_type) {
    case G_TYPE_STRING:
      widget1 = gtk_label_new ("string");
      widget2 = NULL;
      break;
    case G_TYPE_BOOLEAN:
      widget1 = make_checkbox_widget (self, object, property);
      is_boolean = TRUE;
      widget2 = NULL;
      break;
    case G_TYPE_INT:
      widget2 = gtk_label_new (NULL);
      widget1 =
          make_int_range_widget (self, object, property, range_min, range_max);
      break;
    case G_TYPE_UINT:
      widget2 = gtk_label_new (NULL);
      widget1 =
          make_uint_range_widget (self, object, property, range_min, range_max);
      break;
    case G_TYPE_UINT64:
      widget2 = gtk_entry_new ();
      widget1 =
          make_uint64_range_widget (self, object, property, range_min,
          range_max, widget2);
      break;
    case G_TYPE_FLOAT:
      widget2 = gtk_label_new (NULL);
      widget1 =
          make_float_range_widget (self, object, property, range_min,
          range_max);
      break;
    case G_TYPE_DOUBLE:
      widget2 = gtk_label_new (NULL);
      widget1 =
          make_double_range_widget (self, object, property, range_min,
          range_max);
      break;
    case G_TYPE_ENUM:
      if (property->value_type == GSTBT_TYPE_TRIGGER_SWITCH) {
        widget1 = make_checkbox_widget (self, object, property);
        is_boolean = TRUE;
      } else {
        widget1 =
            make_combobox_widget (self, object, property, range_min, range_max);
      }
      widget2 = NULL;
      break;
    default:{
      gchar *str = g_strdup_printf ("unhandled type \"%s\"",
          G_PARAM_SPEC_TYPE_NAME (property));
      widget1 = gtk_label_new (str);
      g_free (str);
      widget2 = NULL;
      break;
    }
  }
  if (range_min) {
    g_free (range_min);
    range_min = NULL;
  }
  if (range_max) {
    g_free (range_max);
    range_max = NULL;
  }
  gtk_widget_set_name (GTK_WIDGET (evb), property->name);
  g_object_set_qdata (G_OBJECT (evb), widget_parent_quark, (gpointer) self);
  g_object_set_qdata (G_OBJECT (evb), widget_param_group_quark, (gpointer) pg);
  gtk_widget_set_name (GTK_WIDGET (widget1), property->name);
  g_object_set_qdata (G_OBJECT (widget1), widget_parent_quark, (gpointer) self);
  g_object_set_qdata (G_OBJECT (widget1), widget_param_group_quark,
      (gpointer) pg);
  g_object_set_qdata (G_OBJECT (widget1), widget_param_num_quark,
      GINT_TO_POINTER (row));
  g_object_set_qdata (G_OBJECT (widget1), widget_peer_quark,
      (gpointer) widget2);
  // update formatted text on labels
  switch (base_type) {
    case G_TYPE_INT:{
      gint value;

      g_object_get (object, property->name, &value, NULL);
      update_int_range_label (self, GTK_RANGE (widget1), object,
          GTK_LABEL (widget2), (gdouble) value);
      break;
    }
    case G_TYPE_UINT:{
      guint value;

      g_object_get (object, property->name, &value, NULL);
      update_uint_range_label (self, GTK_RANGE (widget1), object,
          GTK_LABEL (widget2), (gdouble) value);
      break;
    }
    case G_TYPE_UINT64:{
      guint64 value;

      g_object_get (object, property->name, &value, NULL);
      g_signal_handlers_block_matched (widget2,
          G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
          on_uint64_entry_property_changed, (gpointer) object);
      update_uint64_range_entry (self, GTK_RANGE (widget1), object,
          GTK_ENTRY (widget2), (gdouble) value);
      g_signal_handlers_unblock_matched (widget2,
          G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
          on_uint64_entry_property_changed, (gpointer) object);
      break;
    }
    case G_TYPE_FLOAT:{
      gfloat value;

      g_object_get (object, property->name, &value, NULL);
      update_float_range_label (GTK_LABEL (widget2), (gdouble) value);
      break;
    }
    case G_TYPE_DOUBLE:{
      gdouble value;

      g_object_get (object, property->name, &value, NULL);
      update_double_range_label (GTK_LABEL (widget2), (gdouble) value);
      break;
    }
  }
  if (is_boolean) {
    g_signal_connect (evb, "button-press-event",
        G_CALLBACK (on_boolean_button_press_event), (gpointer) object);
  } else {
    g_signal_connect (evb, "button-press-event",
        G_CALLBACK (on_range_button_press_event), (gpointer) object);
  }
  g_signal_connect (evb, "button-press-event",
      G_CALLBACK (on_label_button_press_event), (gpointer) widget1);
  g_signal_connect (evb, "button-release-event",
      G_CALLBACK (on_button_release_event), (gpointer) object);

  gtk_widget_set_tooltip_text (widget1, tool_tip_text);
  if (!widget2) {
    g_object_set (widget1, "hexpand", TRUE, "margin-left", LABEL_PADDING, NULL);
    gtk_grid_attach (GTK_GRID (table), widget1, 1, row, 2, 1);
  } else {
    gtk_widget_set_tooltip_text (widget2, tool_tip_text);
    g_object_set (widget1, "hexpand", TRUE, "margin-left", LABEL_PADDING,
        "margin-right", LABEL_PADDING, NULL);
    gtk_grid_attach (GTK_GRID (table), widget1, 1, row, 1, 1);
    if (GTK_IS_LABEL (widget2)) {
      evb = gtk_event_box_new ();
      g_object_set (evb, "visible-window", FALSE, NULL);
      gtk_container_add (GTK_CONTAINER (evb), widget2);
      /* float/double formatting uses 8-9 chars, some machine with custom
       * descriptions use more, but ideally text is detected as enums */
      g_object_set (widget2, "ellipsize", PANGO_ELLIPSIZE_END,
          "single-line-mode", TRUE, "xalign", 0.0, "width-chars", 10, NULL);
      g_signal_connect (evb, "button-press-event",
          G_CALLBACK (on_label_button_press_event), (gpointer) widget1);
    } else {
      evb = widget2;
    }
    gtk_widget_set_name (GTK_WIDGET (evb), property->name);
    g_object_set_qdata (G_OBJECT (evb), widget_parent_quark, (gpointer) self);
    g_object_set_qdata (G_OBJECT (evb), widget_peer_quark, (gpointer) widget1);
    g_object_set_qdata (G_OBJECT (evb), widget_param_group_quark,
        (gpointer) pg);
    g_signal_connect (evb, "button-press-event",
        G_CALLBACK (on_range_button_press_event), (gpointer) object);
    g_signal_connect (evb, "button-release-event",
        G_CALLBACK (on_button_release_event), (gpointer) object);
    gtk_grid_attach (GTK_GRID (table), evb, 2, row, 1, 1);
  }
  if (!self->priv->first_widget) {
    self->priv->first_widget = widget1;
  }
}

static GtkWidget *
make_global_param_box (const BtMachinePropertiesDialog * self,
    gulong global_params, GstElement * machine)
{
  GtkWidget *expander = NULL;
  GtkWidget *table;
  GObject *param_parent;
  GParamSpec *property;
  GValue *range_min, *range_max;
  gulong i, k, params;
  BtParameterGroup *pg =
      bt_machine_get_global_param_group (self->priv->machine);

  // determine params to be skipped
  params = global_params;
  for (i = 0; i < global_params; i++) {
    if (bt_parameter_group_is_param_trigger (pg, i))
      params--;
  }
  if (params) {
    expander = gtk_expander_new (_("global properties"));
    gtk_expander_set_expanded (GTK_EXPANDER (expander), TRUE);

    g_hash_table_insert (self->priv->param_groups, expander, pg);
    self->priv->num_global = 1;

    g_signal_connect (expander, "button-press-event",
        G_CALLBACK (on_group_button_press_event), (gpointer) self);

    // add global machine controls into the table
    table = gtk_grid_new ();
    for (i = 0, k = 0; i < global_params; i++) {
      if (bt_parameter_group_is_param_trigger (pg, i))
        continue;

      bt_parameter_group_get_param_details (pg, i, &property, &range_min,
          &range_max);
      param_parent = bt_parameter_group_get_param_parent (pg, i);
      GST_INFO ("global property[%lu/%lu] %p has name '%s','%s'", i,
          global_params, property, property->name,
          g_param_spec_get_nick (property));
      make_param_control (self, param_parent, property, range_min, range_max,
          table, k++, pg);
    }
    gtk_container_add (GTK_CONTAINER (expander), table);
  }
  return expander;
}

static GtkWidget *
make_voice_param_box (const BtMachinePropertiesDialog * self,
    gulong voice_params, gulong voice, GstElement * machine)
{
  GtkWidget *expander = NULL;
  GtkWidget *table;
  GObject *param_parent;
  GParamSpec *property;
  GValue *range_min, *range_max;
  gchar *name;
  gulong i, k, params;
  BtParameterGroup *pg =
      bt_machine_get_voice_param_group (self->priv->machine, voice);

  params = voice_params;
  for (i = 0; i < voice_params; i++) {
    if (bt_parameter_group_is_param_trigger (pg, i))
      params--;
  }
  if (params) {
    name = g_strdup_printf (_("voice %lu properties"), voice + 1);
    expander = gtk_expander_new (name);
    gtk_expander_set_expanded (GTK_EXPANDER (expander), TRUE);
    g_free (name);

    g_hash_table_insert (self->priv->param_groups, expander, pg);

    g_signal_connect (expander, "button-press-event",
        G_CALLBACK (on_group_button_press_event), (gpointer) self);

    // add voice machine controls into the table
    table = gtk_grid_new ();
    for (i = 0, k = 0; i < voice_params; i++) {
      if (bt_parameter_group_is_param_trigger (pg, i))
        continue;

      bt_parameter_group_get_param_details (pg, i, &property, &range_min,
          &range_max);
      param_parent = bt_parameter_group_get_param_parent (pg, i);
      GST_INFO ("voice property[%lu/%lu] %p has name '%s','%s'", i,
          voice_params, property, property->name,
          g_param_spec_get_nick (property));
      make_param_control (self, param_parent, property, range_min,
          range_max, table, k++, pg);
    }
    gtk_container_add (GTK_CONTAINER (expander), table);
  }
  return expander;
}

static void
on_machine_voices_notify (const BtMachine * machine, GParamSpec * arg,
    gpointer user_data)
{
  BtMachinePropertiesDialog *self = BT_MACHINE_PROPERTIES_DIALOG (user_data);
  gulong i, new_voices, voice_params;
  GtkWidget *expander;
  GstElement *machine_object;

  g_object_get ((gpointer) machine,
      "voices", &new_voices,
      "voice-params", &voice_params, "machine", &machine_object, NULL);

  GST_INFO ("voices changed: %lu -> %lu", self->priv->voices, new_voices);

  if (new_voices > self->priv->voices) {
    for (i = self->priv->voices; i < new_voices; i++) {
      // add ui for voice
      if ((expander =
              make_voice_param_box (self, voice_params, i, machine_object))) {
        gtk_box_pack_start (GTK_BOX (self->priv->param_group_box), expander,
            TRUE, TRUE, 0);
        gtk_box_reorder_child (GTK_BOX (self->priv->param_group_box), expander,
            self->priv->num_global + i);
        gtk_widget_show_all (expander);
      }
    }
    gst_object_unref (machine_object);
  } else {
    GList *children, *node;

    children =
        gtk_container_get_children (GTK_CONTAINER (self->
            priv->param_group_box));
    node = g_list_last (children);
    // skip wire param boxes
    for (i = 0; i < self->priv->num_wires; i++)
      node = g_list_previous (node);
    for (i = self->priv->voices; i > new_voices; i--) {
      // remove ui for voice
      gtk_container_remove (GTK_CONTAINER (self->priv->param_group_box),
          GTK_WIDGET (node->data));
      // no need to disconnect signals as the voice_child is already gone
      node = g_list_previous (node);
    }
    g_list_free (children);
  }

  self->priv->voices = new_voices;
}

static GtkWidget *
make_wire_param_box (const BtMachinePropertiesDialog * self, BtWire * wire)
{
  GtkWidget *expander = NULL;
  GtkWidget *table;
  GObject *param_parent;
  GParamSpec *property;
  GValue *range_min, *range_max;
  gulong i, params;
  BtMachine *src;
  BtParameterGroup *pg = bt_wire_get_param_group (wire);

  g_object_get (wire, "num-params", &params, "src", &src, NULL);
  if (params) {
    expander = gtk_expander_new (NULL);
    gtk_expander_set_expanded (GTK_EXPANDER (expander), TRUE);
    g_object_bind_property_full (src, "id", expander, "label",
        G_BINDING_SYNC_CREATE, bt_label_value_changed, NULL,
        _("%s wire properties"), NULL);

    g_hash_table_insert (self->priv->param_groups, expander, pg);
    g_hash_table_insert (self->priv->group_to_object, wire, expander);
    self->priv->num_wires++;

    g_signal_connect (expander, "button-press-event",
        G_CALLBACK (on_group_button_press_event), (gpointer) self);

    // add wire controls into the table
    table = gtk_grid_new ();
    for (i = 0; i < params; i++) {
      bt_parameter_group_get_param_details (pg, i, &property, &range_min,
          &range_max);
      param_parent = bt_parameter_group_get_param_parent (pg, i);
      GST_INFO ("wire property %p has name '%s','%s'", property, property->name,
          g_param_spec_get_nick (property));
      make_param_control (self, param_parent, property, range_min, range_max,
          table, i, pg);
    }
    gtk_container_add (GTK_CONTAINER (expander), table);
  }
  g_object_unref (src);
  return expander;
}

static void
on_wire_added (const BtSetup * setup, BtWire * wire, gpointer user_data)
{
  BtMachinePropertiesDialog *self = BT_MACHINE_PROPERTIES_DIALOG (user_data);
  GtkWidget *expander;
  BtMachine *dst;

  g_object_get (wire, "dst", &dst, NULL);
  if (dst == self->priv->machine) {
    if ((expander = make_wire_param_box (self, wire))) {
      gtk_box_pack_start (GTK_BOX (self->priv->param_group_box), expander, TRUE,
          TRUE, 0);
      gtk_widget_show_all (expander);
    }
  }
  g_object_unref (dst);
}

static void
on_wire_removed (const BtSetup * setup, BtWire * wire, gpointer user_data)
{
  BtMachinePropertiesDialog *self = BT_MACHINE_PROPERTIES_DIALOG (user_data);
  GtkWidget *expander;

  // determine the right expander
  if ((expander = g_hash_table_lookup (self->priv->group_to_object, wire))) {
    gtk_container_remove (GTK_CONTAINER (self->priv->param_group_box),
        GTK_WIDGET (expander));
    g_hash_table_remove (self->priv->group_to_object, wire);
    g_hash_table_remove (self->priv->param_groups, expander);
    self->priv->num_wires--;
  }
}

static gboolean
on_drum_trigger_pressed (GtkWidget * widget, GdkEventButton * event,
    gpointer user_data)
{
  GstObject *param_parent = GST_OBJECT (user_data);
  const gchar *name = gtk_widget_get_name (GTK_WIDGET (widget));

  GST_INFO ("drum pressed: %s", name);
  g_object_set (param_parent, name, TRUE, NULL);
  return FALSE;
}

static gboolean
on_drum_button_pressed (GtkWidget * widget, GdkEventButton * event,
    gpointer user_data)
{
  GstObject *param_parent = GST_OBJECT (user_data);
  const gchar *name = gtk_widget_get_name (GTK_WIDGET (widget));
  GtkAllocation alloc;
  BtParameterGroup *pg;
  GParamSpec *property;
  GValue *range_min, *range_max;
  gint pi;
  gdouble velocity;

  gtk_widget_get_allocation (widget, &alloc);
  velocity = event->x / (gdouble) alloc.width;

  GST_INFO ("drum pressed: %s: velocity = %d / %lf = %lf",
      name, alloc.width, event->x, velocity);
  // get paramspec and scale value
  pg = g_object_get_qdata (G_OBJECT (widget), widget_param_group_quark);
  pi = GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT (widget),
          widget_param_num_quark));
  bt_parameter_group_get_param_details (pg, pi, &property, &range_min,
      &range_max);
  // trigger drum
  switch (property->value_type) {
    case G_TYPE_UINT:{
      guint value_min = g_value_get_uint (range_min);
      guint value_max = g_value_get_uint (range_max);
      guint val = (guint) (value_min + velocity * (value_max - value_min));
      g_object_set (param_parent, name, val, NULL);
      break;
    }
    case G_TYPE_ENUM:
      // trigger switch
      g_object_set (param_parent, name, 1, NULL);
      break;
    default:
      GST_WARNING ("unhandled type: \"%s\"", G_PARAM_SPEC_TYPE_NAME (property));
      break;
  }
  return FALSE;
}

static void
on_keyboard_key_pressed (GtkWidget * widget, GstBtNote key, gpointer user_data)
{
  GstObject *param_parent = GST_OBJECT (user_data);
  const gchar *name = gtk_widget_get_name (GTK_WIDGET (widget));

  GST_INFO ("piano keyboard pressed: %s: key = %d", name, key);
  // play note
  g_object_set (param_parent, name, key, NULL);
}

static void
on_keyboard_key_released (GtkWidget * widget, GstBtNote key, gpointer user_data)
{
  GstObject *param_parent = GST_OBJECT (user_data);
  const gchar *name = gtk_widget_get_name (GTK_WIDGET (widget));

  GST_INFO ("piano keyboard released: %s: key = %d", name, key);
  // stop note
  g_object_set (param_parent, name, GSTBT_NOTE_OFF, NULL);
}



static GtkWidget *
make_mini_button (void)
{
  GtkWidget *button;
  GtkStyleContext *context;

  // TODO(ensonic): a drum icon? trigger indicator, 'note' is not good
  // we can put the icons into ./icons/hicolor/24x24/
  button = gtk_button_new_from_icon_name ("media-playback-stop",
      GTK_ICON_SIZE_BUTTON);
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  context = gtk_widget_get_style_context (button);
  gtk_style_context_add_class (context, "mini");

  return button;
}

static void
make_trigger_control (const BtMachinePropertiesDialog * self, GObject * object,
    GParamSpec * property, GtkWidget * table, gulong row, BtParameterGroup * pg)
{
  GtkWidget *label, *widget, *evb;
  GtkWidget *scrolled_window = NULL;
  GType base_type;
  const gchar *tool_tip_text = g_param_spec_get_blurb (property);

  // label for parameter name
  evb = gtk_event_box_new ();
  g_object_set (evb, "visible-window", FALSE, NULL);
  label = gtk_label_new (g_param_spec_get_nick (property));
  g_object_set (label, "single-line-mode", TRUE, "xalign", 1.0, "tooltip-text",
      tool_tip_text, NULL);
  gtk_container_add (GTK_CONTAINER (evb), label);
  gtk_grid_attach (GTK_GRID (table), evb, 0, row, 1, 1);

  base_type = bt_g_type_get_base_type (property->value_type);
  GST_INFO ("... base type is : %s", g_type_name (base_type));

  switch (base_type) {
    case G_TYPE_BOOLEAN:
      widget = make_mini_button ();
      g_signal_connect (widget, "button-press-event",
          G_CALLBACK (on_drum_trigger_pressed), (gpointer) object);
      break;
    case G_TYPE_UINT:
      // ... various int types
      widget = make_mini_button ();
      g_signal_connect (widget, "button-press-event",
          G_CALLBACK (on_drum_button_pressed), (gpointer) object);
      break;
    case G_TYPE_ENUM:
      if (property->value_type == GSTBT_TYPE_TRIGGER_SWITCH) {
        widget = make_mini_button ();
        g_signal_connect (widget, "button-press-event",
            G_CALLBACK (on_drum_trigger_pressed), (gpointer) object);
      } else if (property->value_type == GSTBT_TYPE_NOTE) {
        GtkWidget *vport = gtk_viewport_new (NULL, NULL);
        scrolled_window = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
            GTK_POLICY_ALWAYS, GTK_POLICY_NEVER);
#if GTK_CHECK_VERSION (3, 16, 0)
        gtk_scrolled_window_set_overlay_scrolling (GTK_SCROLLED_WINDOW
            (scrolled_window), FALSE);
#endif
        // keyboard
        widget = bt_piano_keys_new ();
        g_object_set (widget, "hexpand", TRUE, NULL);
        gtk_scrollable_set_hscroll_policy (GTK_SCROLLABLE (vport),
            GTK_SCROLL_NATURAL);
        gtk_container_add (GTK_CONTAINER (vport), widget);
        gtk_container_add (GTK_CONTAINER (scrolled_window), vport);
        g_signal_connect (widget, "key-pressed",
            G_CALLBACK (on_keyboard_key_pressed), (gpointer) object);
        g_signal_connect (widget, "key-released",
            G_CALLBACK (on_keyboard_key_released), (gpointer) object);
      } else {
        gchar *str = g_strdup_printf ("unhandled type \"%s\"",
            G_PARAM_SPEC_TYPE_NAME (property));
        widget = gtk_label_new (str);
        g_free (str);
      }
      break;
    default:{
      gchar *str = g_strdup_printf ("unhandled type \"%s\"",
          G_PARAM_SPEC_TYPE_NAME (property));
      widget = gtk_label_new (str);
      g_free (str);
    }
  }

  gtk_widget_set_name (GTK_WIDGET (evb), property->name);
  g_object_set_qdata (G_OBJECT (evb), widget_parent_quark, (gpointer) self);
  g_object_set_qdata (G_OBJECT (evb), widget_param_group_quark, (gpointer) pg);
  gtk_widget_set_name (GTK_WIDGET (widget), property->name);
  g_object_set_qdata (G_OBJECT (widget), widget_parent_quark, (gpointer) self);
  g_object_set_qdata (G_OBJECT (widget), widget_param_group_quark,
      (gpointer) pg);
  g_object_set_qdata (G_OBJECT (widget), widget_param_num_quark,
      GINT_TO_POINTER (row));

  g_signal_connect (evb, "button-press-event",
      G_CALLBACK (on_label_button_press_event), (gpointer) widget);
  g_signal_connect (widget, "button-press-event",
      G_CALLBACK (on_trigger_button_press_event), (gpointer) object);
  g_signal_connect (widget, "button-release-event",
      G_CALLBACK (on_button_release_event), (gpointer) object);
  g_signal_connect (evb, "button-press-event",
      G_CALLBACK (on_trigger_button_press_event), (gpointer) object);
  g_signal_connect (evb, "button-press-event",
      G_CALLBACK (on_label_button_press_event), (gpointer) widget);

  gtk_widget_set_tooltip_text (widget, tool_tip_text);
  if (scrolled_window) {
    widget = scrolled_window;
  }
  g_object_set (widget, "hexpand", TRUE, "margin-left", LABEL_PADDING, NULL);
  gtk_grid_attach (GTK_GRID (table), widget, 1, row, 1, 1);
}

static GtkWidget *
make_trigger_param_box (const BtMachinePropertiesDialog * self,
    gulong global_params, gulong voice_params, GstElement * machine)
{
  GtkWidget *expander = NULL;
  GtkWidget *table;
  GObject *param_parent;
  GParamSpec *property;
  gulong i, j = 0;
  BtParameterGroup *pg;

  // we can have global triggers - one monophonic widget for each
  // we can have voice triggers - one polyphonic widget for each
  // in many cases we have just one trigger
  // there are special cases where two notes are used to do a glide

  table = gtk_grid_new ();

  if (global_params) {
    if ((pg = bt_machine_get_global_param_group (self->priv->machine))) {
      for (i = 0; i < global_params; i++) {
        if (bt_parameter_group_is_param_trigger (pg, i)) {
          param_parent = bt_parameter_group_get_param_parent (pg, i);
          property = bt_parameter_group_get_param_spec (pg, i);
          make_trigger_control (self, param_parent, property, table, i, pg);
          j++;
        }
      }
    }
  }
  if (voice_params) {
    if ((pg = bt_machine_get_voice_param_group (self->priv->machine, 0))) {
      for (i = 0; i < voice_params; i++) {
        if (bt_parameter_group_is_param_trigger (pg, i)) {
          param_parent = bt_parameter_group_get_param_parent (pg, i);
          property = bt_parameter_group_get_param_spec (pg, i);
          make_trigger_control (self, param_parent, property, table, i, pg);
          j++;
        }
      }
    }
  }
  if (j) {
    expander = gtk_expander_new (_("trigger properties"));
    gtk_expander_set_expanded (GTK_EXPANDER (expander), TRUE);
    gtk_container_add (GTK_CONTAINER (expander), table);

    // if machine is not connected, set insensitive
    g_object_bind_property_full (self->priv->machine, "parent", table,
        "sensitive", G_BINDING_SYNC_CREATE, bt_pointer_to_boolean, NULL, NULL,
        NULL);
  } else {
    gtk_widget_destroy (table);
  }

  return expander;
}

static gboolean
bt_machine_properties_dialog_init_preset_box (const BtMachinePropertiesDialog *
    self, GstElement * machine)
{
  GtkWidget *scrolled_window, *toolbar;
  GtkToolItem *tool_item, *remove_tool_button, *edit_tool_button;
  GtkTreeSelection *tree_sel;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *tree_col;
  GtkTreeIter selected_iter;
  gchar *selected_preset;

  self->priv->preset_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

  // add preset controls toolbar
  self->priv->preset_toolbar = toolbar = gtk_toolbar_new ();

  tool_item = gtk_tool_button_new_from_icon_name ("list-add", _("_Add"));
  gtk_tool_item_set_tooltip_text (tool_item, _("Add new preset"));
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), tool_item, -1);
  g_signal_connect (tool_item, "clicked",
      G_CALLBACK (on_toolbar_preset_add_clicked), (gpointer) self);

  remove_tool_button = gtk_tool_button_new_from_icon_name ("list-remove",
      _("_Remove"));
  gtk_tool_item_set_tooltip_text (remove_tool_button, _("Remove preset"));
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), remove_tool_button, -1);
  g_signal_connect (remove_tool_button, "clicked",
      G_CALLBACK (on_toolbar_preset_remove_clicked), (gpointer) self);

  edit_tool_button =
      gtk_tool_button_new_from_icon_name ("gtk-edit", _("_Edit"));
  gtk_tool_item_set_tooltip_text (edit_tool_button,
      _("Edit preset name and comment"));
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), edit_tool_button, -1);
  g_signal_connect (edit_tool_button, "clicked",
      G_CALLBACK (on_toolbar_preset_edit_clicked), (gpointer) self);

  gtk_box_pack_start (GTK_BOX (self->priv->preset_box),
      self->priv->preset_toolbar, FALSE, FALSE, 0);

#if GST_CHECK_VERSION(1,5,0)
  // presets are read-only
  if (!gst_preset_is_editable ((GstPreset *) machine)) {
    gtk_widget_set_sensitive (self->priv->preset_toolbar, FALSE);
  }
#endif

  // add preset list
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_SHADOW_IN);
  self->priv->preset_list = gtk_tree_view_new ();
  g_object_set (self->priv->preset_list, "enable-search", FALSE, "rules-hint",
      TRUE, "fixed-height-mode", TRUE, NULL);
  tree_sel =
      gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->preset_list));
  gtk_tree_selection_set_mode (tree_sel, GTK_SELECTION_SINGLE);
  g_object_set (self->priv->preset_list, "has-tooltip", TRUE, NULL);
  g_signal_connect (self->priv->preset_list, "query-tooltip",
      G_CALLBACK (on_preset_list_query_tooltip), (gpointer) self);
  // alternative: gtk_tree_view_set_tooltip_row
  g_signal_connect (self->priv->preset_list, "row-activated",
      G_CALLBACK (on_preset_list_row_activated), (gpointer) self);
  g_signal_connect (tree_sel, "changed",
      G_CALLBACK (on_preset_list_selection_changed),
      (gpointer) remove_tool_button);
  g_signal_connect (tree_sel, "changed",
      G_CALLBACK (on_preset_list_selection_changed),
      (gpointer) edit_tool_button);

  // add cell renderers
  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_set_fixed_size (renderer, PRESET_BOX_WIDTH, -1);
  gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT
      (renderer), 1);
  g_object_set (renderer, "xalign", 0.0, NULL);
  if ((tree_col =
          gtk_tree_view_column_new_with_attributes (_("Preset"), renderer,
              "text", BT_PRESET_LIST_MODEL_LABEL, NULL))) {
    g_object_set (tree_col, "sizing", GTK_TREE_VIEW_COLUMN_FIXED, "fixed-width",
        PRESET_BOX_WIDTH, NULL);
    gtk_tree_view_insert_column (GTK_TREE_VIEW (self->priv->preset_list),
        tree_col, -1);
  } else
    GST_WARNING ("can't create treeview column");
  self->priv->preset_model = bt_preset_list_model_new (machine);
  gtk_tree_view_set_model (GTK_TREE_VIEW (self->priv->preset_list),
      GTK_TREE_MODEL (self->priv->preset_model));
  // activate selected
  selected_preset =
      (gchar *) g_hash_table_lookup (self->priv->properties, "preset");
  if (selected_preset) {
    if (bt_preset_list_model_find_iter (self->priv->preset_model,
            selected_preset, &selected_iter)) {
      GtkTreeSelection *tree_sel =
          gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->preset_list));
      gtk_tree_selection_select_iter (tree_sel, &selected_iter);
    }
  }
  g_object_unref (self->priv->preset_model);    // drop with treeview

  gtk_scrolled_window_set_min_content_width (GTK_SCROLLED_WINDOW
      (scrolled_window), PRESET_BOX_WIDTH);
  gtk_container_add (GTK_CONTAINER (scrolled_window), self->priv->preset_list);
  gtk_box_pack_start (GTK_BOX (self->priv->preset_box),
      GTK_WIDGET (scrolled_window), TRUE, TRUE, 0);
  return TRUE;
}


static void
bt_machine_properties_dialog_init_ui (const BtMachinePropertiesDialog * self)
{
  BtMainWindow *main_window;
  BtSong *song;
  BtSetup *setup;
  GtkWidget *param_box, *hbox;
  GtkWidget *expander, *scrolled_window, *toolbar;
  GtkToolItem *tool_item;
  GdkPixbuf *window_icon = NULL;
  gulong global_params, voice_params;
  GstElement *machine;
  BtSettings *settings;
  GList *wires;

  gtk_widget_set_name (GTK_WIDGET (self), _("machine properties"));

  g_object_get (self->priv->app, "main-window", &main_window, "settings",
      &settings, "song", &song, NULL);
  g_object_get (song, "setup", &setup, NULL);
  gtk_window_set_transient_for (GTK_WINDOW (self), GTK_WINDOW (main_window));
  gtk_window_set_default_size (GTK_WINDOW (self), 300, -1);

  // create and set window icon
  if ((window_icon =
          bt_ui_resources_get_icon_pixbuf_by_machine (self->priv->machine))) {
    gtk_window_set_icon (GTK_WINDOW (self), window_icon);
    g_object_unref (window_icon);
  }
  // get machine data
  g_object_get (self->priv->machine,
      "global-params", &global_params,
      "voice-params", &voice_params,
      "voices", &self->priv->voices, "machine", &machine, NULL);

  GST_INFO
      ("machine has %lu global properties, %lu voice properties and %lu voices",
      global_params, voice_params, self->priv->voices);

  // add widgets to the dialog content area
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  param_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (hbox), param_box, TRUE, TRUE, 0);

  // create preset pane
  if (GST_IS_PRESET (machine)) {
    if (bt_machine_properties_dialog_init_preset_box (self, machine)) {
      gtk_box_pack_end (GTK_BOX (hbox), self->priv->preset_box, FALSE, FALSE,
          0);
    }
  }
  // create toolbar
  self->priv->main_toolbar = toolbar = gtk_toolbar_new ();

  tool_item = gtk_tool_button_new_from_icon_name ("help-about", _("_About"));
  gtk_tool_item_set_tooltip_text (tool_item, _("Info about this machine"));
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), tool_item, -1);
  g_signal_connect (tool_item, "clicked", G_CALLBACK (on_toolbar_about_clicked),
      (gpointer) self);

  tool_item = gtk_tool_button_new_from_icon_name ("help-browser", _("_Help"));
  gtk_tool_item_set_tooltip_text (tool_item, _("Help for this machine"));
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), tool_item, -1);
  if (!self->priv->help_uri) {
    gtk_widget_set_sensitive (GTK_WIDGET (tool_item), FALSE);
  } else {
    g_signal_connect (tool_item, "clicked",
        G_CALLBACK (on_toolbar_help_clicked), (gpointer) self);
  }

  tool_item = gtk_tool_button_new_from_icon_name ("document-new", _("_New"));
  gtk_tool_item_set_tooltip_text (tool_item, _("Randomize parameters"));
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), tool_item, -1);
  g_signal_connect (tool_item, "clicked",
      G_CALLBACK (on_toolbar_random_clicked), (gpointer) self);

  tool_item = gtk_tool_button_new_from_icon_name ("edit-undo", _("_Undo"));
  gtk_tool_item_set_tooltip_text (tool_item, _("Reset parameters to defaults"));
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), tool_item, -1);
  g_signal_connect (tool_item, "clicked", G_CALLBACK (on_toolbar_reset_clicked),
      (gpointer) self);

  // TODO(ensonic): add copy/paste buttons

  tool_item = gtk_toggle_tool_button_new_from_icon_name ("gtk-index",
      _("Presets"));
  gtk_tool_item_set_tooltip_text (tool_item, _("Show/Hide preset pane"));
  gtk_toolbar_insert (GTK_TOOLBAR (toolbar), tool_item, -1);
  if (!GST_IS_PRESET (machine)) {
    gtk_widget_set_sensitive (GTK_WIDGET (tool_item), FALSE);
  } else {
    GHashTable *properties;
    gchar *prop;
    gboolean hidden = TRUE;

    /* TODO(ensonic): add settings for "show presets by default" */
    g_object_get (self->priv->machine, "properties", &properties, NULL);
    if ((prop = (gchar *) g_hash_table_lookup (properties, "presets-shown"))) {
      if (atoi (prop)) {
        hidden = FALSE;
      }
    }
    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (tool_item),
        !hidden);
    gtk_widget_set_no_show_all (self->priv->preset_box, hidden);

    g_signal_connect (tool_item, "clicked",
        G_CALLBACK (on_toolbar_show_hide_clicked), (gpointer) self);
  }

  // let settings control toolbar style
  g_object_bind_property_full (settings, "toolbar-style",
      self->priv->main_toolbar, "toolbar-style", G_BINDING_SYNC_CREATE,
      bt_toolbar_style_changed, NULL, NULL, NULL);
  if (self->priv->preset_toolbar) {
    g_object_bind_property_full (settings, "toolbar-style",
        self->priv->preset_toolbar, "toolbar-style", G_BINDING_SYNC_CREATE,
        bt_toolbar_style_changed, NULL, NULL, NULL);
  }

  gtk_box_pack_start (GTK_BOX (param_box), self->priv->main_toolbar, FALSE,
      FALSE, 0);

  // machine controls inside a scrolled window
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_SHADOW_NONE);
  self->priv->param_group_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_set_border_width (GTK_CONTAINER (self->priv->param_group_box),
      6);
#if GTK_CHECK_VERSION (3, 8, 0)
  gtk_container_add (GTK_CONTAINER (scrolled_window),
      self->priv->param_group_box);
#else
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window),
      self->priv->param_group_box);
#endif
  gtk_box_pack_start (GTK_BOX (param_box), scrolled_window, TRUE, TRUE, 0);
  g_signal_connect (self->priv->param_group_box, "realize",
      G_CALLBACK (on_box_realize), (gpointer) self);

  /* show widgets for global parameters */
  if (global_params) {
    GST_INFO ("adding %lu global properties", global_params);
    if ((expander = make_global_param_box (self, global_params, machine))) {
      gtk_box_pack_start (GTK_BOX (self->priv->param_group_box), expander, TRUE,
          TRUE, 0);
    }
  }
  /* show widgets for voice parameters */
  if (self->priv->voices && voice_params) {
    gulong j;

    GST_INFO ("adding %lu voice properties for %lu voices", voice_params,
        self->priv->voices);
    for (j = 0; j < self->priv->voices; j++) {
      if ((expander = make_voice_param_box (self, voice_params, j, machine))) {
        gtk_box_pack_start (GTK_BOX (self->priv->param_group_box), expander,
            TRUE, TRUE, 0);
      }
    }
  }
  /* show volume/panorama widgets for incomming wires */
  if ((wires = self->priv->machine->dst_wires)) {
    BtWire *wire;
    GList *node;

    GST_INFO ("adding wire properties");
    for (node = wires; node; node = g_list_next (node)) {
      wire = BT_WIRE (node->data);
      if ((expander = make_wire_param_box (self, wire))) {
        gtk_box_pack_start (GTK_BOX (self->priv->param_group_box), expander,
            TRUE, TRUE, 0);
      }
    }
  }
  /* show widgets for trigger params */
  if (global_params || voice_params) {
    GST_INFO ("adding triggers properties");
    if ((expander = make_trigger_param_box (self, global_params,
                voice_params, machine))) {
      gtk_box_pack_start (GTK_BOX (self->priv->param_group_box), expander, TRUE,
          TRUE, 0);
    }
  }
  gtk_container_add (GTK_CONTAINER (self), hbox);

  // set focus on first parameters
  if (self->priv->first_widget) {
    g_signal_connect ((gpointer) self, "show", G_CALLBACK (on_window_show),
        (gpointer) self);
  }
  // dynamically adjust voices
  g_signal_connect_object (self->priv->machine, "notify::voices",
      G_CALLBACK (on_machine_voices_notify), (gpointer) self, 0);
  // dynamically adjust wire params
  g_signal_connect_object (setup, "wire-added", G_CALLBACK (on_wire_added),
      (gpointer) self, 0);
  g_signal_connect_object (setup, "wire-removed", G_CALLBACK (on_wire_removed),
      (gpointer) self, 0);

  // track machine name (keep window title up-to-date)
  g_object_bind_property_full (self->priv->machine, "pretty-name",
      (GObject *) self, "title", G_BINDING_SYNC_CREATE, bt_label_value_changed,
      NULL, _("%s properties"), NULL);

  g_object_unref (machine);
  g_object_unref (setup);
  g_object_unref (song);
  g_object_unref (main_window);
  g_object_unref (settings);
}

//-- constructor methods

/**
 * bt_machine_properties_dialog_new:
 * @machine: the machine to create the dialog for
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtMachinePropertiesDialog *
bt_machine_properties_dialog_new (const BtMachine * machine)
{
  BtMachinePropertiesDialog *self;

  self =
      BT_MACHINE_PROPERTIES_DIALOG (g_object_new
      (BT_TYPE_MACHINE_PROPERTIES_DIALOG, "machine", machine, NULL));
  bt_machine_properties_dialog_init_ui (self);
  return self;
}

//-- methods

//-- wrapper

//-- class internals

static void
bt_machine_properties_dialog_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  BtMachinePropertiesDialog *self = BT_MACHINE_PROPERTIES_DIALOG (object);
  return_if_disposed ();
  switch (property_id) {
    case MACHINE_PROPERTIES_DIALOG_MACHINE:
      self->priv->machine = g_value_dup_object (value);
      if (self->priv->machine) {
        GstElement *element;

        g_object_get (self->priv->machine, "properties",
            &self->priv->properties, "machine", &element, NULL);

        self->priv->help_uri =
            gst_element_factory_get_metadata (gst_element_get_factory (element),
            GST_ELEMENT_METADATA_DOC_URI);
        gst_object_unref (element);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_machine_properties_dialog_dispose (GObject * object)
{
  BtMachinePropertiesDialog *self = BT_MACHINE_PROPERTIES_DIALOG (object);
  gulong j;
  GstElement *machine;
  GObject *machine_voice;
  GList *wires;

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  // disconnect all handlers that are connected to params
  g_object_get (self->priv->machine, "machine", &machine, NULL);
  g_signal_handlers_disconnect_matched (machine, G_SIGNAL_MATCH_FUNC, 0, 0,
      NULL, on_float_range_property_notify, NULL);
  g_signal_handlers_disconnect_matched (machine, G_SIGNAL_MATCH_FUNC, 0, 0,
      NULL, on_double_range_property_notify, NULL);
  g_signal_handlers_disconnect_matched (machine, G_SIGNAL_MATCH_FUNC, 0, 0,
      NULL, on_int_range_property_notify, NULL);
  g_signal_handlers_disconnect_matched (machine, G_SIGNAL_MATCH_FUNC, 0, 0,
      NULL, on_uint_range_property_notify, NULL);
  g_signal_handlers_disconnect_matched (machine, G_SIGNAL_MATCH_FUNC, 0, 0,
      NULL, on_uint64_range_property_notify, NULL);
  g_signal_handlers_disconnect_matched (machine, G_SIGNAL_MATCH_FUNC, 0, 0,
      NULL, on_checkbox_property_notify, NULL);
  g_signal_handlers_disconnect_matched (machine, G_SIGNAL_MATCH_FUNC, 0, 0,
      NULL, on_combobox_property_notify, NULL);
  for (j = 0; j < self->priv->voices; j++) {
    machine_voice =
        gst_child_proxy_get_child_by_index (GST_CHILD_PROXY (machine), j);
    g_signal_handlers_disconnect_matched (machine_voice, G_SIGNAL_MATCH_FUNC, 0,
        0, NULL, on_float_range_property_notify, NULL);
    g_signal_handlers_disconnect_matched (machine_voice, G_SIGNAL_MATCH_FUNC, 0,
        0, NULL, on_double_range_property_notify, NULL);
    g_signal_handlers_disconnect_matched (machine_voice, G_SIGNAL_MATCH_FUNC, 0,
        0, NULL, on_int_range_property_notify, NULL);
    g_signal_handlers_disconnect_matched (machine_voice, G_SIGNAL_MATCH_FUNC, 0,
        0, NULL, on_uint_range_property_notify, NULL);
    g_signal_handlers_disconnect_matched (machine_voice, G_SIGNAL_MATCH_FUNC, 0,
        0, NULL, on_uint64_range_property_notify, NULL);
    g_signal_handlers_disconnect_matched (machine_voice, G_SIGNAL_MATCH_FUNC, 0,
        0, NULL, on_checkbox_property_notify, NULL);
    g_signal_handlers_disconnect_matched (machine_voice, G_SIGNAL_MATCH_FUNC, 0,
        0, NULL, on_combobox_property_notify, NULL);
  }
  // disconnect wire parameters
  if ((wires = self->priv->machine->dst_wires)) {
    BtWire *wire;
    GstObject *gain, *pan;
    GList *node;

    for (node = wires; node; node = g_list_next (node)) {
      wire = BT_WIRE (node->data);
      g_object_get (wire, "gain", &gain, "pan", &pan, NULL);
      if (gain) {
        g_signal_handlers_disconnect_matched (gain, G_SIGNAL_MATCH_FUNC, 0, 0,
            NULL, on_float_range_property_notify, NULL);
        g_signal_handlers_disconnect_matched (gain, G_SIGNAL_MATCH_FUNC, 0, 0,
            NULL, on_double_range_property_notify, NULL);
        gst_object_unref (gain);
      }
      if (pan) {
        g_signal_handlers_disconnect_matched (pan, G_SIGNAL_MATCH_FUNC, 0, 0,
            NULL, on_float_range_property_notify, NULL);
        g_signal_handlers_disconnect_matched (pan, G_SIGNAL_MATCH_FUNC, 0, 0,
            NULL, on_double_range_property_notify, NULL);
        gst_object_unref (pan);
      }
    }
  }
  g_object_unref (machine);

  // get rid of context menus
  if (self->priv->group_menu) {
    gtk_widget_destroy (GTK_WIDGET (self->priv->group_menu));
    g_object_unref (self->priv->group_menu);
  }
  for (j = 0; j < G_N_ELEMENTS (self->priv->param_menu); j++) {
    if (self->priv->param_menu[j]) {
      gtk_widget_destroy (GTK_WIDGET (self->priv->param_menu[j]));
      g_object_unref (self->priv->param_menu[j]);
    }
  }

  g_object_try_unref (self->priv->machine);
  g_object_unref (self->priv->app);

  G_OBJECT_CLASS (bt_machine_properties_dialog_parent_class)->dispose (object);
}

static void
bt_machine_properties_dialog_finalize (GObject * object)
{
  BtMachinePropertiesDialog *self = BT_MACHINE_PROPERTIES_DIALOG (object);

  GST_DEBUG ("!!!! self=%p", self);

  g_hash_table_destroy (self->priv->group_to_object);
  g_hash_table_destroy (self->priv->param_groups);

  G_OBJECT_CLASS (bt_machine_properties_dialog_parent_class)->finalize (object);
}

static void
bt_machine_properties_dialog_init (BtMachinePropertiesDialog * self)
{
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, BT_TYPE_MACHINE_PROPERTIES_DIALOG,
      BtMachinePropertiesDialogPrivate);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();

  self->priv->group_to_object = g_hash_table_new (NULL, NULL);
  self->priv->param_groups = g_hash_table_new_full (NULL, NULL, NULL, NULL);
}

static void
bt_machine_properties_dialog_class_init (BtMachinePropertiesDialogClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  widget_peer_quark =
      g_quark_from_static_string ("BtMachinePropertiesDialog::widget-peer");
  widget_parent_quark =
      g_quark_from_static_string ("BtMachinePropertiesDialog::widget-parent");

  widget_param_group_quark =
      g_quark_from_static_string
      ("BtMachinePropertiesDialog::widget-param-group");
  widget_param_num_quark =
      g_quark_from_static_string
      ("BtMachinePropertiesDialog::widget-param-num");

  widget_child_quark =
      g_quark_from_static_string ("BtInteractionControllerMenu::widget-child");

  g_type_class_add_private (klass, sizeof (BtMachinePropertiesDialogPrivate));

  gobject_class->set_property = bt_machine_properties_dialog_set_property;
  gobject_class->dispose = bt_machine_properties_dialog_dispose;
  gobject_class->finalize = bt_machine_properties_dialog_finalize;

  g_object_class_install_property (gobject_class,
      MACHINE_PROPERTIES_DIALOG_MACHINE, g_param_spec_object ("machine",
          "machine construct prop", "Set machine object, the dialog handles",
          BT_TYPE_MACHINE,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

}
