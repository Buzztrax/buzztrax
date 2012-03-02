/* Buzztard
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
/**
 * SECTION:btwaveformviewer
 * @short_description: the waveform viewer widget
 * @see_also: #BtWave, #BtMainPageWaves
 *
 * Provides an viewer for audio waveforms.
 */

#define BT_EDIT
#define BT_MAIN_PAGE_WAVES_C

#include "bt-edit.h"

enum {
  WAVE_VIEWER_WAVE_LENGTH = 1,
  WAVE_VIEWER_LOOP_BEGIN,
  WAVE_VIEWER_LOOP_END,
  WAVE_VIEWER_PLAYBACK_CURSOR
};

//-- the class

G_DEFINE_TYPE (BtWaveformViewer, bt_waveform_viewer, GTK_TYPE_WIDGET);


static gboolean
bt_waveform_viewer_expose(GtkWidget *widget, GdkEventExpose *event)
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
    if (self->loop_begin != -1)
    {
      int x;
      
      cairo_set_source_rgba(c, 1, 0, 0, 0.75);
      cairo_set_line_width(c, 1.0);
      // casting to double loses precision, but we're not planning to deal with multiterabyte waveforms here :)
      x = (int)(ox + self->loop_begin * (double) sx / self->wave_length);
      cairo_move_to(c, x, oy + sy);
      cairo_line_to(c, x, oy);
      cairo_stroke(c);
      cairo_line_to(c, x + 5, oy);
      cairo_line_to(c, x + 5, oy + 5);
      cairo_line_to(c, x, oy + 5);
      cairo_line_to(c, x, oy);
      cairo_fill(c);

      x = (int)(ox + self->loop_end * (double) sx / self->wave_length) - 1;
      cairo_move_to(c, x, oy + sy);
      cairo_line_to(c, x, oy);
      cairo_stroke(c);
      cairo_line_to(c, x - 5, oy);
      cairo_line_to(c, x - 5, oy + 5);
      cairo_line_to(c, x, oy + 5);
      cairo_line_to(c, x, oy);
      cairo_fill(c);
    }
    if (self->playback_cursor != -1)
    {
      int x;
      cairo_set_source_rgba(c, 1, 1, 0, 0.75);
      cairo_set_line_width(c, 1.0);
      x = (int)(ox + self->playback_cursor * (double) sx / self->wave_length) - 1;
      cairo_move_to(c, x, oy + sy);
      cairo_line_to(c, x, oy);
      cairo_stroke(c);
      cairo_move_to(c, x, oy + sy / 2 - 5);
      cairo_line_to(c, x , oy + sy / 2 + 5);
      cairo_line_to(c, x + 5, oy + sy / 2);
      cairo_line_to(c, x, oy + sy / 2 - 5);
      cairo_fill(c);
    }
  }
  
  cairo_destroy(c);
  
  gtk_paint_shadow(widget->style, widget->window, GTK_STATE_NORMAL, GTK_SHADOW_IN, NULL, widget, NULL, ox - 1, oy - 1, sx + 2, sy + 2);
  // printf("exposed %p %d+%d\n", widget->window, widget->allocation.x, widget->allocation.y);
  
  return TRUE;
}

static void
bt_waveform_viewer_size_allocate(GtkWidget *widget,
                           GtkAllocation *allocation)
{
    g_assert(BT_IS_WAVEFORM_VIEWER(widget));
    g_return_if_fail (allocation != NULL);
    
    GTK_WIDGET_CLASS(bt_waveform_viewer_parent_class)->size_allocate(widget, allocation);
    
    widget->allocation = *allocation;
}

static void
bt_waveform_viewer_finalize(GObject * object)
{
  BtWaveformViewer *self = BT_WAVEFORM_VIEWER(object);
  
  g_free(self->peaks);
  
  G_OBJECT_CLASS(bt_waveform_viewer_parent_class)->finalize(object);
}

