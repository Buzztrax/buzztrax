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

// grid
#define RANGE 300.0
#define RANGE2 (RANGE+RANGE)

// event handler for moving the icon
static gboolean on_canvas_item_event(GnomeCanvasItem *citem, GdkEvent *event, gpointer user_data) {
  static gboolean dragging=FALSE;
  static gdouble dragx,dragy;
  gboolean res=FALSE;

  switch (event->type) {
    case GDK_BUTTON_PRESS:
      if(event->button.button==1) {
        dragx=event->button.x;
        dragy=event->button.y;
        dragging=TRUE;
        res=TRUE;
      }
      break;
    case GDK_BUTTON_RELEASE:
      if(event->button.button==1) {
        dragging=FALSE;
        res=TRUE;
      }
      break;
    case GDK_MOTION_NOTIFY:
      if(dragging) {
        gnome_canvas_item_move(citem, event->button.x-dragx, event->button.y-dragy);
        dragx=event->button.x;
        dragy=event->button.y;
        res=TRUE;        
      }
      break;
    default:
      break;
  }
  return res;
}

static void on_canvas_size_allocate(GtkWidget *widget,GtkAllocation *allocation,gpointer user_data) {
  static gchar title[1000];
  GtkWindow *window=GTK_WINDOW(user_data);
  
  sprintf(title,"SVG canvas demo [canvas size %4d x %4d]",allocation->width,allocation->height);
  gtk_window_set_title (window, title);
}

int main(int argc, char **argv) {
  GtkWidget *window;
  GtkWidget *scrolled_window;
  GnomeCanvas *canvas;
  GnomeCanvasPoints *points;
  GdkPixbuf *pixbuf;
  GnomeCanvasItem *ci;
  GError *error = NULL;
  gint i;

  gtk_init (&argc,&argv);
  rsvg_init ();

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_size_request(GTK_WIDGET(window), 400, 300);
  gtk_window_set_title (GTK_WINDOW(window), "SVG canvas demo");
  g_signal_connect(G_OBJECT(window), "destroy",	G_CALLBACK (destroy), NULL);

  // add a scroll container
  scrolled_window=gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_ETCHED_IN);
  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (scrolled_window));
  
  // add the canvas
  gtk_widget_push_colormap (gdk_rgb_get_colormap ());
  canvas = GNOME_CANVAS (gnome_canvas_new_aa ());
  gtk_widget_set_size_request(GTK_WIDGET(window), RANGE2, RANGE2);
  gtk_widget_pop_colormap ();
  gnome_canvas_set_center_scroll_region (canvas, TRUE);
  gnome_canvas_set_scroll_region (canvas, -RANGE, -RANGE, RANGE, RANGE);
  gnome_canvas_set_pixels_per_unit (canvas, 1.0);
  gtk_container_add (GTK_CONTAINER (scrolled_window), GTK_WIDGET (canvas));
  
  // we have refresh issues as soon as the canvas gets bigger that 961 x 721
  g_signal_connect(G_OBJECT(canvas),"size-allocate",G_CALLBACK(on_canvas_size_allocate),(gpointer)window);

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

  ci=gnome_canvas_item_new (gnome_canvas_root (canvas),
     GNOME_TYPE_CANVAS_PIXBUF,
     "pixbuf", pixbuf,
     "x", 0.0,
     "y", 0.0,
     NULL);
  g_signal_connect(G_OBJECT(ci),"event",G_CALLBACK(on_canvas_item_event),NULL);

  gtk_widget_show_all (window);
  gtk_main ();

  return(0);
}
