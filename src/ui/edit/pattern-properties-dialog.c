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
 * SECTION:btpatternpropertiesdialog
 * @short_description: pattern settings
 *
 * A dialog to (re)configure a #BtPattern.
 */

#define BT_EDIT
#define BT_PATTERN_PROPERTIES_DIALOG_C

#include "bt-edit.h"

//-- property ids

enum
{
  PATTERN_PROPERTIES_DIALOG_PATTERN = 1,
  PATTERN_PROPERTIES_DIALOG_NAME,
  PATTERN_PROPERTIES_DIALOG_LENGTH,
  PATTERN_PROPERTIES_DIALOG_VOICES
};

struct _BtPatternPropertiesDialogPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* the underlying pattern */
  BtPattern *pattern;

  /* the machine and its id the pattern belongs to, needed to veryfy unique name */
  BtMachine *machine;
  gchar *machine_id;

  /* dialog data */
  gchar *name;
  gulong length, voices;

  /* widgets and their handlers */
  GtkWidget *okay_button;
};

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtPatternPropertiesDialog, bt_pattern_properties_dialog,
    GTK_TYPE_DIALOG, 
    G_ADD_PRIVATE(BtPatternPropertiesDialog));


//-- event handler

static void
on_name_changed (GtkEditable * editable, gpointer user_data)
{
  BtPatternPropertiesDialog *self = BT_PATTERN_PROPERTIES_DIALOG (user_data);
  BtCmdPattern *pattern;
  const gchar *name = gtk_entry_get_text (GTK_ENTRY (editable));
  gboolean unique = FALSE;

  GST_DEBUG ("change name");
  // assure uniqueness of the entered data
  if (*name) {
    if ((pattern = bt_machine_get_pattern_by_name (self->priv->machine, name))) {
      if (pattern == (BtCmdPattern *) self->priv->pattern) {
        unique = TRUE;
      }
      g_object_unref (pattern);
    } else {
      unique = TRUE;
    }
  }
  GST_INFO ("%s" "unique '%s'", (unique ? "" : "not "), name);
  gtk_widget_set_sensitive (self->priv->okay_button, unique);
  // update field
  g_free (self->priv->name);
  self->priv->name = g_strdup (name);
}

static void
on_length_changed (GtkEditable * editable, gpointer user_data)
{
  BtPatternPropertiesDialog *self = BT_PATTERN_PROPERTIES_DIALOG (user_data);

  // update field
  self->priv->length = atol (gtk_entry_get_text (GTK_ENTRY (editable)));
}

static void
on_voices_changed (GtkSpinButton * spinbutton, gpointer user_data)
{
  BtPatternPropertiesDialog *self = BT_PATTERN_PROPERTIES_DIALOG (user_data);

  // update field
  self->priv->voices = gtk_spin_button_get_value_as_int (spinbutton);
}

//-- helper methods

