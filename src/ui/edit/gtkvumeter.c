/* GtkVumeter
 * Copyright (C) 2003 Todd Goyen <wettoad@knighthoodofbuh.org>
 *               2007-2009 Buzztard team <buzztard-devel@lists.sf.net>
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/* TODO(ensonic): add properties:
 *   - vertical : gboolean, readonly
 *   - min,max,rms,peak : gint, read/write
 *   - scale_type : enum, read/write
 * TODO(ensonic): revisit cairo usage
 *   - http://www.cairographics.org/FAQ/#sharp_lines
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

#if !GTK_CHECK_VERSION(2,20,0)
#define gtk_widget_set_realized(widget, flag) \
  if (flag) GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED); \
  else GTK_WIDGET_UNSET_FLAGS (widget, GTK_REALIZED)
#define gtk_widget_get_realized(widget) \
  ((GTK_WIDGET_FLAGS (widget) & GTK_REALIZED) != 0)
#endif

#define MIN_HORIZONTAL_VUMETER_WIDTH   150
#define HORIZONTAL_VUMETER_HEIGHT  6
#define VERTICAL_VUMETER_WIDTH     6
#define MIN_VERTICAL_VUMETER_HEIGHT    400

static void gtk_vumeter_finalize (GObject * object);
static void gtk_vumeter_realize (GtkWidget *widget);
static void gtk_vumeter_size_request (GtkWidget *widget, GtkRequisition *requisition);
static void gtk_vumeter_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static gint gtk_vumeter_expose (GtkWidget *widget, GdkEventExpose *event);

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
GtkWidget* gtk_vumeter_new (gboolean vertical)
{
    GtkVUMeter *vumeter;
    vumeter = GTK_VUMETER( g_object_new (GTK_TYPE_VUMETER,NULL));
    vumeter->vertical = vertical;
    return GTK_WIDGET (vumeter);
}

static void gtk_vumeter_init (GtkVUMeter *vumeter)
{
    vumeter->rms_level = 0;
    vumeter->min = 0;
    vumeter->max = 32767;
    vumeter->peak_level = 0;
    vumeter->delay_peak_level = 0;

    vumeter->scale = GTK_VUMETER_SCALE_LINEAR;
}

static void gtk_vumeter_class_init (GtkVUMeterClass *klass)
{
    GObjectClass *gobject_class = (GObjectClass *) klass;
    GtkWidgetClass *widget_class = (GtkWidgetClass*) klass;

    gobject_class->finalize = gtk_vumeter_finalize;

    widget_class->realize = gtk_vumeter_realize;
    widget_class->expose_event = gtk_vumeter_expose;
    widget_class->size_request = gtk_vumeter_size_request;
    widget_class->size_allocate = gtk_vumeter_size_allocate;
}


static void gtk_vumeter_finalize (GObject * object)
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

static void gtk_vumeter_allocate_colors (GtkVUMeter *vumeter)
{
    /* free old gradients */
    if (vumeter->gradient_rms)
        cairo_pattern_destroy (vumeter->gradient_rms);
    if (vumeter->gradient_peak)
        cairo_pattern_destroy (vumeter->gradient_peak);
    if (vumeter->gradient_bg)
        cairo_pattern_destroy (vumeter->gradient_bg);

    if (vumeter->vertical) { /* veritcal */
        gint height = ((GtkWidget *)vumeter)->allocation.height - 1;

        /* setup gradients */
        vumeter->gradient_rms = cairo_pattern_create_linear(1, 1, 1,height);
        cairo_pattern_add_color_stop_rgb(vumeter->gradient_rms, 0, 0.0, 1.0, 0.0);
        cairo_pattern_add_color_stop_rgb(vumeter->gradient_rms, 0.7, 1.0, 1.0, 0.0);
        cairo_pattern_add_color_stop_rgb(vumeter->gradient_rms, 1.0, 1.0, 0.0, 0.0);

        vumeter->gradient_peak = cairo_pattern_create_linear(1, 1, 1, height);
        cairo_pattern_add_color_stop_rgb(vumeter->gradient_peak, 0, 0.0, 0.6, 0.0);
        cairo_pattern_add_color_stop_rgb(vumeter->gradient_peak, 0.7, 0.6, 0.6, 0.0);
        cairo_pattern_add_color_stop_rgb(vumeter->gradient_peak, 1.0, 0.6, 0.0, 0.0);

        vumeter->gradient_bg = cairo_pattern_create_linear(1, 1, 1, height);
        cairo_pattern_add_color_stop_rgb(vumeter->gradient_bg, 0, 0.0, 0.3, 0.0);
        cairo_pattern_add_color_stop_rgb(vumeter->gradient_bg, 0.7, 0.3, 0.3, 0.0);
        cairo_pattern_add_color_stop_rgb(vumeter->gradient_bg, 1.0, 0.3, 0.0, 0.0);

    } else { /* horizontal */
        gint width = ((GtkWidget *)vumeter)->allocation.width - 1;

        /* setup gradients */
        vumeter->gradient_rms = cairo_pattern_create_linear(1, 1, width, 1);
        cairo_pattern_add_color_stop_rgb(vumeter->gradient_rms, 0, 0.0, 1.0, 0.0);
        cairo_pattern_add_color_stop_rgb(vumeter->gradient_rms, 0.7, 1.0, 1.0, 0.0);
        cairo_pattern_add_color_stop_rgb(vumeter->gradient_rms, 1.0, 1.0, 0.0, 0.0);

        vumeter->gradient_peak = cairo_pattern_create_linear(1, 1, width, 1);
        cairo_pattern_add_color_stop_rgb(vumeter->gradient_peak, 0, 0.0, 0.6, 0.0);
        cairo_pattern_add_color_stop_rgb(vumeter->gradient_peak, 0.7, 0.6, 0.6, 0.0);
        cairo_pattern_add_color_stop_rgb(vumeter->gradient_peak, 1.0, 0.6, 0.0, 0.0);

        vumeter->gradient_bg = cairo_pattern_create_linear(1, 1, width, 1);
        cairo_pattern_add_color_stop_rgb(vumeter->gradient_bg, 0, 0.0, 0.3, 0.0);
        cairo_pattern_add_color_stop_rgb(vumeter->gradient_bg, 0.7, 0.3, 0.3, 0.0);
        cairo_pattern_add_color_stop_rgb(vumeter->gradient_bg, 1.0, 0.3, 0.0, 0.0);
    }
}

