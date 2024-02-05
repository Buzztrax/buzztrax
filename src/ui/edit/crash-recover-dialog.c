/* Buzztrax
 * Copyright (C) 2010 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * SECTION:btcrashrecoverdialog
 * @short_description: class for the song recovery dialog
 *
 * Show a dialog listing found unsaved songs. Allows to delete or restore them.
 */

/* what info do we want to show in the list
 * - songs names (ev. try to shorten $HOME or even songdir)
 * - last saved data
 * - number of changes?
 *
 * what actions do we wan to offer
 * - close dialog
 * - delete log (ask for confirmation)
 * - recover log
 *   - show message afterwards and ask to check and save?
 *
 * - other implementations:
 *   - jokosher:
 *     http://blog.mikeasoft.com/2008/08/10/improved-crash-protection-for-jokosher/
 *     - list of entries with restore and delete buttons
 *     - menu entry in file-menu
 *
 * whats the work flow if there are multiple entries found? right now:
 * - we close this dialog after one got restored
 * - one would save or close the file
 * - one would invoke this dialog from the menu again.
 *
 */

#define BT_EDIT
#define BT_CRASH_RECOVER_DIALOG_C

#include "bt-edit.h"
#include "list-item-pointer.h"
#include <glib/gstdio.h>

//-- property ids

enum
{
  CRASH_RECOVER_DIALOG_ENTRIES = 1,
};

struct _BtCrashRecoverDialogPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  GList *entries;
  GtkColumnView *entries_list;

  /* the application */
  BtEditApplication *app;
};

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtCrashRecoverDialog, bt_crash_recover_dialog, GTK_TYPE_WINDOW,
    G_ADD_PRIVATE(BtCrashRecoverDialog));

//-- helper

static gchar *
get_selected (BtCrashRecoverDialog * self)
{
  GtkSingleSelection *selection = GTK_SINGLE_SELECTION (
      gtk_column_view_get_model (GTK_COLUMN_VIEW (self->priv->entries_list)));
  
  BtListItemPointer *item = gtk_single_selection_get_selected_item (selection);
  BtChangeLogFile *file = (BtChangeLogFile *) item->value;
  return file->log_name;
}

static void
remove_selected (BtCrashRecoverDialog * self)
{
  GtkSingleSelection *selection = GTK_SINGLE_SELECTION (
      gtk_column_view_get_model (GTK_COLUMN_VIEW (self->priv->entries_list)));
  
  GListStore *store = G_LIST_STORE (gtk_single_selection_get_model (selection));
  g_list_store_remove (store, gtk_single_selection_get_selected (selection));
}

//-- event handler

static void
on_recover_clicked (GtkButton * button, gpointer user_data)
{
  BtCrashRecoverDialog *self = BT_CRASH_RECOVER_DIALOG (user_data);
  gchar *log_name = get_selected (self);
  gboolean res = FALSE;
  BtMainWindow *main_window;

  if (log_name) {
    BtChangeLog *change_log = bt_change_log_new ();

    GST_INFO ("recovering: %s", log_name);
    if (bt_change_log_recover (change_log, log_name)) {
      remove_selected (self);
      res = TRUE;
    }
    g_free (log_name);
    g_object_try_unref (change_log);
  }
  g_object_get (self->priv->app, "main-window", &main_window, NULL);
  /* close the recovery dialog */
  gtk_window_close (GTK_WINDOW (self));
  if (res) {
    /* the song recovery has been finished */
    bt_dialog_message (main_window, _("Recovery finished"),
        _("The selected song has been recovered successful."),
        _("Please check the song and save it if everything is alright.")
        );
  } else {
    /* FIXME(ensonic): the log is still there
     * - this dialog should be a warning
     * - ev. we want to suggest to ask for support
     */
    /* one or more steps in the recovery did not apply */
    bt_dialog_message (main_window, _("Recovery failed"),
        _("Sorry, the selected song could not be fully recovered."),
        _("Please check the song and save it if still looks good.")
        );
  }
  g_object_unref (main_window);
}

