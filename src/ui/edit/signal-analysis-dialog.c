/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * SECTION:btsignalanalysisdialog
 * @short_description: audio analysis window
 *
 * The dialog shows a spectrum analyzer and level-meters for left/right
 * channel. The spectrum analyzer support mono and stereo display. It has a few
 * settings for logarithmic/linear mapping and precission.
 *
 * Right now the analyser-section can be attached to a #BtWire and the
 * #BtSinkBin.
 *
 * The dialog is not modal.
 */
/* IDEA(ensonic): nice monitoring ideas:
 * http://www.music-software-reviews.com/adobe_audition_2.html
 * oszilloscope: we can e.g. set it to fit a 55Hz signal to the window width
 */
/* TODO(ensonic): shall we add a volume and panorama control to the dialog as well?
 * - volume to the right of the spectrum
 * - panorama below the spectrum
 */
/* frequency spacing in a FFT is always linear:
 * - for 44100 Hz and 256 bands, spacing is 22050/256 = ~86.13
 * - log-grid should have lines at 1,2,5, 10,20,50, 100,200,500, ....
 */
#define BT_EDIT
#define BT_SIGNAL_ANALYSIS_DIALOG_C

#include <math.h>

#include "bt-edit.h"

//-- property ids

enum
{
  SIGNAL_ANALYSIS_DIALOG_ELEMENT = 1
};

/* TODO(ensonic): add more later:
 * waveform (oszilloscope)
 * panorama (spacescope)
 */
typedef enum
{
  // needed to buffer
  ANALYZER_QUEUE = 0,
  // real analyzers
  ANALYZER_LEVEL,
  ANALYZER_SPECTRUM,
  // this can be 'mis'used for an oszilloscope by connecting to hand-off
  ANALYZER_FAKESINK,
  // how many elements are used
  ANALYZER_COUNT
} BtWireAnalyzer;

typedef enum
{
  MAP_LIN = 0,
  MAP_LOG = 1
} BtWireAnalyzerMapping;

#define AXIS_THICKNESS 30
#define LEVEL_HEIGHT 16
#define LOW_VUMETER_VAL -90.0

#define SPECTRUM_FLOOR -70

#define UPDATE_INTERVAL ((GstClockTime)(0.05*GST_SECOND))

struct _BtSignalAnalysisDialogPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* the item to attach the analyzer to */
  GstBin *element;
  GstBus *bus;

  /* the analyzer-graphs */
  GtkWidget *spectrum_drawingarea, *level_drawingarea;
  GtkWidget *spectrum_ruler;
  GdkColor *decay_color, *grid_color;
  cairo_pattern_t *spect_grad;

  /* the gstreamer elements that is used */
  GstElement *analyzers[ANALYZER_COUNT];
  GList *analyzers_list;

  /* the analyzer results (max stereo) */
  gdouble peak[2], decay[2];
  gfloat *spect[2];
  GMutex lock;

  guint spect_channels;
  guint spect_height;
  guint spect_bands;
  gfloat height_scale;
  /* sampling rate from spectrum.sink caps */
  gint srate;
  BtWireAnalyzerMapping frq_map;
  guint frq_precision;

  GstClock *clock;

  /* up to srat=900000 */
  gdouble grid_log10[6 * 10];
  gdouble *graph_log10;

  // DEBUG
  //gdouble min_rms,max_rms, min_peak,max_peak;
  // DEBUG
};

static GQuark bus_msg_level_quark = 0;
static GQuark bus_msg_spectrum_quark = 0;

//-- the class

G_DEFINE_TYPE (BtSignalAnalysisDialog, bt_signal_analysis_dialog,
    GTK_TYPE_WINDOW);


//-- event handler helper

static gboolean
update_spectrum_ruler (gpointer user_data)
{
  BtSignalAnalysisDialog *self = BT_SIGNAL_ANALYSIS_DIALOG (user_data);
  gtk_widget_queue_draw (self->priv->spectrum_ruler);
  return FALSE;
}

static void
update_spectrum_graph_log10 (BtSignalAnalysisDialog * self)
{
  guint i, spect_bands = self->priv->spect_bands * self->priv->frq_precision;
  gdouble *graph_log10 = self->priv->graph_log10;
  gdouble srat2 = self->priv->srate / 2.0;

  for (i = 0; i < spect_bands; i++) {
    graph_log10[i] = log10 (1.0 + (i * srat2) / spect_bands);
  }
}

static void
update_spectrum_analyzer (BtSignalAnalysisDialog * self)
{
  guint spect_bands;

  g_mutex_lock (&self->priv->lock);

  spect_bands = self->priv->spect_bands * self->priv->frq_precision;

  g_free (self->priv->spect[0]);
  self->priv->spect[0] = g_new0 (gfloat, spect_bands);
  g_free (self->priv->spect[1]);
  self->priv->spect[1] = g_new0 (gfloat, spect_bands);

  g_free (self->priv->graph_log10);
  self->priv->graph_log10 = g_new (gdouble, spect_bands);
  update_spectrum_graph_log10 (self);

  if (self->priv->analyzers[ANALYZER_SPECTRUM]) {
    g_object_set ((GObject *) (self->priv->analyzers[ANALYZER_SPECTRUM]),
        "bands", spect_bands, NULL);
  }

  g_mutex_unlock (&self->priv->lock);
}

//-- event handler

static void
on_dialog_realize (GtkWidget * widget, gpointer user_data)
{
  BtSignalAnalysisDialog *self = BT_SIGNAL_ANALYSIS_DIALOG (user_data);

  GST_DEBUG ("dialog realize");
  self->priv->decay_color =
      bt_ui_resources_get_gdk_color (BT_UI_RES_COLOR_ANALYZER_DECAY);
  self->priv->grid_color =
      bt_ui_resources_get_gdk_color (BT_UI_RES_COLOR_GRID_LINES);
}

/*
 * on_level_draw:
 *
 * Draw volume levels.
 */
