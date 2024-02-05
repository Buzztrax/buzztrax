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

//-- property ids

enum
{
  MISSING_SONG_ELEMENTS_DIALOG_MACHINES = 1,
  MISSING_SONG_ELEMENTS_DIALOG_WAVES
};

struct _BtMissingSongElementsDialog
{
  AdwMessageDialog parent;
  
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* list of missing elements */
  GList *machines, *waves;

  GtkBox *vbox;
};

//-- the class

G_DEFINE_TYPE (BtMissingSongElementsDialog, bt_missing_song_elements_dialog,
    ADW_TYPE_MESSAGE_DIALOG);


//-- event handler

//-- helper methods

static void
make_listview (GtkBox * vbox, GList * missing_elements, const gchar * msg)
{
  GtkWidget *label, *missing_list, *missing_list_view;
  gchar *missing_text;

  label = gtk_label_new (msg);
  g_object_set (label, "xalign", 0.0, NULL);
  gtk_box_append (vbox, label);

  missing_text = bt_strjoin_list (missing_elements);

  missing_list = gtk_text_view_new ();
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (missing_list), FALSE);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (missing_list), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (missing_list), GTK_WRAP_WORD);
  gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW
          (missing_list)), missing_text, -1);
  g_free (missing_text);

  missing_list_view = gtk_scrolled_window_new ();
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (missing_list_view),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_parent (missing_list, missing_list_view);

  gtk_box_append (vbox, missing_list_view);
}

static void
bt_missing_song_elements_dialog_init_ui (const BtMissingSongElementsDialog *
    self)
{
  gtk_window_set_title (GTK_WINDOW (self), _("Missing elements in song"));

  if (self->machines) {
    GST_DEBUG ("%d missing machines", g_list_length (self->machines));
    make_listview (self->vbox, self->machines,
        _("The machines listed below are missing or failed to load."));
  }
  if (self->waves) {
    GST_DEBUG ("%d missing core elements", g_list_length (self->waves));
    make_listview (self->vbox, self->waves,
        _("The waves listed below are missing or failed to load."));
  }
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
  return_if_disposed_self ();
  switch (property_id) {
    case MISSING_SONG_ELEMENTS_DIALOG_MACHINES:
      self->machines = g_value_get_pointer (value);
      break;
    case MISSING_SONG_ELEMENTS_DIALOG_WAVES:
      self->waves = g_value_get_pointer (value);
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
  return_if_disposed_self ();
  self->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  g_object_unref (self->app);

  gtk_widget_dispose_template (GTK_WIDGET (self), BT_TYPE_MISSING_SONG_ELEMENTS_DIALOG);
  
  G_OBJECT_CLASS (bt_missing_song_elements_dialog_parent_class)->dispose
      (object);
}

static void
bt_missing_song_elements_dialog_init (BtMissingSongElementsDialog * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  
  GST_DEBUG ("!!!! self=%p", self);
  self->app = bt_edit_application_new ();
}

static void
bt_missing_song_elements_dialog_class_init (BtMissingSongElementsDialogClass *
    klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

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

  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  
  gtk_widget_class_set_template_from_resource (widget_class,
      "/org/buzztrax/ui/missing-song-elements-dialog.ui");

  gtk_widget_class_bind_template_child (widget_class, BtMissingSongElementsDialog, vbox);
}
