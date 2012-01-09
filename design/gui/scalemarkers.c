/*
 * test app for scale markers
 * 
 * gcc -Wall -g `pkg-config gtk+-2.0 --cflags --libs` scalemarkers.c -o scalemarkers
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <glib.h>

static void destroy (GtkWidget *widget,gpointer data) {
  gtk_main_quit ();
}

gint main (gint argc, gchar **argv) {
  GtkWidget *window, *scale, *frame, *box;
  GtkAdjustment *adj;
  gint v = 0;

  gtk_init (&argc,&argv);
  
  if (argc > 1) {
    v = atoi (argv[1]);
  }
  
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Scales");
  g_signal_connect (G_OBJECT (window), "destroy",	G_CALLBACK (destroy), NULL);
  
  box = gtk_vbox_new (FALSE, 0); 
  
  if (v==0 || v==1) {
    frame = gtk_frame_new ("0.0 -> 1.0");

    adj = (GtkAdjustment *)gtk_adjustment_new (0.0, 0.0, 1.0, 0.01, 0.05, 0.0);
    scale = gtk_hscale_new (adj);
    gtk_scale_add_mark (GTK_SCALE (scale),  0.0,  GTK_POS_BOTTOM, "0 %");
    gtk_scale_add_mark (GTK_SCALE (scale),  0.25, GTK_POS_BOTTOM, NULL);
    gtk_scale_add_mark (GTK_SCALE (scale),  0.5,  GTK_POS_BOTTOM, "50 %");
    gtk_scale_add_mark (GTK_SCALE (scale),  0.75, GTK_POS_BOTTOM, NULL);
    gtk_scale_add_mark (GTK_SCALE (scale),  1.0,  GTK_POS_BOTTOM, "100 %");

    gtk_container_add (GTK_CONTAINER (frame), scale);
    gtk_container_add (GTK_CONTAINER (box), frame);
  }
  if (v==0 || v==2) {
    frame = gtk_frame_new ("-1.0 -> 1.0");

    adj = (GtkAdjustment *)gtk_adjustment_new (0.0, -1.0, 1.0, 0.01, 0.05, 0.0);
    scale = gtk_hscale_new (adj);
    gtk_scale_add_mark (GTK_SCALE (scale), -1.0,  GTK_POS_BOTTOM, "-100 %");
    gtk_scale_add_mark (GTK_SCALE (scale), -0.75, GTK_POS_BOTTOM, NULL);
    gtk_scale_add_mark (GTK_SCALE (scale), -0.5,  GTK_POS_BOTTOM, NULL);
    gtk_scale_add_mark (GTK_SCALE (scale), -0.25, GTK_POS_BOTTOM, NULL);
    gtk_scale_add_mark (GTK_SCALE (scale),  0.0,  GTK_POS_BOTTOM, "0 %");
    gtk_scale_add_mark (GTK_SCALE (scale),  0.25, GTK_POS_BOTTOM, NULL);
    gtk_scale_add_mark (GTK_SCALE (scale),  0.5,  GTK_POS_BOTTOM, NULL);
    gtk_scale_add_mark (GTK_SCALE (scale),  0.75, GTK_POS_BOTTOM, NULL);
    gtk_scale_add_mark (GTK_SCALE (scale),  1.0,  GTK_POS_BOTTOM, "100 %");

    gtk_container_add (GTK_CONTAINER (frame), scale);
    gtk_container_add (GTK_CONTAINER (box), frame);
  }
    
  gtk_container_add (GTK_CONTAINER (window), box);

  gtk_widget_show_all (window);
  gtk_main ();

  return 0 ;
}