static gboolean
on_level_draw (GtkWidget * widget, cairo_t * cr, gpointer user_data)
{
  BtSignalAnalysisDialog *self = BT_SIGNAL_ANALYSIS_DIALOG (user_data);

  if (!gtk_widget_get_realized (widget))
    return TRUE;

  gint middle = self->priv->spect_bands >> 1;
  gdouble scl = middle / (-LOW_VUMETER_VAL);
  gdouble peak0, peak1, decay0, decay1;

  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
  cairo_rectangle (cr, 0, 0, self->priv->spect_bands, LEVEL_HEIGHT);
  cairo_fill (cr);

  peak0 = self->priv->peak[0] * scl;
  peak1 = self->priv->peak[1] * scl;
  decay0 = self->priv->decay[0] * scl;
  decay1 = self->priv->decay[1] * scl;
  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_rectangle (cr, middle - peak0, 0, peak0 + peak1, LEVEL_HEIGHT);
  cairo_fill (cr);
  cairo_set_source_rgb (cr, self->priv->decay_color->red / 65535.0,
      self->priv->decay_color->green / 65535.0,
      self->priv->decay_color->blue / 65535.0);
  cairo_rectangle (cr, middle - decay0, 0, 2, LEVEL_HEIGHT);
  cairo_fill (cr);
  cairo_rectangle (cr, (middle - 1) + decay1, 0, 2, LEVEL_HEIGHT);
  cairo_fill (cr);

  /* TODO(ensonic): if stereo draw pan-meter (L-R, R-L) */
  return TRUE;
}

/*
 * on_spectrum_expose:
 *
 * Draw frequency spectrum.
 */
static gboolean
on_spectrum_draw (GtkWidget * widget, cairo_t * cr, gpointer user_data)
{
  BtSignalAnalysisDialog *self = BT_SIGNAL_ANALYSIS_DIALOG (user_data);

  if (!gtk_widget_get_realized (widget))
    return TRUE;

#ifndef GST_DISABLE_DEBUG
  // DEBUG
  GST_INFO ("before spectrum update");
  GstClockTime __ts = gst_util_get_timestamp ();
  // DEBUG
#endif

  guint i, c;
  gdouble x, y, v, f;
  gdouble inc, end, srat2;
  gint mx, my;
  guint spect_bands = self->priv->spect_bands;
  guint spect_height = self->priv->spect_height;
  gdouble *grid_log10 = self->priv->grid_log10;
  gdouble *graph_log10 = self->priv->graph_log10;
  gdouble grid_dash_pattern[] = { 1.0 };
  gdouble prec = self->priv->frq_precision;
  GdkWindow *window = gtk_widget_get_window (widget);

  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
  cairo_rectangle (cr, 0, 0, spect_bands, spect_height);
  cairo_fill (cr);
  /* draw grid lines
   * the bin center frequencies are f(i)=i*srat/spect_bands
   */
  cairo_set_source_rgb (cr, self->priv->grid_color->red / 65535.0,
      self->priv->grid_color->green / 65535.0,
      self->priv->grid_color->blue / 65535.0);
  cairo_set_line_width (cr, 1.0);
  cairo_set_dash (cr, grid_dash_pattern, 1, 0.0);
  y = 0.5 + round (spect_height / 2.0);
  cairo_move_to (cr, 0, y);
  cairo_line_to (cr, spect_bands, y);
  if (self->priv->frq_map == MAP_LIN) {
    for (f = 0.05; f < 1.0; f += 0.05) {
      x = 0.5 + round (spect_bands * f);
      cairo_move_to (cr, x, 0);
      cairo_line_to (cr, x, spect_height);
    }
  } else {
    srat2 = self->priv->srate / 2.0;
    v = spect_bands / log10 (srat2);
    i = 0;
    f = 1.0;
    inc = 1.0;
    end = 10.0;
    while (f < srat2) {
      x = 0.5 + round (v * grid_log10[i++]);
      cairo_move_to (cr, x, 0);
      cairo_line_to (cr, x, spect_height);
      f += inc;
      if (f >= end) {
        f = inc = end;
        end *= 10.0;
      }
    }
  }
  cairo_stroke (cr);
  // draw frequencies
  g_mutex_lock (&self->priv->lock);
  for (c = 0; c < self->priv->spect_channels; c++) {
    if (self->priv->spect[c]) {
      gfloat *spect = self->priv->spect[c];

      // set different color for different channels
      // maybe we also want a different gradient
      if (self->priv->spect_channels == 1) {
        cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
      } else {
        if (c == 0) {
          cairo_set_source_rgba (cr, 1.0, 0.0, 0.7, 0.7);
        } else {
          cairo_set_source_rgba (cr, 0.6, 0.6, 1.0, 0.7);
        }
      }
      cairo_set_line_width (cr, 1.0);
      cairo_set_dash (cr, NULL, 0, 0.0);
      cairo_move_to (cr, 0, spect_height);
      if (self->priv->frq_map == MAP_LIN) {
        for (i = 0; i < (spect_bands * prec); i++) {
          cairo_line_to (cr, (i / prec), spect_height - spect[i]);
        }
      } else {
        srat2 = self->priv->srate / 2.0;
        v = spect_bands / log10 (srat2);
        for (i = 0; i < (spect_bands * prec); i++) {
          // db value
          //y=-20.0*log10(-spect[i]);
          x = 0.5 + v * graph_log10[i];
          cairo_line_to (cr, x, spect_height - spect[i]);
        }
      }
      // close the path
      cairo_line_to (cr, (gdouble) spect_bands - 0.5,
          (gdouble) spect_height - 0.5);
      cairo_line_to (cr, 0.5, (gdouble) spect_height - 0.5);
      cairo_stroke_preserve (cr);
      // in case the gradient is too slow:
      //cairo_set_source_rgb(cr,0.7,0.7,0.7);
      cairo_set_source (cr, self->priv->spect_grad);
      cairo_fill (cr);
    }
  }
  g_mutex_unlock (&self->priv->lock);

  // draw cross-hair for mouse
  // TODO(ensonic): cache GdkDevice* 
  gdk_window_get_device_position (window,
      gdk_device_manager_get_client_pointer (gdk_display_get_device_manager
          (gtk_widget_get_display (widget))), &mx, &my, NULL);
  if ((mx >= 0) && (mx < gtk_widget_get_allocated_width (widget))
      && (my >= 0) && (my < gtk_widget_get_allocated_height (widget))) {
    cairo_set_source_rgba (cr, self->priv->decay_color->red / 65535.0,
        self->priv->decay_color->green / 65535.0,
        self->priv->decay_color->blue / 65535.0, 0.7);
    cairo_rectangle (cr, mx - 1.0, 0.0, 2.0, spect_height);
    cairo_rectangle (cr, 0.0, my - 1.0, spect_bands, 2.0);
    cairo_fill (cr);
  }
#ifndef GST_DISABLE_DEBUG
  // DEBUG
  // these values range from 0.0012 to 0.8348
  GstClockTimeDiff __tsd = GST_CLOCK_DIFF (__ts, gst_util_get_timestamp ());
  GST_INFO ("spectrum update took: %" GST_TIME_FORMAT, GST_TIME_ARGS (__tsd));
  // DEBUG
#endif

  return TRUE;
}

