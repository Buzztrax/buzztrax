/* GtkVumeter
 * Copyright (C) 2003 Todd Goyen <wettoad@knighthoodofbuh.org>
 *               2007-2009 Buzztrax team <buzztrax-devel@buzztrax.org>
 *               2008 Frederic Peters <fpeters@0d.be>
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
/* TODO(ensonic): add properties:
 *   - min,max,rms,peak : gint, read/write
 *   - scale_type : enum, read/write
 *   - led-size : int, read/write, 1=no-leds, >1 led spacing, def=5
 */
/* TODO(ensonic): revisit cairo usage
 *   - http://www.cairographics.org/FAQ/#sharp_lines
 */
/* TODO(ensonic): one needs to swap the levels in gtk_vumeter_set_levels()
 *   to actually see the two shaded parts. Otherwise the decay is alway less then
 *   peak and thus now shown.
 */
/* TODO(ensonic): perfect sync for drawing
 * - GdkFrameClock or gtk_widget_add_tick_callback()
 *   needs Gnome-3.8 (gtk, clutter, mutter, cogl)
 * - irc-chat:
 *   - you typically have almost 32ms latency in the compositor case between the
 *     point where you are told to render a frame and the point where it scans out
 *   - gdk_frame_timings_get_predicted_presentation_time() gives you a time 
 *     (relative to g_get-monotonic_time()) when a frame will display
 */
/**
 * SECTION:gtkvumeter
 * @short_description: vu meter widget
 *
 * Shows a vertical or horizontal gauge with a colorized bar to signal volume
 * level.
 */

#include <math.h>

#include "gtkvumeter.h"

enum
{
  PROP_0,
  PROP_ORIENTATION
};

#define MIN_HORIZONTAL_VUMETER_WIDTH   30
#define HORIZONTAL_VUMETER_HEIGHT  6
#define VERTICAL_VUMETER_WIDTH     6
#define MIN_VERTICAL_VUMETER_HEIGHT    30
#define LED_SIZE 5

static void gtk_vumeter_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gtk_vumeter_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gtk_vumeter_finalize (GObject * object);

static void gtk_vumeter_measure (GtkWidget* widget, GtkOrientation orientation,
    int for_size, int* minimum, int* natural, int* minimum_baseline,
    int* natural_baseline);
static void gtk_vumeter_snapshot (GtkWidget * widget, GtkSnapshot * snapshot);

//-- the class

G_DEFINE_TYPE_WITH_CODE (GtkVUMeter, gtk_vumeter, GTK_TYPE_WIDGET,
    G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE, NULL));

/**
 * gtk_vumeter_new:
 * @orientation: vertical/horizontal
 *
 * Creates a new VUMeter widget.
 *
 * Returns: the new #GtkWidget
 */
GtkWidget *
gtk_vumeter_new (GtkOrientation orientation)
{
  return g_object_new (GTK_TYPE_VUMETER, "orientation", orientation, NULL);
}

static void
gtk_vumeter_init (GtkVUMeter * self)
{
  GtkStyleContext *context;

  self->orientation = GTK_ORIENTATION_HORIZONTAL;

  self->rms_level = 0;
  self->min = 0;
  self->max = 32767;
  self->peak_level = 0;
  self->delay_peak_level = 0;

  self->scale = GTK_VUMETER_SCALE_LINEAR;

#if 0 /// GTK4
  context = gtk_widget_get_style_context (GTK_WIDGET (self));
  gtk_style_context_add_class (context, GTK_STYLE_CLASS_FRAME);
  gtk_style_context_add_class (context, GTK_STYLE_CLASS_TROUGH);

  gtk_widget_set_has_window (GTK_WIDGET (self), FALSE);
#endif
}

