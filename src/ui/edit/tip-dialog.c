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
 * SECTION:bttipdialog
 * @short_description: class for the editor tip-of-the-day dialog
 *
 * Show a tip, that has not yet been shown to the user.
 */
/*
Algorithm:
- max_tips = G_N_ELEMENTS(tips)
- fill gint pending_tips[max_tips] with 0...(max_tips-1)
- read presented-tips from settings
- parse and set pending_tips[i]=-1; (only if i<max_tips)
- loop over pending_tips[] and set pending_tips_compressed[j]=i
  while skipping pending_tips[i]==-1;
- take a random number r < j
- show that tip and set pending_tips[j]=-1;
- loop over pending_tips[] and build new presented-tips string
  to be stored in settings
*/
/* IDEA(ensonic): have a links to related help topics
 * IDEA(ensonic): make this less annoying:
 *   - an info-banner that has an auto-hide
 *   - show as the default status-bar text after startup (with a tip-icon)
 *   - a tips page in the docs
 */

#define BT_EDIT
#define BT_TIP_DIALOG_C

#include "bt-edit.h"
#include <glib/gprintf.h>

static gchar *tips[] = {
  N_("New machines are added in machine view from the context menu."),
  N_("Connect machines by holding the shift key and dragging a connection for the source to the target machine."),
  N_("Songs can be recorded as single waves per track to give it to remixers."),
  N_("Fill the details on the info page. When recording songs, the metadata is added to the recording as tags."),
  N_("Use jackaudio sink in audio device settings to get lower latencies for live machine control."),
  N_("You can use input devices such as joysticks, beside midi devices to live control machine parameters."),
  N_("You can use a upnp media client (e.g. media streamer on nokia tablets) to remote control buzztrax."),
  N_("To enter notes, imagine your pc keyboard as a music keyboard in two rows. Bottom left y/z key becomes a 'c', s a 'c#', x a 'd' and so on."),
  N_("You can get more help from the community on irc://irc.freenode.net/#buzztrax."),
  N_("Pattern layouts are individual for each machine. Look at the statusbar at the bottom for information about the cursor-column."),
  N_("Click the speaker icon in the pattern-view to hear notes as you enter them."),
  N_("Each wire has volume and possible panorama/balance controls. These can also be adjusted in the machine-window of the machine that has the wire as an input."),
  N_("The wire volume control pops up upon a click on the arrow box on the wire."),
  N_("The wire panorama/balance control pops up upon a shift+click on the arrow box on the wire if available."),
  N_("You can copy settings from the machine window (from the context menu of a group) and paste them to patterns and the other way around."),
  N_("Install extra machines from http://github.com/Buzztrax/buzzmachines."),
  N_("Machines can also be renamed in the headers of the sequence view."),
  N_("Press the ',' (comma) key in the pattern editor to insert the current value for this parameter."),
  N_("You can use multiple tracks for a single machine. The patterns used in each track will be overlayed from left to right.")
};

struct _BtTipDialogPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  BtSettings *settings;
  GtkTextView *tip_view;

  /* tip history */
  gint tip_status[G_N_ELEMENTS (tips)];
  gint pending_tips[G_N_ELEMENTS (tips)];
  gint n_pending_tips;

  /* the application */
  BtEditApplication *app;
};

//-- the class

G_DEFINE_TYPE (BtTipDialog, bt_tip_dialog, GTK_TYPE_DIALOG);


//-- event handler

static void
on_refresh_clicked (GtkButton * button, gpointer user_data)
{
  BtTipDialog *self = BT_TIP_DIALOG (user_data);
  guint ix;
  gint i, j;

  if (!self->priv->n_pending_tips) {
    // reset shown tips
    self->priv->n_pending_tips = G_N_ELEMENTS (tips);
    for (i = 0; i < G_N_ELEMENTS (tips); i++) {
      self->priv->tip_status[i] = i;
      self->priv->pending_tips[i] = i;
    }
  }
  // get a tip index we haven't shown yet
  ix = self->priv->pending_tips[g_random_int_range (0,
          self->priv->n_pending_tips)];

  GST_DEBUG ("show [%d]", ix);
  self->priv->tip_status[ix] = -1;
  for (i = j = 0; i < G_N_ELEMENTS (tips); i++) {
    if (self->priv->tip_status[i] > -1)
      self->priv->pending_tips[j++] = i;
  }
  self->priv->n_pending_tips = j;
  gtk_text_buffer_set_text (gtk_text_view_get_buffer (self->priv->tip_view),
      tips[ix], -1);
}

static void
on_show_tips_toggled (GtkToggleButton * togglebutton, gpointer user_data)
{
  BtTipDialog *self = BT_TIP_DIALOG (user_data);

  g_object_set (self->priv->settings, "show-tips",
      gtk_toggle_button_get_active (togglebutton), NULL);
}

static void
on_tip_view_realize (GtkWidget * widget, gpointer user_data)
{
  GtkWidget *parent = gtk_widget_get_parent (widget);
  GtkRequisition requisition;
  gint height, max_height;

  gtk_widget_get_preferred_size (widget, NULL, &requisition);
  bt_gtk_workarea_size (NULL, &max_height);

  GST_DEBUG ("#### tip_view  size req %d x %d (max-height=%d)",
      requisition.width, requisition.height, max_height);

  height = MIN (requisition.height, max_height);
  // TODO(ensonic): is the '4' some border or padding
  gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (parent),
      height + 4);
}

//-- helper methods