static void
draw_hlabel (GtkStyleContext * sc, cairo_t * cr, PangoLayout * l,
    gint lx, gint ly, gint tx, gint ty1, gint ty2)
{
  if (l) {
    gtk_render_layout (sc, cr, lx, ly, l);
  }
  gtk_render_line (sc, cr, tx, ty1, tx, ty2);
}

typedef struct
{
  gint d, r, l;                 // divide, recurse, labels
} BtAxisData;

static void
layout_label (GtkStyleContext * sc, cairo_t * cr, PangoLayout * l,
    BtAxisData * ad, gint d, gint v1, gint v3, gint x1, gint x3, gint w1,
    gint y1, gint y2, gint y3)
{
  PangoRectangle lr;
  gchar str[30];
  gint st = ad[d - 1].d / ad[d].d;
  gint xo = x1, x2 = x1, vo = v1, v2 = v1;
  gdouble xs = (x3 - x1) / (gdouble) st, vs = (v3 - v1) / (gdouble) st;
  gint i, w2;
  gint y4 = y2 + (d - 1) * ((gdouble) (y3 - y2) / 7.0); // make ticks shorter on each level
  gboolean recurse, labels;

  // check if there is enough space for labels and ticks once we enter a new level
  if (ad[d].r == -1)
    ad[d].r = ((x3 - x1) > (st * 10));
  recurse = ad[d].r;
  if (ad[d].l == -1)
    ad[d].l = (((x3 - w1) - (x1 + w1)) > (st * w1));
  labels = ad[d].l;

  //GST_DEBUG("depth=%d, steps=%d, vs=%lf, xs=%lf",d,st,vs,xs);

  for (i = 1; i < st; i++) {
    v2 = v1 + ((gdouble) i * vs);
    x2 = x1 + ((gdouble) i * xs);
    sprintf (str, "<small>%d</small>", abs (v2));
    pango_layout_set_markup (l, str, -1);
    pango_layout_get_pixel_extents (l, NULL, &lr);
    w2 = lr.width / 2;
    if ((d < 4) && labels) {    // long tick with label
      draw_hlabel (sc, cr, l, x2 - w2, y1, x2, y2, y3);
    } else {                    // short tick without label
      draw_hlabel (sc, cr, NULL, x2 - w2, y1, x2, y4, y3);
    }
    if ((d < 5) && recurse) {
      layout_label (sc, cr, l, ad, d + 1, vo, v2, xo, x2, w1, y1, y2, y3);
    }
    vo = v2;
    xo = x2;
  }
  if ((d < 5) && recurse) {
    layout_label (sc, cr, l, ad, d + 1, v2, v3, x2, x3, w1, y1, y2, y3);
  }
}

static gboolean
on_spectrum_freq_axis_draw (GtkWidget * widget, cairo_t * cr,
    gpointer user_data)
{
  BtSignalAnalysisDialog *self = BT_SIGNAL_ANALYSIS_DIALOG (user_data);

  if (!gtk_widget_get_realized (GTK_WIDGET (self)))
    return (TRUE);

  GdkRectangle rect = { 0, 0, self->priv->spect_bands, AXIS_THICKNESS };
  GdkWindow *window = gtk_widget_get_window (widget);
  GtkStyleContext *sc = gtk_widget_get_style_context (widget);
  PangoLayout *layout = gtk_widget_create_pango_layout (widget, NULL);
  PangoRectangle lr;
  gint x1, x2, x3, y1, y2, y3, w1;
  gint m = self->priv->srate / 2;
  gchar str[30];
  BtAxisData ad[] = {
    {10000, 1, 1}
    ,
    {5000, -1, -1}
    ,
    {1000, -1, -1}
    ,
    {500, -1, -1}
    ,
    {100, -1, -1}
    ,
    {50, -1, -1}
    ,
    {10, -1, -1}
    ,
    {5, -1, -1}
    ,
    {1, -1, -1}
    ,
    {0, -1, -1}
  };

  gdk_window_begin_paint_rect (window, &rect);

  gtk_style_context_save (sc);
  gtk_style_context_add_class (sc, GTK_STYLE_CLASS_SEPARATOR);
  gtk_style_context_add_class (sc, GTK_STYLE_CLASS_MARK);

  // draw beg,end
  x1 = 0;
  x3 = gtk_widget_get_allocated_width (widget);
  y1 = 0;
  y2 = (AXIS_THICKNESS - 1) / 2;
  y3 = AXIS_THICKNESS - 1;
  //GST_WARNING("draw freq axis: %d..%d x %d..%d",x1,x3,y1,y3);

  // draw beg,end
  sprintf (str, "<small>0</small>");
  pango_layout_set_markup (layout, str, -1);
  pango_layout_get_pixel_extents (layout, NULL, &lr);
  draw_hlabel (sc, cr, layout, x1, y1, x1, y2, y3);

  sprintf (str, "<small>%d</small>", m);
  pango_layout_set_markup (layout, str, -1);
  pango_layout_get_pixel_extents (layout, NULL, &lr);
  w1 = lr.width;
  draw_hlabel (sc, cr, layout, x3 - w1, y1, x3 - 1, y2, y3);

  if (self->priv->frq_map == MAP_LIN) {
    // recurse
    layout_label (sc, cr, layout, ad, 1, 0, m, x1, x3, w1, y1, y2, y3);
  } else {
    gdouble v = self->priv->spect_bands / log10 (m);
    gdouble f = 0.0, inc = 1.0, end = 10.0;
    gdouble *grid_log10 = self->priv->grid_log10;
    gint i = 0, w2;

    while (f < m) {
      x2 = 0.5 + v * grid_log10[i++];
      f += inc;
      if (f >= end) {
        sprintf (str, "<small>%d</small>", (gint) end);
        pango_layout_set_markup (layout, str, -1);
        pango_layout_get_pixel_extents (layout, NULL, &lr);
        w2 = lr.width / 2;
        if ((x2 + w2) < (x3 - w1)) {
          draw_hlabel (sc, cr, layout, x2 - w2, y1, x2, y2, y3);
        } else {
          draw_hlabel (sc, cr, NULL, x2, y1, x2, y2, y3);
        }
        f = inc = end;
        end *= 10.0;
      } else {
        draw_hlabel (sc, cr, NULL, x2, y1, x2, y2, y3);
      }
    }
  }

  gdk_window_end_paint (window);
  gtk_style_context_restore (sc);
  g_object_unref (layout);
  return (TRUE);
}

