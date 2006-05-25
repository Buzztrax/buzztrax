// $Id: volume-popup.c,v 1.3 2006-05-25 16:29:21 ensonic Exp $
/* GNOME Volume Applet
 * Copyright (C) 2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * popup.c: floating window containing volume widgets
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

#define BT_EDIT
#define BT_WIRE_CANVAS_ITEM_C

#include "bt-edit.h"

static GtkWindowClass *parent_class = NULL;

//-- event handler

/*
 * React if user presses +/- buttons.
 */
static gboolean
cb_timeout(gpointer data)
{
  BtVolumePopup *popup = data;
  GtkAdjustment *adj;
  gfloat volume;
  gboolean res = TRUE;

  if (!popup->timeout)
    return FALSE;

  adj = gtk_range_get_adjustment(popup->scale);
  volume = gtk_range_get_value(popup->scale);
  volume += popup->direction * adj->step_increment;

  if (volume <= adj->lower) {
    volume = adj->lower;
    res = FALSE;
  } else if (volume >= adj->upper) {
    volume = adj->upper;
    res = FALSE;
  }
  gtk_range_set_value(popup->scale, volume);

  if (!res)
    popup->timeout = 0;

  return res;
}

static gboolean
cb_button_press(GtkWidget *widget, GdkEventButton *button, gpointer data)
{
  BtVolumePopup *popup = data;

  popup->direction = (GTK_BUTTON(widget) == popup->plus) ? 1 : -1;
  if (popup->timeout)
    g_source_remove(popup->timeout);
  popup->timeout = g_timeout_add(100, cb_timeout, data);
  cb_timeout(data);

  return TRUE;
}

static gboolean
cb_button_release(GtkWidget *widget, GdkEventButton *button, gpointer data)
{
  BtVolumePopup *popup = data;

  if (popup->timeout) {
    g_source_remove(popup->timeout);
    popup->timeout = 0;
  }

  return TRUE;
}

/*
 * hide popup when clicking outside
 */
static gboolean
cb_dock_press (GtkWidget * widget, GdkEventButton * event, gpointer data)
{
  BtVolumePopup *self = BT_VOLUME_POPUP(data);

  if (event->type == GDK_BUTTON_PRESS) {
    /*
    GdkEventButton *e;
    gboolean retval;
    GtkWidget *parent=GTK_WIDGET(gtk_window_get_transient_for(GTK_WINDOW(self)));
    //GtkWidget *parent=gtk_widget_get_parent(GTK_WIDGET(self));
    
    GST_INFO("FORWARD : popup=%p, widget=%p, parent=%p", self, widget, parent);
    */

    bt_volume_popup_hide(self);

    /* I can't get this to work, which way ever I do it, the main_window never
     * sees the event
     *
    // forward event
    e = (GdkEventButton *) gdk_event_copy ((GdkEvent *) event);
    //e->window = widget->window;
    e->window = parent->window;
    //e->window = self->parent_widget->window;
    e->type = GDK_BUTTON_PRESS;
    //gtk_widget_event (widget, (GdkEvent *) e);
    retval=gtk_widget_event (parent, (GdkEvent *) e);
    GST_INFO("  result =%d", retval);
    //gtk_widget_event (self->parent_widget, (GdkEvent *) e);
    gtk_main_do_event ((GdkEvent *) e);
    //g_signal_emit_by_name(self->parent_widget, "event", 0, &retval, e);
    //g_signal_emit_by_name(parent, "event", 0, &retval, e);
    //GST_INFO("  result =%d", retval);
    e->window = event->window;
    gdk_event_free ((GdkEvent *) e);
    */

    return TRUE;
  }
  return FALSE;
}

//-- helper methods

//-- constructor methods

