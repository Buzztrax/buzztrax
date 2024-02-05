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
 * SECTION:btmachinerenamedialog
 * @short_description: machine settings
 *
 * A dialog to (re)configure a #BtMachine.
 */

#define BT_EDIT
#define BT_MACHINE_RENAME_DIALOG_C

#include "bt-edit.h"

//-- property ids

enum
{
  MACHINE_RENAME_DIALOG_MACHINE = 1,
  MACHINE_RENAME_DIALOG_NAME
};

struct _BtMachineRenameDialog {
  GtkDialog parent;
  
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* the underlying machine */
  BtMachine *machine;

  BtSetup *setup;

  BtChangeLog *change_log;

  /* dialog data */
  gchar *name;

  /* widgets and their handlers */
  GtkWidget *okay_button;
};

//-- the class

G_DEFINE_TYPE (BtMachineRenameDialog, bt_machine_rename_dialog, ADW_TYPE_MESSAGE_DIALOG);


//-- event handler

static void
on_name_changed (GtkEditable * editable, gpointer user_data)
{
  BtMachineRenameDialog *self = BT_MACHINE_RENAME_DIALOG (user_data);
  const gchar *name = gtk_editable_get_text (GTK_EDITABLE (editable));
  gboolean unique = FALSE;

  GST_DEBUG ("change name");
  // ensure uniqueness of the entered data
  if (*name) {
    BtMachine *machine;
    if ((machine = bt_setup_get_machine_by_id (self->setup, name))) {
      if (machine == self->machine) {
        unique = TRUE;
      }
      g_object_unref (machine);
    } else {
      unique = TRUE;
    }
  }
  GST_INFO ("%s" "unique '%s'", (unique ? "not " : ""), name);
  gtk_widget_set_sensitive (self->okay_button, unique);
  // update field
  g_free (self->name);
  self->name = g_strdup (name);
}

//-- helper methods

static void
bt_machine_rename_dialog_init_ui (BtMachineRenameDialog * self)
{
  BtSong *song;
  gchar *title;

  // get dialog data
  g_object_get (self->machine, "id", &self->name,
      "song", &song, NULL);
  g_object_get (song, "setup", &self->setup, NULL);
  g_object_unref (song);

  // set dialog title
  title = g_strdup_printf (_("%s name"), self->name);
  adw_message_dialog_set_heading (ADW_MESSAGE_DIALOG (self), title);
  g_free (title);

  // grab okay button, so that we can block if input is not valid
#if 0 /// GTK4
  self->okay_button =
      gtk_dialog_get_widget_for_response (GTK_DIALOG (self),
      GTK_RESPONSE_ACCEPT);
#endif
}

static void
bt_machine_rename_dialog_cb (GObject * source_object, GAsyncResult * res, gpointer data) {
  BtMachineRenameDialog *self = BT_MACHINE_RENAME_DIALOG (source_object);
  
  const gchar *response = adw_message_dialog_choose_finish (
      ADW_MESSAGE_DIALOG (self), res);
  
  gchar *undo_str, *redo_str;
  if (g_strcmp0 (response, "ok")) {
    gchar *mid;
    g_object_get (self->machine, "id", &mid, NULL);

    const gchar *new_name = gtk_editable_get_text (GTK_EDITABLE (self->name));
    
    bt_change_log_start_group (self->change_log);
    undo_str =
      g_strdup_printf ("set_machine_property \"%s\",\"name\",\"%s\"",
          new_name, mid);
    redo_str =
      g_strdup_printf ("set_machine_property \"%s\",\"name\",\"%s\"", mid,
                       new_name);
    bt_change_log_add (self->change_log, BT_CHANGE_LOGGER (self),
                       undo_str, redo_str);
    bt_change_log_end_group (self->change_log);

    g_object_set (self->machine, "id", new_name, NULL);
    
    g_free (mid);
  }
}

//-- constructor methods

/**
 * bt_machine_rename_dialog_show:
 * @machine: the machine for which to create the dialog for
 *
 * Create and show a machine rename interface dialog
 *
 * Returns: the new instance
 */
BtMachineRenameDialog *
bt_machine_rename_dialog_show (BtMachine * machine, BtChangeLog * log)
{
  BtMachineRenameDialog *self;

  self =
      BT_MACHINE_RENAME_DIALOG (g_object_new (BT_TYPE_MACHINE_RENAME_DIALOG,
          "machine", machine, NULL));

  self->change_log = g_object_ref (log);
  bt_machine_rename_dialog_init_ui (self);
  
  adw_message_dialog_choose (ADW_MESSAGE_DIALOG (self), NULL,
      bt_machine_rename_dialog_cb, NULL);
  
  return self;
}

//-- methods

//-- wrapper

//-- class internals

static void
bt_machine_rename_dialog_get_property (GObject * const object,
    const guint property_id, GValue * const value, GParamSpec * const pspec)
{
  BtMachineRenameDialog *self = BT_MACHINE_RENAME_DIALOG (object);
  return_if_disposed_self ();
  switch (property_id) {
    case MACHINE_RENAME_DIALOG_NAME:
      g_value_set_string (value, self->name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_machine_rename_dialog_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  BtMachineRenameDialog *self = BT_MACHINE_RENAME_DIALOG (object);
  return_if_disposed_self ();
  switch (property_id) {
    case MACHINE_RENAME_DIALOG_MACHINE:
      g_object_try_unref (self->machine);
      self->machine = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_machine_rename_dialog_dispose (GObject * object)
{
  BtMachineRenameDialog *self = BT_MACHINE_RENAME_DIALOG (object);

  return_if_disposed_self ();
  self->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  g_object_try_unref (self->machine);
  g_object_try_unref (self->setup);
  g_object_unref (self->app);
  g_clear_object (&self->change_log);

  gtk_widget_dispose_template (GTK_WIDGET (self), BT_TYPE_MACHINE_RENAME_DIALOG);
  
  G_OBJECT_CLASS (bt_machine_rename_dialog_parent_class)->dispose (object);
}

static void
bt_machine_rename_dialog_finalize (GObject * object)
{
  BtMachineRenameDialog *self = BT_MACHINE_RENAME_DIALOG (object);

  GST_DEBUG ("!!!! self=%p", self);

  g_free (self->name);

  G_OBJECT_CLASS (bt_machine_rename_dialog_parent_class)->finalize (object);
}

static void
bt_machine_rename_dialog_init (BtMachineRenameDialog * self)
{
  self = bt_machine_rename_dialog_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  self->app = bt_edit_application_new ();
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
bt_machine_rename_dialog_class_init (BtMachineRenameDialogClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = bt_machine_rename_dialog_get_property;
  gobject_class->set_property = bt_machine_rename_dialog_set_property;
  gobject_class->dispose = bt_machine_rename_dialog_dispose;
  gobject_class->finalize = bt_machine_rename_dialog_finalize;

  g_object_class_install_property (gobject_class, MACHINE_RENAME_DIALOG_MACHINE,
      g_param_spec_object ("machine", "machine construct prop",
          "Set machine object, the dialog handles", BT_TYPE_MACHINE,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, MACHINE_RENAME_DIALOG_NAME,
      g_param_spec_string ("name", "name prop",
          "the display-name of the machine", "unamed",
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  gtk_widget_class_set_template_from_resource (widget_class,
      "/org/buzztrax/ui/machine-rename-dialog.ui");

  gtk_widget_class_bind_template_child (widget_class, BtMachineRenameDialog,
      name);
  
  gtk_widget_class_bind_template_callback (widget_class, on_name_changed);
}
