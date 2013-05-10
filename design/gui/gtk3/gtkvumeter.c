/* GtkVumeter
 * Copyright (C) 2003 Todd Goyen <wettoad@knighthoodofbuh.org>
 *               2007-2009 Buzztrax team <buzztrax-devel@lists.sf.net>
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

#define MIN_HORIZONTAL_VUMETER_WIDTH   150
#define HORIZONTAL_VUMETER_HEIGHT  6
#define VERTICAL_VUMETER_WIDTH     6
#define MIN_VERTICAL_VUMETER_HEIGHT    400
#define LED_SIZE 5

static void gtk_vumeter_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gtk_vumeter_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gtk_vumeter_finalize (GObject * object);

static void gtk_vumeter_get_preferred_width (GtkWidget * widget,
    gint * minimal_width, gint * natural_width);
static void gtk_vumeter_get_preferred_height (GtkWidget * widget,
    gint * minimal_height, gint * natural_height);
static gboolean gtk_vumeter_draw (GtkWidget * widget, cairo_t * cr);
static void gtk_vumeter_size_allocate (GtkWidget * widget,
    GtkAllocation * allocation);

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
gtk_vumeter_init (GtkVUMeter * vumeter)
{
  GtkStyleContext *context;

  vumeter->orientation = GTK_ORIENTATION_HORIZONTAL;

  vumeter->rms_level = 0;
  vumeter->min = 0;
  vumeter->max = 32767;
  vumeter->peak_level = 0;
  vumeter->delay_peak_level = 0;

  vumeter->scale = GTK_VUMETER_SCALE_LINEAR;

  context = gtk_widget_get_style_context (GTK_WIDGET (vumeter));
  gtk_style_context_add_class (context, GTK_STYLE_CLASS_FRAME);

  gtk_widget_set_has_window (GTK_WIDGET (vumeter), FALSE);
}

static void
gtk_vumeter_class_init (GtkVUMeterClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GtkWidgetClass *widget_class = (GtkWidgetClass *) klass;

  gobject_class->set_property = gtk_vumeter_set_property;
  gobject_class->get_property = gtk_vumeter_get_property;
  gobject_class->finalize = gtk_vumeter_finalize;

  widget_class->draw = gtk_vumeter_draw;
  widget_class->get_preferred_width = gtk_vumeter_get_preferred_width;
  widget_class->get_preferred_height = gtk_vumeter_get_preferred_height;
  widget_class->size_allocate = gtk_vumeter_size_allocate;

  g_object_class_override_property (gobject_class, PROP_ORIENTATION,
      "orientation");
}

static void
gtk_vumeter_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GtkVUMeter *vumeter = GTK_VUMETER (object);

  switch (prop_id) {
    case PROP_ORIENTATION:
      vumeter->orientation = g_value_get_enum (value);
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
  GtkVUMeter *vumeter = GTK_VUMETER (object);

  switch (prop_id) {
    case PROP_ORIENTATION:
      g_value_set_enum (value, vumeter->orientation);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gtk_vumeter_finalize (GObject * object)
{
  GtkVUMeter *vumeter = GTK_VUMETER (object);

  /* free old gradients */
  if (vumeter->gradient_rms)
    cairo_pattern_destroy (vumeter->gradient_rms);
  if (vumeter->gradient_peak)
    cairo_pattern_destroy (vumeter->gradient_peak);
  if (vumeter->gradient_bg)
    cairo_pattern_destroy (vumeter->gradient_bg);

  G_OBJECT_CLASS (gtk_vumeter_parent_class)->finalize (object);
}

static void
gtk_vumeter_allocate_colors (GtkVUMeter * vumeter)
{
  cairo_pattern_t *gradient;
  gint width, height;

  /* free old gradients */
  if (vumeter->gradient_rms)
    cairo_pattern_destroy (vumeter->gradient_rms);
  if (vumeter->gradient_peak)
    cairo_pattern_destroy (vumeter->gradient_peak);
  if (vumeter->gradient_bg)
    cairo_pattern_destroy (vumeter->gradient_bg);

  if (vumeter->orientation == GTK_ORIENTATION_VERTICAL) {
    height = gtk_widget_get_allocated_height ((GtkWidget *) vumeter) - 1;
    width = 1;
  } else {
    height = 1;
    width = gtk_widget_get_allocated_width ((GtkWidget *) vumeter) - 1;
  }

  /* setup gradients */
  gradient = cairo_pattern_create_linear (1, 1, width, height);
  cairo_pattern_add_color_stop_rgb (gradient, 0.0, 0.0, 1.0, 0.0);
  cairo_pattern_add_color_stop_rgb (gradient, 0.7, 1.0, 1.0, 0.0);
  cairo_pattern_add_color_stop_rgb (gradient, 1.0, 1.0, 0.0, 0.0);
  vumeter->gradient_rms = gradient;

  gradient = cairo_pattern_create_linear (1, 1, width, height);
  cairo_pattern_add_color_stop_rgb (gradient, 0.0, 0.0, 0.6, 0.0);
  cairo_pattern_add_color_stop_rgb (gradient, 0.7, 0.6, 0.6, 0.0);
  cairo_pattern_add_color_stop_rgb (gradient, 1.0, 0.6, 0.0, 0.0);
  vumeter->gradient_peak = gradient;

  gradient = cairo_pattern_create_linear (1, 1, width, height);
  cairo_pattern_add_color_stop_rgb (gradient, 0.0, 0.0, 0.3, 0.0);
  cairo_pattern_add_color_stop_rgb (gradient, 0.7, 0.3, 0.3, 0.0);
  cairo_pattern_add_color_stop_rgb (gradient, 1.0, 0.3, 0.0, 0.0);
  vumeter->gradient_bg = gradient;
}

