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

//-- property ids

enum
{
  MACHINE_PRESET_PROPERTIES_DIALOG_MACHINE = 1,
  MACHINE_PRESET_PROPERTIES_DIALOG_NAME,
  MACHINE_PRESET_PROPERTIES_DIALOG_COMMENT
};

struct _BtMachinePresetPropertiesDialogPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* the element that has the presets */
  GstElement *machine;
  gchar **presets;

  /* dialog data */
  gchar *name, *comment;
  gchar **name_ptr, **comment_ptr;

  /* widgets and their handlers */
  GtkWidget *okay_button;
};

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtMachinePresetPropertiesDialog,
    bt_machine_preset_properties_dialog, GTK_TYPE_DIALOG, 
    G_ADD_PRIVATE(BtMachinePresetPropertiesDialog));

//-- event handler

static gboolean
check_name_exists (BtMachinePresetPropertiesDialog * self, const gchar * name)
{
  gchar **preset;

  if (!self->priv->presets)
    return FALSE;

  for (preset = self->priv->presets; *preset; preset++) {
    if (!strcmp (name, *preset))
      return TRUE;
  }
  return FALSE;
}

static void
on_name_changed (GtkEditable * editable, gpointer user_data)
{
  BtMachinePresetPropertiesDialog *self =
      BT_MACHINE_PRESET_PROPERTIES_DIALOG (user_data);
  const gchar *name = gtk_entry_get_text (GTK_ENTRY (editable));
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

static void
on_comment_changed (GtkEditable * editable, gpointer user_data)
{
  BtMachinePresetPropertiesDialog *self =
      BT_MACHINE_PRESET_PROPERTIES_DIALOG (user_data);
  const gchar *comment = gtk_entry_get_text (GTK_ENTRY (editable));

  GST_INFO ("preset comment changed");

  g_free (self->priv->comment);
  self->priv->comment = g_strdup (comment);
}

//-- helper methods

static void
bt_machine_preset_properties_dialog_init_ui (const
    BtMachinePresetPropertiesDialog * self)
{
  GtkWidget *label, *widget, *table;
  //GdkPixbuf *window_icon=NULL;

  gtk_widget_set_name (GTK_WIDGET (self), "preset name and comment");

  // create and set window icon
  /*
     if((window_icon=bt_ui_resources_get_icon_pixbuf_by_machine(self->priv->machine))) {
     gtk_window_set_icon(GTK_WINDOW(self),window_icon);
     g_object_unref(window_icon);
     }
   */

  // set dialog title
  gtk_window_set_title (GTK_WINDOW (self), _("preset name and comment"));

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

  // GtkEntry : preset name
  label = gtk_label_new (_("name"));
  g_object_set (label, "xalign", 1.0, NULL);
  gtk_grid_attach (GTK_GRID (table), label, 0, 0, 1, 1);

  widget = gtk_entry_new ();
  if (self->priv->name)
    gtk_entry_set_text (GTK_ENTRY (widget), self->priv->name);
  else
    gtk_widget_set_sensitive (self->priv->okay_button, FALSE);
  gtk_entry_set_activates_default (GTK_ENTRY (widget), TRUE);
  g_object_set (widget, "hexpand", TRUE, "margin-left", LABEL_PADDING, NULL);
  gtk_grid_attach (GTK_GRID (table), widget, 1, 0, 1, 1);
  g_signal_connect (widget, "changed", G_CALLBACK (on_name_changed),
      (gpointer) self);

  // GtkEntry : preset comment
  label = gtk_label_new (_("comment"));
  g_object_set (label, "xalign", 1.0, NULL);
  gtk_grid_attach (GTK_GRID (table), label, 0, 1, 1, 1);

  widget = gtk_entry_new ();
  if (self->priv->comment)
    gtk_entry_set_text (GTK_ENTRY (widget), self->priv->comment);
  gtk_entry_set_activates_default (GTK_ENTRY (widget), TRUE);
  g_object_set (widget, "hexpand", TRUE, "margin-left", LABEL_PADDING, NULL);
  gtk_grid_attach (GTK_GRID (table), widget, 1, 1, 1, 1);
  g_signal_connect (widget, "changed", G_CALLBACK (on_comment_changed),
      (gpointer) self);
}

//-- constructor methods

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
BtMachinePresetPropertiesDialog *
bt_machine_preset_properties_dialog_new (GstElement * machine, gchar ** name,
    gchar ** comment)
{
  BtMachinePresetPropertiesDialog *self;

  self =
      BT_MACHINE_PRESET_PROPERTIES_DIALOG (g_object_new
      (BT_TYPE_MACHINE_PRESET_PROPERTIES_DIALOG, "machine", machine, "name",
          name, "comment", comment, NULL));
  bt_machine_preset_properties_dialog_init_ui (self);
  return self;
}

//-- methods

/**
 * bt_machine_preset_properties_dialog_apply:
 * @self: the dialog which settings to apply
 *
 * Makes the dialog settings effective.
 */
void
bt_machine_preset_properties_dialog_apply (const BtMachinePresetPropertiesDialog
    * self)
{
  GST_INFO ("applying preset changes settings");

  *self->priv->name_ptr = self->priv->name;
  self->priv->name = NULL;
  *self->priv->comment_ptr = self->priv->comment;
  self->priv->comment = NULL;
}

//-- wrapper

//-- class internals

static void
bt_machine_preset_properties_dialog_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec)
{
  BtMachinePresetPropertiesDialog *self =
      BT_MACHINE_PRESET_PROPERTIES_DIALOG (object);
  return_if_disposed ();
  switch (property_id) {
    case MACHINE_PRESET_PROPERTIES_DIALOG_MACHINE:
      g_value_set_object (value, self->priv->machine);
      break;
    case MACHINE_PRESET_PROPERTIES_DIALOG_NAME:
      g_value_set_pointer (value, self->priv->name_ptr);
      break;
    case MACHINE_PRESET_PROPERTIES_DIALOG_COMMENT:
      g_value_set_pointer (value, self->priv->comment_ptr);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_machine_preset_properties_dialog_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec)
{
  BtMachinePresetPropertiesDialog *self =
      BT_MACHINE_PRESET_PROPERTIES_DIALOG (object);
  return_if_disposed ();
  switch (property_id) {
    case MACHINE_PRESET_PROPERTIES_DIALOG_MACHINE:
      g_object_try_unref (self->priv->machine);
      self->priv->machine = g_value_dup_object (value);
      GST_DEBUG ("set the machine for preset_dialog: %p", self->priv->machine);
      self->priv->presets =
          self->priv->machine ? gst_preset_get_preset_names (GST_PRESET (self->
              priv->machine)) : NULL;
      break;
    case MACHINE_PRESET_PROPERTIES_DIALOG_NAME:
      self->priv->name_ptr = g_value_get_pointer (value);
      if (self->priv->name_ptr) {
        self->priv->name = g_strdup (*self->priv->name_ptr);
      }
      break;
    case MACHINE_PRESET_PROPERTIES_DIALOG_COMMENT:
      self->priv->comment_ptr = g_value_get_pointer (value);
      if (self->priv->comment_ptr) {
        self->priv->comment = g_strdup (*self->priv->comment_ptr);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_machine_preset_properties_dialog_dispose (GObject * object)
{
  BtMachinePresetPropertiesDialog *self =
      BT_MACHINE_PRESET_PROPERTIES_DIALOG (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  g_object_try_unref (self->priv->machine);
  g_object_unref (self->priv->app);

  G_OBJECT_CLASS (bt_machine_preset_properties_dialog_parent_class)->dispose
      (object);
}

static void
bt_machine_preset_properties_dialog_finalize (GObject * object)
{
  BtMachinePresetPropertiesDialog *self =
      BT_MACHINE_PRESET_PROPERTIES_DIALOG (object);

  GST_DEBUG ("!!!! self=%p", self);

  g_free (self->priv->name);
  g_free (self->priv->comment);
  g_strfreev (self->priv->presets);

  G_OBJECT_CLASS (bt_machine_preset_properties_dialog_parent_class)->finalize
      (object);
}

static void
bt_machine_preset_properties_dialog_init (BtMachinePresetPropertiesDialog *
    self)
{
  self->priv = bt_machine_preset_properties_dialog_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();
}

static void
    bt_machine_preset_properties_dialog_class_init
    (BtMachinePresetPropertiesDialogClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property =
      bt_machine_preset_properties_dialog_set_property;
  gobject_class->get_property =
      bt_machine_preset_properties_dialog_get_property;
  gobject_class->dispose = bt_machine_preset_properties_dialog_dispose;
  gobject_class->finalize = bt_machine_preset_properties_dialog_finalize;

  g_object_class_install_property (gobject_class,
      MACHINE_PRESET_PROPERTIES_DIALOG_MACHINE, g_param_spec_object ("machine",
          "machine construct prop", "Set machine object, the dialog handles",
          GST_TYPE_ELEMENT, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE |
          G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      MACHINE_PRESET_PROPERTIES_DIALOG_NAME, g_param_spec_pointer ("name",
          "name-pointer prop", "address of preset name",
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      MACHINE_PRESET_PROPERTIES_DIALOG_COMMENT, g_param_spec_pointer ("comment",
          "comment-pointer prop", "address of preset comment",
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}
