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
 * Since the code was deprecated and removed from gtk+ in commit
 * e0fb7a86e59278dccd7c276c754f9e8b7fdab0d5
 * on Wed Nov 24 16:44:16 2010 +0100
 * the code was revived here. this copy contains fixes for these bugs:
 * https://bugzilla.gnome.org/show_bug.cgi?id=145491
 * https://bugzilla.gnome.org/show_bug.cgi?id=151944
 *
 * plus extra clean-up and features.
 *
 * some features/api discussion at http://live.gnome.org/GTK%2B/GtkRuler
 *
 * TODO:
 * - make other instance vars private
 * - use GtkAdjustment
 * - add non-linear tick mapping
 * - consider removing max-size parameter (use size-request instead)
 */
/**
 * SECTION:btruler
 * @Short_description: A ruler widget
 *
 * The Ruler widget is utilized around other widgets such as a text widget or a
 * graph. The ruler is used to show the location of the mouse on the window and
 * to show the size of the window in specified units. The available units of
 * measurement are GTK_PIXELS, GTK_INCHES and GTK_CENTIMETERS.
 * GTK_PIXELS is the default unit of measurement. The ruler widget can be
 * oriented vertically or horizontally.
 */

#include "config.h"

#include <math.h>
#include <string.h>

#include "ruler.h"


#define RULER_WIDTH           14
#define MINIMUM_INCR          5

#define ROUND(x) ((int) ((x) + 0.5))

enum
{
  PROP_0,
  PROP_ORIENTATION,
  PROP_LOWER,
  PROP_UPPER,
  PROP_POSITION,
  PROP_MAX_SIZE,
  PROP_METRIC,
  PROP_DRAW_POS
};

struct _BtRulerPrivate
{
  GtkOrientation orientation;
};

static void bt_ruler_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void bt_ruler_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);
static void bt_ruler_realize (GtkWidget * widget);
static void bt_ruler_unrealize (GtkWidget * widget);
static void bt_ruler_size_request (GtkWidget * widget,
    GtkRequisition * requisition);
static void bt_ruler_size_allocate (GtkWidget * widget,
    GtkAllocation * allocation);
static gboolean bt_ruler_motion_notify (GtkWidget * widget,
    GdkEventMotion * event);
static gboolean bt_ruler_expose (GtkWidget * widget, GdkEventExpose * event);
static void bt_ruler_make_pixmap (BtRuler * ruler);
static void bt_ruler_real_draw_ticks (BtRuler * ruler);
static void bt_ruler_real_draw_pos (BtRuler * ruler);


static const BtRulerMetric ruler_metrics[] = {
  {"Pixel", "Pi", 1.0, 
    {1, 2, 5, 10, 25, 50, 100, 250, 500, 1000, 2500, 5000, 10000}, 
    {1, 5, 10, 50, 100, 500, 1000, 5000}
  },
  {"Inches", "In", 72.0, 
    {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096}, 
    {1, 2, 4, 8, 16, 32, 64, 128}
  },
  {"Centimeters", "Cm", 28.35, 
    {1, 2, 5, 10, 25, 50, 100, 250, 500, 1000, 2500, 5000, 10000}, 
    {1, 5, 10, 50, 100, 500, 1000, 5000}
  },
};


G_DEFINE_TYPE_WITH_CODE (BtRuler, bt_ruler, GTK_TYPE_WIDGET,
    G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE, NULL))


