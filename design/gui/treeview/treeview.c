/** $Id: treeview.c,v 1.1 2007-05-13 19:42:59 ensonic Exp $
 * test own treemodel and cellrenderers
 *
 * http://scentric.net/tutorial/
 *
 * gcc -Wall -g `pkg-config gtk+-2.0 --cflags --libs` treeview.c -o treeview
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>

GtkWidget *window=NULL;
GtkWidget *tree_view=NULL;

// own model


// cell renderer


// test-app

static void destroy(GtkWidget *widget,gpointer data) {
  gtk_main_quit();
}

static void init() {
  gint i;
  GtkWidget *scrolled_window=NULL;
  GtkCellRenderer *renderer;
  GtkListStore *store;
  GtkTreeIter tree_iter;

  window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "Syncronized views");
  gtk_window_resize(GTK_WINDOW(window),300,200);
  g_signal_connect(G_OBJECT(window), "destroy",	G_CALLBACK (destroy), NULL);

  // first view
  scrolled_window=gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window),GTK_SHADOW_NONE);
  tree_view=gtk_tree_view_new();
  g_object_set(GTK_TREE_VIEW(tree_view),"enable-search",FALSE,"rules-hint",TRUE,NULL);
  gtk_widget_set_size_request(GTK_WIDGET(tree_view),100,40);
  gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view)),GTK_SELECTION_BROWSE);
  renderer=gtk_cell_renderer_text_new();
  g_object_set(G_OBJECT(renderer),"xalign",1.0,NULL);
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view),-1,"Col 1",renderer,"text",0,NULL);
  renderer=gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_view),-1,"Col 2",renderer,"text",1,NULL);
  gtk_container_add(GTK_CONTAINER(scrolled_window),tree_view);
  gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(scrolled_window));

  // list store
  store=gtk_list_store_new(2,G_TYPE_INT,G_TYPE_INT);

  for(i=0;i<15;i++) {
    gtk_list_store_append(store, &tree_iter);
    gtk_list_store_set(store,&tree_iter,0,i,1,i*i,-1);
  }
  gtk_tree_view_set_model(GTK_TREE_VIEW(tree_view),GTK_TREE_MODEL(store));
  g_object_unref(store); // drop with treeview

  gtk_widget_show_all(window);
}

static void done() {

}

int main(int argc, char **argv) {
  gtk_init(&argc,&argv);

  init();
  gtk_main();
  done();

  return(0);
}