/**
 * bt_volume_popup_new:
 * @adj: the adjustment for the popup
 * @parent: parent for event-forwarding when poping down
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
GtkWidget *
bt_volume_popup_new(GtkAdjustment *adj, GtkWidget *parent) {
  GtkWidget *table, *button, *scale, *frame;
  BtVolumePopup *self;

  self = g_object_new(BT_TYPE_VOLUME_POPUP, "type", GTK_WINDOW_POPUP, NULL);
  self->parent_widget = parent;

  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);

  table = gtk_table_new(1,3, FALSE);

  button = gtk_button_new_with_label(_("+"));
  self->plus = GTK_BUTTON(button);
  gtk_button_set_relief(self->plus, GTK_RELIEF_NONE);
  gtk_table_attach_defaults(GTK_TABLE(table), button, 0,1, 0,1);
  g_signal_connect(button, "button-press-event", G_CALLBACK(cb_button_press), self);
  g_signal_connect(button, "button-release-event", G_CALLBACK(cb_button_release), self);
  gtk_widget_show(button);

  // @todo: add ruler ? */
  scale = gtk_vscale_new(adj);
  self->scale = GTK_RANGE(scale);
  gtk_widget_set_size_request(scale, -1,100);
  gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
  gtk_range_set_inverted(self->scale, TRUE);
  gtk_table_attach_defaults(GTK_TABLE(table), scale, 0,1, 1,2);
  gtk_widget_show(scale);

  button = gtk_button_new_with_label(_("-"));
  self->minus = GTK_BUTTON(button);
  gtk_button_set_relief(self->minus, GTK_RELIEF_NONE);
  gtk_table_attach_defaults(GTK_TABLE(table), button, 0,1, 2,3);
  g_signal_connect(button, "button-press-event", G_CALLBACK(cb_button_press), self);
  g_signal_connect(button, "button-release-event", G_CALLBACK(cb_button_release), self);
  gtk_widget_show(button);

  gtk_container_add(GTK_CONTAINER(frame), table);
  gtk_widget_show(table);

  gtk_container_add(GTK_CONTAINER(self), frame);
  gtk_widget_show(frame);
  
  g_signal_connect(self, "button-press-event", G_CALLBACK (cb_dock_press), self);

  return GTK_WIDGET(self);
}

//-- methods

/**
 * bt_volume_popup_show:
 * @self: the popup widget
 *
 * Show and activate the widget
 */
void bt_volume_popup_show(BtVolumePopup *self) {
  gtk_widget_show_all(GTK_WIDGET(self));
  //gtk_window_set_focus(GTK_WINDOW(popup),GTK_WIDGET(self->priv->vol_popup->scale));

  /* grab focus */
  gtk_widget_grab_focus(GTK_WIDGET(self));
  gtk_grab_add(GTK_WIDGET(self));
  gdk_pointer_grab(GTK_WIDGET(self)->window, TRUE,
        GDK_BUTTON_PRESS_MASK |
        GDK_BUTTON_RELEASE_MASK |
        GDK_POINTER_MOTION_MASK,
        NULL, NULL, GDK_CURRENT_TIME);
  gdk_keyboard_grab(GTK_WIDGET(self)->window, TRUE, GDK_CURRENT_TIME);
}

/**
 * bt_volume_popup_hide:
 * @self: the popup widget
 *
 * Hide and deactivate the widget
 */
void bt_volume_popup_hide(BtVolumePopup *self) {
  /* ungrab focus */
  gdk_keyboard_ungrab (GDK_CURRENT_TIME);
  gdk_pointer_ungrab (GDK_CURRENT_TIME);
  gtk_grab_remove (GTK_WIDGET(self));

  gtk_widget_hide_all(GTK_WIDGET(self));
}

//-- wrapper

//-- class internals

static void
bt_volume_popup_dispose(GObject *object)
{
  BtVolumePopup *popup = BT_VOLUME_POPUP(object);

  //bt_volume_popup_change (popup, NULL);

  if (popup->timeout) {
    g_source_remove(popup->timeout);
    popup->timeout = 0;
  }

  G_OBJECT_CLASS(parent_class)->dispose (object);
}

static void
bt_volume_popup_class_init(BtVolumePopupClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->dispose = bt_volume_popup_dispose;
}

static void
bt_volume_popup_init(BtVolumePopup *popup)
{
  popup->timeout = 0;
}

GType
bt_volume_popup_get_type(void)
{
  static GType type = 0;

  if (!type) {
    static const GTypeInfo info = {
      sizeof (BtVolumePopupClass),
      NULL,
      NULL,
      (GClassInitFunc) bt_volume_popup_class_init,
      NULL,
      NULL,
      sizeof (BtVolumePopup),
      0,
      (GInstanceInitFunc) bt_volume_popup_init,
      NULL
    };
    type = g_type_register_static(GTK_TYPE_WINDOW, "BtVolumePopup", &info, 0);
  }
  return type;
}