static gboolean
on_level_axis_draw (GtkWidget * widget, cairo_t * cr, gpointer user_data)
{
  BtSignalAnalysisDialog *self = BT_SIGNAL_ANALYSIS_DIALOG (user_data);

  if (!gtk_widget_get_realized (GTK_WIDGET (self)))
    return (TRUE);

  GdkRectangle rect = { 0, 0, self->priv->spect_bands, AXIS_THICKNESS };
  GdkWindow *window = gtk_widget_get_window (widget);
  GtkStyleContext *sc = gtk_widget_get_style_context (widget);
  PangoLayout *layout = gtk_widget_create_pango_layout (widget, NULL);
  PangoRectangle lr;
  gint x1, x3, y1, y2, y3, w1;
  gchar str[30];
  BtAxisData ad[] = {
    {200, 1, 1}
    ,
    {100, -1, -1}
    ,
    {50, -1, -1}
    ,
    {10, -1, -1}
    ,
    {5, -1, -1}
    ,
    {1, -1, -1}
    ,
    {0, -1, -1}
  };

  gdk_window_begin_paint_rect (window, &rect);

  gtk_style_context_save (sc);
  gtk_style_context_add_class (sc, GTK_STYLE_CLASS_SEPARATOR);
  gtk_style_context_add_class (sc, GTK_STYLE_CLASS_MARK);

  x1 = 0;
  x3 = gtk_widget_get_allocated_width (widget);
  y1 = 0;
  y2 = (AXIS_THICKNESS - 1) / 2;
  y3 = AXIS_THICKNESS - 1;
  //GST_DEBUG("draw level axis: %d..%d x %d..%d",x1,x3,y1,y3);

  // draw beg,end
  sprintf (str, "<small>100</small>");
  pango_layout_set_markup (layout, str, -1);
  pango_layout_get_pixel_extents (layout, NULL, &lr);
  w1 = lr.width;

  draw_hlabel (sc, cr, layout, x1, y1, x1, y2, y3);
  draw_hlabel (sc, cr, layout, x3 - w1, y1, x3 - 1, y2, y3);

  // draw mid and recurse, d : 0  ,1  , 2, 3,4,5
  // we'd like to subdiv by  : 200,100,50,10,5,1
  // for labels we stop at d=3 or not enough space
  // for ticks  we stop at d=5 or not enough space
  layout_label (sc, cr, layout, ad, 1, -100, 100, x1, x3, w1, y1, y2, y3);

  gdk_window_end_paint (window);
  gtk_style_context_restore (sc);
  g_object_unref (layout);
  return (TRUE);
}

static gboolean
on_delayed_idle_signal_analyser_change (gpointer user_data)
{
  gconstpointer *const params = (gconstpointer *) user_data;
  BtSignalAnalysisDialog *self = BT_SIGNAL_ANALYSIS_DIALOG (params[0]);
  GstMessage *message = (GstMessage *) params[1];
  const GstStructure *structure = gst_message_get_structure (message);
  const GQuark name_id = gst_structure_get_name_id (structure);

  if (!self)
    goto done;

  g_object_remove_weak_pointer ((gpointer) self, (gpointer *) & params[0]);

  if (name_id == bus_msg_level_quark) {
    const GValue *values;
    GValueArray *peak_arr, *decay_arr;
    guint i;
    gdouble val;

    //GST_INFO("get level data");
    values = (GValue *) gst_structure_get_value (structure, "peak");
    peak_arr = (GValueArray *) g_value_get_boxed (values);
    values = (GValue *) gst_structure_get_value (structure, "decay");
    decay_arr = (GValueArray *) g_value_get_boxed (values);
    // size of list is number of channels
    switch (decay_arr->n_values) {
      case 1:                  // mono
        val = g_value_get_double (g_value_array_get_nth (peak_arr, 0));
        if (isinf (val) || isnan (val))
          val = LOW_VUMETER_VAL;
        self->priv->peak[0] = -(LOW_VUMETER_VAL - val);
        self->priv->peak[1] = self->priv->peak[0];
        val = g_value_get_double (g_value_array_get_nth (decay_arr, 0));
        if (isinf (val) || isnan (val))
          val = LOW_VUMETER_VAL;
        self->priv->decay[0] = -(LOW_VUMETER_VAL - val);
        self->priv->decay[1] = self->priv->decay[0];
        break;
      case 2:                  // stereo
        for (i = 0; i < 2; i++) {
          val = g_value_get_double (g_value_array_get_nth (peak_arr, i));
          if (isinf (val) || isnan (val))
            val = LOW_VUMETER_VAL;
          self->priv->peak[i] = -(LOW_VUMETER_VAL - val);
          val = g_value_get_double (g_value_array_get_nth (decay_arr, i));
          if (isinf (val) || isnan (val))
            val = LOW_VUMETER_VAL;
          self->priv->decay[i] = -(LOW_VUMETER_VAL - val);
        }
        break;
    }
    gtk_widget_queue_draw (self->priv->level_drawingarea);
  } else if (name_id == bus_msg_spectrum_quark) {
    const GValue *data;
    const GValue *value;
    guint i, size, spect_bands, spect_height = self->priv->spect_height;
    gfloat height_scale = self->priv->height_scale;
    gfloat *spect;

    g_mutex_lock (&self->priv->lock);
    spect_bands = self->priv->spect_bands * self->priv->frq_precision;
    //GST_INFO("get spectrum data");
    if ((data = gst_structure_get_value (structure, "magnitude"))) {
      if (GST_VALUE_HOLDS_ARRAY (data)) {
        const GValue *cdata;
        guint c = gst_value_array_get_size (data);

        self->priv->spect_channels = MIN (2, c);
        for (c = 0; c < self->priv->spect_channels; c++) {
          cdata = gst_value_array_get_value (data, c);
          size = gst_value_array_get_size (cdata);
          if (size != spect_bands)
            break;
          spect = self->priv->spect[c];
          for (i = 0; i < spect_bands; i++) {
            value = gst_value_array_get_value (cdata, i);
            spect[i] = spect_height - height_scale * g_value_get_float (value);
          }
        }
        if (c == self->priv->spect_channels) {
          gtk_widget_queue_draw (self->priv->spectrum_drawingarea);
          GST_INFO ("trigger spectrum update");
        }
      } else if (GST_VALUE_HOLDS_LIST (data)) {
        // only in the multi-channel=FALSE case
        size = gst_value_list_get_size (data);
        if (size == spect_bands) {
          spect = self->priv->spect[0];
          for (i = 0; i < spect_bands; i++) {
            value = gst_value_list_get_value (data, i);
            spect[i] = spect_height - height_scale * g_value_get_float (value);
          }
          gtk_widget_queue_draw (self->priv->spectrum_drawingarea);
          GST_INFO ("trigger spectrum update");
        }
      }
    }
    g_mutex_unlock (&self->priv->lock);
  }

done:
  gst_message_unref (message);
  g_slice_free1 (2 * sizeof (gconstpointer), params);
  return (FALSE);
}

