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

struct _BtMachineRenameDialogPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* the underlying machine */
  BtMachine *machine;

  BtSetup *setup;

  /* dialog data */
  gchar *name;

  /* widgets and their handlers */
  GtkWidget *okay_button;
};

//-- the class

G_DEFINE_TYPE (BtMachineRenameDialog, bt_machine_rename_dialog,
    GTK_TYPE_DIALOG);


//-- event handler

static void
on_name_changed (GtkEditable * editable, gpointer user_data)
{
  BtMachineRenameDialog *self = BT_MACHINE_RENAME_DIALOG (user_data);
  const gchar *name = gtk_entry_get_text (GTK_ENTRY (editable));
  gboolean unique = FALSE;

  GST_DEBUG ("change name");
  // ensure uniqueness of the entered data
  if (*name) {
    BtMachine *machine;
    if ((machine = bt_setup_get_machine_by_id (self->priv->setup, name))) {
      if (machine == self->priv->machine) {
        unique = TRUE;
      }
      g_object_unref (machine);
    } else {
      unique = TRUE;
    }
  }
  GST_INFO ("%s" "unique '%s'", (unique ? "not " : ""), name);
  gtk_widget_set_sensitive (self->priv->okay_button, unique);
  // update field
  g_free (self->priv->name);
  self->priv->name = g_strdup (name);
}

//-- helper methods

static void
bt_machine_rename_dialog_init_ui (const BtMachineRenameDialog * self)
{
  GtkWidget *label, *widget, *table;
  BtSong *song;
  gchar *title;
  //GdkPixbuf *window_icon=NULL;

  gtk_widget_set_name (GTK_WIDGET (self), "rename machine");

  // create and set window icon
  /* e.v. tab_machine.png
     if((window_icon=bt_ui_resources_get_icon_pixbuf_by_machine(self->priv->machine))) {
     gtk_window_set_icon(GTK_WINDOW(self),window_icon);
     g_object_unref(window_icon);
     }
   */

  // get dialog data
  g_object_get (self->priv->machine, "id", &self->priv->name,
      "song", &song, NULL);
  g_object_get (song, "setup", &self->priv->setup, NULL);
  g_object_unref (song);

  // set dialog title
  title = g_strdup_printf (_("%s name"), self->priv->name);
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

  // GtkEntry : machine name
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
}

//-- constructor methods

/**
 * bt_machine_rename_dialog_new:
 * @machine: the machine for which to create the dialog for
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtMachineRenameDialog *
bt_machine_rename_dialog_new (const BtMachine * machine)
{
  BtMachineRenameDialog *self;

  self =
      BT_MACHINE_RENAME_DIALOG (g_object_new (BT_TYPE_MACHINE_RENAME_DIALOG,
          "machine", machine, NULL));
  bt_machine_rename_dialog_init_ui (self);
  return self;
}

//-- methods

/**
 * bt_machine_rename_dialog_apply:
 * @self: the dialog which settings to apply
 *
 * Makes the dialog settings effective.
 */
void
bt_machine_rename_dialog_apply (const BtMachineRenameDialog * self)
{
  GST_INFO ("applying machine settings");

  g_object_set (self->priv->machine, "id", self->priv->name, NULL);
}

//-- wrapper

//-- class internals

static void
bt_machine_rename_dialog_get_property (GObject * const object,
    const guint property_id, GValue * const value, GParamSpec * const pspec)
{
  BtMachineRenameDialog *self = BT_MACHINE_RENAME_DIALOG (object);
  return_if_disposed ();
  switch (property_id) {
    case MACHINE_RENAME_DIALOG_NAME:
      g_value_set_string (value, self->priv->name);
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
  return_if_disposed ();
  switch (property_id) {
    case MACHINE_RENAME_DIALOG_MACHINE:
      g_object_try_unref (self->priv->machine);
      self->priv->machine = g_value_dup_object (value);
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

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  g_object_try_unref (self->priv->machine);
  g_object_try_unref (self->priv->setup);
  g_object_unref (self->priv->app);

  G_OBJECT_CLASS (bt_machine_rename_dialog_parent_class)->dispose (object);
}

static void
bt_machine_rename_dialog_finalize (GObject * object)
{
  BtMachineRenameDialog *self = BT_MACHINE_RENAME_DIALOG (object);

  GST_DEBUG ("!!!! self=%p", self);

  g_free (self->priv->name);

  G_OBJECT_CLASS (bt_machine_rename_dialog_parent_class)->finalize (object);
}

static void
bt_machine_rename_dialog_init (BtMachineRenameDialog * self)
{
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, BT_TYPE_MACHINE_RENAME_DIALOG,
      BtMachineRenameDialogPrivate);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();
}

static void
bt_machine_rename_dialog_class_init (BtMachineRenameDialogClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BtMachineRenameDialogPrivate));

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
}
