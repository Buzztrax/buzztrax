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
 * SECTION:btmachinepreferencesdialog
 * @short_description: machine non-realtime parameters
 * @see_also: #BtMachine
 *
 * A dialog to configure static settings of a #BtMachine.
 */

/* TODO(ensonic): we have a few notifies, but not for all types
 * - do we need them at all? who else could change things?
 * - do we do this for entry fields with multiple controls (slider + entry box)?
 */
/* TODO(ensonic): save the chosen settings somewhere
 * - right now we save them as 'prefsdata' with the song
 *   - this is not ideal, as we want to configure elements once and reuse this
 *     for songs (especially if the setting sapply to search-path or quality
 *     settings)
 *   - on the other hand, some of the settings (e.g. the fluidsynth soundfont)
 *     are part of the song and we want different values per instance)
 * - the gsettings-api has support for plugins, but we'd still need schema files
 *   - we could technically generate the schema-xml from the properties
 *     on-the-fly, but in that case also compile them
 *   - maybe we can reuse the song-io mechanism and serialize them to small
 *     xml-files?
 */

#define BT_EDIT
#define BT_MACHINE_PREFERENCES_DIALOG_C

#include "bt-edit.h"

//-- property ids

enum
{
  MACHINE_PREFERENCES_DIALOG_MACHINE = 1
};

struct _BtMachinePreferencesDialogPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* the underlying machine */
  BtMachine *machine;
};

static GQuark widget_parent_quark = 0;

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtMachinePreferencesDialog, bt_machine_preferences_dialog,
    GTK_TYPE_WINDOW, 
    G_ADD_PRIVATE(BtMachinePreferencesDialog));

//-- event handler

static void on_combobox_property_changed (GtkComboBox * combobox,
    gpointer user_data);

static void
on_range_property_notify (const GstElement * machine, GParamSpec * property,
    gpointer user_data)
{
  GtkWidget *widget = GTK_WIDGET (user_data);
  gdouble value;

  //GST_INFO("preferences value notify received for: '%s'",property->name);

  g_object_get ((gpointer) machine, property->name, &value, NULL);
  gtk_range_set_value (GTK_RANGE (widget), value);
}

static void
on_double_entry_property_notify (const GstElement * machine,
    GParamSpec * property, gpointer user_data)
{
  GtkWidget *widget = GTK_WIDGET (user_data);
  gdouble value;
  gchar *str_value;

  //GST_INFO("preferences value notify received for: '%s'",property->name);

  g_object_get ((gpointer) machine, property->name, &value, NULL);
  str_value = g_strdup_printf ("%7.2lf", value);
  gtk_entry_set_text (GTK_ENTRY (widget), str_value);
  g_free (str_value);
}

static void
on_combobox_property_notify (const GstElement * machine, GParamSpec * property,
    gpointer user_data)
{
  GtkWidget *widget = GTK_WIDGET (user_data);
  gint ivalue, nvalue;
  GtkTreeModel *store;
  GtkTreeIter iter;

  g_object_get ((gpointer) machine, property->name, &nvalue, NULL);
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
  gtk_combo_box_set_active_iter (GTK_COMBO_BOX (widget), &iter);
  g_signal_handlers_unblock_matched (widget,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
      on_combobox_property_changed, (gpointer) machine);
}


static void
on_entry_property_changed (GtkEditable * editable, gpointer user_data)
{
  GstElement *machine = GST_ELEMENT (user_data);
  const gchar *name = gtk_widget_get_name (GTK_WIDGET (editable));
  BtMachinePreferencesDialog *self =
      BT_MACHINE_PREFERENCES_DIALOG (g_object_get_qdata (G_OBJECT (editable),
          widget_parent_quark));

  //GST_INFO("preferences value change received for: '%s'",name);
  g_object_set (machine, name, gtk_entry_get_text (GTK_ENTRY (editable)), NULL);
  bt_edit_application_set_song_unsaved (self->priv->app);
}

static void
on_checkbox_property_toggled (GtkToggleButton * togglebutton,
    gpointer user_data)
{
  GstElement *machine = GST_ELEMENT (user_data);
  const gchar *name = gtk_widget_get_name (GTK_WIDGET (togglebutton));
  BtMachinePreferencesDialog *self =
      BT_MACHINE_PREFERENCES_DIALOG (g_object_get_qdata (G_OBJECT
          (togglebutton), widget_parent_quark));

  //GST_INFO("preferences value change received for: '%s'",name);
  g_object_set (machine, name, gtk_toggle_button_get_active (togglebutton),
      NULL);
  bt_edit_application_set_song_unsaved (self->priv->app);
}