static void
bt_pattern_properties_dialog_init_ui (const BtPatternPropertiesDialog * self)
{
  GtkWidget *label, *widget, *table;
  GtkAdjustment *spin_adjustment;
  gchar *title, *length_str;
  //GdkPixbuf *window_icon=NULL;

  gtk_widget_set_name (GTK_WIDGET (self), "pattern properties");

  // create and set window icon
  /* e.v. tab_pattern.png
     if((window_icon=bt_ui_resources_get_icon_pixbuf_by_machine(self->priv->machine))) {
     gtk_window_set_icon(GTK_WINDOW(self),window_icon);
     g_object_unref(window_icon);
     }
   */

  // get dialog data
  g_object_get (self->priv->pattern, "name", &self->priv->name, "length",
      &self->priv->length, "voices", &self->priv->voices, "machine",
      &self->priv->machine, NULL);
  g_object_get (self->priv->machine, "id", &self->priv->machine_id, NULL);

  // set dialog title
  title = g_strdup_printf (_("%s properties"), self->priv->name);
  gtk_window_set_title (GTK_WINDOW (self), title);
  g_free (title);

  // add dialog commision widgets (okay, cancel)
  gtk_dialog_add_buttons (GTK_DIALOG (self),
      _("_OK"), GTK_RESPONSE_ACCEPT, _("_Cancel"), GTK_RESPONSE_REJECT, NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_ACCEPT);

  // grab okay button, so that we can block if input is not valid
  self->priv->okay_button =
      gtk_dialog_get_widget_for_response (GTK_DIALOG (self),
      GTK_RESPONSE_ACCEPT);

  // add widgets to the dialog content area
  table = gtk_grid_new ();
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))),
      table, TRUE, TRUE, 0);

  // GtkEntry : pattern name
  label = gtk_label_new (_("name"));
  g_object_set (label, "xalign", 1.0, NULL);
  gtk_grid_attach (GTK_GRID (table), label, 0, 0, 1, 1);

  widget = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (widget), self->priv->name);
  gtk_entry_set_activates_default (GTK_ENTRY (widget), TRUE);
  g_object_set (widget, "hexpand", TRUE, "margin-left", LABEL_PADDING, NULL);
  gtk_grid_attach (GTK_GRID (table), widget, 1, 0, 1, 1);
  g_signal_connect (widget, "changed", G_CALLBACK (on_name_changed),
      (gpointer) self);

  // GtkComboBox : pattern length
  label = gtk_label_new (_("length"));
  g_object_set (label, "xalign", 1.0, NULL);
  gtk_grid_attach (GTK_GRID (table), label, 0, 1, 1, 1);
  length_str = g_strdup_printf ("%lu", self->priv->length);
  widget = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (widget), length_str);
  g_free (length_str);
  gtk_entry_set_activates_default (GTK_ENTRY (widget), TRUE);
  g_object_set (widget, "hexpand", TRUE, "margin-left", LABEL_PADDING, NULL);
  gtk_grid_attach (GTK_GRID (table), widget, 1, 1, 1, 1);
  g_signal_connect (widget, "changed", G_CALLBACK (on_length_changed),
      (gpointer) self);

  // GtkSpinButton : number of voices
  label = gtk_label_new (_("voices"));
  g_object_set (label, "xalign", 1.0, NULL);
  gtk_grid_attach (GTK_GRID (table), label, 0, 2, 1, 1);
  spin_adjustment = gtk_adjustment_new (1.0, 1.0, 16.0, 1.0, 4.0, 0.0);
  widget =
      gtk_spin_button_new (spin_adjustment, (gdouble) (self->priv->voices), 0);
  if (bt_machine_is_polyphonic (self->priv->machine)) {
    GstElement *elem;
    GParamSpecULong *pspec;

    g_object_get (self->priv->machine, "machine", &elem, NULL);
    if ((pspec = (GParamSpecULong *)
            g_object_class_find_property (G_OBJECT_GET_CLASS (elem),
                "children"))) {
      g_object_set (spin_adjustment, "lower", (gdouble) pspec->minimum, "upper",
          (gdouble) pspec->maximum, NULL);
      gtk_widget_set_sensitive (widget, (pspec->minimum < pspec->maximum));
    }
    gst_object_unref (elem);

    gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget),
        (gdouble) self->priv->voices);
    gtk_entry_set_activates_default (GTK_ENTRY (widget), TRUE);
    g_signal_connect (widget, "value-changed", G_CALLBACK (on_voices_changed),
        (gpointer) self);
  } else {
    gtk_widget_set_sensitive (widget, FALSE);
  }
  g_object_set (widget, "hexpand", TRUE, "margin-left", LABEL_PADDING, NULL);
  gtk_grid_attach (GTK_GRID (table), widget, 1, 2, 1, 1);
  GST_INFO ("dialog done");
}

//-- constructor methods

/**
 * bt_pattern_properties_dialog_new:
 * @pattern: the pattern for which to create the dialog for
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtPatternPropertiesDialog *
bt_pattern_properties_dialog_new (const BtPattern * pattern)
{
  BtPatternPropertiesDialog *self;

  self =
      BT_PATTERN_PROPERTIES_DIALOG (g_object_new
      (BT_TYPE_PATTERN_PROPERTIES_DIALOG, "pattern", pattern, NULL));
  bt_pattern_properties_dialog_init_ui (self);
  return self;
}

//-- methods

/**
 * bt_pattern_properties_dialog_apply:
 * @self: the dialog which settings to apply
 *
 * Makes the dialog settings effective.
 */
