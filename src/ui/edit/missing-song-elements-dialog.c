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
 * SECTION:btmissingsongelementsdialog
 * @short_description: missing song machine and wave elements
 *
 * A dialog to inform about missing song machine and wave elements.
 */
/*
 * TODO(ensonic): support gst-codec-install (see missing-framework-elements-dialog.c)
 */
#define BT_EDIT
#define BT_MISSING_SONG_ELEMENTS_DIALOG_C

#include "bt-edit.h"
#include <glib/gprintf.h>

//-- property ids

enum
{
  MISSING_SONG_ELEMENTS_DIALOG_MACHINES = 1,
  MISSING_SONG_ELEMENTS_DIALOG_WAVES
};

struct _BtMissingSongElementsDialogPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* list of missing elements */
  GList *machines, *waves;

  GtkWidget *ignore_button;
};

//-- the class

G_DEFINE_TYPE (BtMissingSongElementsDialog, bt_missing_song_elements_dialog,
    GTK_TYPE_DIALOG);


//-- event handler

//-- helper methods

static void
make_listview (GtkWidget * vbox, GList * missing_elements, const gchar * msg)
{
  GtkWidget *label, *missing_list, *missing_list_view;
  GList *node;
  gchar *missing_text, *ptr;
  gint length = 0;

  label = gtk_label_new (msg);
  g_object_set (label, "xalign", 0.0, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  for (node = missing_elements; node; node = g_list_next (node)) {
    length += 2 + strlen ((gchar *) (node->data));
  }
  ptr = missing_text = g_malloc (length);
  gint length_total = length;
  for (node = missing_elements; node; node = g_list_next (node)) {
    length = g_sprintf (ptr, "%s\n", (gchar *) (node->data));
	length_total -= length;
	g_assert(length_total >= 0);
    ptr = &ptr[length];
  }
  ptr[-1] = '\0';               // remove last '\n'

  missing_list = gtk_text_view_new ();
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (missing_list), FALSE);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (missing_list), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (missing_list), GTK_WRAP_WORD);
  gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW
          (missing_list)), missing_text, -1);
  gtk_widget_show (missing_list);
  g_free (missing_text);

  missing_list_view = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (missing_list_view),
      GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (missing_list_view),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (missing_list_view), missing_list);

  gtk_widget_show (missing_list_view);
  gtk_box_pack_start (GTK_BOX (vbox), missing_list_view, TRUE, TRUE, 0);
}

static void
bt_missing_song_elements_dialog_init_ui (const BtMissingSongElementsDialog *
    self)
{
  GtkWidget *label, *icon, *hbox, *vbox;
  gchar *str;
  //GdkPixbuf *window_icon=NULL;

  gtk_widget_set_name (GTK_WIDGET (self), "Missing elements in song");

  // create and set window icon
  /*
     if((window_icon=bt_ui_resources_get_icon_pixbuf_by_machine(self->priv->machine))) {
     gtk_window_set_icon(GTK_WINDOW(self),window_icon);
     g_object_unref(window_icon);
     }
   */

  // set dialog title
  gtk_window_set_title (GTK_WINDOW (self), _("Missing elements in song"));

  // add dialog commision widgets (okay, cancel)
  gtk_dialog_add_button (GTK_DIALOG (self), _("_OK"), GTK_RESPONSE_ACCEPT);

  gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_ACCEPT);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);

  icon = gtk_image_new_from_icon_name ("dialog-warning", GTK_ICON_SIZE_DIALOG);
  gtk_container_add (GTK_CONTAINER (hbox), icon);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  label = gtk_label_new (NULL);
  str = g_strdup_printf ("<big><b>%s</b></big>", _("Missing elements in song"));
  gtk_label_set_markup (GTK_LABEL (label), str);
  g_object_set (label, "xalign", 0.0, NULL);
  g_free (str);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  if (self->priv->machines) {
    GST_DEBUG ("%d missing machines", g_list_length (self->priv->machines));
    make_listview (vbox, self->priv->machines,
        _("The machines listed below are missing or failed to load."));
  }
  if (self->priv->waves) {
    GST_DEBUG ("%d missing core elements", g_list_length (self->priv->waves));
    make_listview (vbox, self->priv->waves,
        _("The waves listed below are missing or failed to load."));
  }
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))),
      hbox, TRUE, TRUE, 0);
}

//-- constructor methods

/**
 * bt_missing_song_elements_dialog_new:
 * @machines: list of missing machine elements
 * @waves: list of missing wave files
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtMissingSongElementsDialog *
bt_missing_song_elements_dialog_new (GList * machines, GList * waves)
{
  BtMissingSongElementsDialog *self;

  self =
      BT_MISSING_SONG_ELEMENTS_DIALOG (g_object_new
      (BT_TYPE_MISSING_SONG_ELEMENTS_DIALOG, "machines", machines, "waves",
          waves, NULL));
  bt_missing_song_elements_dialog_init_ui (self);
  return self;
}

//-- methods

//-- wrapper

//-- class internals

static void
bt_missing_song_elements_dialog_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec)
{
  BtMissingSongElementsDialog *self = BT_MISSING_SONG_ELEMENTS_DIALOG (object);
  return_if_disposed ();
  switch (property_id) {
    case MISSING_SONG_ELEMENTS_DIALOG_MACHINES:
      self->priv->machines = g_value_get_pointer (value);
      break;
    case MISSING_SONG_ELEMENTS_DIALOG_WAVES:
      self->priv->waves = g_value_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_missing_song_elements_dialog_dispose (GObject * object)
{
  BtMissingSongElementsDialog *self = BT_MISSING_SONG_ELEMENTS_DIALOG (object);
  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  g_object_unref (self->priv->app);

  G_OBJECT_CLASS (bt_missing_song_elements_dialog_parent_class)->dispose
      (object);
}

static void
bt_missing_song_elements_dialog_init (BtMissingSongElementsDialog * self)
{
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, BT_TYPE_MISSING_SONG_ELEMENTS_DIALOG,
      BtMissingSongElementsDialogPrivate);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();
}

static void
bt_missing_song_elements_dialog_class_init (BtMissingSongElementsDialogClass *
    klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BtMissingSongElementsDialogPrivate));

  gobject_class->set_property = bt_missing_song_elements_dialog_set_property;
  gobject_class->dispose = bt_missing_song_elements_dialog_dispose;

  g_object_class_install_property (gobject_class,
      MISSING_SONG_ELEMENTS_DIALOG_MACHINES, g_param_spec_pointer ("machines",
          "machines construct prop",
          "Set missing machines list, the dialog handles",
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      MISSING_SONG_ELEMENTS_DIALOG_WAVES, g_param_spec_pointer ("waves",
          "waves construct prop", "Set missing waves list, the dialog handles",
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));
}