static void
on_range_property_changed (GtkRange * range, gpointer user_data)
{
  GstElement *machine = GST_ELEMENT (user_data);
  const gchar *name = gtk_widget_get_name (GTK_WIDGET (range));
  BtMachinePreferencesDialog *self =
      BT_MACHINE_PREFERENCES_DIALOG (g_object_get_qdata (G_OBJECT (range),
          widget_parent_quark));

  //GST_INFO("preferences value change received for: '%s'",name);
  g_object_set (machine, name, gtk_range_get_value (range), NULL);
  bt_edit_application_set_song_unsaved (self->priv->app);
}

static void
on_double_entry_property_changed (GtkEditable * editable, gpointer user_data)
{
  GstElement *machine = GST_ELEMENT (user_data);
  gdouble value;
  const gchar *name = gtk_widget_get_name (GTK_WIDGET (editable));
  BtMachinePreferencesDialog *self =
      BT_MACHINE_PREFERENCES_DIALOG (g_object_get_qdata (G_OBJECT (editable),
          widget_parent_quark));

  //GST_INFO("preferences value change received for: '%s'",name);
  value = g_ascii_strtod (gtk_entry_get_text (GTK_ENTRY (editable)), NULL);
  g_object_set (machine, name, value, NULL);
  bt_edit_application_set_song_unsaved (self->priv->app);
}

static void
on_spinbutton_property_changed (GtkSpinButton * spinbutton, gpointer user_data)
{
  GstElement *machine = GST_ELEMENT (user_data);
  gint value;
  const gchar *name = gtk_widget_get_name (GTK_WIDGET (spinbutton));
  BtMachinePreferencesDialog *self =
      BT_MACHINE_PREFERENCES_DIALOG (g_object_get_qdata (G_OBJECT (spinbutton),
          widget_parent_quark));

  GST_INFO ("preferences value change received for: '%s'", name);
  value = gtk_spin_button_get_value_as_int (spinbutton);
  g_object_set (machine, name, value, NULL);
  bt_edit_application_set_song_unsaved (self->priv->app);
}

static void
on_combobox_property_changed (GtkComboBox * combobox, gpointer user_data)
{
  GstElement *machine = GST_ELEMENT (user_data);
  gint value;
  const gchar *name = gtk_widget_get_name (GTK_WIDGET (combobox));
  BtMachinePreferencesDialog *self =
      BT_MACHINE_PREFERENCES_DIALOG (g_object_get_qdata (G_OBJECT (combobox),
          widget_parent_quark));
  GtkTreeModel *store;
  GtkTreeIter iter;

  GST_INFO ("preferences value change received for: '%s'", name);
  store = gtk_combo_box_get_model (combobox);
  if (gtk_combo_box_get_active_iter (combobox, &iter)) {
    gtk_tree_model_get (store, &iter, 0, &value, -1);
    GST_INFO ("change preferences value for: '%s' to %d", name, value);
    g_signal_handlers_block_matched (machine,
        G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
        on_combobox_property_notify, (gpointer) combobox);
    g_object_set (machine, name, value, NULL);
    g_signal_handlers_unblock_matched (machine,
        G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL,
        on_combobox_property_notify, (gpointer) combobox);
  }
  bt_edit_application_set_song_unsaved (self->priv->app);
}

/*
 * on_table_realize:
 *
 * we adjust the scrollable-window size to contain the whole area
 */
static void
on_table_realize (GtkWidget * widget, gpointer user_data)
{
  //BtMachinePreferencesDialog *self=BT_MACHINE_PREFERENCES_DIALOG(user_data);
  GtkScrolledWindow *parent =
      GTK_SCROLLED_WINDOW (gtk_widget_get_parent (gtk_widget_get_parent
          (widget)));
  GtkRequisition minimum, natural, requisition;
  gint height, max_heigth, width, max_width, border;

  gtk_widget_get_preferred_size (widget, &minimum, &natural);
  border = gtk_container_get_border_width (GTK_CONTAINER (widget));

  requisition.width = MAX (minimum.width, natural.width) + border;
  requisition.height = MAX (minimum.height, natural.height) + border;
  bt_gtk_workarea_size (&max_width, &max_heigth);

  GST_DEBUG ("#### table size req %d x %d (max %d x %d)", requisition.width,
      requisition.height, max_width, max_heigth);

  // constrain the size by screen size minus some space for panels and deco
  height = MIN (requisition.height, max_heigth);
  width = MIN (requisition.width, max_width);

  gtk_scrolled_window_set_min_content_height (parent, height);
  gtk_scrolled_window_set_min_content_width (parent, width);
}