static void
on_delete_clicked (GtkButton * button, gpointer user_data)
{
  BtCrashRecoverDialog *self = BT_CRASH_RECOVER_DIALOG (user_data);
  gchar *log_name = get_selected (self);

  if (log_name) {
    if (g_remove (log_name)) {
      GST_WARNING ("failed removing '%s': %s", log_name, g_strerror (errno));
    }
    remove_selected (self);
    g_free (log_name);

    GtkSelectionModel *selection = gtk_column_view_get_model (
        GTK_COLUMN_VIEW (self->priv->entries_list));
    
    GtkBitset* bitset = gtk_selection_model_get_selection (selection);
    if (gtk_bitset_is_empty (bitset)) {
      gtk_window_close (GTK_WINDOW (self));
    }
    gtk_bitset_unref (bitset);
  }
}

//-- helper methods

static void
bt_crash_recover_dialog_listitem_setup (GtkListItemFactory *factory,
                                        GtkListItem        *list_item)
{
  GtkWidget *label = gtk_label_new ("");
  gtk_list_item_set_child (list_item, label);
}

static void
bt_crash_recover_dialog_listitem_songfile (GtkListItemFactory *factory,
                                           GtkListItem        *list_item)
{
  GtkWidget *label = gtk_list_item_get_child (list_item);
  BtListItemPointer *obj = BT_LIST_ITEM_POINTER (gtk_list_item_get_item (list_item));
  BtChangeLogFile *clf = (BtChangeLogFile*) bt_list_item_pointer_get (obj);
  gtk_label_set_label (GTK_LABEL (label), clf->song_file_name);
}

static void
bt_crash_recover_dialog_listitem_change_ts (GtkListItemFactory *factory,
                                            GtkListItem        *list_item)
{
  GtkWidget *label = gtk_list_item_get_child (list_item);
  BtListItemPointer *obj = BT_LIST_ITEM_POINTER (gtk_list_item_get_item (list_item));
  BtChangeLogFile *clf = (BtChangeLogFile*) bt_list_item_pointer_get (obj);
  gtk_label_set_label (GTK_LABEL (label), clf->change_ts);
}

