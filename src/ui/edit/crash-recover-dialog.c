/* $Id$
 *
 * Buzztard
 * Copyright (C) 2010 Buzztard team <buzztard-devel@lists.sf.net>
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/**
 * SECTION:bttipdialog
 * @short_description: class for the editor tip-of-the-day dialog
 *
 * Show a tip, that has not yet been shown to the user.
 */

#define BT_EDIT
#define BT_CRASH_RECOVER_DIALOG_C

#include "bt-edit.h"

//-- property ids

enum {
  CRASH_RECOVER_DIALOG_ENTRIES=1,
};

enum {
  COL_LOG_NAME=0,
  COL_SONG_FILE_NAME,
  COL_CHANGE_TS
};

struct _BtCrashRecoverDialogPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  GList *entries;
  GtkTreeView *entries_list;

  /* the application */
  BtEditApplication *app;
};

//-- the class

G_DEFINE_TYPE (BtCrashRecoverDialog, bt_crash_recover_dialog, GTK_TYPE_DIALOG);

//-- event handler

static void on_recover_clicked(GtkButton *button, gpointer user_data) {
  BtCrashRecoverDialog *self = BT_CRASH_RECOVER_DIALOG(user_data);
  GtkTreeSelection *selection;
  GtkTreeModel     *model;
  GtkTreeIter       iter;

  GST_INFO("entries list cursor changed");
  selection=gtk_tree_view_get_selection(GTK_TREE_VIEW(self->priv->entries_list));
  if(gtk_tree_selection_get_selected(selection, &model, &iter)) {
    BtChangeLog *change_log=bt_change_log_new();
    gchar *log_name;

    gtk_tree_model_get(model,&iter,COL_LOG_NAME,&log_name,-1);
    GST_INFO("recovering: %s",log_name);
    bt_change_log_recover(change_log,log_name);
    g_free(log_name);
    g_object_try_unref(change_log);
  }
}

//-- helper methods