static void
gtk_vumeter_get_preferred_width (GtkWidget * widget, gint * minimal_width,
    gint * natural_width)
{
  GtkVUMeter *vumeter = GTK_VUMETER (widget);
  gint border_padding = vumeter->border.left + vumeter->border.right;

  if (vumeter->orientation == GTK_ORIENTATION_VERTICAL) {
    *minimal_width = *natural_width = VERTICAL_VUMETER_WIDTH + border_padding;
  } else {
    *minimal_width = *natural_width =
        MIN_HORIZONTAL_VUMETER_WIDTH + border_padding;
  }
}

static void
gtk_vumeter_get_preferred_height (GtkWidget * widget, gint * minimal_height,
    gint * natural_height)
{
  GtkVUMeter *vumeter = GTK_VUMETER (widget);
  gint border_padding = vumeter->border.top + vumeter->border.bottom;

  if (vumeter->orientation == GTK_ORIENTATION_VERTICAL) {
    *minimal_height = *natural_height =
        MIN_VERTICAL_VUMETER_HEIGHT + border_padding;
  } else {
    *minimal_height = *natural_height =
        HORIZONTAL_VUMETER_HEIGHT + border_padding;
  }
}

static gint
gtk_vumeter_sound_level_to_draw_level (GtkVUMeter * vumeter,
    gint sound_level, gdouble length)
{
  gdouble draw_level;
  gdouble level, min, max;

  level = (gdouble) sound_level;
  min = (gdouble) vumeter->min;
  max = (gdouble) vumeter->max;

  if (vumeter->scale == GTK_VUMETER_SCALE_LINEAR) {
    draw_level = (level - min) / (max - min) * length;
  } else {
    gdouble log_level, log_max;

    log_level = log10 ((level - min + 1) / (max - min + 1));
    log_max = log10 (1 / (max - min + 1));      /* FIXME(ensonic): could be cached */
    draw_level = length - log_level / log_max * length;
  }

  return (gint) draw_level;
}