static gboolean
on_delayed_signal_analyser_change (GstClock * clock, GstClockTime time,
    GstClockID id, gpointer user_data)
{
  // the callback is called from a clock thread
  if (GST_CLOCK_TIME_IS_VALID (time))
    g_idle_add_full (G_PRIORITY_HIGH, on_delayed_idle_signal_analyser_change,
        user_data, NULL);
  else {
    gconstpointer *const params = (gconstpointer *) user_data;
    GstMessage *message = (GstMessage *) params[1];
    gst_message_unref (message);
    g_slice_free1 (2 * sizeof (gconstpointer), user_data);
    GST_WARNING_OBJECT (GST_MESSAGE_SRC (message),
        "dropped analyzer update due to invalid ts");
  }
  return (TRUE);
}

static void
on_signal_analyser_change (GstBus * bus, GstMessage * message,
    gpointer user_data)
{
  const GstStructure *s = gst_message_get_structure (message);
  const GQuark name_id = gst_structure_get_name_id (s);

  if ((name_id == bus_msg_level_quark) || (name_id == bus_msg_spectrum_quark)) {
    BtSignalAnalysisDialog *self = BT_SIGNAL_ANALYSIS_DIALOG (user_data);
    GstElement *meter = GST_ELEMENT (GST_MESSAGE_SRC (message));

    if ((meter == self->priv->analyzers[ANALYZER_LEVEL]) ||
        (meter == self->priv->analyzers[ANALYZER_SPECTRUM])) {
      GstClockTime waittime = bt_gst_analyzer_get_waittime (meter, s,
          (name_id == bus_msg_level_quark));

      if (GST_CLOCK_TIME_IS_VALID (waittime)) {
        gpointer *data = (gpointer *) g_slice_alloc (2 * sizeof (gpointer));
        GstClockID clock_id;
        GstClockReturn clk_ret;
        GstClockTime basetime = gst_element_get_base_time (meter);

        /* debug
           gint64 playtime;
           gst_element_query_position (meter, GST_FORMAT_TIME, &playtime);
           GST_LOG_OBJECT (meter, "update for wait + basetime %" GST_TIME_FORMAT
           " + %" GST_TIME_FORMAT " at %" GST_TIME_FORMAT,
           GST_TIME_ARGS (waittime), GST_TIME_ARGS (basetime),
           GST_TIME_ARGS (playtime));
           debug */

        data[0] = (gpointer) self;
        data[1] = (gpointer) gst_message_ref (message);
        g_object_add_weak_pointer ((gpointer) self, (gpointer *) & data[0]);

        clock_id =
            gst_clock_new_single_shot_id (self->priv->clock,
            waittime + basetime);
        if ((clk_ret = gst_clock_id_wait_async (clock_id,
                    on_delayed_signal_analyser_change,
                    (gpointer) data, NULL)) != GST_CLOCK_OK) {
          GST_WARNING_OBJECT (meter, "clock wait failed: %d", clk_ret);
          gst_message_unref (message);
          g_slice_free1 (2 * sizeof (gconstpointer), data);
        }
        gst_clock_id_unref (clock_id);
      }
    }
  }
}

static void
on_size_allocate (GtkWidget * widget, GtkAllocation * allocation,
    gpointer user_data)
{
  BtSignalAnalysisDialog *self = BT_SIGNAL_ANALYSIS_DIALOG (user_data);
  guint old_heigth = self->priv->spect_height;
  guint old_bands = self->priv->spect_bands;

  /*GST_INFO ("%d x %d", allocation->width, allocation->height); */
  self->priv->spect_height = allocation->height;
  self->priv->height_scale =
      (gdouble) allocation->height / (gdouble) SPECTRUM_FLOOR;
  self->priv->spect_bands = allocation->width;

  if (old_heigth != self->priv->spect_height || !self->priv->spect_grad) {
    if (self->priv->spect_grad)
      cairo_pattern_destroy (self->priv->spect_grad);
    // this looks nice, but seems to be expensive
    self->priv->spect_grad =
        cairo_pattern_create_linear (0.0, self->priv->spect_height, 0.0, 0.0);
    cairo_pattern_add_color_stop_rgba (self->priv->spect_grad, 0.00, 0.8, 0.8,
        0.8, 0.7);
    cairo_pattern_add_color_stop_rgba (self->priv->spect_grad, 1.00, 0.8, 0.8,
        0.8, 0.0);
  }
  if (old_bands != self->priv->spect_bands) {
    update_spectrum_analyzer (self);
  }
  gtk_widget_queue_draw (self->priv->spectrum_drawingarea);
}

