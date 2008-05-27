/* $Id: wave-viewer.c  $
 *
 * Buzztard
 * Copyright (C) 2006-2008 Buzztard team <buzztard-devel@lists.sf.net>
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
#define BT_MAIN_PAGE_WAVES_C

#include "bt-edit.h"

static gboolean
bt_waveform_viewer_expose (GtkWidget *widget, GdkEventExpose *event)
{
  int i, ch;
    
  BtWaveformViewer *self = BT_WAVEFORM_VIEWER(widget);
  int ox = widget->allocation.x + 1, oy = widget->allocation.y + 1;
  int sx = widget->allocation.width - 2, sy = widget->allocation.height - 2;
  
  cairo_t *c = gdk_cairo_create(GDK_DRAWABLE(widget->window));

  GdkColor sc = { 0, 0, 0, 0 };

  g_assert(BT_IS_WAVEFORM_VIEWER(widget));
  
  gdk_cairo_set_source_color(c, &sc);
  cairo_rectangle(c, ox, oy, sx, sy);
  cairo_fill(c);

  if (self->peaks) {
    GdkColor sc2 = { 0, 0, 65535, 65535 };
    GdkColor sc3 = { 0, 0, 24575/2, 24575/2 };
    cairo_set_line_join(c, CAIRO_LINE_JOIN_ROUND);
    cairo_set_line_width(c, 0.5);

    for (ch = 0; ch < self->channels; ch++)
    {
      int lsy = sy / self->channels;
      int loy = oy + sy * ch / self->channels;
      for (i = 0; i < 4 * sx; i++)
      {
        int imirror = i < 2 * sx ? i : 4 * sx - 1 - i;
        int item = imirror * self->peaks_size / (2 * sx);
        int sign = i < 2 * sx ? +1 : -1;
        double y = (loy + lsy / 2 - (lsy / 2 - 1) * sign * self->peaks[item * self->channels + ch]);
        if (y < loy) y = loy;
        if (y >= loy + lsy) y = loy + lsy - 1;
        if (i)
          cairo_line_to(c, ox + imirror * 0.5, y);
        else
          cairo_move_to(c, ox, y);
      }

      gdk_cairo_set_source_color(c, &sc3);
      cairo_fill_preserve(c);
      gdk_cairo_set_source_color(c, &sc2);
      cairo_stroke(c);
    }
  }
  
  cairo_destroy(c);
  
  gtk_paint_shadow(widget->style, widget->window, GTK_STATE_NORMAL, GTK_SHADOW_IN, NULL, widget, NULL, ox - 1, oy - 1, sx + 2, sy + 2);
  // printf("exposed %p %d+%d\n", widget->window, widget->allocation.x, widget->allocation.y);
  
  return TRUE;
}

static void
bt_waveform_viewer_size_request (GtkWidget *widget,
                           GtkRequisition *requisition)
{
    g_assert(BT_IS_WAVEFORM_VIEWER(widget));
    
    // CalfLineGraph *lg = bt_waveform_viewer(widget);
}

static void
bt_waveform_viewer_size_allocate (GtkWidget *widget,
                           GtkAllocation *allocation)
{
    g_assert(BT_IS_WAVEFORM_VIEWER(widget));
    
    widget->allocation = *allocation;
    // printf("allocation %d x %d\n", allocation->width, allocation->height);
}

static void
bt_waveform_viewer_finalize (GObject * object)
{
  BtWaveformViewer *self = BT_WAVEFORM_VIEWER(object);
  
  g_free (self->peaks);
}

static void
bt_waveform_viewer_class_init (BtWaveformViewer *klass)
{
  GObjectClass * gobject_class = G_OBJECT_CLASS(klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

  widget_class->expose_event = bt_waveform_viewer_expose;
  widget_class->size_request = bt_waveform_viewer_size_request;
  widget_class->size_allocate = bt_waveform_viewer_size_allocate;
  
  gobject_class->finalize = bt_waveform_viewer_finalize;
}

static void
bt_waveform_viewer_init (BtWaveformViewer *self)
{
  GtkWidget *widget = GTK_WIDGET(self);
  GTK_WIDGET_SET_FLAGS (widget, GTK_NO_WINDOW);
  widget->requisition.width = 40;
  widget->requisition.height = 40;
  self->active = 0;
  self->channels = 2;
  self->peaks_size = 1000;
  self->peaks = malloc(sizeof(float) * self->channels * self->peaks_size);
}

void 
bt_waveform_viewer_update(BtWaveformViewer *self, int16_t *data, int channels, int length)
{
  // untested, probably doesn't work
  int i, c, p, cc = channels;
  int64_t len = length;
  self->channels = channels;
  if (!data || !length)
  {
    self->active = 0;
    gtk_widget_queue_draw(GTK_WIDGET(self));
    return;
  }
  for (i = 0; i < self->peaks_size; i++)
  {
    int p1 = len * i / self->peaks_size;
    int p2 = len * (i + 1) / self->peaks_size;
    for (c = 0; c < self->channels; c++)
    {
      float vmin = data[p1 * cc + c], vmax = data[p1 * cc + c];
      for (p = p1 + 1; p < p2; p++)
      {
        float d = data[p * cc + c];
        if (d < vmin) vmin = d;
        if (d > vmax) vmax = d;
      }
      self->peaks[i * cc + c] = (vmax - vmin) / 32768.0;
    }
  }
  self->active = 1;
  gtk_widget_queue_draw(GTK_WIDGET(self));
}

GtkWidget *
bt_waveform_viewer_new()
{
  return GTK_WIDGET( g_object_new (BT_TYPE_WAVEFORM_VIEWER, NULL ));
}

GType
bt_waveform_viewer_get_type (void)
{
  static GType type = 0;
  if (!type) {
    static const GTypeInfo type_info = {
      sizeof(BtWaveformViewerClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc)bt_waveform_viewer_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof(BtWaveformViewer),
      0,    /* n_preallocs */
      (GInstanceInitFunc)bt_waveform_viewer_init
    };

    type = g_type_register_static( GTK_TYPE_WIDGET,
                                   "BtWaveformViewer",
                                   &type_info,
                                   (GTypeFlags)0);
  }
  return type;
}

