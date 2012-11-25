/*
 * try zoomable svg images on canvas
 *
 * gcc -Wall -g -lm svgcanvas2.c -o svgcanvas2 `pkg-config glib-2.0 gtk+-2.0 libgnomecanvas-2.0 librsvg-2.0 --cflags --libs`
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
//-- gtk+
#include <gtk/gtk.h>
#include <gdk/gdk.h>
//-- libgnomecanvas
#include <libgnomecanvas/libgnomecanvas.h>
//-- librsvg
#include <librsvg/rsvg.h>

// grid
#define RANGE 250.0
#define RANGE2 (RANGE+RANGE)

static gdouble zoom = 1.0;

// own svg canvas item class

enum
{
  PROP_0,
  PROP_FILE_NAME
};

GType gnome_canvas_svg_get_type (void);

#define GNOME_TYPE_CANVAS_SVG            (gnome_canvas_svg_get_type ())
#define GNOME_CANVAS_SVG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_CANVAS_SVG, GnomeCanvasSVG))
#define GNOME_CANVAS_SVG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_CANVAS_SVG, GnomeCanvasSVGClass))
#define GNOME_IS_CANVAS_SVG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_CANVAS_SVG))
#define GNOME_IS_CANVAS_SVG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_CANVAS_SVG))
#define GNOME_CANVAS_SVG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GNOME_TYPE_CANVAS_SVG, GnomeCanvasSVGClass))

typedef struct _GnomeCanvasSVG GnomeCanvasSVG;
typedef struct _GnomeCanvasSVGClass GnomeCanvasSVGClass;
typedef struct _GnomeCanvasSVGPrivate GnomeCanvasSVGPrivate;

struct _GnomeCanvasSVG
{
  GnomeCanvasPixbuf item;

  /* Private data */
  GnomeCanvasSVGPrivate *priv;
};

struct _GnomeCanvasSVGClass
{
  GnomeCanvasPixbufClass parent_class;
};

struct _GnomeCanvasSVGPrivate
{
  gboolean dispose_has_run;

  gchar *file_name;
  gdouble width, height;
  gint rw, rh;
  GdkPixbuf *pixbuf;
};

static GObjectClass *parent_class = NULL;

static void
gnome_canvas_svg_render (GnomeCanvasItem * item, GnomeCanvasBuf * buf)
{
  GnomeCanvasSVG *self = GNOME_CANVAS_SVG (item);
  gdouble render_affine[6];
  GError *error = NULL;
  gint rw, rh;

  g_object_get (self, "width", &self->priv->width, "height",
      &self->priv->height, NULL);

  gnome_canvas_item_i2c_affine (item, render_affine);
  rw = floor (self->priv->width * render_affine[0] + 0.5);
  rh = floor (self->priv->height * render_affine[3] + 0.5);

  printf ("render svg\n");
  printf (" i_w,i_h: %lf, %lf\n", self->priv->width, self->priv->height);
  printf (" z_x,z_y: %lf, %lf\n", render_affine[0], render_affine[3]);
  printf (" r_w,r_h: %d, %d\n", rw, rh);

  if (!self->priv->pixbuf || rw != self->priv->rw || rh != self->priv->rh) {
    if (self->priv->pixbuf)
      g_object_unref (self->priv->pixbuf);
    // this is deprecated
    self->priv->pixbuf = rsvg_pixbuf_from_file_at_size (self->priv->file_name,
        rw, rh, &error);
    if (error) {
      fprintf (stderr, "loading svg failed : %s", error->message);
      g_error_free (error);
      exit (1);
    }

    /* desaturate */
    {
      guint x, y, rowstride, gray;
      guchar *p;

      g_assert (gdk_pixbuf_get_colorspace (self->priv->pixbuf) ==
          GDK_COLORSPACE_RGB);
      g_assert (gdk_pixbuf_get_bits_per_sample (self->priv->pixbuf) == 8);
      g_assert (gdk_pixbuf_get_has_alpha (self->priv->pixbuf));
      g_assert (gdk_pixbuf_get_n_channels (self->priv->pixbuf) == 4);

      rowstride = gdk_pixbuf_get_rowstride (self->priv->pixbuf) - (rw * 4);
      p = gdk_pixbuf_get_pixels (self->priv->pixbuf);

      printf (" p_w,p_h: %d, %d\n",
          gdk_pixbuf_get_width (self->priv->pixbuf),
          gdk_pixbuf_get_height (self->priv->pixbuf));

      for (y = 0; y < rh; y++) {
        for (x = 0; x < rw; x++) {
          gray = ((guint) p[0] + (guint) p[1] + (guint) p[2]);
          p[0] = (guchar) (((guint) p[0] + gray) >> 2);
          p[1] = (guchar) (((guint) p[1] + gray) >> 2);
          p[2] = (guchar) (((guint) p[2] + gray) >> 2);
          /*p[3] = 255 - p[3]; */
          p += 4;
        }
        p += rowstride;
      }
    }

    g_object_set (self, "pixbuf", self->priv->pixbuf, NULL);
    self->priv->rw = rw;
    self->priv->rh = rh;
  }

  GNOME_CANVAS_ITEM_CLASS (parent_class)->render (item, buf);
}