static void
on_caps_negotiated (GstPad * pad, GParamSpec * arg, gpointer user_data)
{
  BtSignalAnalysisDialog *self = BT_SIGNAL_ANALYSIS_DIALOG (user_data);
  GstCaps *caps;

  if ((caps = (GstCaps *) gst_pad_get_current_caps (pad))) {
    if (GST_CAPS_IS_SIMPLE (caps)) {
      gint old_srate = self->priv->srate;
      gst_structure_get_int (gst_caps_get_structure (caps, 0), "rate",
          &self->priv->srate);
      if (self->priv->srate != old_srate) {
        update_spectrum_graph_log10 (self);
        // need to call this via g_idle_add as it triggers the redraw
        bt_g_object_idle_add ((GObject *) self, G_PRIORITY_DEFAULT_IDLE,
            update_spectrum_ruler);
      }
    } else {
      GST_WARNING_OBJECT (pad, "expecting simple caps");
    }
    gst_caps_unref (caps);
  }
}

static void
on_spectrum_frequency_mapping_changed (GtkComboBox * combo, gpointer user_data)
{
  BtSignalAnalysisDialog *self = BT_SIGNAL_ANALYSIS_DIALOG (user_data);

  self->priv->frq_map = gtk_combo_box_get_active (combo);
  update_spectrum_ruler (self);
  gtk_widget_queue_draw (self->priv->spectrum_drawingarea);
}

static void
on_spectrum_frequency_precision_changed (GtkComboBox * combo,
    gpointer user_data)
{
  BtSignalAnalysisDialog *self = BT_SIGNAL_ANALYSIS_DIALOG (user_data);

  self->priv->frq_precision = 1 + gtk_combo_box_get_active (combo);
  update_spectrum_analyzer (self);
}

static gboolean
on_spectrum_drawingarea_motion_notify_event (GtkWidget * widget,
    GdkEventMotion * event, gpointer user_data)
{
  BtSignalAnalysisDialog *self = BT_SIGNAL_ANALYSIS_DIALOG (user_data);

  gtk_widget_queue_draw (self->priv->spectrum_drawingarea);
  return (FALSE);
}

//-- helper methods

/*
 * bt_signal_analysis_dialog_make_element:
 * @self: the signal-analysis dialog
 * @part: which analyzer element to create
 * @factory_name: the element-factories name (also used to build the elemnt name)
 *
 * Create analyzer elements. Store them in the analyzers array and link them into the list.
 */
static gboolean
bt_signal_analysis_dialog_make_element (const BtSignalAnalysisDialog * self,
    BtWireAnalyzer part, const gchar * factory_name)
{
  gboolean res = FALSE;
  gchar *name;

  // add analyzer element
  name = g_alloca (strlen ("analyzer_") + strlen (factory_name) + 16);
  g_sprintf (name, "analyzer_%s_%p", factory_name, self->priv->element);
  if (!(self->priv->analyzers[part] =
          gst_element_factory_make (factory_name, name))) {
    GST_ERROR ("failed to create %s", factory_name);
    goto Error;
  }
  self->priv->analyzers_list =
      g_list_prepend (self->priv->analyzers_list, self->priv->analyzers[part]);
  res = TRUE;
Error:
  return (res);
}