static void
bt_ruler_class_init (BtRulerClass * class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  gobject_class->set_property = bt_ruler_set_property;
  gobject_class->get_property = bt_ruler_get_property;

  widget_class->realize = bt_ruler_realize;
  widget_class->unrealize = bt_ruler_unrealize;
  widget_class->size_request = bt_ruler_size_request;
  widget_class->size_allocate = bt_ruler_size_allocate;
  widget_class->motion_notify_event = bt_ruler_motion_notify;
  widget_class->expose_event = bt_ruler_expose;

  class->draw_ticks = bt_ruler_real_draw_ticks;
  class->draw_pos = bt_ruler_real_draw_pos;

  g_type_class_add_private (gobject_class, sizeof (BtRulerPrivate));

  g_object_class_override_property (gobject_class,
      PROP_ORIENTATION, "orientation");

  g_object_class_install_property (gobject_class,
      PROP_LOWER,
      g_param_spec_double ("lower",
          "Lower",
          "Lower limit of ruler",
          -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_UPPER,
      g_param_spec_double ("upper",
          "Upper",
          "Upper limit of ruler",
          -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_POSITION,
      g_param_spec_double ("position",
          "Position",
          "Position of mark on the ruler",
          -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_MAX_SIZE,
      g_param_spec_double ("max-size",
          "Max Size",
          "Maximum size of the ruler",
          -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_METRIC,
      g_param_spec_enum ("metric",
          "Metric",
          "The metric used for the ruler",
          GTK_TYPE_METRIC_TYPE, GTK_PIXELS, G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_DRAW_POS,
      g_param_spec_boolean ("draw-pos",
          "Draw position",
          "Wheter the position should be marked at the ruler",
          TRUE, G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));
}

static void
bt_ruler_init (BtRuler * ruler)
{
  GtkWidget *widget = GTK_WIDGET (ruler);

  ruler->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (ruler, BT_TYPE_RULER, BtRulerPrivate);

  ruler->priv->orientation = GTK_ORIENTATION_HORIZONTAL;

  widget->requisition.width = widget->style->xthickness * 2 + 1;
  widget->requisition.height = widget->style->ythickness * 2 + RULER_WIDTH;

  ruler->backing_store = NULL;
  ruler->xsrc = 0;
  ruler->ysrc = 0;
  ruler->slider_size = 0;
  ruler->lower = 0;
  ruler->upper = 0;
  ruler->position = 0;
  ruler->max_size = 0;
  ruler->draw_pos = TRUE;

  bt_ruler_set_metric (ruler, GTK_PIXELS);
}

static void
bt_ruler_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  BtRuler *ruler = BT_RULER (object);

  switch (prop_id) {
    case PROP_ORIENTATION:
      ruler->priv->orientation = g_value_get_enum (value);
      gtk_widget_queue_resize (GTK_WIDGET (ruler));
      break;
    case PROP_LOWER:
      bt_ruler_set_range (ruler, g_value_get_double (value),
          ruler->upper, ruler->position, ruler->max_size);
      break;
    case PROP_UPPER:
      bt_ruler_set_range (ruler, ruler->lower,
          g_value_get_double (value), ruler->position, ruler->max_size);
      break;
    case PROP_POSITION:
      bt_ruler_set_range (ruler, ruler->lower, ruler->upper,
          g_value_get_double (value), ruler->max_size);
      break;
    case PROP_MAX_SIZE:
      bt_ruler_set_range (ruler, ruler->lower, ruler->upper,
          ruler->position, g_value_get_double (value));
      break;
    case PROP_METRIC:
      bt_ruler_set_metric (ruler, g_value_get_enum (value));
      break;
    case PROP_DRAW_POS:
      ruler->draw_pos = g_value_get_boolean (value);
      break;
    default:G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
bt_ruler_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
  BtRuler *ruler = BT_RULER (object);

  switch (prop_id) {
    case PROP_ORIENTATION:
      g_value_set_enum (value, ruler->priv->orientation);
      break;
    case PROP_LOWER:
      g_value_set_double (value, ruler->lower);
      break;
    case PROP_UPPER:
      g_value_set_double (value, ruler->upper);
      break;
    case PROP_POSITION:
      g_value_set_double (value, ruler->position);
      break;
    case PROP_MAX_SIZE:
      g_value_set_double (value, ruler->max_size);
      break;
    case PROP_METRIC:
      g_value_set_enum (value, bt_ruler_get_metric (ruler));
      break;
    case PROP_DRAW_POS:
      g_value_set_boolean (value, ruler->draw_pos);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

void
bt_ruler_set_metric (BtRuler * ruler, GtkMetricType metric)
{
  g_return_if_fail (BT_IS_RULER (ruler));

  ruler->metric = (BtRulerMetric *) & ruler_metrics[metric];

  if (gtk_widget_is_drawable (GTK_WIDGET (ruler)))
    gtk_widget_queue_draw (GTK_WIDGET (ruler));

  g_object_notify (G_OBJECT (ruler), "metric");
}

/**
 * bt_ruler_get_metric:
 * @ruler: a #BtRuler
 *
 * Gets the units used for a #BtRuler. See bt_ruler_set_metric().
 *
 * Return value: the units currently used for @ruler
 *
 */
GtkMetricType
bt_ruler_get_metric (BtRuler * ruler) {
  gint i;

  g_return_val_if_fail (BT_IS_RULER (ruler), 0);

  for (i = 0; i < G_N_ELEMENTS (ruler_metrics); i++)
    if (ruler->metric == &ruler_metrics[i])
      return i;

  g_assert_not_reached ();

  return 0;
}

/**
 * bt_ruler_set_range:
 * @ruler: the gtkruler
 * @lower: the lower limit of the ruler
 * @upper: the upper limit of the ruler
 * @position: the mark on the ruler
 * @max_size: the maximum size of the ruler used when calculating the space to
 * leave for the text
 *
 * This sets the range of the ruler. 
 */
void
bt_ruler_set_range (BtRuler * ruler, gdouble lower, gdouble upper, gdouble position, gdouble max_size)
{
  g_return_if_fail (BT_IS_RULER (ruler));

  g_object_freeze_notify (G_OBJECT (ruler));
  if (ruler->lower != lower) {
    ruler->lower = lower;
    g_object_notify (G_OBJECT (ruler), "lower");
  }
  if (ruler->upper != upper) {
    ruler->upper = upper;
    g_object_notify (G_OBJECT (ruler), "upper");
  }
  if (ruler->position != position) {
    ruler->position = position;
    g_object_notify (G_OBJECT (ruler), "position");
  }
  if (ruler->max_size != max_size) {
    ruler->max_size = max_size;
    g_object_notify (G_OBJECT (ruler), "max-size");
  }
  g_object_thaw_notify (G_OBJECT (ruler));

  if (gtk_widget_is_drawable (GTK_WIDGET (ruler)))
    gtk_widget_queue_draw (GTK_WIDGET (ruler));
}

/**
 * bt_ruler_get_range:
 * @ruler: a #BtRuler
 * @lower: (allow-none): location to store lower limit of the ruler, or %NULL
 * @upper: (allow-none): location to store upper limit of the ruler, or %NULL
 * @position: (allow-none): location to store the current position of the mark on the ruler, or %NULL
 * @max_size: location to store the maximum size of the ruler used when calculating
 *            the space to leave for the text, or %NULL.
 *
 * Retrieves values indicating the range and current position of a #BtRuler.
 * See bt_ruler_set_range().
 *
 */
void 
bt_ruler_get_range (BtRuler * ruler, gdouble * lower, gdouble * upper, gdouble * position, gdouble * max_size)
{
  g_return_if_fail (BT_IS_RULER (ruler));

  if (lower)
    *lower = ruler->lower;
  if (upper)
    *upper = ruler->upper;
  if (position)
    *position = ruler->position;
  if (max_size)
    *max_size = ruler->max_size;
}

/**
 * bt_ruler_new:
 * @orientation: wheter the ruler is vertcal or horizontal
 * @draw_pos wheter the uler will show a position marker
 *
 * Creates a new ruler
 *
 * Returns: a new #BtRuler.
 */
GtkWidget *
bt_ruler_new (GtkOrientation orientation, gboolean draw_pos)
{
  return g_object_new (BT_TYPE_RULER, "orientation", orientation, "draw-pos", draw_pos, NULL);
}

/* vmethods */

static void 
bt_ruler_draw_ticks (BtRuler * ruler)
{
  g_return_if_fail (BT_IS_RULER (ruler));

  if (BT_RULER_GET_CLASS (ruler)->draw_ticks)
    BT_RULER_GET_CLASS (ruler)->draw_ticks (ruler);
}

static void 
bt_ruler_draw_pos (BtRuler * ruler)
{
  g_return_if_fail (BT_IS_RULER (ruler));

  if (ruler->draw_pos &&  BT_RULER_GET_CLASS (ruler)->draw_pos)
    BT_RULER_GET_CLASS (ruler)->draw_pos (ruler);
}

static void
bt_ruler_realize (GtkWidget * widget)
{
  BtRuler *ruler;
  GdkWindowAttr attributes;
  gint attributes_mask;

  ruler = BT_RULER (widget);

  gtk_widget_set_realized (widget, TRUE);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= (GDK_EXPOSURE_MASK |
      GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

  widget->window =
      gdk_window_new (gtk_widget_get_parent_window (widget), &attributes,
      attributes_mask);
  gdk_window_set_user_data (widget->window, ruler);

  widget->style = gtk_style_attach (widget->style, widget->window);
  gtk_style_set_background (widget->style, widget->window, GTK_STATE_ACTIVE);

  bt_ruler_make_pixmap (ruler);
}

static void
bt_ruler_unrealize (GtkWidget * widget)
{
  BtRuler *ruler = BT_RULER (widget);

  if (ruler->backing_store) {
    g_object_unref (ruler->backing_store);
    ruler->backing_store = NULL;
  }

  GTK_WIDGET_CLASS (bt_ruler_parent_class)->unrealize (widget);
}

static void
bt_ruler_size_request (GtkWidget * widget, GtkRequisition * requisition)
{
  BtRuler *ruler = BT_RULER (widget);

  if (ruler->priv->orientation == GTK_ORIENTATION_HORIZONTAL) {
    requisition->width = widget->style->xthickness * 2 + 1;
    requisition->height = widget->style->ythickness * 2 + RULER_WIDTH;
  } else {
    requisition->width = widget->style->xthickness * 2 + RULER_WIDTH;
    requisition->height = widget->style->ythickness * 2 + 1;
  }
}

static void
bt_ruler_size_allocate (GtkWidget * widget, GtkAllocation * allocation)
{
  BtRuler *ruler = BT_RULER (widget);

  widget->allocation = *allocation;

  if (gtk_widget_get_realized (widget)) {
    gdk_window_move_resize (widget->window,
        allocation->x, allocation->y, allocation->width, allocation->height);

    bt_ruler_make_pixmap (ruler);
  }
}

static gboolean
bt_ruler_motion_notify (GtkWidget * widget, GdkEventMotion * event)
{
  BtRuler *ruler = BT_RULER (widget);
  gint x;
  gint y;

  gdk_event_request_motions (event);
  x = event->x;
  y = event->y;

  if (ruler->priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    ruler->position =
        ruler->lower + ((ruler->upper -
            ruler->lower) * x) / widget->allocation.width;
  else
    ruler->position =
        ruler->lower + ((ruler->upper -
            ruler->lower) * y) / widget->allocation.height;

  g_object_notify (G_OBJECT (ruler), "position");

  /*  Make sure the ruler has been allocated already  */
  if (ruler->backing_store != NULL)
    bt_ruler_draw_pos (ruler);

  return FALSE;
}

static
gboolean bt_ruler_expose (GtkWidget * widget, GdkEventExpose * event)
{
  if (gtk_widget_is_drawable (widget)) {
    BtRuler *ruler = BT_RULER (widget);
    cairo_t *cr;

    bt_ruler_draw_ticks (ruler);

    cr = gdk_cairo_create (widget->window);
    gdk_cairo_set_source_pixmap (cr, ruler->backing_store, 0, 0);
    gdk_cairo_rectangle (cr, &event->area);
    cairo_fill (cr);
    cairo_destroy (cr);

    bt_ruler_draw_pos (ruler);
  }

  return FALSE;
}

static void
bt_ruler_make_pixmap (BtRuler * ruler)
{
  GtkWidget *widget = GTK_WIDGET (ruler);

  if (ruler->backing_store) {
    if ((ruler->w == widget->allocation.width)
        && (ruler->h == widget->allocation.height))
      return;
    /* size has changed */
    g_object_unref (ruler->backing_store);
  }

  ruler->w = widget->allocation.width;
  ruler->h = widget->allocation.height;
  ruler->backing_store = gdk_pixmap_new (widget->window, ruler->w, ruler->h, -1);
  ruler->xsrc = ruler->ysrc = 0;
}

// we need to use a differet mechanism to pick ticks that get labels
//#define USE_LOG10 

#ifdef USE_LOG10
static gdouble
map_log10 (gdouble in,gdouble upper,gdouble end)
{
  return (end/log10(upper)) * log10(1.0+(in*upper)/end);
}
#endif

static void
bt_ruler_real_draw_ticks (BtRuler * ruler)
{
  GtkWidget *widget = GTK_WIDGET (ruler);
  cairo_t *cr;
  gint i, j;
  gint width, height, ruler_size;
  gint xthickness, ythickness;
  gint length, ideal_length;
  gdouble lower, upper;         /* Upper and lower limits, in ruler units */
  gdouble increment;            /* Number of pixels per unit */
  gint scale;                   /* Number of units per major unit */
  gdouble subd_incr;
  gdouble start, end, cur;
  gchar unit_str[32];
  gint digit_height;
  gint digit_offset;
  gint text_width;
  gint text_height;
  gint pos;
  PangoLayout *layout;
  PangoRectangle logical_rect, ink_rect;
  GtkOrientation orientation;

  if (!gtk_widget_is_drawable (widget))
    return;

  xthickness = widget->style->xthickness;
  ythickness = widget->style->ythickness;
  orientation = ruler->priv->orientation;

  layout = gtk_widget_create_pango_layout (widget, "012456789");
  pango_layout_get_extents (layout, &ink_rect, &logical_rect);

  digit_height = PANGO_PIXELS (ink_rect.height) + 2;
  digit_offset = ink_rect.y;

  if (orientation == GTK_ORIENTATION_HORIZONTAL) {
    ruler_size = width = widget->allocation.width;
    height = widget->allocation.height - ythickness * 2;
  } else {
    ruler_size = width = widget->allocation.height;
    height = widget->allocation.width - ythickness * 2;
  }

  gtk_paint_box (widget->style, ruler->backing_store,
      GTK_STATE_NORMAL, GTK_SHADOW_OUT,
      NULL, widget,
      orientation == GTK_ORIENTATION_HORIZONTAL ?"hruler" : "vruler",
      0, 0, widget->allocation.width, widget->allocation.height);

  cr = gdk_cairo_create (ruler->backing_store);
  gdk_cairo_set_source_color (cr, &widget->style->fg[widget->state]);

  if (orientation == GTK_ORIENTATION_HORIZONTAL) {
    cairo_rectangle (cr, xthickness, height + ythickness, 
        widget->allocation.width - 2 * xthickness, 1);
  } else {
    cairo_rectangle (cr, height + xthickness, ythickness, 
        1, widget->allocation.height - 2 * ythickness);
  }

  upper = ruler->upper / ruler->metric->pixels_per_unit;
  lower = ruler->lower / ruler->metric->pixels_per_unit;

  if ((upper - lower) == 0)
    goto out;

  increment = (gdouble) width / (upper - lower);

  /* determine the scale H
   *  We calculate the text size as for the vruler instead of using
   *  text_width = gdk_string_width(font, unit_str), so that the result
   *  for the scale looks consistent with an accompanying vruler
   */
  /* determine the scale V
   *   use the maximum extents of the ruler to determine the largest
   *   possible number to be displayed.  Calculate the height in pixels
   *   of this displayed text. Use this height to find a scale which
   *   leaves sufficient room for drawing the ruler.
   */
  scale = ceil (ruler->max_size / ruler->metric->pixels_per_unit);
  g_snprintf (unit_str, sizeof (unit_str), "%d", scale);

  if (orientation == GTK_ORIENTATION_HORIZONTAL) {
    text_width = strlen (unit_str) * digit_height + 1;

    for (scale = 0; scale < BT_RULER_MAXIMUM_SCALES; scale++)
      if (ruler->metric->ruler_scale[scale] * fabs (increment) > 2 * text_width)
        break;
  } else {
    text_height = strlen (unit_str) * digit_height + 1;

    for (scale = 0; scale < BT_RULER_MAXIMUM_SCALES; scale++)
      if (ruler->metric->ruler_scale[scale] * fabs (increment) >
          2 * text_height)
        break;
  }

  if (scale == BT_RULER_MAXIMUM_SCALES)
    scale = BT_RULER_MAXIMUM_SCALES - 1;

  /* drawing starts here */
  length = 0;
  for (i = BT_RULER_MAXIMUM_SUBDIVIDE - 1; i >= 0; i--) {
    subd_incr = (gdouble) ruler->metric->ruler_scale[scale] /
        (gdouble) ruler->metric->subdivide[i];
    if (subd_incr * fabs (increment) <= MINIMUM_INCR)
      continue;

    /* Calculate the length of the tickmarks. Make sure that
     * this length increases for each set of ticks
     */
    ideal_length = height / (i + 1) - 1;
    if (ideal_length > ++length)
      length = ideal_length;

    if (lower < upper) {
      start = floor (lower / subd_incr) * subd_incr;
      end = ceil (upper / subd_incr) * subd_incr;
    } else {
      start = floor (upper / subd_incr) * subd_incr;
      end = ceil (lower / subd_incr) * subd_incr;
    }

    for (cur = start; cur <= end; cur += subd_incr) {
      pos = ROUND ((cur - lower) * increment);
      
#ifdef USE_LOG10
      pos = map_log10 (pos, ruler->upper, ruler_size);
#endif

      if (orientation == GTK_ORIENTATION_HORIZONTAL) {
        cairo_rectangle (cr, pos, height + ythickness - length, 1, length);
      } else {
        cairo_rectangle (cr, height + xthickness - length, pos, length, 1);
      }

      /* draw label */
      if (i == 0) {
        g_snprintf (unit_str, sizeof (unit_str), "%d", (gint) cur);

        if (orientation == GTK_ORIENTATION_HORIZONTAL) {
          pango_layout_set_text (layout, unit_str, -1);
          pango_layout_get_extents (layout, &logical_rect, NULL);

          gtk_paint_layout (widget->style,
              ruler->backing_store,
              gtk_widget_get_state (widget),
              FALSE,
              NULL,
              widget,
              "hruler",
              pos + 2,
              ythickness + PANGO_PIXELS (logical_rect.y - digit_offset),
              layout);
        } else {
          for (j = 0; j < (gint) strlen (unit_str); j++) {
            pango_layout_set_text (layout, unit_str + j, 1);
            pango_layout_get_extents (layout, NULL, &logical_rect);

            gtk_paint_layout (widget->style,
                ruler->backing_store,
                gtk_widget_get_state (widget),
                FALSE,
                NULL,
                widget,
                "vruler",
                xthickness + 1,
                pos + digit_height * j + 2 + PANGO_PIXELS (logical_rect.y -
                    digit_offset), layout);
          }
        }
      }
    }
  }

  cairo_fill (cr);
out:
  cairo_destroy (cr);

  g_object_unref (layout);
}

static void
bt_ruler_real_draw_pos (BtRuler * ruler)
{
  GtkWidget *widget = GTK_WIDGET (ruler);
  gint width, height, ruler_size;
  gint xthickness, ythickness;
  gint bs_width, bs_height;
  GtkOrientation orientation;

  if (!gtk_widget_is_drawable (widget))
    return;

  xthickness = widget->style->xthickness;
  ythickness = widget->style->ythickness;
  width = widget->allocation.width;
  height = widget->allocation.height;
  orientation = ruler->priv->orientation;

  if (orientation == GTK_ORIENTATION_HORIZONTAL) {
    ruler_size = width;
    height -= ythickness * 2;

    bs_width = height / 2 + 2;
    bs_width |= 1;            /* make sure it's odd */
    bs_height = bs_width / 2 + 1;
  } else {
    ruler_size = height;
    width -= xthickness * 2;

    bs_height = width / 2 + 2;
    bs_height |= 1;           /* make sure it's odd */
    bs_width = bs_height / 2 + 1;
  }

  if ((bs_width > 0) && (bs_height > 0)) {
    cairo_t *cr = gdk_cairo_create (widget->window);
    gint x, y, pos;
    gdouble increment;

    /*  If a backing store exists, restore the ruler  */
    if (ruler->backing_store) {
      gdk_cairo_set_source_pixmap (cr, ruler->backing_store, 0, 0);
      cairo_rectangle (cr, ruler->xsrc, ruler->ysrc, bs_width, bs_height);
      cairo_fill (cr);
    }

    pos = ruler->position;
#ifdef USE_LOG10
    pos = map_log10 (pos, ruler->upper, ruler_size);
#endif
    if (orientation == GTK_ORIENTATION_HORIZONTAL) {
      increment = (gdouble) width / (ruler->upper - ruler->lower);

      x = ROUND ((pos - ruler->lower) * increment) + (xthickness - bs_width) / 2 - 1;
      y = (height + bs_height) / 2 + ythickness;
    } else {
      increment = (gdouble) height / (ruler->upper - ruler->lower);

      x = (width + bs_width) / 2 + xthickness;
      y = ROUND ((pos - ruler->lower) * increment) + (ythickness - bs_height) / 2 - 1;
    }

    gdk_cairo_set_source_color (cr, &widget->style->fg[widget->state]);

    cairo_move_to (cr, x, y);

    if (orientation == GTK_ORIENTATION_HORIZONTAL) {
      cairo_line_to (cr, x + bs_width / 2.0, y + bs_height);
      cairo_line_to (cr, x + bs_width, y);
    } else {
      cairo_line_to (cr, x + bs_width, y + bs_height / 2.0);
      cairo_line_to (cr, x, y + bs_height);
    }

    cairo_fill (cr);

    cairo_destroy (cr);

    ruler->xsrc = x;
    ruler->ysrc = y;
  }
}