static void gtk_vumeter_realize (GtkWidget *widget)
{
    GtkVUMeter *vumeter;
    GdkWindowAttr attributes;
    gint attributes_mask;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GTK_IS_VUMETER (widget));

    gtk_widget_set_realized(widget, TRUE);
    vumeter = GTK_VUMETER (widget);

    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.event_mask = gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK;
    attributes.visual = gtk_widget_get_visual (widget);
    attributes.colormap = gtk_widget_get_colormap (widget);
    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

    widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
    widget->style = gtk_style_attach (widget->style, widget->window);
    gdk_window_set_user_data (widget->window, widget);
    gtk_style_set_background (widget->style, widget->window,  GTK_STATE_NORMAL);

    gtk_vumeter_allocate_colors (vumeter);
}

static void gtk_vumeter_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
    GtkVUMeter *vumeter;

    g_return_if_fail (GTK_IS_VUMETER (widget));
    g_return_if_fail (requisition != NULL);

    vumeter = GTK_VUMETER (widget);

    if (vumeter->vertical) {
        requisition->width = VERTICAL_VUMETER_WIDTH;
        requisition->height = MIN_VERTICAL_VUMETER_HEIGHT;
    } else {
        requisition->width = MIN_HORIZONTAL_VUMETER_WIDTH;
        requisition->height = HORIZONTAL_VUMETER_HEIGHT;
    }
}