static gboolean
bt_signal_analysis_dialog_init_ui (const BtSignalAnalysisDialog * self)
{
  BtMainWindow *main_window;
  BtSong *song;
  GstBin *bin;
  GstPad *pad;
  gchar *title = NULL;
  //GdkPixbuf *window_icon=NULL;
  GtkWidget *vbox, *table;
  GtkWidget *ruler, *combo;
  gboolean res = TRUE;

  gtk_widget_set_name (GTK_WIDGET (self), "wire analysis");

  g_object_get (self->priv->app, "main-window", &main_window, "song", &song,
      NULL);
  gtk_window_set_transient_for (GTK_WINDOW (self), GTK_WINDOW (main_window));

  /* TODO(ensonic): create and set *proper* window icon (analyzer, scope)
     if((window_icon=bt_ui_resources_get_pixbuf_by_wire(self->priv->element))) {
     gtk_window_set_icon(GTK_WINDOW(self),window_icon);
     }
   */

  /* DEBUG
     self->priv->min_rms=1000.0;
     self->priv->max_rms=-1000.0;
     self->priv->min_peak=1000.0;
     self->priv->max_peak=-1000.0;
     // DEBUG */

  // leave the choice of width to gtk
  gtk_window_set_default_size (GTK_WINDOW (self), -1, 200);

  // TODO(ensonic): different names for wire or sink machine
  if (BT_IS_WIRE (self->priv->element)) {
    BtMachine *src_machine, *dst_machine;
    gchar *src_id, *dst_id;

    g_object_get (self->priv->element, "src", &src_machine, "dst",
        &dst_machine, NULL);
    g_object_get (src_machine, "id", &src_id, NULL);
    g_object_get (dst_machine, "id", &dst_id, NULL);
    // set dialog title : machine -> machine analysis
    title = g_strdup_printf (_("%s -> %s analysis"), src_id, dst_id);
    g_object_unref (src_machine);
    g_object_unref (dst_machine);
    g_free (src_id);
    g_free (dst_id);
  } else if (BT_IS_SINK_MACHINE (self->priv->element)) {
    title = g_strdup (_("master analysis"));
  } else {
    GST_WARNING ("unsupported object for signal analyser: %s,%p",
        G_OBJECT_TYPE_NAME (self->priv->element), self->priv->element);
  }
  if (title) {
    gtk_window_set_title (GTK_WINDOW (self), title);
    g_free (title);
  }

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

  /* add scales for spectrum analyzer drawable */
  /* TODO(ensonic): we need to use a gtk_table() and also add a vruler with levels */
  ruler = gtk_drawing_area_new ();
  g_signal_connect (ruler, "draw",
      G_CALLBACK (on_spectrum_freq_axis_draw), (gpointer) self);
  gtk_widget_set_size_request (ruler, -1, AXIS_THICKNESS);
  gtk_box_pack_start (GTK_BOX (vbox), ruler, FALSE, FALSE, 0);
  self->priv->spectrum_ruler = ruler;
  update_spectrum_ruler ((gpointer) self);

  /* add spectrum canvas */
  self->priv->spectrum_drawingarea = gtk_drawing_area_new ();
  gtk_widget_set_size_request (self->priv->spectrum_drawingarea,
      self->priv->spect_bands, self->priv->spect_height);
  gtk_widget_add_events (GTK_WIDGET (self->priv->spectrum_drawingarea),
      GDK_POINTER_MOTION_MASK);
  g_signal_connect (G_OBJECT (self->priv->spectrum_drawingarea),
      "size-allocate", G_CALLBACK (on_size_allocate), (gpointer) self);
  g_signal_connect (self->priv->spectrum_drawingarea, "motion-notify-event",
      G_CALLBACK (on_spectrum_drawingarea_motion_notify_event),
      (gpointer) self);
  gtk_box_pack_start (GTK_BOX (vbox), self->priv->spectrum_drawingarea, TRUE,
      TRUE, 0);

  /* spacer */
  gtk_box_pack_start (GTK_BOX (vbox), gtk_label_new (" "), FALSE, FALSE, 0);

  /* add scales for level meter */
  ruler = gtk_drawing_area_new ();
  gtk_widget_set_size_request (ruler, -1, AXIS_THICKNESS);
  g_signal_connect (ruler, "draw", G_CALLBACK (on_level_axis_draw),
      (gpointer) self);
  gtk_box_pack_start (GTK_BOX (vbox), ruler, FALSE, FALSE, 0);

  /* add level-meter canvas */
  self->priv->level_drawingarea = gtk_drawing_area_new ();
  gtk_widget_set_size_request (self->priv->level_drawingarea,
      self->priv->spect_bands, LEVEL_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox), self->priv->level_drawingarea, FALSE,
      FALSE, 0);

  /* spacer */
  gtk_box_pack_start (GTK_BOX (vbox), gtk_label_new (" "), FALSE, FALSE, 0);

  /* settings */
  table = gtk_table_new (2, 2, FALSE);

  /* scale: linear and logarithmic */
  combo = gtk_combo_box_text_new ();
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("lin."));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("log."));
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
  g_signal_connect (combo, "changed",
      G_CALLBACK (on_spectrum_frequency_mapping_changed), (gpointer) self);
  gtk_table_attach (GTK_TABLE (table), gtk_label_new (_("frequency mapping")),
      0, 1, 0, 1, 0, 0, 3, 3);
  gtk_table_attach (GTK_TABLE (table), combo, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL,
      0, 3, 3);

  combo = gtk_combo_box_text_new ();
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("single"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("double"));
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), _("triple"));
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
  g_signal_connect (combo, "changed",
      G_CALLBACK (on_spectrum_frequency_precision_changed), (gpointer) self);
  gtk_table_attach (GTK_TABLE (table), gtk_label_new (_("spectrum precision")),
      0, 1, 1, 2, 0, 0, 3, 3);
  gtk_table_attach (GTK_TABLE (table), combo, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL,
      0, 3, 3);

  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  gtk_container_set_border_width (GTK_CONTAINER (self), 6);
  gtk_container_add (GTK_CONTAINER (self), vbox);

  /* TODO(ensonic): better error handling
   * - don't fail if we miss only spectrum or bt_child_proxy_set (self->priv->app, "level
   * - also don't return false, but instead add a label with the error message
   */

  // create fakesink
  if (!bt_signal_analysis_dialog_make_element (self, ANALYZER_FAKESINK,
          "fakesink")) {
    res = FALSE;
    goto Error;
  }
  g_object_set (self->priv->analyzers[ANALYZER_FAKESINK],
      "sync", FALSE, "qos", FALSE, "silent", TRUE, "async", FALSE, NULL);
  // create spectrum analyzer
  if (!bt_signal_analysis_dialog_make_element (self, ANALYZER_SPECTRUM,
          "spectrum")) {
    res = FALSE;
    goto Error;
  }
  g_object_set (self->priv->analyzers[ANALYZER_SPECTRUM], "interval",
      UPDATE_INTERVAL, "post-messages", TRUE, "bands",
      self->priv->spect_bands * self->priv->frq_precision, "threshold",
      SPECTRUM_FLOOR, "multi-channel", TRUE, NULL);
  if ((pad =
          gst_element_get_static_pad (self->priv->analyzers[ANALYZER_SPECTRUM],
              "sink"))) {
    g_signal_connect (pad, "notify::caps", G_CALLBACK (on_caps_negotiated),
        (gpointer) self);
    gst_object_unref (pad);
  }
  // create level meter
  if (!bt_signal_analysis_dialog_make_element (self, ANALYZER_LEVEL, "level")) {
    res = FALSE;
    goto Error;
  }
  g_object_set (self->priv->analyzers[ANALYZER_LEVEL],
      "interval", UPDATE_INTERVAL, "post-messages", TRUE,
      "peak-ttl", UPDATE_INTERVAL * 2, "peak-falloff", 80.0, NULL);
  // create queue
  if (!bt_signal_analysis_dialog_make_element (self, ANALYZER_QUEUE, "queue")) {
    res = FALSE;
    goto Error;
  }
  // leave "max-size-buffer >> 1, if 1 every buffer gets marked as discont!
  g_object_set (self->priv->analyzers[ANALYZER_QUEUE],
      "max-size-buffers", 10, "max-size-bytes", 0, "max-size-time",
      G_GUINT64_CONSTANT (0), "leaky", 2, "silent", TRUE, NULL);

  if (BT_IS_WIRE (self->priv->element)) {
    g_object_set (self->priv->element, "analyzers", self->priv->analyzers_list,
        NULL);
  } else if (BT_IS_SINK_MACHINE (self->priv->element)) {
    GstElement *machine;
    g_object_get (self->priv->element, "machine", &machine, NULL);
    g_object_set (machine, "analyzers", self->priv->analyzers_list, NULL);
    g_object_unref (machine);
  }

  g_object_get (song, "bin", &bin, NULL);
  self->priv->bus = gst_element_get_bus (GST_ELEMENT (bin));
  g_signal_connect (self->priv->bus, "sync-message::element",
      G_CALLBACK (on_signal_analyser_change), (gpointer) self);
  self->priv->clock = gst_pipeline_get_clock (GST_PIPELINE (bin));
  gst_object_unref (bin);

  // allocate visual ressources after the window has been realized
  g_signal_connect ((gpointer) self, "realize", G_CALLBACK (on_dialog_realize),
      (gpointer) self);
  // redraw when needed
  g_signal_connect (self->priv->level_drawingarea, "draw",
      G_CALLBACK (on_level_draw), (gpointer) self);
  g_signal_connect (self->priv->spectrum_drawingarea, "draw",
      G_CALLBACK (on_spectrum_draw), (gpointer) self);