static void
bt_waveform_viewer_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtWaveformViewer *self = BT_WAVEFORM_VIEWER(object);

  switch (property_id) {
    case WAVE_VIEWER_WAVE_LENGTH: {
      g_value_set_int64(value, self->wave_length);
    } break;
    case WAVE_VIEWER_LOOP_BEGIN: {
      g_value_set_int64(value, self->loop_begin);
    } break;
    case WAVE_VIEWER_LOOP_END: {
      g_value_set_int64(value, self->loop_end);
    } break;
    case WAVE_VIEWER_PLAYBACK_CURSOR: {
      g_value_set_int64(value, self->playback_cursor);
    } break;
    default: {
       G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void
bt_waveform_viewer_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtWaveformViewer *self = BT_WAVEFORM_VIEWER(object);

  switch (property_id) {
    case WAVE_VIEWER_LOOP_BEGIN: {
      self->loop_begin = g_value_get_int64(value);
      if(gtk_widget_get_realized(GTK_WIDGET(self))) {
        gtk_widget_queue_draw(GTK_WIDGET(self));
      }
    } break;
    case WAVE_VIEWER_LOOP_END: {
      self->loop_end = g_value_get_int64(value);
      if(gtk_widget_get_realized(GTK_WIDGET(self))) {
        gtk_widget_queue_draw(GTK_WIDGET(self));
      }
    } break;
    case WAVE_VIEWER_PLAYBACK_CURSOR: {
      self->playback_cursor = g_value_get_int64(value);
      if(gtk_widget_get_realized(GTK_WIDGET(self))) {
        gtk_widget_queue_draw(GTK_WIDGET(self));
      }
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
  /* printf("SetProperty: loop_begin=%d loop_end=%d\n", (int)self->loop_begin, (int)self->loop_end); */
}

static void
bt_waveform_viewer_class_init(BtWaveformViewerClass *klass)
{
  GObjectClass * gobject_class = G_OBJECT_CLASS(klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

  widget_class->expose_event = bt_waveform_viewer_expose;
  widget_class->size_allocate = bt_waveform_viewer_size_allocate;
  
  gobject_class->set_property = bt_waveform_viewer_set_property;
  gobject_class->get_property = bt_waveform_viewer_get_property;
  gobject_class->finalize = bt_waveform_viewer_finalize;

  g_object_class_install_property(gobject_class, WAVE_VIEWER_WAVE_LENGTH,
                                  g_param_spec_int64("wave-length",
                                     "waveform length property",
                                     "The current waveform length",
                                     0,
                                     G_MAXINT64,
                                     0,
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class, WAVE_VIEWER_LOOP_BEGIN,
                                  g_param_spec_int64("loop-begin",
                                     "waveform loop start position",
                                     "First sample of the loop or -1 if there is no loop",
                                     -1,
                                     G_MAXINT64,
                                     -1,
                                     G_PARAM_WRITABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class, WAVE_VIEWER_LOOP_END,
                                  g_param_spec_int64("loop-end",
                                     "waveform loop end position",
                                     "First sample after the loop or -1 if there is no loop",
                                     -1,
                                     G_MAXINT64,
                                     -1,
                                     G_PARAM_WRITABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class, WAVE_VIEWER_PLAYBACK_CURSOR,
                                  g_param_spec_int64("playback-cursor",
                                     "playback cursor position",
                                     "Current playback position within a waveform or -1 if sample is not played",
                                     -1,
                                     G_MAXINT64,
                                     -1,
                                     G_PARAM_WRITABLE|G_PARAM_STATIC_STRINGS));
}

static void
bt_waveform_viewer_init(BtWaveformViewer *self)
{
  GtkWidget *widget = GTK_WIDGET(self);

  gtk_widget_set_has_window(widget, FALSE);
  widget->requisition.width = 40;
  widget->requisition.height = 40;

  self->active = 0;
  self->channels = 2;
  self->peaks_size = 1000;
  self->peaks = g_malloc(sizeof(float) * self->channels * self->peaks_size);
  self->wave_length = 0;
  self->loop_begin = -1;
  self->loop_end = -1;
}

/**
 * bt_waveform_viewer_set_wave:
 * @self: the widget
 * @data: memory block of samples (interleaved for channels>1)
 * @channels: number channels
 * @length: number samples per channel
 *
 * Set wave data to show in the widget.
 */
void 
bt_waveform_viewer_set_wave(BtWaveformViewer *self, int16_t *data, int channels, int length)
{
  int i, c, p, cc = channels;
  int64_t len = length;

  self->channels = channels;
  self->wave_length = length;
  self->loop_begin = -1;
  self->loop_end = -1;
  self->playback_cursor = -1;

  if (!data || !length)
  {
    self->active = 0;
    memset(self->peaks,0,sizeof(float)*self->peaks_size);
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
  g_object_notify((gpointer)self, "wave-length");
  g_object_notify((gpointer)self, "loop-begin");
  g_object_notify((gpointer)self, "loop-end");
  g_object_notify((gpointer)self, "playback-cursor");
  gtk_widget_queue_draw(GTK_WIDGET(self));
}

/**
 * bt_waveform_viewer_new:
 *
 * Create a new waveform viewer widget. Use bt_waveform_viewer_set_wave() to
 * pass wave data.
 *
 * Returns: the widget
 */
GtkWidget *
bt_waveform_viewer_new(void)
{
  return GTK_WIDGET( g_object_new (BT_TYPE_WAVEFORM_VIEWER, NULL ));
}