static gboolean
gtk_vumeter_draw (GtkWidget * widget, cairo_t * cr)
{
  GtkVUMeter *vumeter = GTK_VUMETER (widget);
  GtkStyleContext *context;
  gint rms_level, peak_level;
  gint width, height;
  gdouble left, top;
  guint i;

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);
  context = gtk_widget_get_style_context (widget);

  /* draw border */
  gtk_render_background (context, cr, 0, 0, width, height);
  gtk_render_frame (context, cr, 0, 0, width, height);

  left = vumeter->border.left;
  top = vumeter->border.top;
  width -= vumeter->border.left + vumeter->border.right;
  height -= vumeter->border.top + vumeter->border.bottom;

  if (vumeter->orientation == GTK_ORIENTATION_VERTICAL) {
    rms_level = LED_SIZE * (gtk_vumeter_sound_level_to_draw_level (vumeter,
            vumeter->rms_level, height) / LED_SIZE);
    peak_level = LED_SIZE * (gtk_vumeter_sound_level_to_draw_level (vumeter,
            vumeter->peak_level, height) / LED_SIZE);

    /* draw normal level */
    cairo_set_source (cr, vumeter->gradient_rms);
    cairo_rectangle (cr, left, top, width, rms_level);
    cairo_fill (cr);

    /* draw peak */
    if (peak_level > rms_level) {
      cairo_set_source (cr, vumeter->gradient_peak);
      cairo_rectangle (cr, left, top + rms_level, width,
          peak_level - rms_level);
      cairo_fill (cr);
    }

    /* draw background for the rest */
    if (height > peak_level) {
      cairo_set_source (cr, vumeter->gradient_bg);
      cairo_rectangle (cr, left, top + peak_level, width, height - peak_level);
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
    rms_level = LED_SIZE * (gtk_vumeter_sound_level_to_draw_level (vumeter,
            vumeter->rms_level, width) / LED_SIZE);
    peak_level = LED_SIZE * (gtk_vumeter_sound_level_to_draw_level (vumeter,
            vumeter->peak_level, width) / LED_SIZE);

    /* draw normal level */
    cairo_set_source (cr, vumeter->gradient_rms);
    cairo_rectangle (cr, left, top, rms_level, height);
    cairo_fill (cr);

    /* draw peak */
    if (peak_level > rms_level) {
      cairo_set_source (cr, vumeter->gradient_peak);
      cairo_rectangle (cr, left + rms_level, top, peak_level - rms_level,
          height);
      cairo_fill (cr);
    }

    /* draw background for the rest */
    if (width > peak_level) {
      cairo_set_source (cr, vumeter->gradient_bg);
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

  return FALSE;
}

static void
gtk_vumeter_size_allocate (GtkWidget * widget, GtkAllocation * allocation)
{
  GtkVUMeter *vumeter = GTK_VUMETER (widget);
  GtkStyleContext *context;
  GtkStateFlags state;

  gtk_widget_set_allocation (widget, allocation);

  context = gtk_widget_get_style_context (widget);
  state = gtk_widget_get_state_flags (widget);
  gtk_style_context_get_padding (context, state, &vumeter->border);

  gtk_vumeter_allocate_colors (vumeter);
}

/**
 * gtk_vumeter_set_min_max:
 * @vumeter: the vumeter widget to change the level bounds
 * @min: the new minimum level shown (level that is 0%)
 * @max: the new maximum level shown (level that is 100%)
 *
 * Sets the minimum and maximum of the VU Meters scale.
 * The positions are irrelevant as the widget will rearrange them.
 * It will also increment max by one if min == max.
 * And finally it will clamp the current level into the min,max range.
 */
void
gtk_vumeter_set_min_max (GtkVUMeter * vumeter, gint min, gint max)
{
  gint old_rms_level = vumeter->rms_level;
  gint old_peak_level = vumeter->peak_level;

  g_return_if_fail (GTK_IS_VUMETER (vumeter));

  vumeter->max = MAX (max, min);
  vumeter->min = MIN (min, max);
  if (vumeter->max == vumeter->min) {
    vumeter->max++;
  }
  vumeter->rms_level = CLAMP (vumeter->rms_level, vumeter->min, vumeter->max);
  vumeter->peak_level = CLAMP (vumeter->peak_level, vumeter->min, vumeter->max);

  if ((old_rms_level != vumeter->rms_level)
      || (old_peak_level != vumeter->peak_level)) {
    gtk_widget_queue_draw (GTK_WIDGET (vumeter));
  }
}

/**
 * gtk_vumeter_set_levels:
 * @vumeter: the vumeter widget to change the current level
 * @rms: the new rms level shown
 * @peak: the new peak level shown
 *
 * Sets new level values for the level display. The @peak level is the current max
 * level. The @rmx level is the decaying level part.
 * They are clamped to the min max range.
 */
void
gtk_vumeter_set_levels (GtkVUMeter * vumeter, gint rms, gint peak)
{
  gint old_rms_level = vumeter->rms_level;
  gint old_peak_level = vumeter->peak_level;

  g_return_if_fail (GTK_IS_VUMETER (vumeter));

  vumeter->rms_level = CLAMP (rms, vumeter->min, vumeter->max);
  vumeter->peak_level = CLAMP (peak, vumeter->min, vumeter->max);

  if ((old_rms_level != vumeter->rms_level)
      || (old_peak_level != vumeter->peak_level)) {
    gtk_widget_queue_draw (GTK_WIDGET (vumeter));
  }
}

/**
 * gtk_vumeter_set_scale:
 * @vumeter: the vumeter widget to change the scaling type
 * @scale: the scale type, either GTK_VUMETER_SCALE_LINEAR or GTK_VUMETER_SCALE_LOG
 *
 * Sets the scale of the VU Meter.
 * It is either log or linear and defaults to linear.
 * No matter which scale you set the input should always be linear, #GtkVUMeter
 * does the log calculation. 0db is red. -6db is yellow. -18db is green.
 * Whatever min turns into is dark green.
 */
void
gtk_vumeter_set_scale (GtkVUMeter * vumeter, gint scale)
{
  g_return_if_fail (GTK_IS_VUMETER (vumeter));

  if (scale != vumeter->scale) {
    vumeter->scale =
        CLAMP (scale, GTK_VUMETER_SCALE_LINEAR, GTK_VUMETER_SCALE_LAST - 1);
    if (gtk_widget_get_realized (GTK_WIDGET (vumeter))) {
      gtk_widget_queue_draw (GTK_WIDGET (vumeter));
    }
  }
}
