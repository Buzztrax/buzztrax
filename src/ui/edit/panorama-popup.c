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
 * SECTION:btpanoramapopup
 * @short_description: panorama popup widget
 *
 * Shows a popup window containing a horizontal slider
 */
/* @bug: http://bugzilla.gnome.org/show_bug.cgi?id=511470
 * level of range widgets are draw always from left-to-right for some themes.
 */

#define BT_EDIT
#define PANORAMA_POPUP_C

#include "bt-edit.h"
#include <glib/gprintf.h>

//-- the class

struct _BtPanoramaPopup {
  GtkPopover parent;

  /* us */
  GtkRange *scale;
  GtkButton *plus, *minus;

  /* timeout for buttons */
  guint timeout;
  /* for +/- buttons */
  gint direction;
};

G_DEFINE_TYPE (BtPanoramaPopup, bt_panorama_popup, GTK_TYPE_POPOVER);

//-- event handler

static void
cb_scale_changed (GtkRange * range, gpointer user_data)
{
  GtkLabel *label = GTK_LABEL (user_data);
  gchar str[6];

  g_snprintf (str, sizeof(str), "%3d %%", (gint) (gtk_range_get_value (range)));
  gtk_label_set_text (label, str);
}

//-- helper methods

//-- constructor methods

/**
 * bt_panorama_popup_new:
 * @adj: the adjustment for the popup
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
GtkWidget *
bt_panorama_popup_new (GtkAdjustment * adj)
{
  GtkWidget *box, *scale, *frame, *label;
  BtPanoramaPopup *self = g_object_new (BT_TYPE_PANORAMA_POPUP,
      "can-focus", TRUE,
      NULL);

  frame = gtk_frame_new (NULL);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

  label = gtk_label_new ("");
  gtk_widget_set_size_request (label, 40, -1);
  gtk_box_append (GTK_BOX (box), label);
  gtk_box_append (GTK_BOX (box),
      gtk_separator_new (GTK_ORIENTATION_VERTICAL));

  scale = gtk_scale_new (GTK_ORIENTATION_HORIZONTAL, adj);
  self->scale = GTK_RANGE (scale);
  gtk_widget_set_size_request (scale, 200, -1);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gtk_scale_add_mark (GTK_SCALE (scale), -100.0, GTK_POS_BOTTOM,
      "<small>100 %</small>");
  gtk_scale_add_mark (GTK_SCALE (scale), -75.0, GTK_POS_BOTTOM, NULL);
  gtk_scale_add_mark (GTK_SCALE (scale), -50.0, GTK_POS_BOTTOM,
      "<small>50 %</small>");
  gtk_scale_add_mark (GTK_SCALE (scale), -25.0, GTK_POS_BOTTOM, NULL);
  gtk_scale_add_mark (GTK_SCALE (scale), 0.0, GTK_POS_BOTTOM,
      "<small>0 %</small>");
  gtk_scale_add_mark (GTK_SCALE (scale), 25.0, GTK_POS_BOTTOM, NULL);
  gtk_scale_add_mark (GTK_SCALE (scale), 50.0, GTK_POS_BOTTOM,
      "<small>50 %</small>");
  gtk_scale_add_mark (GTK_SCALE (scale), 75.0, GTK_POS_BOTTOM, NULL);
  gtk_scale_add_mark (GTK_SCALE (scale), 100.0, GTK_POS_BOTTOM,
      "<small>100 %</small>");

  g_signal_connect_object (self->scale, "value-changed", G_CALLBACK (cb_scale_changed),
      label, 0);
  cb_scale_changed (self->scale, label);
  gtk_box_append (GTK_BOX (box), scale);

  gtk_frame_set_child (GTK_FRAME (frame), box);
  gtk_popover_set_child (GTK_POPOVER (self), frame);

  return GTK_WIDGET (self);
}

//-- methods

/**
 * bt_panorama_popup_show:
 * @self: the popup widget
 *
 * Show and activate the widget
 */
void
bt_panorama_popup_show (BtPanoramaPopup * self)
{
  gtk_popover_popup (GTK_POPOVER (self));
}

/**
 * bt_panorama_popup_hide:
 * @self: the popup widget
 *
 * Hide and deactivate the widget
 */
void
bt_panorama_popup_hide (BtPanoramaPopup * self)
{
  gtk_popover_popdown (GTK_POPOVER (self));
}

//-- wrapper

//-- class internals

static void
bt_panorama_popup_dispose (GObject * object)
{
  BtPanoramaPopup *popup = BT_PANORAMA_POPUP (object);

  if (popup->timeout) {
    g_source_remove (popup->timeout);
    popup->timeout = 0;
  }

  G_OBJECT_CLASS (bt_panorama_popup_parent_class)->dispose (object);
}

static void
bt_panorama_popup_class_init (BtPanoramaPopupClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = bt_panorama_popup_dispose;
}

static void
bt_panorama_popup_init (BtPanoramaPopup * popup)
{
  popup->timeout = 0;
}