static void bt_crash_recover_dialog_init_ui(const BtCrashRecoverDialog *self) {
  GtkWidget *label,*icon,*hbox,*vbox,*btn,*entries_view;
  gchar *str;
  GtkCellRenderer *renderer;
  GtkListStore *store;
  GtkTreeIter tree_iter;
  GList *node;
  BtChangeLogFile *crash_entry;

  GST_DEBUG("prepare crash recover dialog");

  gtk_widget_set_name(GTK_WIDGET(self),"Previous unfinished sessions found");
  gtk_window_set_title(GTK_WINDOW(self),_("Previous unfinished sessions found"));

  // add dialog commision widgets (okay)
  gtk_dialog_add_buttons(GTK_DIALOG(self),
                          GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                          NULL);

  // content area
  hbox=gtk_hbox_new(FALSE,12);
  gtk_container_set_border_width(GTK_CONTAINER(hbox),6);
  
  icon=gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO,GTK_ICON_SIZE_DIALOG);
  gtk_box_pack_start(GTK_BOX(hbox),icon,FALSE,FALSE,0);
  
  vbox=gtk_vbox_new(FALSE,6);
  str=g_strdup_printf("<big><b>%s</b></big>\n",_("Previous unfinished sessions found"));
  label=g_object_new(GTK_TYPE_LABEL,"use-markup",TRUE,"label",str,NULL);
  g_free(str);
  gtk_box_pack_start(GTK_BOX(vbox),label,FALSE,FALSE,0);

  self->priv->entries_list=GTK_TREE_VIEW(gtk_tree_view_new());
  gtk_tree_selection_set_mode(gtk_tree_view_get_selection(self->priv->entries_list),GTK_SELECTION_BROWSE);
  renderer=gtk_cell_renderer_text_new();
  gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer), 1);
  /* column listing song file names to recover */
  gtk_tree_view_insert_column_with_attributes(self->priv->entries_list,-1,_("Song file"),renderer,"text",COL_SONG_FILE_NAME,NULL);
  renderer=gtk_cell_renderer_text_new();
  gtk_cell_renderer_text_set_fixed_height_from_font(GTK_CELL_RENDERER_TEXT(renderer), 1);
  /* column listing song file names to recover */
  gtk_tree_view_insert_column_with_attributes(self->priv->entries_list,-1,_("Last changed"),renderer,"text",COL_CHANGE_TS,NULL);

  // fill model from self->priv->entries
  store=gtk_list_store_new(3,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING);
  for(node=self->priv->entries;node;node=g_list_next(node)) {
    crash_entry=(BtChangeLogFile *)node->data;
    gtk_list_store_append(store, &tree_iter);
    gtk_list_store_set(store,&tree_iter,
      COL_LOG_NAME,crash_entry->log_name,
      COL_SONG_FILE_NAME,crash_entry->song_file_name,
      COL_CHANGE_TS,crash_entry->change_ts,
      -1);
  }
  gtk_tree_view_set_model(self->priv->entries_list,GTK_TREE_MODEL(store));
  g_object_unref(store); // drop with treeview
 
  entries_view = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(entries_view), GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(entries_view), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(entries_view),GTK_WIDGET(self->priv->entries_list));
  
  gtk_box_pack_start(GTK_BOX(vbox),entries_view,TRUE,TRUE,0);

  gtk_box_pack_start(GTK_BOX(hbox),vbox,TRUE,TRUE,0);
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(self))),hbox,TRUE,TRUE,0);
  
  // add "undelete" button to action area
  btn=gtk_button_new_from_stock(GTK_STOCK_UNDELETE);
  g_signal_connect(btn, "clicked", G_CALLBACK(on_recover_clicked), (gpointer)self);
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_action_area(GTK_DIALOG(self))),btn,FALSE,FALSE,0);

  gtk_dialog_set_has_separator(GTK_DIALOG(self),TRUE);
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
BtCrashRecoverDialog *bt_crash_recover_dialog_new(GList *crash_entries) {
  BtCrashRecoverDialog *self;

  self=BT_CRASH_RECOVER_DIALOG(g_object_new(BT_TYPE_CRASH_RECOVER_DIALOG,"entries",crash_entries,NULL));
  bt_crash_recover_dialog_init_ui(self);
  return(self);
}

//-- methods

//-- wrapper

//-- class internals

static void bt_crash_recover_dialog_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec) {
  BtCrashRecoverDialog *self = BT_CRASH_RECOVER_DIALOG(object);
  return_if_disposed();
  switch (property_id) {
    case CRASH_RECOVER_DIALOG_ENTRIES: {
      self->priv->entries = g_value_get_pointer(value);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_crash_recover_dialog_dispose(GObject *object) {
  BtCrashRecoverDialog *self = BT_CRASH_RECOVER_DIALOG(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;
  
  GST_DEBUG("!!!! self=%p",self);
  g_object_unref(self->priv->app);

  G_OBJECT_CLASS(bt_crash_recover_dialog_parent_class)->dispose(object);
}

static void bt_crash_recover_dialog_init(BtCrashRecoverDialog *self) {
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_CRASH_RECOVER_DIALOG, BtCrashRecoverDialogPrivate);
  GST_DEBUG("!!!! self=%p",self);
  self->priv->app = bt_edit_application_new();
}

static void bt_crash_recover_dialog_class_init(BtCrashRecoverDialogClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  g_type_class_add_private(klass,sizeof(BtCrashRecoverDialogPrivate));

  gobject_class->set_property = bt_crash_recover_dialog_set_property;
  gobject_class->dispose      = bt_crash_recover_dialog_dispose;

  g_object_class_install_property(gobject_class,CRASH_RECOVER_DIALOG_ENTRIES,
                                  g_param_spec_pointer("entries",
                                     "entries construct prop",
                                     "Set crash-entry list, the dialog handles",
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_WRITABLE|G_PARAM_STATIC_STRINGS));

}

