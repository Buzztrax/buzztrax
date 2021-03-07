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
#include <glib/gstdio.h>

//-- property ids

enum
{
  CRASH_RECOVER_DIALOG_ENTRIES = 1,
};

enum
{
  COL_LOG_NAME = 0,
  COL_SONG_FILE_NAME,
  COL_CHANGE_TS
};

struct _BtCrashRecoverDialogPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  GList *entries;
  GtkTreeView *entries_list;

  /* the application */
  BtEditApplication *app;
};

//-- the class

G_DEFINE_TYPE_WITH_CODE (BtCrashRecoverDialog, bt_crash_recover_dialog, GTK_TYPE_DIALOG, 
    G_ADD_PRIVATE(BtCrashRecoverDialog));

//-- helper

static gboolean
check_selection (BtCrashRecoverDialog * self, GtkTreeModel ** model,
    GtkTreeIter * iter)
{
  GtkTreeSelection *selection;

  selection =
      gtk_tree_view_get_selection (GTK_TREE_VIEW (self->priv->entries_list));
  return gtk_tree_selection_get_selected (selection, model, iter);
}

static gchar *
get_selected (BtCrashRecoverDialog * self)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *log_name = NULL;

  if (check_selection (self, &model, &iter)) {
    gtk_tree_model_get (model, &iter, COL_LOG_NAME, &log_name, -1);
  }
  return log_name;
}

static void
remove_selected (BtCrashRecoverDialog * self)
{
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (check_selection (self, &model, &iter)) {
    GtkTreeIter next_iter = iter;
    gboolean have_next = gtk_tree_model_iter_next (model, &next_iter);

    gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
    if (!have_next) {
      have_next = gtk_tree_model_get_iter_first (model, &next_iter);
    }
    if (have_next) {
      gtk_tree_selection_select_iter (gtk_tree_view_get_selection (GTK_TREE_VIEW
              (self->priv->entries_list)), &next_iter);
    }
  }
}

//-- event handler

static void
on_list_realize (GtkWidget * widget, gpointer user_data)
{
  GtkWidget *parent = gtk_widget_get_parent (widget);
  GtkRequisition requisition;
  gint height, max_height;

  gtk_widget_get_preferred_size (widget, NULL, &requisition);
  bt_gtk_workarea_size (NULL, &max_height);
  /* make sure the dialog resize without scrollbar until it would reach half
   * screen height */
  max_height /= 2;

  GST_DEBUG ("#### crash_recover_list  size req %d x %d (max-height=%d)",
      requisition.width, requisition.height, max_height);

  height = MIN (requisition.height, max_height);
  // TODO(ensonic): is the '2' some border or padding
  gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (parent),
      height + 2);
}

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
  gtk_dialog_response (GTK_DIALOG (self), GTK_RESPONSE_CLOSE);
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
    /* if that was the last entry, close dialog */
    if (!check_selection (self, NULL, NULL)) {
      gtk_dialog_response (GTK_DIALOG (self), GTK_RESPONSE_CLOSE);
    }
  }
}

//-- helper methods

static void
bt_crash_recover_dialog_init_ui (const BtCrashRecoverDialog * self)
{
  GtkWidget *label, *icon, *hbox, *vbox, *btn, *entries_view;
  gchar *str;
  GtkCellRenderer *renderer;
  GtkListStore *store;
  GtkTreeIter tree_iter;
  GList *node;
  BtChangeLogFile *crash_entry;

  GST_DEBUG ("prepare crash recover dialog");

  gtk_widget_set_name (GTK_WIDGET (self), "Unsaved song recovery");
  gtk_window_set_title (GTK_WINDOW (self), _("Unsaved song recovery"));

  // add dialog commision widgets (okay)
  // FIXME(ensonic): add Okay, Cancel, Delete
  // select song + okay -> recover
  // select song + delete -> remove log
  gtk_dialog_add_button (GTK_DIALOG (self), _("_Close"), GTK_RESPONSE_CLOSE);

  // content area
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);

  icon =
      gtk_image_new_from_icon_name ("dialog-information", GTK_ICON_SIZE_DIALOG);
  gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, FALSE, 0);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  str =
      g_strdup_printf ("<big><b>%s</b></big>\n%s\n", _("Unsaved songs found"),
      _("Select them one by one and choose 'recover' or 'delete'."));
  label = g_object_new (GTK_TYPE_LABEL, "use-markup", TRUE, "label", str, NULL);
  g_free (str);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  self->priv->entries_list = GTK_TREE_VIEW (gtk_tree_view_new ());
  g_object_set (self->priv->entries_list,
      "enable-search", FALSE, "rules-hint", TRUE,
      /*"fixed-height-mode",TRUE, */// causes the first column to be not shown (or getting width=0)
      NULL);
  gtk_tree_selection_set_mode (gtk_tree_view_get_selection (self->
          priv->entries_list), GTK_SELECTION_BROWSE);
  g_signal_connect (self->priv->entries_list, "realize",
      G_CALLBACK (on_list_realize), (gpointer) self);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT
      (renderer), 1);
  /* column listing song file names to recover */
  gtk_tree_view_insert_column_with_attributes (self->priv->entries_list, -1,
      _("Song file"), renderer, "text", COL_SONG_FILE_NAME, NULL);
  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT
      (renderer), 1);
  /* column listing song file names to recover */
  gtk_tree_view_insert_column_with_attributes (self->priv->entries_list, -1,
      _("Last changed"), renderer, "text", COL_CHANGE_TS, NULL);

  // fill model from self->priv->entries
  store = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  for (node = self->priv->entries; node; node = g_list_next (node)) {
    crash_entry = (BtChangeLogFile *) node->data;
    gtk_list_store_append (store, &tree_iter);
    gtk_list_store_set (store, &tree_iter,
        COL_LOG_NAME, crash_entry->log_name,
        COL_SONG_FILE_NAME, crash_entry->song_file_name,
        COL_CHANGE_TS, crash_entry->change_ts, -1);
  }
  gtk_tree_view_set_model (self->priv->entries_list, GTK_TREE_MODEL (store));
  g_object_unref (store);       // drop with treeview

  entries_view = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (entries_view),
      GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (entries_view),
      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (entries_view),
      GTK_WIDGET (self->priv->entries_list));

  gtk_box_pack_start (GTK_BOX (vbox), entries_view, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))),
      hbox, TRUE, TRUE, 0);

  // add "undelete" button to action area
  // GTK_STOCK_REVERT_TO_SAVED
  btn = gtk_dialog_add_button (GTK_DIALOG (self), _("Recover"), GTK_RESPONSE_NONE);     // we send close once we recovered the log
  g_signal_connect (btn, "clicked", G_CALLBACK (on_recover_clicked),
      (gpointer) self);

  btn = gtk_dialog_add_button (GTK_DIALOG (self), _("Delete"),
      GTK_RESPONSE_NONE);
  g_signal_connect (btn, "clicked", G_CALLBACK (on_delete_clicked),
      (gpointer) self);
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