static void
bt_crash_recover_dialog_init_ui (const BtCrashRecoverDialog * self)
{
  GtkWidget *label, *hbox, *vbox, *entries_view;
  gchar *str;
  GList *node;

  GST_DEBUG ("prepare crash recover dialog");

  gtk_widget_set_name (GTK_WIDGET (self), "Unsaved song recovery");
  gtk_window_set_title (GTK_WINDOW (self), _("Unsaved song recovery"));

  // add dialog commision widgets (okay)
  // FIXME(ensonic): add Okay, Cancel, Delete
  // select song + okay -> recover
  // select song + delete -> remove log
  /// GTK4 gtk_dialog_add_button (GTK_DIALOG (self), _("_Close"), GTK_RESPONSE_CLOSE);

  // content area
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_window_set_child (GTK_WINDOW (self), hbox);
  /// GTK4 gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);

  /// GTK4
/*  icon =
      gtk_image_new_from_icon_name ("dialog-information", GTK_ICON_SIZE_DIALOG);
      gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, FALSE, 0);*/

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  str =
      g_strdup_printf ("<big><b>%s</b></big>\n%s\n", _("Unsaved songs found"),
      _("Select them one by one and choose 'recover' or 'delete'."));
  label = g_object_new (GTK_TYPE_LABEL, "use-markup", TRUE, "label", str, NULL);
  g_free (str);
  gtk_box_append (GTK_BOX (vbox), label);

  // Note: store is appropriated by gtk_column_view_new, no need for unref.
  GListStore* store = g_list_store_new (bt_list_item_pointer_get_type());
  
  // fill model from self->priv->entries
  for (node = self->priv->entries; node; node = g_list_next (node)) {
    g_list_store_append (store, bt_list_item_pointer_new (node->data));
  }
  
  self->priv->entries_list = GTK_COLUMN_VIEW (
    gtk_column_view_new (
      GTK_SELECTION_MODEL (gtk_single_selection_new (G_LIST_MODEL (store)))));

  GtkListItemFactory* factory; 
  factory = gtk_signal_list_item_factory_new (); // note: appropriated by gtk_column_view_column_new
  g_signal_connect (factory, "setup", G_CALLBACK (bt_crash_recover_dialog_listitem_setup), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bt_crash_recover_dialog_listitem_songfile), NULL);
  
  GtkColumnViewColumn* col;
  
  col = gtk_column_view_column_new (_("Song file"), factory);
  
  gtk_column_view_append_column (self->priv->entries_list, col);
  g_object_unref(col);

  factory = gtk_signal_list_item_factory_new (); // note: appropriated by gtk_column_view_column_new
  g_signal_connect (factory, "setup", G_CALLBACK (bt_crash_recover_dialog_listitem_setup), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bt_crash_recover_dialog_listitem_change_ts), NULL);
  
  col = gtk_column_view_column_new (_("Last changed"), factory);
  
  gtk_column_view_append_column (self->priv->entries_list, col);
  g_object_unref(col);

  entries_view = gtk_scrolled_window_new ();
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (entries_view),
      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (entries_view),
      GTK_WIDGET (self->priv->entries_list));

  gtk_box_append (GTK_BOX (vbox), entries_view);

  gtk_box_append (GTK_BOX (hbox), vbox);

  // add "undelete" button to action area

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 25);
  gtk_box_append (GTK_BOX (vbox), hbox);
  
  GtkWidget* btn;
  btn = gtk_button_new_with_label (_("Recover"));     // we send close once we recovered the log
  g_signal_connect (btn, "clicked", G_CALLBACK (on_recover_clicked),
      (gpointer) self);
  gtk_box_append (GTK_BOX (hbox), btn);

  btn = gtk_button_new_with_label (_("Delete"));
  g_signal_connect (btn, "clicked", G_CALLBACK (on_delete_clicked),
      (gpointer) self);
  gtk_box_append (GTK_BOX (hbox), btn);
}

//-- constructor methods

/**
 * bt_crash_recover_dialog_new:
 * @crash_entries: list of found change-logs
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtCrashRecoverDialog *
bt_crash_recover_dialog_new (GList * crash_entries)
{
  BtCrashRecoverDialog *self =
      BT_CRASH_RECOVER_DIALOG (g_object_new (BT_TYPE_CRASH_RECOVER_DIALOG,
          "entries", crash_entries, NULL));
  bt_crash_recover_dialog_init_ui (self);
  return self;
}

//-- methods

//-- wrapper

//-- class internals

static void
bt_crash_recover_dialog_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  BtCrashRecoverDialog *self = BT_CRASH_RECOVER_DIALOG (object);
  return_if_disposed ();
  switch (property_id) {
    case CRASH_RECOVER_DIALOG_ENTRIES:
      self->priv->entries = g_value_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_crash_recover_dialog_dispose (GObject * object)
{
  BtCrashRecoverDialog *self = BT_CRASH_RECOVER_DIALOG (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);
  g_object_unref (self->priv->app);

  G_OBJECT_CLASS (bt_crash_recover_dialog_parent_class)->dispose (object);
}

static void
bt_crash_recover_dialog_init (BtCrashRecoverDialog * self)
{
  self->priv = bt_crash_recover_dialog_get_instance_private(self);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();
}

static void
bt_crash_recover_dialog_class_init (BtCrashRecoverDialogClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = bt_crash_recover_dialog_set_property;
  gobject_class->dispose = bt_crash_recover_dialog_dispose;

  g_object_class_install_property (gobject_class, CRASH_RECOVER_DIALOG_ENTRIES,
      g_param_spec_pointer ("entries",
          "entries construct prop",
          "Set crash-entry list, the dialog handles",
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

}
