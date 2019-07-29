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

G_DEFINE_TYPE (BtVolumePopup, bt_volume_popup, GTK_TYPE_WINDOW);

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

/*
 * hide popup when clicking outside
 */
static gboolean
cb_dock_press (GtkWidget * widget, GdkEventButton * event, gpointer data)
{
  BtVolumePopup *self = BT_VOLUME_POPUP (data);

  //if(!gtk_widget_get_realized(GTK_WIDGET(self)) return FALSE;

  if (event->type == GDK_BUTTON_PRESS) {
    GdkEventButton *e;
    //GST_INFO("type=%4d, window=%p, send_event=%3d, time=%8d",event->type,event->window,event->send_event,event->time);
    //GST_INFO("x=%6.4lf, y=%6.4lf, axes=%p, state=%4d",event->x,event->y,event->axes,event->state);
    //GST_INFO("button=%4d, device=%p, x_root=%6.4lf, y_root=%6.4lf\n",event->button,event->device,event->x_root,event->y_root);

    /*
       GtkWidget *parent=GTK_WIDGET(gtk_window_get_transient_for(GTK_WINDOW(self)));
       //GtkWidget *parent=gtk_widget_get_parent(GTK_WIDGET(self));
       //gboolean retval;

       GST_INFO("FORWARD : popup=%p, widget=%p", self, widget);
       GST_INFO("FORWARD : parent=%p, parent->window=%p", parent, parent->window);
     */

    bt_volume_popup_hide (self);

    // forward event
    e = (GdkEventButton *) gdk_event_copy ((GdkEvent *) event);
    //GST_INFO("type=%4d, window=%p, send_event=%3d, time=%8d",e->type,e->window,e->send_event,e->time);
    //GST_INFO("x=%6.4lf, y=%6.4lf, axes=%p, state=%4d",e->x,e->y,e->axes,e->state);
    //GST_INFO("button=%4d, device=%p, x_root=%6.4lf, y_root=%6.4lf\n",e->button,e->device,e->x_root,e->y_root);
    //e->window = widget->window;
    //e->window = parent->window;
    //e->type = GDK_BUTTON_PRESS;

    gtk_main_do_event ((GdkEvent *) e);
    //retval=gtk_widget_event (widget, (GdkEvent *) e);
    //retval=gtk_widget_event (parent, (GdkEvent *) e);
    //retval=gtk_widget_event (self->parent_widget, (GdkEvent *) e);
    //GST_INFO("  result =%d", retval);
    //g_signal_emit_by_name(self->parent_widget, "event", 0, &retval, e);
    //g_signal_emit_by_name(parent, "event", 0, &retval, e);
    //GST_INFO("  result =%d", retval);
    //e->window = event->window;
    gdk_event_free ((GdkEvent *) e);

    return TRUE;
  }
  return FALSE;
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
      "type", GTK_WINDOW_POPUP,
      NULL);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

  label = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box),
      gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 0);

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
  gtk_box_pack_start (GTK_BOX (box), scale, TRUE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (frame), box);
  gtk_container_add (GTK_CONTAINER (self), frame);
  gtk_widget_show_all (frame);

  g_signal_connect (self, "button-press-event", G_CALLBACK (cb_dock_press),
      self);

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
bt_volume_popup_show (BtVolumePopup * self)
{
  gtk_widget_show_all (GTK_WIDGET (self));

  /* grab focus */
  gtk_widget_grab_focus_savely (GTK_WIDGET (self));
  gtk_grab_add (GTK_WIDGET (self));
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
  gtk_grab_remove (GTK_WIDGET (self));

  gtk_widget_hide (GTK_WIDGET (self));
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
}

static void
bt_volume_popup_init (BtVolumePopup * popup)
{
  popup->timeout = 0;
}