static void
gtk_vumeter_class_init (GtkVUMeterClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GtkWidgetClass *widget_class = (GtkWidgetClass *) klass;

  gobject_class->set_property = gtk_vumeter_set_property;
  gobject_class->get_property = gtk_vumeter_get_property;
  gobject_class->finalize = gtk_vumeter_finalize;

  widget_class->snapshot = gtk_vumeter_snapshot;
  widget_class->measure = gtk_vumeter_measure;

  g_object_class_override_property (gobject_class, PROP_ORIENTATION,
      "orientation");
}

static void
gtk_vumeter_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GtkVUMeter *self = GTK_VUMETER (object);

  switch (prop_id) {
    case PROP_ORIENTATION:
      self->orientation = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gtk_vumeter_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GtkVUMeter *self = GTK_VUMETER (object);

  switch (prop_id) {
    case PROP_ORIENTATION:
      g_value_set_enum (value, self->orientation);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gtk_vumeter_finalize (GObject * object)
{
  GtkVUMeter *self = GTK_VUMETER (object);

  /* free old gradients */
  if (self->gradient_rms)
    cairo_pattern_destroy (self->gradient_rms);
  if (self->gradient_peak)
    cairo_pattern_destroy (self->gradient_peak);
  if (self->gradient_bg)
    cairo_pattern_destroy (self->gradient_bg);

  G_OBJECT_CLASS (gtk_vumeter_parent_class)->finalize (object);
}

static void
gtk_vumeter_allocate_colors (GtkVUMeter * self)
{
  cairo_pattern_t *gradient;
  gint width, height;
  gdouble pos[3];

  /* free old gradients */
  if (self->gradient_rms)
    cairo_pattern_destroy (self->gradient_rms);
  if (self->gradient_peak)
    cairo_pattern_destroy (self->gradient_peak);
  if (self->gradient_bg)
    cairo_pattern_destroy (self->gradient_bg);

  if (self->orientation == GTK_ORIENTATION_VERTICAL) {
    height = gtk_widget_get_allocated_height ((GtkWidget *) self) - 1;
    width = 1;
    pos[0] = 1.0;
    pos[1] = 0.7;
    pos[2] = 0.0;
  } else {
    height = 1;
    width = gtk_widget_get_allocated_width ((GtkWidget *) self) - 1;
    pos[0] = 0.0;
    pos[1] = 0.7;
    pos[2] = 1.0;
  }

  /* setup gradients */
  gradient = cairo_pattern_create_linear (1, 1, width, height);
  cairo_pattern_add_color_stop_rgb (gradient, pos[0], 0.0, 1.0, 0.0);
  cairo_pattern_add_color_stop_rgb (gradient, pos[1], 1.0, 1.0, 0.0);
  cairo_pattern_add_color_stop_rgb (gradient, pos[2], 1.0, 0.0, 0.0);
  self->gradient_rms = gradient;

  gradient = cairo_pattern_create_linear (1, 1, width, height);
  cairo_pattern_add_color_stop_rgb (gradient, pos[0], 0.0, 0.6, 0.0);
  cairo_pattern_add_color_stop_rgb (gradient, pos[1], 0.6, 0.6, 0.0);
  cairo_pattern_add_color_stop_rgb (gradient, pos[2], 0.6, 0.0, 0.0);
  self->gradient_peak = gradient;

  gradient = cairo_pattern_create_linear (1, 1, width, height);
  cairo_pattern_add_color_stop_rgb (gradient, pos[0], 0.0, 0.3, 0.0);
  cairo_pattern_add_color_stop_rgb (gradient, pos[1], 0.3, 0.3, 0.0);
  cairo_pattern_add_color_stop_rgb (gradient, pos[2], 0.3, 0.0, 0.0);
  self->gradient_bg = gradient;
}

static void
gtk_vumeter_measure (GtkWidget* widget, GtkOrientation orientation,
    int for_size, int* minimum, int* natural, int* minimum_baseline,
    int* natural_baseline)
{
  GtkVUMeter *self = GTK_VUMETER (widget);
  gint border_padding = self->border.left + self->border.right;

  if (orientation == GTK_ORIENTATION_VERTICAL) {
    *minimum = *natural = VERTICAL_VUMETER_WIDTH + border_padding;
  } else {
    *minimum = *natural = HORIZONTAL_VUMETER_HEIGHT + border_padding;
  }
}

static gint
gtk_vumeter_sound_level_to_draw_level (GtkVUMeter * self, gint sound_level,
    gdouble length)
{
  gdouble draw_level;
  gdouble level, min, max;

  level = (gdouble) sound_level;
  min = (gdouble) self->min;
  max = (gdouble) self->max;

  if (self->scale == GTK_VUMETER_SCALE_LINEAR) {
    draw_level = (level - min) / (max - min) * length;
  } else {
    gdouble log_level, log_max;

    log_level = log10 ((level - min + 1) / (max - min + 1));
    log_max = log10 (1 / (max - min + 1));      /* FIXME(ensonic): could be cached */
    draw_level = length - log_level / log_max * length;
  }

  return (gint) draw_level;
}

static void
gtk_vumeter_snapshot (GtkWidget * widget, GtkSnapshot * snapshot)
{
  GtkVUMeter *self = GTK_VUMETER (widget);
  GtkStyleContext *context;
  gint rms_level, peak_level;
  gint width, height;
  gdouble left, top;
  guint i;

  width = gtk_widget_get_width (widget);
  height = gtk_widget_get_height (widget);

  cairo_t *cr = gtk_snapshot_append_cairo (snapshot,
      &GRAPHENE_RECT_INIT (0, 0, width, height));
  
  context = gtk_widget_get_style_context (widget);

  /* draw border */
  gtk_render_background (context, cr, 0, 0, width, height);
  gtk_render_frame (context, cr, 0, 0, width, height);

  left = self->border.left;
  top = self->border.top;
  width -= self->border.left + self->border.right;
  height -= self->border.top + self->border.bottom;

  if (self->orientation == GTK_ORIENTATION_VERTICAL) {
    gdouble bottom = top + height;

    rms_level = LED_SIZE * (gtk_vumeter_sound_level_to_draw_level (self,
            self->rms_level, height) / LED_SIZE);
    peak_level = LED_SIZE * (gtk_vumeter_sound_level_to_draw_level (self,
            self->peak_level, height) / LED_SIZE);

    /* draw normal level */
    cairo_set_source (cr, self->gradient_rms);
    cairo_rectangle (cr, left, bottom - rms_level, width, rms_level);
    cairo_fill (cr);

    /* draw peak */
    if (peak_level > rms_level) {
      cairo_set_source (cr, self->gradient_peak);
      cairo_rectangle (cr, left, bottom - peak_level, width,
          peak_level - rms_level);
      cairo_fill (cr);
    }

    /* draw background for the rest */
    if (height > peak_level) {
      cairo_set_source (cr, self->gradient_bg);
      cairo_rectangle (cr, left, top, width, height - peak_level);
      cairo_fill (cr);
    }

    /* shade every LED_SIZE-th line */
    cairo_set_source_rgba (cr, 0, 0, 0, 0.5);
    cairo_set_line_width (cr, 1.0);
    for (i = 0; i < height; i += LED_SIZE) {
      cairo_move_to (cr, left, top + 0.5 + i);
      cairo_line_to (cr, left + width, top + 0.5 + i);
    }
    cairo_stroke (cr);

  } else {                      /* Horizontal */
    rms_level = LED_SIZE * (gtk_vumeter_sound_level_to_draw_level (self,
            self->rms_level, width) / LED_SIZE);
    peak_level = LED_SIZE * (gtk_vumeter_sound_level_to_draw_level (self,
            self->peak_level, width) / LED_SIZE);

    /* draw normal level */
    cairo_set_source (cr, self->gradient_rms);
    cairo_rectangle (cr, left, top, rms_level, height);
    cairo_fill (cr);

    /* draw peak */
    if (peak_level > rms_level) {
      cairo_set_source (cr, self->gradient_peak);
      cairo_rectangle (cr, left + rms_level, top, peak_level - rms_level,
          height);
      cairo_fill (cr);
    }

    /* draw background for the rest */
    if (width > peak_level) {
      cairo_set_source (cr, self->gradient_bg);
      cairo_rectangle (cr, left + peak_level, top, width - peak_level, height);
      cairo_fill (cr);
    }

    /* shade every LED_SIZE-th line */
    cairo_set_source_rgba (cr, 0, 0, 0, 0.5);
    cairo_set_line_width (cr, 1.0);
    for (i = 0; i < width; i += LED_SIZE) {
      cairo_move_to (cr, left + 0.5 + i, top);
      cairo_line_to (cr, left + 0.5 + i, top + height);
    }
    cairo_stroke (cr);
  }

  cairo_destroy (cr);
}

/**
 * gtk_vumeter_set_min_max:
 * @self: the vumeter widget to change the level bounds
 * @min: the new minimum level shown (level that is 0%)
 * @max: the new maximum level shown (level that is 100%)
 *
 * Sets the minimum and maximum of the VU Meters scale.
 * The positions are irrelevant as the widget will rearrange them.
 * It will also increment max by one if min == max.
 * And finally it will clamp the current level into the min,max range.
 */
void
gtk_vumeter_set_min_max (GtkVUMeter * self, gint min, gint max)
{
  gint old_rms_level = self->rms_level;
  gint old_peak_level = self->peak_level;

  g_return_if_fail (GTK_IS_VUMETER (self));

  self->max = MAX (max, min);
  self->min = MIN (min, max);
  if (self->max == self->min) {
    self->max++;
  }
  self->rms_level = CLAMP (self->rms_level, self->min, self->max);
  self->peak_level = CLAMP (self->peak_level, self->min, self->max);

  if ((old_rms_level != self->rms_level)
      || (old_peak_level != self->peak_level)) {
    gtk_widget_queue_draw (GTK_WIDGET (self));
  }
}

/**
 * gtk_vumeter_set_levels:
 * @self: the vumeter widget to change the current level
 * @rms: the new rms level shown
 * @peak: the new peak level shown
 *
 * Sets new level values for the level display. The @peak level is the current max
 * level. The @rmx level is the decaying level part.
 * They are clamped to the min max range.
 */
void
gtk_vumeter_set_levels (GtkVUMeter * self, gint rms, gint peak)
{
  gint old_rms_level = self->rms_level;
  gint old_peak_level = self->peak_level;

  g_return_if_fail (GTK_IS_VUMETER (self));

  self->rms_level = CLAMP (rms, self->min, self->max);
  self->peak_level = CLAMP (peak, self->min, self->max);

  if ((old_rms_level != self->rms_level)
      || (old_peak_level != self->peak_level)) {
    gtk_widget_queue_draw (GTK_WIDGET (self));
  }
}

/**
 * gtk_vumeter_set_scale:
 * @self: the vumeter widget to change the scaling type
 * @scale: the scale type, either GTK_VUMETER_SCALE_LINEAR or GTK_VUMETER_SCALE_LOG
 *
 * Sets the scale of the VU Meter.
 * It is either log or linear and defaults to linear.
 * No matter which scale you set the input should always be linear, #GtkVUMeter
 * does the log calculation. 0db is red. -6db is yellow. -18db is green.
 * Whatever min turns into is dark green.
 */
void
gtk_vumeter_set_scale (GtkVUMeter * self, gint scale)
{
  g_return_if_fail (GTK_IS_VUMETER (self));

  if (scale != self->scale) {
    self->scale =
        CLAMP (scale, GTK_VUMETER_SCALE_LINEAR, GTK_VUMETER_SCALE_LAST - 1);
    if (gtk_widget_get_realized (GTK_WIDGET (self))) {
      gtk_widget_queue_draw (GTK_WIDGET (self));
    }
  }
}