void
bt_pattern_properties_dialog_apply (const BtPatternPropertiesDialog * self)
{
  gchar *name;
  gulong length, voices;

  GST_INFO ("applying pattern settings");
  g_object_get (self->priv->pattern, "name", &name, "length", &length, "voices",
      &voices, NULL);

  // avoid notifies of properties that actualy have not changed
  g_object_freeze_notify ((GObject *) (self->priv->pattern));
  if (strcmp (self->priv->name, name)) {
    GST_DEBUG ("set name from '%s' -> '%s'", self->priv->name, name);
    g_object_set (self->priv->pattern, "name", self->priv->name, NULL);
  }
  g_free (name);
  if (self->priv->length != length) {
    GST_DEBUG ("set length from %lu -> %lu", self->priv->length, length);
    g_object_set (self->priv->pattern, "length", self->priv->length, NULL);
  }
  if (self->priv->voices != voices) {
    GST_DEBUG ("set voices from %lu -> %lu", self->priv->voices, voices);
    g_object_set (self->priv->machine, "voices", self->priv->voices, NULL);
  }
  g_object_thaw_notify ((GObject *) (self->priv->pattern));
}

//-- wrapper

//-- class internals

static void
bt_pattern_properties_dialog_get_property (GObject * const object,
    const guint property_id, GValue * const value, GParamSpec * const pspec)
{
  BtPatternPropertiesDialog *self = BT_PATTERN_PROPERTIES_DIALOG (object);
  return_if_disposed ();
  switch (property_id) {
    case PATTERN_PROPERTIES_DIALOG_NAME:
      g_value_set_string (value, self->priv->name);
      break;
    case PATTERN_PROPERTIES_DIALOG_LENGTH:
      g_value_set_ulong (value, self->priv->length);
      break;
    case PATTERN_PROPERTIES_DIALOG_VOICES:
      g_value_set_ulong (value, self->priv->voices);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_pattern_properties_dialog_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  BtPatternPropertiesDialog *self = BT_PATTERN_PROPERTIES_DIALOG (object);
  return_if_disposed ();
  switch (property_id) {
    case PATTERN_PROPERTIES_DIALOG_PATTERN:
      g_object_try_unref (self->priv->pattern);
      self->priv->pattern = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_pattern_properties_dialog_dispose (GObject * object)
{
  BtPatternPropertiesDialog *self = BT_PATTERN_PROPERTIES_DIALOG (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  g_object_try_unref (self->priv->pattern);
  g_object_try_unref (self->priv->machine);
  g_object_unref (self->priv->app);

  G_OBJECT_CLASS (bt_pattern_properties_dialog_parent_class)->dispose (object);
}

static void
bt_pattern_properties_dialog_finalize (GObject * object)
{
  BtPatternPropertiesDialog *self = BT_PATTERN_PROPERTIES_DIALOG (object);

  GST_DEBUG ("!!!! self=%p", self);

  g_free (self->priv->machine_id);
  g_free (self->priv->name);

  G_OBJECT_CLASS (bt_pattern_properties_dialog_parent_class)->finalize (object);
}

static void
bt_pattern_properties_dialog_init (BtPatternPropertiesDialog * self)
{
  self->priv = bt_pattern_properties_dialog_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();
}

static void
bt_pattern_properties_dialog_class_init (BtPatternPropertiesDialogClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = bt_pattern_properties_dialog_get_property;
  gobject_class->set_property = bt_pattern_properties_dialog_set_property;
  gobject_class->dispose = bt_pattern_properties_dialog_dispose;
  gobject_class->finalize = bt_pattern_properties_dialog_finalize;

  g_object_class_install_property (gobject_class,
      PATTERN_PROPERTIES_DIALOG_PATTERN, g_param_spec_object ("pattern",
          "pattern construct prop", "Set pattern object, the dialog handles",
          BT_TYPE_PATTERN,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PATTERN_PROPERTIES_DIALOG_NAME, g_param_spec_string ("name", "name prop",
          "the display-name of the pattern", "unamed",
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PATTERN_PROPERTIES_DIALOG_LENGTH, g_param_spec_ulong ("length",
          "length prop", "length of the pattern in ticks", 1, G_MAXULONG, 1,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PATTERN_PROPERTIES_DIALOG_VOICES, g_param_spec_ulong ("voices",
          "voices prop", "number of voices in the pattern", 0, G_MAXULONG, 0,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}
