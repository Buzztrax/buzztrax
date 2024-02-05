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

struct _BtPatternPropertiesDialog
{
  AdwPreferencesWindow parent;
  
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

G_DEFINE_TYPE (BtPatternPropertiesDialog, bt_pattern_properties_dialog, ADW_TYPE_PREFERENCES_WINDOW);


//-- event handler

static void
on_name_changed (GtkEditable * editable, gpointer user_data)
{
#if 0 /// GTK 4
  BtPatternPropertiesDialog *self = BT_PATTERN_PROPERTIES_DIALOG (user_data);
  BtCmdPattern *pattern;
  const gchar *name = gtk_entry_get_text (GTK_ENTRY (editable));
  gboolean unique = FALSE;

  GST_DEBUG ("change name");
  // assure uniqueness of the entered data
  if (*name) {
    if ((pattern = bt_machine_get_pattern_by_name (self->machine, name))) {
      if (pattern == (BtCmdPattern *) self->pattern) {
        unique = TRUE;
      }
      g_object_unref (pattern);
    } else {
      unique = TRUE;
    }
  }
  GST_INFO ("%s" "unique '%s'", (unique ? "" : "not "), name);
  gtk_widget_set_sensitive (self->okay_button, unique);
  // update field
  g_free (self->name);
  self->name = g_strdup (name);
#endif
}

static void
on_length_changed (GtkEditable * editable, gpointer user_data)
{
  // BtPatternPropertiesDialog *self = BT_PATTERN_PROPERTIES_DIALOG (user_data);

  // update field
  /// GTK4 self->length = atol (gtk_entry_get_text (GTK_ENTRY (editable)));
}

static void
on_voices_changed (GtkSpinButton * spinbutton, gpointer user_data)
{
  // BtPatternPropertiesDialog *self = BT_PATTERN_PROPERTIES_DIALOG (user_data);

  // update field
  /// GTK4 self->voices = gtk_spin_button_get_value_as_int (spinbutton);
}

//-- helper methods

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
  g_object_get (self->pattern, "name", &name, "length", &length, "voices",
      &voices, NULL);

  // avoid notifies of properties that actualy have not changed
  g_object_freeze_notify ((GObject *) (self->pattern));
  if (strcmp (self->name, name)) {
    GST_DEBUG ("set name from '%s' -> '%s'", self->name, name);
    g_object_set (self->pattern, "name", self->name, NULL);
  }
  g_free (name);
  if (self->length != length) {
    GST_DEBUG ("set length from %lu -> %lu", self->length, length);
    g_object_set (self->pattern, "length", self->length, NULL);
  }
  if (self->voices != voices) {
    GST_DEBUG ("set voices from %lu -> %lu", self->voices, voices);
    g_object_set (self->machine, "voices", self->voices, NULL);
  }
  g_object_thaw_notify ((GObject *) (self->pattern));
}

//-- wrapper

//-- class internals

static void
bt_pattern_properties_dialog_get_property (GObject * const object,
    const guint property_id, GValue * const value, GParamSpec * const pspec)
{
  BtPatternPropertiesDialog *self = BT_PATTERN_PROPERTIES_DIALOG (object);
  return_if_disposed_self ();
  switch (property_id) {
    case PATTERN_PROPERTIES_DIALOG_NAME:
      g_value_set_string (value, self->name);
      break;
    case PATTERN_PROPERTIES_DIALOG_LENGTH:
      g_value_set_ulong (value, self->length);
      break;
    case PATTERN_PROPERTIES_DIALOG_VOICES:
      g_value_set_ulong (value, self->voices);
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
  return_if_disposed_self ();
  switch (property_id) {
    case PATTERN_PROPERTIES_DIALOG_PATTERN:
      g_object_try_unref (self->pattern);
      self->pattern = g_value_dup_object (value);
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

  return_if_disposed_self ();
  self->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  g_object_try_unref (self->pattern);
  g_object_try_unref (self->machine);
  g_object_unref (self->app);

  gtk_widget_dispose_template (GTK_WIDGET (self), BT_TYPE_PATTERN_PROPERTIES_DIALOG);
  
  G_OBJECT_CLASS (bt_pattern_properties_dialog_parent_class)->dispose (object);
}

static void
bt_pattern_properties_dialog_finalize (GObject * object)
{
  BtPatternPropertiesDialog *self = BT_PATTERN_PROPERTIES_DIALOG (object);

  GST_DEBUG ("!!!! self=%p", self);

  g_free (self->machine_id);
  g_free (self->name);

  G_OBJECT_CLASS (bt_pattern_properties_dialog_parent_class)->finalize (object);
}

static void
bt_pattern_properties_dialog_init (BtPatternPropertiesDialog * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  
  self = bt_pattern_properties_dialog_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  self->app = bt_edit_application_new ();
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

  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class,
      "/org/buzztrax/ui/pattern-properties-dialog.ui");
}