static void gtk_vumeter_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
    GtkVUMeter *vumeter;
    GtkAllocation *a;

    g_return_if_fail (GTK_IS_VUMETER (widget));
    g_return_if_fail (allocation != NULL);

    GTK_WIDGET_CLASS (gtk_vumeter_parent_class)->size_allocate (widget, allocation);

    vumeter = GTK_VUMETER (widget);

    a = &widget->allocation;
#if 0
    a->x = allocation->x;
    a->y = allocation->y;
    if (vumeter->vertical) {
       a->width = MAX(allocation->width, VERTICAL_VUMETER_WIDTH);
       a->height = MAX(allocation->height, MIN_VERTICAL_VUMETER_HEIGHT);
       //printf("V geometry: %dx%d - %dx%d\n",a->x,a->y,a->width,a->height);
    } else {
       a->width = MAX(allocation->width, MIN_HORIZONTAL_VUMETER_WIDTH);
       a->height = MAX(allocation->height, HORIZONTAL_VUMETER_HEIGHT);
       //printf("H geometry: %dx%d - %dx%d\n",a->x,a->y,a->width,a->height);
    }
#else
    *a = *allocation;
#endif

    if (gtk_widget_get_realized (widget)) {
        gdk_window_move_resize (widget->window, a->x, a->y, a->width, a->height);
    }
    gtk_vumeter_allocate_colors (vumeter);
}

static gint gtk_vumeter_sound_level_to_draw_level (GtkVUMeter *vumeter,
               gint sound_level, gdouble length)
{
    gdouble draw_level;
    gdouble level, min, max;

    level = (gdouble)sound_level;
    min = (gdouble)vumeter->min;
    max = (gdouble)vumeter->max;

    if (vumeter->scale == GTK_VUMETER_SCALE_LINEAR) {
        draw_level = (level - min)/(max - min) * length;
    } else {
        gdouble log_level, log_max;

        log_level = log10((level - min + 1)/(max - min + 1));
        log_max = log10(1/(max - min + 1)); /* FIXME(ensonic): could be cached */
        draw_level = length - log_level/log_max * length;
    }

    return (gint)draw_level;
}

