/* $Id$
 *
 * GNOME Volume Applet
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
/**
 * SECTION:btpanoramapopup
 * @short_description: panorama popup widget
 *
 * Shows a popup window containing a horizontal slider
 */
/*
 * level of range widgets are draw always from left-to-right for some themes.
 * http://bugzilla.gnome.org/show_bug.cgi?id=511470
 */

#define BT_EDIT
#define PANORAMA_POPUP_C

#include "bt-edit.h"

static GtkWindowClass *parent_class = NULL;

//-- event handler

static void
cb_scale_changed(GtkRange *range, gpointer  user_data)
{
  GtkLabel *label=GTK_LABEL(user_data);
  gchar str[6];

  g_sprintf(str,"%3d %%",(gint)(100.0*gtk_range_get_value(range)));
  gtk_label_set_text(label,str);
}

/*
 * hide popup when clicking outside
 */
static gboolean
cb_dock_press (GtkWidget * widget, GdkEventButton * event, gpointer data)
{
  BtPanoramaPopup *self = BT_PANORAMA_POPUP(data);

  //if(!GTK_WIDGET_REALIZED(self)) return FALSE;

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

    bt_panorama_popup_hide(self);

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
 * bt_panorama_popup_new:
 * @adj: the adjustment for the popup
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
GtkWidget *
bt_panorama_popup_new(GtkAdjustment *adj) {
  GtkWidget *table, *scale, *frame,*ruler, *label;
  BtPanoramaPopup *self;

  self = g_object_new(BT_TYPE_PANORAMA_POPUP, "type", GTK_WINDOW_POPUP, NULL);

  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);

  table = gtk_table_new(2,2, FALSE);


  label=gtk_label_new("");
  gtk_widget_set_size_request(label, 40, -1);
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0,1, 0,2);

  scale=gtk_hscale_new(adj);
  self->scale=GTK_RANGE(scale);
  gtk_widget_set_size_request(scale, 200, -1);
  gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
  gtk_range_set_inverted(self->scale, FALSE);
  g_signal_connect(self->scale, "value-changed", G_CALLBACK(cb_scale_changed), label);
  cb_scale_changed(self->scale,label);
  gtk_table_attach_defaults(GTK_TABLE(table), scale, 1,2, 1,2);


  // add ruler
  ruler=gtk_hruler_new();
  /* we use -X instead of 0.0 because of:
   * http://bugzilla.gnome.org/show_bug.cgi?id=465041
   * @todo: take slider knob size into account
   * gtk_widget_style_get(scale,"slider-length",slider_length,NULL);
   */
  gtk_ruler_set_range(GTK_RULER(ruler),-120.0,120.0,000.0,30.0);
  gtk_widget_set_size_request(ruler,-1,30);
  GTK_RULER_GET_CLASS(ruler)->draw_pos = NULL;
  gtk_table_attach_defaults(GTK_TABLE(table), ruler, 1,2, 0,1);


  gtk_container_add(GTK_CONTAINER(frame), table);

  gtk_container_add(GTK_CONTAINER(self), frame);
  gtk_widget_show_all(frame);

  g_signal_connect(self, "button-press-event", G_CALLBACK (cb_dock_press), self);

  return GTK_WIDGET(self);
}

//-- methods

/**
 * bt_panorama_popup_show:
 * @self: the popup widget
 *
 * Show and activate the widget
 */
void bt_panorama_popup_show(BtPanoramaPopup *self) {
  GdkWindow *window = gtk_widget_get_window(GTK_WIDGET(self));

  gtk_widget_show_all(GTK_WIDGET(self));

  /* grab focus */
  gtk_widget_grab_focus_savely(GTK_WIDGET(self));
  gtk_grab_add(GTK_WIDGET(self));
  gdk_pointer_grab(window, TRUE,
        GDK_BUTTON_PRESS_MASK |
        GDK_BUTTON_RELEASE_MASK |
        GDK_POINTER_MOTION_MASK,
        NULL, NULL, GDK_CURRENT_TIME);
  gdk_keyboard_grab(window, TRUE, GDK_CURRENT_TIME);
}

/**
 * bt_panorama_popup_hide:
 * @self: the popup widget
 *
 * Hide and deactivate the widget
 */
void bt_panorama_popup_hide(BtPanoramaPopup *self) {
  /* ungrab focus */
  gdk_keyboard_ungrab(GDK_CURRENT_TIME);
  gdk_pointer_ungrab(GDK_CURRENT_TIME);
  gtk_grab_remove(GTK_WIDGET(self));

  gtk_widget_hide_all(GTK_WIDGET(self));
}

//-- wrapper

//-- class internals

static void
bt_panorama_popup_dispose(GObject *object)
{
  BtPanoramaPopup *popup = BT_PANORAMA_POPUP(object);

  //bt_panorama_popup_change (popup, NULL);

  if (popup->timeout) {
    g_source_remove(popup->timeout);
    popup->timeout = 0;
  }

  G_OBJECT_CLASS(parent_class)->dispose (object);
}

static void
bt_panorama_popup_class_init(BtPanoramaPopupClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->dispose = bt_panorama_popup_dispose;
}

static void
bt_panorama_popup_init(BtPanoramaPopup *popup)
{
  popup->timeout = 0;
}

GType
bt_panorama_popup_get_type(void)
{
  static GType type = 0;

  if (!type) {
    const GTypeInfo info = {
      sizeof (BtPanoramaPopupClass),
      NULL,
      NULL,
      (GClassInitFunc) bt_panorama_popup_class_init,
      NULL,
      NULL,
      sizeof (BtPanoramaPopup),
      0,
      (GInstanceInitFunc) bt_panorama_popup_init,
      NULL
    };
    type = g_type_register_static(GTK_TYPE_WINDOW, "BtPanoramaPopup", &info, 0);
  }
  return type;
}
