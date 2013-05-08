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
 *   - vertical : gboolean, readonly
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

#define MIN_HORIZONTAL_VUMETER_WIDTH   150
#define HORIZONTAL_VUMETER_HEIGHT  6
#define VERTICAL_VUMETER_WIDTH     6
#define MIN_VERTICAL_VUMETER_HEIGHT    400

static void gtk_vumeter_finalize (GObject * object);

static void gtk_vumeter_realize (GtkWidget * widget);
static void gtk_vumeter_unrealize (GtkWidget * widget);
static void gtk_vumeter_map (GtkWidget * widget);
static void gtk_vumeter_unmap (GtkWidget * widget);
static void gtk_vumeter_get_preferred_width (GtkWidget * widget,
    gint * minimal_width, gint * natural_width);
static void gtk_vumeter_get_preferred_height (GtkWidget * widget,
    gint * minimal_height, gint * natural_height);

static void gtk_vumeter_size_allocate (GtkWidget * widget,
    GtkAllocation * allocation);
static gboolean gtk_vumeter_draw (GtkWidget * widget, cairo_t * cr);

//-- the class

G_DEFINE_TYPE (GtkVUMeter, gtk_vumeter, GTK_TYPE_WIDGET);


/**
 * gtk_vumeter_new:
 * @vertical: %TRUE for a vertical VUMeter, %FALSE for horizontal VUMeter.
 *
 * Creates a new VUMeter widget.
 *
 * Returns: the new #GtkWidget
 */
GtkWidget *
gtk_vumeter_new (gboolean vertical)
{
  GtkVUMeter *vumeter;
  vumeter = GTK_VUMETER (g_object_new (GTK_TYPE_VUMETER, NULL));
  vumeter->vertical = vertical;
  return GTK_WIDGET (vumeter);
}

static void
gtk_vumeter_init (GtkVUMeter * vumeter)
{
  GtkStyleContext *context;

  vumeter->rms_level = 0;
  vumeter->min = 0;
  vumeter->max = 32767;
  vumeter->peak_level = 0;
  vumeter->delay_peak_level = 0;

  vumeter->scale = GTK_VUMETER_SCALE_LINEAR;

  context = gtk_widget_get_style_context (GTK_WIDGET (vumeter));
  gtk_style_context_add_class (context, GTK_STYLE_CLASS_FRAME);
}