static void
bt_tip_dialog_init_ui (const BtTipDialog * self)
{
  GtkWidget *label, *icon, *hbox, *vbox, *btn, *chk, *tip_view;
  gboolean show_tips;
  gchar *str;
  gint i, j;

  GST_DEBUG ("read settings");

  g_object_get (self->priv->app, "settings", &self->priv->settings, NULL);
  g_object_get (self->priv->settings, "show-tips", &show_tips, "presented-tips",
      &str, NULL);
  GST_DEBUG ("read [%s]", str);

  // parse str to update tip status
  for (i = 0; i < G_N_ELEMENTS (tips); i++) {
    self->priv->tip_status[i] = i;
  }
  if (str) {
    gint ix;
    gchar *p1, *p2;

    p1 = str;
    p2 = strchr (p1, ',');
    while (p2) {
      *p2 = '\0';
      ix = atoi (p1);
      if (ix < G_N_ELEMENTS (tips))
        self->priv->tip_status[ix] = -1;
      p1 = &p2[1];
      p2 = strchr (p1, ',');
    }
    ix = atoi (p1);
    if (ix < G_N_ELEMENTS (tips))
      self->priv->tip_status[ix] = -1;
    g_free (str);
  }
  for (i = j = 0; i < G_N_ELEMENTS (tips); i++) {
    if (self->priv->tip_status[i] > -1)
      self->priv->pending_tips[j++] = i;
  }
  self->priv->n_pending_tips = j;

  GST_DEBUG ("prepare tips dialog");

  gtk_widget_set_name (GTK_WIDGET (self), "Tip of the day");

  gtk_window_set_title (GTK_WINDOW (self), _("Tip of the day"));

  // add dialog commision widgets (okay)
  gtk_dialog_add_button (GTK_DIALOG (self), _("_OK"), GTK_RESPONSE_ACCEPT);

  gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_ACCEPT);

  // content area
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);

  icon =
      gtk_image_new_from_icon_name ("dialog-information", GTK_ICON_SIZE_DIALOG);
  gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, FALSE, 0);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  str = g_strdup_printf ("<big><b>%s</b></big>\n", _("Tip of the day"));
  label = g_object_new (GTK_TYPE_LABEL, "use-markup", TRUE, "label", str, NULL);
  g_free (str);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  self->priv->tip_view = GTK_TEXT_VIEW (gtk_text_view_new ());
  gtk_text_view_set_cursor_visible (self->priv->tip_view, FALSE);
  gtk_text_view_set_editable (self->priv->tip_view, FALSE);
  gtk_text_view_set_wrap_mode (self->priv->tip_view, GTK_WRAP_WORD);
  g_signal_connect (self->priv->tip_view, "realize",
      G_CALLBACK (on_tip_view_realize), (gpointer) self);

  tip_view = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (tip_view),
      GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (tip_view),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (tip_view),
      GTK_WIDGET (self->priv->tip_view));

  gtk_box_pack_start (GTK_BOX (vbox), tip_view, TRUE, TRUE, 0);

  chk = gtk_check_button_new_with_label (_("Show tips on startup"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (chk), show_tips);
  g_signal_connect (chk, "toggled", G_CALLBACK (on_show_tips_toggled),
      (gpointer) self);
  gtk_box_pack_start (GTK_BOX (vbox), chk, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (self))),
      hbox, TRUE, TRUE, 0);

  // add "refresh" button to action area
  btn = gtk_dialog_add_button (GTK_DIALOG (self), _("Refresh"),
      GTK_RESPONSE_NONE);
  g_signal_connect (btn, "clicked", G_CALLBACK (on_refresh_clicked),
      (gpointer) self);

  on_refresh_clicked (GTK_BUTTON (btn), (gpointer) self);
}

//-- constructor methods

/**
 * bt_tip_dialog_new:
 *
 * Create a new instance
 *
 * Returns: the new instance
 */
BtTipDialog *
bt_tip_dialog_new (void)
{
  BtTipDialog *self = BT_TIP_DIALOG (g_object_new (BT_TYPE_TIP_DIALOG, NULL));
  bt_tip_dialog_init_ui (self);
  return self;
}

//-- methods

//-- wrapper

//-- class internals

static void
bt_tip_dialog_dispose (GObject * object)
{
  BtTipDialog *self = BT_TIP_DIALOG (object);
  gchar *str;
  gint i, j, shown;

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  // update tip-status in settings
  shown = G_N_ELEMENTS (tips) - self->priv->n_pending_tips;
  gint len = 6 * shown;
  str = g_malloc (len);
  for (i = j = 0; i < G_N_ELEMENTS (tips); i++) {
    if (self->priv->tip_status[i] == -1) {
      j += g_snprintf (&str[j], len - j, "%d,", i);
    }
  }
  if (j) {
    str[j - 1] = '\0';
  }
  GST_DEBUG ("write [%s]", str);
  g_object_set (self->priv->settings, "presented-tips", str, NULL);
  g_free (str);

  GST_DEBUG ("!!!! self=%p", self);
  g_object_unref (self->priv->app);
  g_object_unref (self->priv->settings);

  G_OBJECT_CLASS (bt_tip_dialog_parent_class)->dispose (object);
}

static void
bt_tip_dialog_init (BtTipDialog * self)
{
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, BT_TYPE_TIP_DIALOG,
      BtTipDialogPrivate);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();
}

static void
bt_tip_dialog_class_init (BtTipDialogClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BtTipDialogPrivate));

  gobject_class->dispose = bt_tip_dialog_dispose;
}
