/* GNOME Volume Applet
 * Copyright (C) 2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *               2008 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * SECTION:btvolumepopup
 * @short_description: volume popup widget
 *
 * Shows a popup window containing a vertical slider
 */
#define BT_EDIT
#define VOLUME_POPUP_C

#include "bt-edit.h"
#include <glib/gprintf.h>

//-- the class

struct _BtVolumePopup {
  GtkPopover parent;

  /* us */
  GtkRange *scale;
  GtkButton *plus, *minus;

  /* timeout for buttons */
  guint timeout;
  /* for +/- buttons */
  gint direction;
};


G_DEFINE_TYPE (BtVolumePopup, bt_volume_popup, GTK_TYPE_POPOVER);

//-- event handler

static void
cb_scale_changed (GtkRange * range, gpointer user_data)
{
  GtkLabel *label = GTK_LABEL (user_data);
  gchar str[6];

  // FIXME(ensonic): workaround for https://bugzilla.gnome.org/show_bug.cgi?id=667598
  g_snprintf (str, sizeof(str), "%3d %%", 400 - (gint) (gtk_range_get_value (range)));
  gtk_label_set_text (label, str);
}

//-- helper methods

//-- constructor methods

/**
 * bt_volume_popup_new:
 * @adj: the adjustment for the popup
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
GtkWidget *
bt_volume_popup_new (GtkAdjustment * adj)
{
  GtkWidget *box, *scale, *frame, *label;
  BtVolumePopup *self = g_object_new (BT_TYPE_VOLUME_POPUP,
      "can-focus", TRUE,
      NULL);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_popover_set_child (GTK_POPOVER (self), box);

  label = gtk_label_new ("");
  gtk_box_prepend (GTK_BOX (box), label);
  gtk_box_prepend (GTK_BOX (box),
      gtk_separator_new (GTK_ORIENTATION_HORIZONTAL));

  scale = gtk_scale_new (GTK_ORIENTATION_VERTICAL, adj);
  self->scale = GTK_RANGE (scale);
  gtk_widget_set_size_request (scale, -1, 200);
  // FIXME(ensonic): workaround for https://bugzilla.gnome.org/show_bug.cgi?id=667598
  //gtk_range_set_inverted(self->scale, TRUE);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
#if 0
  gtk_scale_add_mark (GTK_SCALE (scale), 0.0, GTK_POS_LEFT,
      "<small>0 %</small>");
  gtk_scale_add_mark (GTK_SCALE (scale), 25.0, GTK_POS_LEFT, NULL);
  gtk_scale_add_mark (GTK_SCALE (scale), 50.0, GTK_POS_LEFT, NULL);
  gtk_scale_add_mark (GTK_SCALE (scale), 75.0, GTK_POS_LEFT, NULL);
  gtk_scale_add_mark (GTK_SCALE (scale), 100.0, GTK_POS_LEFT,
      "<small>100 %</small>");
  gtk_scale_add_mark (GTK_SCALE (scale), 150.0, GTK_POS_LEFT, NULL);
  gtk_scale_add_mark (GTK_SCALE (scale), 200.0, GTK_POS_LEFT,
      "<small>200 %</small>");
  gtk_scale_add_mark (GTK_SCALE (scale), 250.0, GTK_POS_LEFT, NULL);
  gtk_scale_add_mark (GTK_SCALE (scale), 300.0, GTK_POS_LEFT,
      "<small>300 %</small>");
  gtk_scale_add_mark (GTK_SCALE (scale), 350.0, GTK_POS_LEFT, NULL);
  gtk_scale_add_mark (GTK_SCALE (scale), 400.0, GTK_POS_LEFT,
      "<small>400 %</small>");
#else
  gtk_scale_add_mark (GTK_SCALE (scale), 400.0 - 0.0, GTK_POS_LEFT, NULL);
  gtk_scale_add_mark (GTK_SCALE (scale), 400.0 - 25.0, GTK_POS_LEFT, NULL);
  gtk_scale_add_mark (GTK_SCALE (scale), 400.0 - 50.0, GTK_POS_LEFT, NULL);
  gtk_scale_add_mark (GTK_SCALE (scale), 400.0 - 75.0, GTK_POS_LEFT, NULL);
  gtk_scale_add_mark (GTK_SCALE (scale), 400.0 - 100.0, GTK_POS_LEFT,
      "<small>100 %</small>");
  gtk_scale_add_mark (GTK_SCALE (scale), 400.0 - 150.0, GTK_POS_LEFT, NULL);
  gtk_scale_add_mark (GTK_SCALE (scale), 400.0 - 200.0, GTK_POS_LEFT,
      "<small>200 %</small>");
  gtk_scale_add_mark (GTK_SCALE (scale), 400.0 - 250.0, GTK_POS_LEFT, NULL);
  gtk_scale_add_mark (GTK_SCALE (scale), 400.0 - 300.0, GTK_POS_LEFT,
      "<small>300 %</small>");
  gtk_scale_add_mark (GTK_SCALE (scale), 400.0 - 350.0, GTK_POS_LEFT, NULL);
  gtk_scale_add_mark (GTK_SCALE (scale), 400.0 - 400.0, GTK_POS_LEFT,
      "<small>400 %</small>");
#endif

  g_signal_connect (self->scale, "value-changed", G_CALLBACK (cb_scale_changed),
      label);
  cb_scale_changed (self->scale, label);
  gtk_box_prepend (GTK_BOX (box), scale);

  return GTK_WIDGET (self);
}

//-- methods

/**
 * bt_volume_popup_show:
 * @self: the popup widget
 *
 * Show and activate the widget
 */
void
bt_volume_popup_show (BtVolumePopup * self, GtkWidget* parent)
{
  gtk_widget_set_parent (GTK_WIDGET (self), parent);
  gtk_popover_popup (GTK_POPOVER (self));

  /* grab focus */
  gtk_widget_grab_focus_savely (GTK_WIDGET (self));
}

/**
 * bt_volume_popup_hide:
 * @self: the popup widget
 *
 * Hide and deactivate the widget
 */
void
bt_volume_popup_hide (BtVolumePopup * self)
{
  /* ungrab focus */
  gtk_popover_popdown (GTK_POPOVER (self));
}

//-- wrapper

//-- class internals

static void
bt_volume_popup_dispose (GObject * object)
{
  BtVolumePopup *popup = BT_VOLUME_POPUP (object);

  if (popup->timeout) {
    g_source_remove (popup->timeout);
    popup->timeout = 0;
  }

  G_OBJECT_CLASS (bt_volume_popup_parent_class)->dispose (object);
}

static void
bt_volume_popup_class_init (BtVolumePopupClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = bt_volume_popup_dispose;

  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BOX_LAYOUT);
}

static void
bt_volume_popup_init (BtVolumePopup * popup)
{
  popup->timeout = 0;
}