static void
gtk_vumeter_class_init (GtkVUMeterClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GtkWidgetClass *widget_class = (GtkWidgetClass *) klass;

  gobject_class->finalize = gtk_vumeter_finalize;

  widget_class->realize = gtk_vumeter_realize;
  widget_class->unrealize = gtk_vumeter_unrealize;
  widget_class->map = gtk_vumeter_map;
  widget_class->unmap = gtk_vumeter_unmap;
  widget_class->draw = gtk_vumeter_draw;
  widget_class->size_allocate = gtk_vumeter_size_allocate;

  widget_class->get_preferred_width = gtk_vumeter_get_preferred_width;
  widget_class->get_preferred_height = gtk_vumeter_get_preferred_height;
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

  if (vumeter->vertical) {      /* veritcal */
    height = gtk_widget_get_allocated_height ((GtkWidget *) vumeter) - 1;
    width = 1;
  } else {                      /* horizontal */
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
gtk_vumeter_realize (GtkWidget * widget)
{
  GtkVUMeter *vumeter = GTK_VUMETER (widget);
  GdkWindow *parent_window;
  GdkWindowAttr attributes;
  gint attributes_mask;
  GtkAllocation allocation;

  gtk_widget_set_realized (widget, TRUE);
  parent_window = gtk_widget_get_parent_window (widget);
  gtk_widget_set_window (widget, parent_window);
  g_object_ref (parent_window);

  gtk_widget_get_allocation (widget, &allocation);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.event_mask = gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK;
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;

  vumeter->window = gdk_window_new (parent_window, &attributes,
      attributes_mask);
  //gtk_widget_register_window (widget, vumeter->window);
  gdk_window_set_user_data (vumeter->window, widget);

  gtk_vumeter_allocate_colors (vumeter);

  printf ("realized\n");
}

static void
gtk_vumeter_unrealize (GtkWidget * widget)
{
  GtkVUMeter *vumeter = GTK_VUMETER (widget);

  if (vumeter->window != NULL) {
    //gtk_widget_unregister_window (widget, priv->event_window);
    gdk_window_set_user_data (vumeter->window, NULL);
    gdk_window_destroy (vumeter->window);
    vumeter->window = NULL;
  }

  GTK_WIDGET_CLASS (gtk_vumeter_parent_class)->unrealize (widget);
}

static void
gtk_vumeter_map (GtkWidget * widget)
{
  GtkVUMeter *vumeter = GTK_VUMETER (widget);

  GTK_WIDGET_CLASS (gtk_vumeter_parent_class)->map (widget);

  if (vumeter->window) {
    gdk_window_show (vumeter->window);
    printf ("shown\n");
  }
}

static void
gtk_vumeter_unmap (GtkWidget * widget)
{
  GtkVUMeter *vumeter = GTK_VUMETER (widget);

  if (vumeter->window) {
    gdk_window_hide (vumeter->window);
    printf ("hidden\n");
  }

  GTK_WIDGET_CLASS (gtk_vumeter_parent_class)->unmap (widget);
}


static void
gtk_vumeter_size_request (GtkWidget * widget, GtkRequisition * requisition)
{
  GtkVUMeter *vumeter = GTK_VUMETER (widget);

  if (vumeter->vertical) {
    requisition->width = VERTICAL_VUMETER_WIDTH;
    requisition->height = MIN_VERTICAL_VUMETER_HEIGHT;
  } else {
    requisition->width = MIN_HORIZONTAL_VUMETER_WIDTH;
    requisition->height = HORIZONTAL_VUMETER_HEIGHT;
  }
}

static void
gtk_vumeter_get_preferred_width (GtkWidget * widget, gint * minimal_width,
    gint * natural_width)
{
  GtkRequisition requisition;

  gtk_vumeter_size_request (widget, &requisition);
  *minimal_width = *natural_width = requisition.width;

  printf ("preferred_width: %d\n", *natural_width);
}

static void
gtk_vumeter_get_preferred_height (GtkWidget * widget, gint * minimal_height,
    gint * natural_height)
{
  GtkRequisition requisition;

  gtk_vumeter_size_request (widget, &requisition);
  *minimal_height = *natural_height = requisition.height;

  printf ("preferred_height: %d\n", *natural_height);
}


static void
gtk_vumeter_size_allocate (GtkWidget * widget, GtkAllocation * allocation)
{
  GtkVUMeter *vumeter = GTK_VUMETER (widget);

  gtk_widget_set_allocation (widget, allocation);
  printf ("x,y: %d,%d, w,h: %d,%d\n", allocation->x, allocation->y,
      allocation->width, allocation->height);

  if (gtk_widget_get_realized (widget)) {
    gdk_window_move_resize (vumeter->window, allocation->x, allocation->y,
        allocation->width, allocation->height);
  }
  gtk_vumeter_allocate_colors (GTK_VUMETER (widget));
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
  GtkVUMeter *vumeter;
  GtkStyleContext *context;
  gint rms_level, peak_level;
  gint width, height;
  guint i;

  g_return_val_if_fail (GTK_IS_VUMETER (widget), FALSE);

  vumeter = GTK_VUMETER (widget);
  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);
  context = gtk_widget_get_style_context (widget);

  printf ("draw: w,h: %d,%d\n", width, height);

  /* draw border */
  gtk_render_background (context, cr, 0, 0, width, height);
  gtk_render_frame (context, cr, 0, 0, width, height);

  if (vumeter->vertical) {
    width -= 3;
    height -= 2;

    rms_level = gtk_vumeter_sound_level_to_draw_level (vumeter,
        vumeter->rms_level, height - 1);
    peak_level = gtk_vumeter_sound_level_to_draw_level (vumeter,
        vumeter->peak_level, height - 1);

    /* draw normal level */
    cairo_set_source (cr, vumeter->gradient_rms);
    cairo_rectangle (cr, 1.5, 1.5, width, rms_level);
    cairo_fill (cr);

    /* draw peak */
    if (peak_level > rms_level) {
      cairo_set_source (cr, vumeter->gradient_peak);
      cairo_rectangle (cr, 1.5, rms_level + 0.5, width, peak_level - rms_level);
      cairo_fill (cr);
    }

    /* draw background for the rest */
    if (height > peak_level) {
      cairo_set_source (cr, vumeter->gradient_bg);
      cairo_rectangle (cr, 1.5, peak_level + 0.5, width, height - peak_level);
      cairo_fill (cr);
    }

    /* shade every 4th line */
    cairo_set_source_rgba (cr, 0, 0, 0, 0.5);
    cairo_set_line_width (cr, 1.0);
    for (i = 1; i < height; i += 5) {
      cairo_move_to (cr, 1.5, i + 0.5);
      cairo_line_to (cr, width + 0.5, i + 1.5);
    }
    cairo_stroke (cr);

  } else {                      /* Horizontal */
    width -= 2;
    height -= 3;

    rms_level = gtk_vumeter_sound_level_to_draw_level (vumeter,
        vumeter->rms_level, width - 1);
    peak_level = gtk_vumeter_sound_level_to_draw_level (vumeter,
        vumeter->peak_level, width - 1);

    /* draw normal level */
    cairo_set_source (cr, vumeter->gradient_rms);
    cairo_rectangle (cr, 1.5, 1.5, rms_level, height);
    cairo_fill (cr);

    /* draw peak */
    if (peak_level > rms_level) {
      cairo_set_source (cr, vumeter->gradient_peak);
      cairo_rectangle (cr, rms_level + 0.5, 1.5, peak_level - rms_level,
          height);
      cairo_fill (cr);
    }

    /* draw background for the rest */
    if (width > peak_level) {
      cairo_set_source (cr, vumeter->gradient_bg);
      cairo_rectangle (cr, peak_level + 0.5, 1.5, width - peak_level, height);
      cairo_fill (cr);
    }

    /* shade every 4th line */
    cairo_set_source_rgba (cr, 0, 0, 0, 0.5);
    cairo_set_line_width (cr, 1.0);
    for (i = 1; i < width; i += 5) {
      cairo_move_to (cr, i + 0.5, 1.5);
      cairo_line_to (cr, i + 0.5, height + 1.5);
    }
    cairo_stroke (cr);
  }

  return FALSE;
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