//-- helper methods

#define _MAKE_SPIN_BUTTON(t,T,p)                                               \
	case G_TYPE_ ## T: {                                                         \
		GParamSpec ## p *p=G_PARAM_SPEC_ ## T(property);                           \
		g ## t value;                                                              \
		gdouble step;                                                              \
                                                                               \
		g_object_get(machine,property->name,&value,NULL);                          \
		step=(gdouble)(p->maximum-p->minimum)/1024.0;                              \
		spin_adjustment=GTK_ADJUSTMENT(gtk_adjustment_new(                         \
			  (gdouble)value,(gdouble)p->minimum, (gdouble)p->maximum,1.0,step,0.0));\
		widget1=gtk_spin_button_new(spin_adjustment,1.0,0);                        \
		gtk_widget_set_name(GTK_WIDGET(widget1),property->name);                   \
		g_object_set_qdata(G_OBJECT(widget1),widget_parent_quark,(gpointer)self);  \
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget1),(gdouble)value);        \
		widget2=NULL;                                                              \
		g_signal_connect(widget1,"value-changed",G_CALLBACK(on_spinbutton_property_changed),(gpointer)machine); \
	} break;


static void
bt_machine_preferences_dialog_init_ui (const BtMachinePreferencesDialog * self)
{
  BtMainWindow *main_window;
  GtkWidget *label, *widget1, *widget2, *table, *scrolled_window;
  GtkAdjustment *spin_adjustment;
  GdkPixbuf *window_icon = NULL;
  GstElement *machine;
  BtParameterGroup *pg;
  GParamSpec **properties, *property;
  gulong i, number_of_properties;
  gchar *signal_name;
  gchar *tool_tip_text;
  GType param_type, base_type;

  gtk_widget_set_name (GTK_WIDGET (self), "machine preferences");

  g_object_get (self->priv->app, "main-window", &main_window, NULL);
  gtk_window_set_transient_for (GTK_WINDOW (self), GTK_WINDOW (main_window));
  gtk_window_set_default_size (GTK_WINDOW (self), 300, -1);

  // create and set window icon
  if ((window_icon =
          bt_ui_resources_get_icon_pixbuf_by_machine (self->priv->machine))) {
    gtk_window_set_icon (GTK_WINDOW (self), window_icon);
    g_object_unref (window_icon);
  }

  g_object_get (self->priv->machine, "machine", &machine, NULL);

  // get machine properties
  pg = bt_machine_get_prefs_param_group (self->priv->machine);
  g_object_get (pg, "num-params", &number_of_properties, "params", &properties,
      NULL);

  // machine preferences inside a scrolled window
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW
      (scrolled_window), GTK_SHADOW_NONE);

  // add machine preferences into the table
  table = gtk_grid_new ();
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  g_signal_connect (table, "realize",
      G_CALLBACK (on_table_realize), (gpointer) self);

  for (i = 0; i < number_of_properties; i++) {
    property = properties[i];

    GST_INFO ("property %p has name '%s'", property, property->name);
    tool_tip_text = (gchar *) g_param_spec_get_blurb (property);

    // get name
    label = gtk_label_new (g_param_spec_get_nick (property));
    g_object_set (label, "single-line-mode", TRUE, "xalign", 1.0,
        "tooltip-text", tool_tip_text, NULL);
    gtk_grid_attach (GTK_GRID (table), label, 0, i, 1, 1);

    param_type = property->value_type;
    while ((base_type = g_type_parent (param_type)))
      param_type = base_type;
    GST_INFO ("... base type is : %s", g_type_name (param_type));

    switch (param_type) {
      case G_TYPE_STRING:{
        gchar *value;

        g_object_get (machine, property->name, &value, NULL);
        widget1 = gtk_entry_new ();
        gtk_widget_set_name (GTK_WIDGET (widget1), property->name);
        g_object_set_qdata (G_OBJECT (widget1), widget_parent_quark,
            (gpointer) self);
        gtk_entry_set_text (GTK_ENTRY (widget1), safe_string (value));
        g_free (value);
        widget2 = NULL;
        // connect handlers
        g_signal_connect (widget1, "changed",
            G_CALLBACK (on_entry_property_changed), (gpointer) machine);
        break;
      }
      case G_TYPE_BOOLEAN:{
        gboolean value;

        g_object_get (machine, property->name, &value, NULL);
        widget1 = gtk_check_button_new ();
        gtk_widget_set_name (GTK_WIDGET (widget1), property->name);
        g_object_set_qdata (G_OBJECT (widget1), widget_parent_quark,
            (gpointer) self);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget1), value);
        widget2 = NULL;
        // connect handlers
        g_signal_connect (widget1, "toggled",
            G_CALLBACK (on_checkbox_property_toggled), (gpointer) machine);
        break;
      }
        _MAKE_SPIN_BUTTON (int, INT, Int)
          _MAKE_SPIN_BUTTON (uint, UINT, UInt)
          _MAKE_SPIN_BUTTON (int64, INT64, Int64)
          _MAKE_SPIN_BUTTON (uint64, UINT64, UInt64)
          _MAKE_SPIN_BUTTON (long, LONG, Long)
          _MAKE_SPIN_BUTTON (ulong, ULONG, ULong)
        case G_TYPE_DOUBLE:
      {
        GParamSpecDouble *p = G_PARAM_SPEC_DOUBLE (property);
        gdouble step, value;
        gchar *str_value;

        g_object_get (machine, property->name, &value, NULL);
        // get max(max,-min), count digits -> to determine needed length of field
        str_value = g_strdup_printf ("%7.2lf", value);
        step = (p->maximum - p->minimum) / 1024.0;
        widget1 = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL,
            p->minimum, p->maximum, step);
        gtk_widget_set_name (GTK_WIDGET (widget1), property->name);
        g_object_set_qdata (G_OBJECT (widget1), widget_parent_quark,
            (gpointer) self);
        gtk_scale_set_draw_value (GTK_SCALE (widget1), FALSE);
        gtk_range_set_value (GTK_RANGE (widget1), value);
        widget2 = gtk_entry_new ();
        gtk_widget_set_name (GTK_WIDGET (widget2), property->name);
        g_object_set_qdata (G_OBJECT (widget1), widget_parent_quark,
            (gpointer) self);
        gtk_entry_set_text (GTK_ENTRY (widget2), str_value);
        g_object_set (widget2, "max-length", 9, "width-chars", 9, NULL);
        g_free (str_value);
        signal_name = g_strdup_printf ("notify::%s", property->name);
        g_signal_connect_object (machine, signal_name,
            G_CALLBACK (on_range_property_notify), (gpointer) widget1, 0);
        g_signal_connect_object (machine, signal_name,
            G_CALLBACK (on_double_entry_property_notify), (gpointer) widget2,
            0);
        g_signal_connect (widget1, "value-changed",
            G_CALLBACK (on_range_property_changed), (gpointer) machine);
        g_signal_connect (widget2, "changed",
            G_CALLBACK (on_double_entry_property_changed), (gpointer) machine);
        g_free (signal_name);
        break;
      }
      case G_TYPE_ENUM:{
        GParamSpecEnum *enum_property = G_PARAM_SPEC_ENUM (property);
        GEnumClass *enum_class = enum_property->enum_class;
        GEnumValue *enum_value;
        GtkCellRenderer *renderer;
        GtkListStore *store;
        GtkTreeIter iter;
        gint value;

        widget1 = gtk_combo_box_new ();
        GST_INFO ("enum range: %d, %d", enum_class->minimum,
            enum_class->maximum);
        // need a real model because of sparse enums
        store = gtk_list_store_new (2, G_TYPE_INT, G_TYPE_STRING);
        for (value = enum_class->minimum; value <= enum_class->maximum; value++) {
          if ((enum_value = g_enum_get_value (enum_class, value))) {
            //GST_INFO("enum value: %d, '%s', '%s'",enum_value->value,enum_value->value_name,enum_value->value_nick);
            gtk_list_store_append (store, &iter);
            gtk_list_store_set (store, &iter,
                0, enum_value->value, 1, enum_value->value_nick, -1);
          }
        }
        renderer = gtk_cell_renderer_text_new ();
        gtk_cell_renderer_set_fixed_size (renderer, 1, -1);
        gtk_cell_renderer_text_set_fixed_height_from_font
            (GTK_CELL_RENDERER_TEXT (renderer), 1);
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget1), renderer, TRUE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (widget1), renderer,
            "text", 1, NULL);
        gtk_combo_box_set_model (GTK_COMBO_BOX (widget1),
            GTK_TREE_MODEL (store));
        gtk_widget_set_name (GTK_WIDGET (widget1), property->name);
        g_object_set_qdata (G_OBJECT (widget1), widget_parent_quark,
            (gpointer) self);

        on_combobox_property_notify (machine, property, (gpointer) widget1);

        signal_name = g_strdup_printf ("notify::%s", property->name);
        g_signal_connect_object (machine, signal_name,
            G_CALLBACK (on_combobox_property_notify), (gpointer) widget1, 0);
        g_signal_connect (widget1, "changed",
            G_CALLBACK (on_combobox_property_changed), (gpointer) machine);
        g_free (signal_name);
        widget2 = NULL;
        break;
      }
      default:{
        gchar *str = g_strdup_printf ("unhandled type \"%s\"",
            G_PARAM_SPEC_TYPE_NAME (property));
        widget1 = gtk_label_new (str);
        g_free (str);
        widget2 = NULL;
        break;
      }
    }
    gtk_widget_set_tooltip_text (widget1, tool_tip_text);
    if (!widget2) {
      g_object_set (widget1, "hexpand", TRUE, "margin-left", LABEL_PADDING,
          NULL);
      gtk_grid_attach (GTK_GRID (table), widget1, 1, i, 2, 1);
    } else {
      gtk_widget_set_tooltip_text (widget2, tool_tip_text);
      g_object_set (widget1, "hexpand", TRUE, "margin-left", LABEL_PADDING,
          "margin-right", LABEL_PADDING, NULL);
      gtk_grid_attach (GTK_GRID (table), widget1, 1, i, 1, 1);
      gtk_grid_attach (GTK_GRID (table), widget2, 2, i, 1, 1);
    }
  }
  // eat remaning space
