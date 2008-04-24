/*
 * try svg images on canvas
 *
 * gcc -Wall -g `pkg-config gtk+-2.0 libgnomecanvas-2.0 librsvg-2.0 --cflags --libs` svgcanvas.c -o svgcanvas
 */
 
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

static void destroy(GtkWidget *widget,gpointer data) {
  gtk_main_quit ();
}


#define RANGE 500.0

int main(int argc, char **argv) {
  GtkWidget *window;
  GnomeCanvas *canvas;
  GnomeCanvasPoints *points;
  GdkPixbuf *pixbuf;
  GError *error = NULL;
  gint i;

  gtk_init (&argc,&argv);
  rsvg_init ();

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW(window), "Style colors");
  g_signal_connect(G_OBJECT(window), "destroy",	G_CALLBACK (destroy), NULL);

  // add the canvas
  gtk_widget_push_colormap (gdk_rgb_get_colormap ());
  canvas = GNOME_CANVAS (gnome_canvas_new_aa ());
  gtk_widget_pop_colormap ();
  gnome_canvas_set_center_scroll_region (canvas, TRUE);
  gnome_canvas_set_scroll_region (canvas, -RANGE, -RANGE, RANGE, RANGE);
  gnome_canvas_set_pixels_per_unit (canvas, 1.0);
  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (canvas));

  // add some background
  points = gnome_canvas_points_new (2);
  points->coords[1]=-RANGE;
  points->coords[3]= RANGE;
  for (i=-RANGE;i<RANGE;i+=10) {
    points->coords[0]=points->coords[2]=(gdouble)i;
    gnome_canvas_item_new (gnome_canvas_root (canvas),
                            GNOME_TYPE_CANVAS_LINE,
                            "points", points,
                            "fill-color", "gray",
                            "width-pixels", 1,
                            NULL);
  }
  points->coords[0]=-RANGE;
  points->coords[2]= RANGE;
  for (i=-RANGE;i<RANGE;i+=10) {
    points->coords[1]=points->coords[3]=(gdouble)i;
    gnome_canvas_item_new (gnome_canvas_root (canvas),
                            GNOME_TYPE_CANVAS_LINE,
                            "points", points,
                            "fill-color", "gray",
                            "width-pixels", 1,
                            NULL);
  }
  gnome_canvas_points_free(points);
  
  // add the svg
#if 0
  RsvgHandle *svg;
  RsvgDimensionData dim;
  const gchar *str;

  svg = rsvg_handle_new_from_file ("generator-solo.svg", &error);
  if(error) {
    fprintf (stderr, "loading svg failed : %s", error->message);
    g_error_free(error);
    exit(1);
  }
  if ((str = rsvg_handle_get_desc (svg))) {
    printf("svg description: %s\n",str);
  }
  if ((str = rsvg_handle_get_title (svg))) {
    printf("svg title: %s\n",str);
  }
  rsvg_handle_get_dimensions (svg, &dim);
  printf("svg dimensions: w,h = %d,%d,  em,ex = %lf,%lf\n",
    dim.width, dim.height, dim.em, dim.ex);
  
  // I can affect the size :(
  //rsvg_handle_set_dpi (svg, 10.0);
  //rsvg_handle_set_dpi_x_y (svg, 30.0, 30.0);
  pixbuf = rsvg_handle_get_pixbuf (svg);
  g_object_unref (svg);
  if (!pixbuf) {
    fprintf (stderr, "no pixmap rendered");
    exit(1);
  }
#else
  // this is deprecated
  pixbuf = rsvg_pixbuf_from_file_at_size ("generator-solo.svg", 96, 96, &error);
  if(error) {
    fprintf (stderr, "loading svg failed : %s", error->message);
    g_error_free(error);
    exit(1);
  }
#endif
  
  gnome_canvas_item_new (gnome_canvas_root (canvas),
     GNOME_TYPE_CANVAS_PIXBUF,
     "pixbuf", pixbuf,
     "x", 0.0,
     "y", 0.0,
     NULL);

  gtk_widget_show_all (window);
  gtk_main ();

  return(0);
}
