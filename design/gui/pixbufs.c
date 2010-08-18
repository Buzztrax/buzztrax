/** $Id$
 * test svg pixbufs
 *
 * gcc -Wall -g `pkg-config gtk+-2.0 --cflags --libs` pixbufs.c -o pixbufs
 *
 * GTK_DEBUG=icontheme ./pixbufs
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <glib.h>

static GtkWidget *window=NULL;


static void destroy(GtkWidget *widget,gpointer data) {
  gtk_main_quit();
}

static void init() {
  const gchar img_name[] = "buzztard_master";
  const gchar img_path[] = "master.svg";
  const gint size = 100;
  
  GtkWidget *image, *box;
  GdkPixbuf *pixbuf;
  GtkIconTheme *it = gtk_icon_theme_get_default();
  GError *error = NULL;

  window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "Pixbuf");
  g_signal_connect(G_OBJECT(window), "destroy",	G_CALLBACK (destroy), NULL);

  box = gtk_vbox_new(FALSE,3);
  gtk_container_add(GTK_CONTAINER(window),box);

  /* from file */
  pixbuf = gdk_pixbuf_new_from_file(img_path, &error);
  if(!pixbuf) {
    fprintf(stderr, "Failed to load pixbuf from file: %s: %s\n", img_path, error->message);
    g_error_free(error);
    gtk_container_add(GTK_CONTAINER(box), gtk_label_new("-"));
  } else {
    image = gtk_image_new_from_pixbuf(pixbuf);
    gtk_container_add(GTK_CONTAINER(box),image);
  }

  /* from theme */

  /* @todo: docs recommend to listen to GtkWidget::style-set and update icon or
   * do gdk_pixbuf_copy() to avoid gtk keeping icon-theme loaded if it changes
  */
  pixbuf=gtk_icon_theme_load_icon(it,img_name,size,0,&error);
  if(!pixbuf) {
    fprintf(stderr, "Failed to load pixbuf from theme %s %dx%d icon: %s\n", img_name, size, size, error->message);
    g_error_free(error);
    gtk_container_add(GTK_CONTAINER(box), gtk_label_new("-"));
  } else {
    if (gdk_pixbuf_get_width (pixbuf) != size) {
      fprintf(stderr, "Size is %dx%d instead of %d,%d\n", gdk_pixbuf_get_width (pixbuf), gdk_pixbuf_get_height(pixbuf), size, size);
    }
    
    image = gtk_image_new_from_pixbuf(pixbuf);
    gtk_container_add(GTK_CONTAINER(box),image);
  }

  gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
  gtk_init(&argc,&argv);

  init();
  gtk_main();

  return(0);
}