#if GTK_CHECK_VERSION (3, 8, 0)
  gtk_container_add (GTK_CONTAINER (scrolled_window), table);
#else
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window),
      table);
#endif
  gtk_container_add (GTK_CONTAINER (self), scrolled_window);

  // track machine name (keep window title up-to-date)
  g_object_bind_property_full (self->priv->machine, "pretty-name",
      (GObject *) self, "title", G_BINDING_SYNC_CREATE, bt_label_value_changed,
      NULL, _("%s preferences"), NULL);

  g_object_unref (machine);
  g_object_unref (main_window);
}

//-- constructor methods

/**
 * bt_machine_preferences_dialog_new:
 * @machine: the machine to create the dialog for
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtMachinePreferencesDialog *
bt_machine_preferences_dialog_new (const BtMachine * machine)
{
  BtMachinePreferencesDialog *self;

  self =
      BT_MACHINE_PREFERENCES_DIALOG (g_object_new
      (BT_TYPE_MACHINE_PREFERENCES_DIALOG, "machine", machine, NULL));
  bt_machine_preferences_dialog_init_ui (self);
  return self;
}

//-- methods

//-- wrapper

//-- class internals

static void
bt_machine_preferences_dialog_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  BtMachinePreferencesDialog *self = BT_MACHINE_PREFERENCES_DIALOG (object);
  return_if_disposed ();
  switch (property_id) {
    case MACHINE_PREFERENCES_DIALOG_MACHINE:
      g_object_try_unref (self->priv->machine);
      self->priv->machine = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_machine_preferences_dialog_dispose (GObject * object)
{
  BtMachinePreferencesDialog *self = BT_MACHINE_PREFERENCES_DIALOG (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  g_object_try_unref (self->priv->machine);
  g_object_unref (self->priv->app);

  G_OBJECT_CLASS (bt_machine_preferences_dialog_parent_class)->dispose (object);
}

static void
bt_machine_preferences_dialog_init (BtMachinePreferencesDialog * self)
{
  self->priv = bt_machine_preferences_dialog_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();
}

static void
bt_machine_preferences_dialog_class_init (BtMachinePreferencesDialogClass *
    klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  widget_parent_quark =
      g_quark_from_static_string ("BtMachinePreferencesDialog::widget-parent");

  gobject_class->set_property = bt_machine_preferences_dialog_set_property;
  gobject_class->dispose = bt_machine_preferences_dialog_dispose;

  g_object_class_install_property (gobject_class,
      MACHINE_PREFERENCES_DIALOG_MACHINE, g_param_spec_object ("machine",
          "machine construct prop", "Set machine object, the dialog handles",
          BT_TYPE_MACHINE,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

}