static gint gtk_vumeter_expose (GtkWidget *widget, GdkEventExpose *event)
{
    GtkVUMeter *vumeter;
    gint rms_level, peak_level;
    gint width, height;
    cairo_t *cr;
    guint i;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (GTK_IS_VUMETER (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    if (event->count > 0)
        return FALSE;

    vumeter = GTK_VUMETER (widget);
    cr = gdk_cairo_create (widget->window);

    /* draw border */
    /* detail for part of progressbar would be called "trough" */
    gtk_paint_box (widget->style, widget->window, GTK_STATE_NORMAL, GTK_SHADOW_IN,
            NULL, widget, NULL/*detail*/, 0, 0, widget->allocation.width, widget->allocation.height);

    if (vumeter->vertical) {
        width = widget->allocation.width - 3;
        height = widget->allocation.height - 2;

        rms_level = gtk_vumeter_sound_level_to_draw_level (vumeter,
                       vumeter->rms_level, height-1);
        peak_level = gtk_vumeter_sound_level_to_draw_level (vumeter,
                       vumeter->peak_level, height-1);

        /* draw normal level */
        cairo_set_source (cr, vumeter->gradient_rms);
        cairo_rectangle (cr, 1.5, 1.5, width, rms_level);
        cairo_fill (cr);

        /* draw peak */
        if (peak_level > rms_level) {
            cairo_set_source (cr, vumeter->gradient_peak);
            cairo_rectangle (cr, 1.5, rms_level+0.5, width, peak_level-rms_level);
            cairo_fill (cr);
        }

        /* draw background for the rest */
        if (height > peak_level) {
            cairo_set_source (cr, vumeter->gradient_bg);
            cairo_rectangle (cr, 1.5, peak_level+0.5, width, height-peak_level);
            cairo_fill (cr);
        }

        /* shade every 4th line */
        cairo_set_source_rgba (cr, 0, 0, 0, 0.5);
        cairo_set_line_width (cr, 1.0);
        for (i = 1; i < height; i += 5) {
          cairo_move_to (cr, 1.5, i+0.5);
          cairo_line_to (cr, width+0.5, i+1.5);
        }
        cairo_stroke (cr);

    } else { /* Horizontal */
        width = widget->allocation.width - 2;
        height = widget->allocation.height - 3;

        rms_level = gtk_vumeter_sound_level_to_draw_level (vumeter,
                       vumeter->rms_level, width-1);
        peak_level = gtk_vumeter_sound_level_to_draw_level (vumeter,
                       vumeter->peak_level, width-1);

        /* draw normal level */
        cairo_set_source (cr, vumeter->gradient_rms);
        cairo_rectangle (cr, 1.5, 1.5, rms_level, height);
        cairo_fill (cr);

        /* draw peak */
        if (peak_level > rms_level) {
            cairo_set_source (cr, vumeter->gradient_peak);
            cairo_rectangle (cr, rms_level+0.5, 1.5, peak_level-rms_level, height);
            cairo_fill (cr);
        }

        /* draw background for the rest */
        if (width > peak_level) {
            cairo_set_source (cr, vumeter->gradient_bg);
            cairo_rectangle (cr, peak_level+0.5, 1.5, width-peak_level, height);
            cairo_fill (cr);
        }

        /* shade every 4th line */
        cairo_set_source_rgba (cr, 0, 0, 0, 0.5);
        cairo_set_line_width (cr, 1.0);
        for (i = 1; i < width; i += 5) {
          cairo_move_to (cr, i+0.5, 1.5);
          cairo_line_to (cr, i+0.5, height+1.5);
        }
        cairo_stroke (cr);
    }

    cairo_destroy (cr);

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
void gtk_vumeter_set_min_max (GtkVUMeter *vumeter, gint min, gint max)
{
    gint old_rms_level = vumeter->rms_level;
    gint old_peak_level = vumeter->peak_level;

    g_return_if_fail (GTK_IS_VUMETER (vumeter));

    vumeter->max = MAX(max, min);
    vumeter->min = MIN(min, max);
    if (vumeter->max == vumeter->min) {
        vumeter->max++;
    }
    vumeter->rms_level = CLAMP (vumeter->rms_level, vumeter->min, vumeter->max);
    vumeter->peak_level = CLAMP (vumeter->peak_level, vumeter->min, vumeter->max);

    if ((old_rms_level != vumeter->rms_level) || (old_peak_level != vumeter->peak_level)) {
        gtk_widget_queue_draw (GTK_WIDGET(vumeter));
    }
}

/**
 * gtk_vumeter_set_levels:
 * @vumeter: the vumeter widget to change the current level
 * @rms: the new rms level shown
 * @peak: the new peak level shown
 *
 * Sets new level values for the level display.
 * They are clamped to the min max range.
 */
void gtk_vumeter_set_levels (GtkVUMeter *vumeter, gint rms, gint peak)
{
    gint old_rms_level = vumeter->rms_level;
    gint old_peak_level = vumeter->peak_level;

    g_return_if_fail (GTK_IS_VUMETER (vumeter));

    vumeter->rms_level = CLAMP (rms, vumeter->min, vumeter->max);
    vumeter->peak_level = CLAMP (peak, vumeter->min, vumeter->max);

    if ((old_rms_level != vumeter->rms_level) || (old_peak_level != vumeter->peak_level)) {
        gtk_widget_queue_draw (GTK_WIDGET(vumeter));
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
void gtk_vumeter_set_scale (GtkVUMeter *vumeter, gint scale)
{
    g_return_if_fail (GTK_IS_VUMETER (vumeter));

    if (scale != vumeter->scale) {
        vumeter->scale = CLAMP(scale, GTK_VUMETER_SCALE_LINEAR, GTK_VUMETER_SCALE_LAST - 1);
        if (gtk_widget_get_realized (GTK_WIDGET(vumeter))) {
            gtk_widget_queue_draw (GTK_WIDGET(vumeter));
        }
    }
}
