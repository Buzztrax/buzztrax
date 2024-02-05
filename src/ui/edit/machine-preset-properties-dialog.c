/* Buzztrax
 * Copyright (C) 2007 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * SECTION:btmachinepresetpropertiesdialog
 * @short_description: machine preset settings
 *
 * A dialog to (re)configure machine presets.
 */

#define BT_EDIT
#define BT_MACHINE_PRESET_PROPERTIES_DIALOG_C

#include "bt-edit.h"
#include "machine-preset-properties-dialog.h"

#if 0 /// GTK4
static void
on_name_changed (GtkEditable * editable, gpointer user_data)
{
  BtMachinePresetPropertiesDialog *self =
      BT_MACHINE_PRESET_PROPERTIES_DIALOG (user_data);
  const gchar *name = gtk_editable_get_text (GTK_EDITABLE (editable));
  gboolean valid = TRUE;

  GST_INFO ("preset text changed: '%s'", name);

  // assure validity & uniquness of the entered data
  if (!(*name))
    // empty box
    valid = FALSE;
  else if (*self->priv->name_ptr && strcmp (name, *self->priv->name_ptr)
      && check_name_exists (self, name))
    // non empty old name && name has changed && name already exists
    valid = FALSE;
  else if (!(*self->priv->name_ptr) && check_name_exists (self, name))
    // name already exists
    valid = FALSE;

  gtk_widget_set_sensitive (self->priv->okay_button, valid);
  g_free (self->priv->name);
  self->priv->name = g_strdup (name);
}
#endif

#if 0 /// GTK4
static void
on_comment_changed (GtkEditable * editable, gpointer user_data)
{
  BtMachinePresetPropertiesDialog *self =
      BT_MACHINE_PRESET_PROPERTIES_DIALOG (user_data);
  const gchar *comment = gtk_editable_get_text (GTK_EDITABLE (editable));

  GST_INFO ("preset comment changed");

  g_free (self->priv->comment);
  self->priv->comment = g_strdup (comment);
}
#endif

gchar* bt_machine_preset_properties_dialog_get_existing_preset_name(AdwMessageDialog* dlg)
{
  return (gchar*) g_object_get_data (G_OBJECT (dlg), "bt-existing-preset-name");
}

/**
 * bt_machine_preset_properties_dialog_new:
 * @machine: the machine for which to create the dialog for
 * @name: the preset name
 * @comment: the comment name
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
AdwMessageDialog*
bt_machine_preset_properties_dialog_new (GstPreset* presets, gchar* existing_preset_name)
{
  GtkBuilder* builder = gtk_builder_new_from_resource (
      "/org/buzztrax/ui/machine-preset-properties.ui");
  
  AdwMessageDialog *dialog = ADW_MESSAGE_DIALOG (gtk_builder_get_object (builder, "dialog"));

  if (existing_preset_name) {
    gchar * comment;

    gst_preset_get_meta (presets, existing_preset_name, "comment", &comment);
    
    gchar* name = g_strdup (existing_preset_name);
    g_object_set_data_full (G_OBJECT (dialog), "bt-existing-preset-name", name, g_free);

    g_free (comment);
  } else {
    // set name "New Preset" or ""...
  }
  
  gtk_window_present (GTK_WINDOW (dialog));
  g_object_unref (builder);
  
  return dialog;
}