static void
gnome_canvas_svg_get_property (GObject * const object, const guint property_id,
    GValue * const value, GParamSpec * const pspec)
{
  GnomeCanvasSVG *self = GNOME_CANVAS_SVG (object);

  switch (property_id) {
    case PROP_FILE_NAME:
      g_value_set_string (value, self->priv->file_name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
gnome_canvas_svg_set_property (GObject * const object, const guint property_id,
    const GValue * const value, GParamSpec * const pspec)
{
  GnomeCanvasSVG *self = GNOME_CANVAS_SVG (object);

  switch (property_id) {
    case PROP_FILE_NAME:
      g_free (self->priv->file_name);
      self->priv->file_name = g_value_dup_string (value);
      gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
gnome_canvas_svg_dispose (GObject * const object)
{
  GnomeCanvasSVG *self = GNOME_CANVAS_SVG (object);

  if (self->priv->dispose_has_run)
    return;

  self->priv->dispose_has_run = TRUE;
  if (self->priv->pixbuf)
    g_object_unref (self->priv->pixbuf);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gnome_canvas_svg_finalize (GObject * object)
{
  GnomeCanvasSVG *self = GNOME_CANVAS_SVG (object);

  g_free (self->priv->file_name);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gnome_canvas_svg_init (GnomeCanvasSVG * self)
{
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, GNOME_TYPE_CANVAS_SVG,
      GnomeCanvasSVGPrivate);
}

static void
gnome_canvas_svg_class_init (GnomeCanvasPixbufClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;;
  GnomeCanvasItemClass *item_class = (GnomeCanvasItemClass *) klass;

  g_type_class_add_private (klass, sizeof (GnomeCanvasSVGPrivate));
  parent_class = g_type_class_peek_parent (klass);

  gobject_class->set_property = gnome_canvas_svg_set_property;
  gobject_class->get_property = gnome_canvas_svg_get_property;
  gobject_class->dispose = gnome_canvas_svg_dispose;
  gobject_class->finalize = gnome_canvas_svg_finalize;

  /*
     item_class->update = gnome_canvas_svg_update;
     item_class->draw = gnome_canvas_svg_draw;
   */
  item_class->render = gnome_canvas_svg_render;
  /*
     item_class->point = gnome_canvas_sgv_point;
     item_class->bounds = gnome_canvas_svg_bounds;
   */

  g_object_class_install_property (gobject_class, PROP_FILE_NAME,
      g_param_spec_string ("file-name", "svg image file name",
          "full path to the svg image file",
          NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

GType
gnome_canvas_svg_get_type (void)
{
  static GType type;

  if (!type) {
    static const GTypeInfo object_info = {
      sizeof (GnomeCanvasSVGClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) gnome_canvas_svg_class_init,
      (GClassFinalizeFunc) NULL,
      NULL,                     /* class_data */
      sizeof (GnomeCanvasSVG),
      0,                        /* n_preallocs */
      (GInstanceInitFunc) gnome_canvas_svg_init,
      NULL                      /* value_table */
    };

    type = g_type_register_static (GNOME_TYPE_CANVAS_PIXBUF, "GnomeCanvasSVG",
        &object_info, 0);
  }

  return type;
}


// demo code

static void
destroy (GtkWidget * widget, gpointer data)
{
  gtk_main_quit ();
}

// event handler for moving the icon
static gboolean
on_canvas_item_event (GnomeCanvasItem * citem, GdkEvent * event,
    gpointer user_data)
{
  static gboolean dragging = FALSE;
  static gdouble dragx, dragy;
  gboolean res = FALSE;

  switch (event->type) {
    case GDK_BUTTON_PRESS:
      if (event->button.button == 1) {
        dragx = event->button.x;
        dragy = event->button.y;
        dragging = TRUE;
        res = TRUE;
      }
      break;
    case GDK_BUTTON_RELEASE:
      if (event->button.button == 1) {
        dragging = FALSE;
        res = TRUE;
      }
      break;
    case GDK_MOTION_NOTIFY:
      if (dragging) {
        gnome_canvas_item_move (citem, event->button.x - dragx,
            event->button.y - dragy);
        dragx = event->button.x;
        dragy = event->button.y;
        res = TRUE;
      }
      break;
    default:
      break;
  }
  return res;
}

// track the canvas size
static void
on_canvas_size_allocate (GtkWidget * widget, GtkAllocation * allocation,
    gpointer user_data)
{
  static gchar title[1000];
  GtkWindow *window = GTK_WINDOW (user_data);

  sprintf (title, "SVG canvas demo [canvas size %4d x %4d]", allocation->width,
      allocation->height);
  gtk_window_set_title (window, title);
}

// zoom controls
static void
on_zoom_in (GtkToolButton * toolbutton, gpointer user_data)
{
  GnomeCanvas *canvas = GNOME_CANVAS (user_data);

  zoom *= 1.5;
  gnome_canvas_set_pixels_per_unit (canvas, zoom);
}

static void
on_zoom_reset (GtkToolButton * toolbutton, gpointer user_data)
{
  GnomeCanvas *canvas = GNOME_CANVAS (user_data);

  zoom = 1.0;
  gnome_canvas_set_pixels_per_unit (canvas, zoom);
}

static void
on_zoom_out (GtkToolButton * toolbutton, gpointer user_data)
{
  GnomeCanvas *canvas = GNOME_CANVAS (user_data);

  zoom /= 1.5;
  gnome_canvas_set_pixels_per_unit (canvas, zoom);
}


int
main (int argc, char **argv)
{
  GtkWidget *window, *vbox, *tb;
  GtkWidget *scrolled_window;
  GtkToolItem *ti;
  GnomeCanvas *canvas;
  GnomeCanvasPoints *points;
  GdkPixbuf *pixbuf;
  GnomeCanvasItem *ci;
  GError *error = NULL;
  gint i;

  gtk_init (&argc, &argv);
  rsvg_init ();

  // create window and layout container
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_size_request (GTK_WIDGET (window), 400, 400);
  gtk_window_set_title (GTK_WINDOW (window), "SVG canvas demo");
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (destroy), NULL);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  // create the canvas
  gtk_widget_push_colormap (gdk_rgb_get_colormap ());
  canvas = GNOME_CANVAS (gnome_canvas_new_aa ());
  gtk_widget_pop_colormap ();

  // add a toolbar
  tb = gtk_toolbar_new ();
  gtk_box_pack_start (GTK_BOX (vbox), tb, FALSE, FALSE, 0);

  ti = gtk_tool_button_new_from_stock (GTK_STOCK_ZOOM_IN);
  g_signal_connect (G_OBJECT (ti), "clicked", G_CALLBACK (on_zoom_in),
      (gpointer) canvas);
  gtk_toolbar_insert (GTK_TOOLBAR (tb), ti, -1);
  ti = gtk_tool_button_new_from_stock (GTK_STOCK_ZOOM_100);
  g_signal_connect (G_OBJECT (ti), "clicked", G_CALLBACK (on_zoom_reset),
      (gpointer) canvas);
  gtk_toolbar_insert (GTK_TOOLBAR (tb), ti, -1);
  ti = gtk_tool_button_new_from_stock (GTK_STOCK_ZOOM_OUT);
  g_signal_connect (G_OBJECT (ti), "clicked", G_CALLBACK (on_zoom_out),
      (gpointer) canvas);
  gtk_toolbar_insert (GTK_TOOLBAR (tb), ti, -1);

  // add a scroll container
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);

  // add the canvas
  gnome_canvas_set_center_scroll_region (canvas, TRUE);
  gnome_canvas_set_scroll_region (canvas, -RANGE, -RANGE, RANGE, RANGE);
  gnome_canvas_set_pixels_per_unit (canvas, zoom);
  gtk_container_add (GTK_CONTAINER (scrolled_window), GTK_WIDGET (canvas));

  // we have refresh issues as soon as the canvas gets bigger that 961 x 721
  g_signal_connect (G_OBJECT (canvas), "size-allocate",
      G_CALLBACK (on_canvas_size_allocate), (gpointer) window);

  // add some background
  points = gnome_canvas_points_new (2);
  points->coords[1] = -RANGE;
  points->coords[3] = RANGE;
  for (i = -RANGE; i < RANGE; i += 10) {
    points->coords[0] = points->coords[2] = (gdouble) i;
    gnome_canvas_item_new (gnome_canvas_root (canvas),
        GNOME_TYPE_CANVAS_LINE,
        "points", points, "fill-color", "gray", "width-pixels", 1, NULL);
  }
  points->coords[0] = -RANGE;
  points->coords[2] = RANGE;
  for (i = -RANGE; i < RANGE; i += 10) {
    points->coords[1] = points->coords[3] = (gdouble) i;
    gnome_canvas_item_new (gnome_canvas_root (canvas),
        GNOME_TYPE_CANVAS_LINE,
        "points", points, "fill-color", "gray", "width-pixels", 1, NULL);
  }
  gnome_canvas_points_free (points);

  // add one svg as a pixbuf item
#ifdef USE_V1
  RsvgHandle *svg;
  RsvgDimensionData dim;
  const gchar *str;

  svg = rsvg_handle_new_from_file ("generator-solo.svg", &error);
  if (error) {
    fprintf (stderr, "loading svg failed : %s", error->message);
    g_error_free (error);
    exit (1);
  }
  if ((str = rsvg_handle_get_desc (svg))) {
    printf ("svg description: %s\n", str);
  }
  if ((str = rsvg_handle_get_title (svg))) {
    printf ("svg title: %s\n", str);
  }
  rsvg_handle_get_dimensions (svg, &dim);
  printf ("svg dimensions: w,h = %d,%d,  em,ex = %lf,%lf\n",
      dim.width, dim.height, dim.em, dim.ex);

  // I can't affect the size :(
  //rsvg_handle_set_dpi (svg, 10.0);
  //rsvg_handle_set_dpi_x_y (svg, 30.0, 30.0);
  pixbuf = rsvg_handle_get_pixbuf (svg);
  g_object_unref (svg);
  if (!pixbuf) {
    fprintf (stderr, "no pixmap rendered");
    exit (1);
  }
#else
  // this is deprecated
  pixbuf = rsvg_pixbuf_from_file_at_size ("generator-solo.svg", 96, 96, &error);
  if (error) {
    fprintf (stderr, "loading svg failed : %s", error->message);
    g_error_free (error);
    exit (1);
  }
#endif

  ci = gnome_canvas_item_new (gnome_canvas_root (canvas),
      GNOME_TYPE_CANVAS_PIXBUF, "pixbuf", pixbuf, "x", -50.0, "y", -20.0, NULL);
  g_signal_connect (G_OBJECT (ci), "event", G_CALLBACK (on_canvas_item_event),
      NULL);

  // add another svg as a svg item
  ci = gnome_canvas_item_new (gnome_canvas_root (canvas),
      GNOME_TYPE_CANVAS_SVG,
      "file-name", "effect-mute.svg",
      "pixbuf", gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 96, 96),
      "x", 0.0,
      "y", 0.0,
      "width", 96.0,
      "height", 96.0, "width-in-pixels", TRUE, "height-in-pixels", TRUE, NULL);
  g_signal_connect (G_OBJECT (ci), "event", G_CALLBACK (on_canvas_item_event),
      NULL);

  // add another svg again as a pixbuf, but using pixbuf api
  ci = gnome_canvas_item_new (gnome_canvas_root (canvas),
      GNOME_TYPE_CANVAS_PIXBUF,
      "pixbuf", gdk_pixbuf_new_from_file ("master.svg", NULL),
      "x", 50.0, "y", 20.0, NULL);
  g_signal_connect (G_OBJECT (ci), "event", G_CALLBACK (on_canvas_item_event),
      NULL);


  // show and run
  gtk_widget_show_all (window);
  gtk_main ();

  return (0);
}
