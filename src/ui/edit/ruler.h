/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

/*
 * NOTE this widget is considered too specialized/little-used for
 * GTK+, and will in the future be moved to some other package.  If
 * your application needs this widget, feel free to use it, as the
 * widget does work and is useful in some applications; it's just not
 * of general interest. However, we are not accepting new features for
 * the widget, and it will eventually move out of the GTK+
 * distribution.
 */

#ifndef __BT_RULER_H__
#define __BT_RULER_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BT_TYPE_RULER            (bt_ruler_get_type ())
#define BT_RULER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_RULER, BtRuler))
#define BT_RULER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_RULER, BtRulerClass))
#define BT_IS_RULER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_RULER))
#define BT_IS_RULER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_RULER))
#define BT_RULER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_RULER, BtRulerClass))

typedef struct _BtRuler        BtRuler;
typedef struct _BtRulerClass   BtRulerClass;
typedef struct _BtRulerPrivate BtRulerPrivate;

typedef struct _BtRulerMetric  BtRulerMetric;

/* All distances below are in 1/72nd's of an inch. (According to
 * Adobe that's a point, but points are really 1/72.27 in.)
 */
struct _BtRuler
{
  GtkWidget widget;

  GdkPixmap *backing_store;
  BtRulerMetric *metric;
  gint xsrc, ysrc;
  gint w, h;
  gint slider_size;
  gboolean draw_pos;

  /* The upper limit of the ruler (in points) */
  gdouble lower;
  /* The lower limit of the ruler */
  gdouble upper;
  /* The position of the mark on the ruler */
  gdouble position;
  /* The maximum size of the ruler */
  gdouble max_size;
  
  BtRulerPrivate *priv;
};

struct _BtRulerClass
{
  GtkWidgetClass parent_class;

  void (* draw_ticks) (BtRuler *ruler);
  void (* draw_pos)   (BtRuler *ruler);
};

#define BT_RULER_MAXIMUM_SCALES        13
#define BT_RULER_MAXIMUM_SUBDIVIDE     8

struct _BtRulerMetric
{
  gchar *metric_name;
  gchar *abbrev;
  /* This should be points_per_unit. This is the size of the unit
   * in 1/72nd's of an inch and has nothing to do with screen pixels */
  gdouble pixels_per_unit;
  gdouble ruler_scale[BT_RULER_MAXIMUM_SCALES];
  gint subdivide[BT_RULER_MAXIMUM_SUBDIVIDE];  /* possible modes of subdivision */
};


GType         bt_ruler_get_type   (void) G_GNUC_CONST;
void          bt_ruler_set_metric (BtRuler *ruler, GtkMetricType   metric);
GtkMetricType bt_ruler_get_metric (BtRuler *ruler);
void          bt_ruler_set_range  (BtRuler *ruler, gdouble lower, gdouble upper, gdouble position, gdouble max_size);
void          bt_ruler_get_range  (BtRuler *ruler, gdouble *lower, gdouble *upper, gdouble *position, gdouble *max_size);

void          bt_ruler_draw_ticks (BtRuler       *ruler);
void          bt_ruler_draw_pos   (BtRuler       *ruler);

G_END_DECLS

#endif /* __BT_RULER_H__ */

