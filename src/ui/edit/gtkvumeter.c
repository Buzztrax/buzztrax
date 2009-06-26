/* $Id$
 *
 * GtkVumeter
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
/* @todo:
 * - add properties:
 *   - vertical : gboolean, readonly
 *   - min,max,rms,peak : gint, read/write
 *   - peaks_falloff,scale_type : enum, read/write
 */
/**
 * SECTION:gtkvumeter
 * @short_description: vu meter widget
 *
 * Shows a vertical or horizontal gauge with a colorized bar to signal volume
 * level.
 */

#include <math.h>
#include <gtk/gtk.h>
#include "gtkvumeter.h"

#define MIN_HORIZONTAL_VUMETER_WIDTH   150
#define HORIZONTAL_VUMETER_HEIGHT  6
#define VERTICAL_VUMETER_WIDTH     6
#define MIN_VERTICAL_VUMETER_HEIGHT    400

static void gtk_vumeter_init (GtkVUMeter *vumeter);
static void gtk_vumeter_class_init (GtkVUMeterClass *class);
static void gtk_vumeter_finalize (GObject * object);
static void gtk_vumeter_realize (GtkWidget *widget);
static void gtk_vumeter_size_request (GtkWidget *widget, GtkRequisition *requisition);
static void gtk_vumeter_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static gint gtk_vumeter_expose (GtkWidget *widget, GdkEventExpose *event);
static gint gtk_vumeter_sound_level_to_draw_level (GtkVUMeter *vumeter, gint level);

static GtkWidgetClass *parent_class = NULL;

GType gtk_vumeter_get_type (void)
{
    static GType vumeter_type = 0;

    if (G_UNLIKELY(!vumeter_type)) {
        const GTypeInfo vumeter_info = {
            sizeof (GtkVUMeterClass),
            NULL, NULL,
            (GClassInitFunc) gtk_vumeter_class_init,
            NULL, NULL, sizeof (GtkVUMeter), 0,
            (GInstanceInitFunc) gtk_vumeter_init,
        };
        vumeter_type = g_type_register_static (GTK_TYPE_WIDGET, "GtkVUMeter", &vumeter_info, 0);
    }

    return vumeter_type;
}

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
    vumeter->peaks_falloff = GTK_VUMETER_PEAKS_FALLOFF_MEDIUM;
    vumeter->peak_level = 0;
    vumeter->delay_peak_level = 0;

    vumeter->scale = GTK_VUMETER_SCALE_LINEAR;
}

