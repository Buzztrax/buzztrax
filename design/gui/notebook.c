/* test adding pages to notebook
 *
 * gcc -Wall -g `pkg-config gtk+-2.0 --cflags --libs` notebook.c -o notebook
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

static void on_page_switched(GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, gpointer user_data) {
  printf("page has switched : page=%d\n",page_num);
}

static void add_page(GtkNotebook *notebook,const gchar *name) {
  GtkWidget *child,*label,*event_box;

  child=gtk_label_new(name);

  label=gtk_label_new(name);
  gtk_widget_show(label);
  event_box=gtk_event_box_new();
  g_object_set(event_box,"visible-window",FALSE,NULL);
  gtk_container_add(GTK_CONTAINER(event_box),label);

  gtk_notebook_insert_page(notebook,child,event_box,-1);
}

static void init() {
  GtkWidget *notebook;

  window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "Notebook with pages");
  g_signal_connect(G_OBJECT(window), "destroy",	G_CALLBACK (destroy), NULL);

  notebook=gtk_notebook_new();

  add_page(GTK_NOTEBOOK(notebook),"page 1");
  add_page(GTK_NOTEBOOK(notebook),"page 2");
  add_page(GTK_NOTEBOOK(notebook),"page 3");
  add_page(GTK_NOTEBOOK(notebook),"page 4");

  g_signal_connect(notebook,"switch-page", G_CALLBACK(on_page_switched),NULL);

  gtk_container_add(GTK_CONTAINER(window),notebook);
  gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
  gtk_init(&argc,&argv);

  init();
  gtk_main();

  return(0);
}
