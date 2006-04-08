// $Id: gtkvumeter.h,v 1.3 2006-04-08 16:18:26 ensonic Exp $
/*
 * gtkvumeter.h
 *
 * Fri Jan 10 20:06:41 2003
 * Copyright  2003  Todd Goyen
 * wettoad@knighthoodofbuh.org
 *
 * heavily modified by ensonic@user.sf.net
 */

#ifndef __GTKVUMETER_H__
#define __GTKVUMETER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTK_TYPE_VUMETER                (gtk_vumeter_get_type ())
#define GTK_VUMETER(obj)                (GTK_CHECK_CAST ((obj), GTK_TYPE_VUMETER, GtkVUMeter))
#define GTK_VUMETER_CLASS(klass)        (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_VUMETER GtkVUMeterClass))
#define GTK_IS_VUMETER(obj)             (GTK_CHECK_TYPE ((obj), GTK_TYPE_VUMETER))
#define GTK_IS_VUMETER_CLASS(klass)     (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_VUMETER))
#define GTK_VUMETER_GET_CLASS(obj)      (GTK_CHECK_GET_CLASS ((obj), GTK_TYPE_VUMETER, GtkVUMeterClass))

typedef struct _GtkVUMeter      GtkVUMeter;
typedef struct _GtkVUMeterClass GtkVUMeterClass;
    
struct _GtkVUMeter {
    GtkWidget   widget;
  
    GdkColormap *colormap;
    gint        colors;
  
    GdkGC       **f_gc;
    GdkGC       **b_gc;
    GdkColor    *f_colors;
    GdkColor    *b_colors;
  
    gboolean    vertical;
    gint        rms_level;
    gint        min;
    gint        max;
  
    gint        peaks_falloff;
    gint        delay_peak_level;
    gint        peak_level;
    
    gint        scale;
};

struct _GtkVUMeterClass {
    GtkWidgetClass  parent_class;
};

enum {
    GTK_VUMETER_PEAKS_FALLOFF_SLOW,
    GTK_VUMETER_PEAKS_FALLOFF_MEDIUM,
    GTK_VUMETER_PEAKS_FALLOFF_FAST,
    GTK_VUMETER_PEAKS_FALLOFF_LAST    
};

enum {
    GTK_VUMETER_SCALE_LINEAR,
    GTK_VUMETER_SCALE_LOG,
    GTK_VUMETER_SCALE_LAST    
};

GtkType    gtk_vumeter_get_type (void) G_GNUC_CONST;
GtkWidget *gtk_vumeter_new (gboolean vertical);
void gtk_vumeter_set_min_max (GtkVUMeter *vumeter, gint min, gint max);
void gtk_vumeter_set_levels (GtkVUMeter *vumeter, gint rms, gint peak);
void gtk_vumeter_set_peaks_falloff (GtkVUMeter *vumeter, gint peaks_falloff);
void gtk_vumeter_set_scale (GtkVUMeter *vumeter, gint scale);

G_END_DECLS

#endif /* __GTKVUMETER_H__ */