Error:
  g_object_unref (song);
  g_object_unref (main_window);
  return (res);
}

//-- constructor methods

/**
 * bt_signal_analysis_dialog_new:
 * @element: the wire/machine object to create the dialog for
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtSignalAnalysisDialog *
bt_signal_analysis_dialog_new (const GstBin * element)
{
  BtSignalAnalysisDialog *self;

  self =
      BT_SIGNAL_ANALYSIS_DIALOG (g_object_new (BT_TYPE_SIGNAL_ANALYSIS_DIALOG,
          "element", element, NULL));
  // generate UI
  if (!bt_signal_analysis_dialog_init_ui (self)) {
    goto Error;
  }
  gtk_widget_show_all (GTK_WIDGET (self));
  GST_DEBUG ("dialog created and shown");
  return (self);
Error:
  gtk_widget_destroy (GTK_WIDGET (self));
  return (NULL);
}

//-- methods

//-- wrapper

//-- class internals

static void
bt_signal_analysis_dialog_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  BtSignalAnalysisDialog *self = BT_SIGNAL_ANALYSIS_DIALOG (object);
  return_if_disposed ();
  switch (property_id) {
    case SIGNAL_ANALYSIS_DIALOG_ELEMENT:
      g_object_try_unref (self->priv->element);
      self->priv->element = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_signal_analysis_dialog_dispose (GObject * object)
{
  BtSignalAnalysisDialog *self = BT_SIGNAL_ANALYSIS_DIALOG (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  /* DEBUG
     GST_DEBUG("levels: rms =%7.4lf .. %7.4lf",self->priv->min_rms ,self->priv->max_rms);
     GST_DEBUG("levels: peak=%7.4lf .. %7.4lf",self->priv->min_peak,self->priv->max_peak);
     // DEBUG */

  if (self->priv->clock)
    gst_object_unref (self->priv->clock);

  if (self->priv->spect_grad)
    cairo_pattern_destroy (self->priv->spect_grad);

  GST_DEBUG ("!!!! removing signal handler");

  if (self->priv->bus) {
    g_signal_handlers_disconnect_by_func (self->priv->bus,
        on_signal_analyser_change, self);
    gst_object_unref (self->priv->bus);
  }
  // this destroys the analyzers too
  GST_DEBUG ("!!!! free analyzers");
  if (BT_IS_WIRE (self->priv->element)) {
    g_object_set (self->priv->element, "analyzers", NULL, NULL);
  } else {
    bt_child_proxy_set (self->priv->element, "machine::analyzers", NULL, NULL);
  }

  g_object_unref (self->priv->element);
  g_object_unref (self->priv->app);

  GST_DEBUG ("!!!! done");

  G_OBJECT_CLASS (bt_signal_analysis_dialog_parent_class)->dispose (object);
}

static void
bt_signal_analysis_dialog_finalize (GObject * object)
{
  BtSignalAnalysisDialog *self = BT_SIGNAL_ANALYSIS_DIALOG (object);

  GST_DEBUG ("!!!! self=%p", self);

  g_free (self->priv->spect[0]);
  g_free (self->priv->spect[1]);
  g_free (self->priv->graph_log10);
  g_list_free (self->priv->analyzers_list);
  g_mutex_clear (&self->priv->lock);

  GST_DEBUG ("!!!! done");

  G_OBJECT_CLASS (bt_signal_analysis_dialog_parent_class)->finalize (object);
}

static void
bt_signal_analysis_dialog_init (BtSignalAnalysisDialog * self)
{
  gdouble *grid_log10;
  guint i;
  gdouble f, inc, end;

  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, BT_TYPE_SIGNAL_ANALYSIS_DIALOG,
      BtSignalAnalysisDialogPrivate);
  GST_DEBUG ("!!!! self=%p", self);
  self->priv->app = bt_edit_application_new ();

  g_mutex_init (&self->priv->lock);

  self->priv->spect_height = 64;
  self->priv->spect_bands = 256;
  self->priv->height_scale = 1.0;

  self->priv->frq_map = MAP_LIN;
  self->priv->frq_precision = 1;

  update_spectrum_analyzer (self);

  self->priv->srate = GST_AUDIO_DEF_RATE;

  /* precalc some log10 values */
  grid_log10 = self->priv->grid_log10;
  i = 0;
  f = 1.0;
  inc = 1.0;
  end = 10.0;
  while (i < 60) {
    grid_log10[i++] = log10 (1.0 + f);
    f += inc;
    if (f >= end) {
      f = inc = end;
      end *= 10.0;
    }
  }
}

static void
bt_signal_analysis_dialog_class_init (BtSignalAnalysisDialogClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  bus_msg_level_quark = g_quark_from_static_string ("level");
  bus_msg_spectrum_quark = g_quark_from_static_string ("spectrum");

  g_type_class_add_private (klass, sizeof (BtSignalAnalysisDialogPrivate));

  gobject_class->set_property = bt_signal_analysis_dialog_set_property;
  gobject_class->dispose = bt_signal_analysis_dialog_dispose;
  gobject_class->finalize = bt_signal_analysis_dialog_finalize;

  g_object_class_install_property (gobject_class,
      SIGNAL_ANALYSIS_DIALOG_ELEMENT, g_param_spec_object ("element",
          "element construct prop",
          "Set wire/machine object, the dialog handles", GST_TYPE_BIN,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

}
