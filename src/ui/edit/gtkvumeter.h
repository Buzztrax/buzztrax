/* GtkVumeter
 * Copyright (C) 2003 Todd Goyen <wettoad@knighthoodofbuh.org>
 *               2007 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#ifndef __GTKVUMETER_H__
#define __GTKVUMETER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

G_DECLARE_FINAL_TYPE (GtkVUMeter, gtk_vumeter, GTK, VUMETER, GtkWidget);
#define GTK_TYPE_VUMETER                (gtk_vumeter_get_type ())

/**
 * GtkVUMeter:
 *
 * a volume meter widget
 */
struct _GtkVUMeter {
    GtkWidget   widget;

    /* < private > */
    /* properties */
    GtkOrientation  orientation;
    gint            rms_level;
    gint            min;
    gint            max;

    gint            delay_peak_level;
    gint            peak_level;

    /* internal state */
    gint            scale;
    GtkBorder       border;
    cairo_pattern_t *gradient_rms, *gradient_peak, *gradient_bg;
};

enum {
    GTK_VUMETER_SCALE_LINEAR,
    GTK_VUMETER_SCALE_LOG,
    GTK_VUMETER_SCALE_LAST
};

GType    gtk_vumeter_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_vumeter_new (GtkOrientation orientation);
void gtk_vumeter_set_min_max (GtkVUMeter *self, gint min, gint max);
void gtk_vumeter_set_levels (GtkVUMeter *self, gint rms, gint peak);
void gtk_vumeter_set_peaks_falloff (GtkVUMeter *self, gint peaks_falloff);
void gtk_vumeter_set_scale (GtkVUMeter *self, gint scale);

G_END_DECLS

#endif /* __GTKVUMETER_H__ */