static void gtk_vumeter_class_init (GtkVUMeterClass *klass)
{
    GObjectClass *gobject_class = (GObjectClass *) klass;
    GtkWidgetClass *widget_class = (GtkWidgetClass*) klass;

    parent_class = g_type_class_peek_parent (klass);

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

    G_OBJECT_CLASS(parent_class)->finalize (object);
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

    GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
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

    if (vumeter->vertical == TRUE) {
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

    g_return_if_fail (widget != NULL);
    g_return_if_fail (GTK_IS_VUMETER (widget));
    g_return_if_fail (allocation != NULL);

    GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

    widget->allocation = *allocation;
    vumeter = GTK_VUMETER (widget);

    if (GTK_WIDGET_REALIZED (widget)) {
        if (vumeter->vertical) {
            /* veritcal */
            gdk_window_move_resize (widget->window, allocation->x, allocation->y,
                MAX(allocation->width,VERTICAL_VUMETER_WIDTH),
                MIN(allocation->height, MIN_VERTICAL_VUMETER_HEIGHT));

        } else {
            /* horizontal */
            gdk_window_move_resize (widget->window, allocation->x, allocation->y,
                MIN(allocation->width, MIN_HORIZONTAL_VUMETER_WIDTH),
                MAX(allocation->height,HORIZONTAL_VUMETER_HEIGHT));
        }
    }
    gtk_vumeter_allocate_colors (vumeter);
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
    // @todo: this should not be needed
    //cairo_push_group (cr);

    rms_level = gtk_vumeter_sound_level_to_draw_level (vumeter,
                   vumeter->rms_level);
    peak_level = gtk_vumeter_sound_level_to_draw_level (vumeter,
                   vumeter->peak_level);

    /* draw border */
    /* detail for part of progressbar would be called "trough" */
    gtk_paint_box (widget->style, widget->window, GTK_STATE_NORMAL, GTK_SHADOW_IN,
            NULL, widget, NULL/*detail*/, 0, 0, widget->allocation.width, widget->allocation.height);

    if (vumeter->vertical) {
        width = widget->allocation.width - 2;
        height = widget->allocation.height;

        /* draw normal level */
        cairo_set_source (cr, vumeter->gradient_rms);
        cairo_rectangle (cr, 1, 1, width, rms_level);
        cairo_fill (cr);

        /* draw peak */
        if (peak_level > rms_level) {
            cairo_set_source (cr, vumeter->gradient_peak);
            cairo_rectangle (cr, 1, rms_level+1, width, peak_level-rms_level);
            cairo_fill (cr);
        }

        /* draw background for the rest */
        if (peak_level+1 < height-2) {
            cairo_set_source (cr, vumeter->gradient_bg);
            cairo_rectangle (cr, 1, peak_level+1, width, height-peak_level-2);
            cairo_fill (cr);
        }

        /* shade every 4th line */
        cairo_set_source_rgba (cr, 0, 0, 0, 0.5);
        cairo_set_line_width (cr, 1.0);
        for (i = 1; i < height - 1; i += 4) {
          cairo_move_to (cr, 1, i);
          cairo_line_to (cr, width + 1, i);
        }
        cairo_stroke (cr);

    } else { /* Horizontal */
        width = widget->allocation.width;
        height = widget->allocation.height - 2;

        /* draw normal level */
        cairo_set_source (cr, vumeter->gradient_rms);
        cairo_rectangle (cr, 1, 1, rms_level, height);
        cairo_fill (cr);

        /* draw peak */
        if (peak_level > rms_level) {
            cairo_set_source (cr, vumeter->gradient_peak);
            cairo_rectangle (cr, rms_level+1, 1, peak_level-rms_level, height);
            cairo_fill (cr);
        }

        /* draw background for the rest */
        if (peak_level+1 < width-2) {
            cairo_set_source (cr, vumeter->gradient_bg);
            cairo_rectangle (cr, peak_level+1, 1, width-peak_level-2, height);
            cairo_fill (cr);
        }

        /* shade every 4th line */
        cairo_set_source_rgba (cr, 0, 0, 0, 0.5);
        cairo_set_line_width (cr, 1.0);
        for (i = 1; i < width - 1; i += 4) {
          cairo_move_to (cr, i, 1);
          cairo_line_to (cr, i, height + 1);
        }
        cairo_stroke (cr);
    }

    // @todo: this should not be needed
    //cairo_pop_group_to_source (cr);
    //cairo_paint (cr);
    cairo_destroy (cr);

    return FALSE;
}

static gint gtk_vumeter_sound_level_to_draw_level (GtkVUMeter *vumeter,
               gint sound_level)
{
    gdouble draw_level;
    gdouble level, min, max, length;
    gdouble log_level, log_max;

    level = (gdouble)sound_level;
    min = (gdouble)vumeter->min;
    max = (gdouble)vumeter->max;

    if (vumeter->vertical == TRUE) {
        length = GTK_WIDGET(vumeter)->allocation.height - 2;
    } else { /* Horizontal */
        length = GTK_WIDGET(vumeter)->allocation.width - 2;
    }

    if (vumeter->scale == GTK_VUMETER_SCALE_LINEAR) {
        draw_level = (level - min)/(max - min) * length;
    } else {
        log_level = log10((level - min + 1)/(max - min + 1));
        log_max = log10(1/(max - min + 1));
        draw_level = length - log_level/log_max * length;
    }

    return (gint)draw_level;
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
    g_return_if_fail (GTK_IS_VUMETER (vumeter));

    vumeter->max = MAX(max, min);
    vumeter->min = MIN(min, max);
    if (vumeter->max == vumeter->min) {
        vumeter->max++;
    }
    vumeter->rms_level = CLAMP (vumeter->rms_level, vumeter->min, vumeter->max);
    vumeter->peak_level = CLAMP (vumeter->peak_level, vumeter->min, vumeter->max);
    gtk_widget_queue_draw (GTK_WIDGET(vumeter));
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
    
    if ((old_rms_level != vumeter->rms_level) && (old_peak_level != vumeter->peak_level)) {
      gtk_widget_queue_draw (GTK_WIDGET(vumeter));
    }
}

/**
 * gtk_vumeter_set_peaks_falloff:
 * @vumeter: the vumeter widget to change the current level
 * @peaks_falloff: controls the speed to the peak decay
 *
 * Sets the speed for the peaks to decay.
 *
 * PLEASE NOTE: the falloff is not yet implemented
 */
void gtk_vumeter_set_peaks_falloff (GtkVUMeter *vumeter, gint peaks_falloff)
{
    g_return_if_fail (GTK_IS_VUMETER (vumeter));

    if (peaks_falloff > GTK_VUMETER_PEAKS_FALLOFF_FAST) {
        peaks_falloff = GTK_VUMETER_PEAKS_FALLOFF_FAST;
    }
    vumeter->peaks_falloff=peaks_falloff;
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
        if (GTK_WIDGET_REALIZED(vumeter)) {
            gtk_widget_queue_draw (GTK_WIDGET(vumeter));
        }
    }
}
