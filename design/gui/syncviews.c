/* test synchonized scrolling of two treeviews
 *
 * gcc -Wall -g syncviews.c -o syncviews `pkg-config gtk+-2.0 --cflags --libs`
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>

GtkWidget *window = NULL;
GtkWidget *scrolled_window1 = NULL;
GtkWidget *scrolled_window2 = NULL;
GtkWidget *tree_view1 = NULL;
GtkWidget *tree_view2 = NULL;

static void
destroy (GtkWidget * widget, gpointer data)
{
  gtk_main_quit ();
}

static void
init ()
{
  gint i;
  GtkWidget *box;
  GtkCellRenderer *renderer;
  GtkListStore *store;
  GtkTreeIter tree_iter;
  GtkAdjustment *vadjust;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Syncronized views");
  //gtk_widget_set_size_request(GTK_WIDGET(window),300,200);
  //gtk_widget_set_usize(GTK_WIDGET(window), 300, 200);
  // if we use this, that sizing does not work as expected
  //gtk_window_set_default_size(GTK_WINDOW(window),300, 250);
  gtk_window_resize (GTK_WINDOW (window), 300, 200);
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (destroy), NULL);

  box = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), box);

  // first view
  scrolled_window1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window1),
      GTK_POLICY_NEVER, GTK_POLICY_NEVER);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window1),
      GTK_SHADOW_NONE);
  tree_view1 = gtk_tree_view_new ();
  g_object_set (GTK_TREE_VIEW (tree_view1), "enable-search", FALSE,
      "rules-hint", TRUE, NULL);
  gtk_widget_set_size_request (GTK_WIDGET (tree_view1), 100, 40);
  gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW
          (tree_view1)), GTK_SELECTION_BROWSE);
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tree_view1), -1,
      "Col 1.1", renderer, "text", 0, NULL);
  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tree_view1), -1,
      "Col 1.2", renderer, "text", 1, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_window1), tree_view1);
  gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (scrolled_window1), FALSE,
      FALSE, 0);

  // add vertical separator
  gtk_box_pack_start (GTK_BOX (box), gtk_vseparator_new (), FALSE, FALSE, 0);

  // second view
  scrolled_window2 = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window2),
      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window2),
      GTK_SHADOW_NONE);
  tree_view2 = gtk_tree_view_new ();
  g_object_set (GTK_TREE_VIEW (tree_view2), "enable-search", FALSE,
      "rules-hint", TRUE, NULL);
  gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW
          (tree_view2)), GTK_SELECTION_BROWSE);
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tree_view2), -1,
      "Col 2.1", renderer, "text", 0, NULL);
  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tree_view2), -1,
      "Col 2.2", renderer, "text", 1, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_window2), tree_view2);
  gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (scrolled_window2), TRUE, TRUE,
      0);

  // shared list store
  store = gtk_list_store_new (2, G_TYPE_INT, G_TYPE_INT);

  for (i = 0; i < 15; i++) {
    gtk_list_store_append (store, &tree_iter);
    gtk_list_store_set (store, &tree_iter, 0, i, 1, i * i, -1);
  }
  gtk_tree_view_set_model (GTK_TREE_VIEW (tree_view1), GTK_TREE_MODEL (store));
  gtk_tree_view_set_model (GTK_TREE_VIEW (tree_view2), GTK_TREE_MODEL (store));
  g_object_unref (store);       // drop with treeview

  // make first scrolled-window also use the horiz-scrollbar of the second scrolled-window
  vadjust =
      gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW
      (scrolled_window2));
  gtk_scrolled_window_set_vadjustment (GTK_SCROLLED_WINDOW (scrolled_window1),
      vadjust);

  gtk_widget_show_all (window);
}

static void
done ()
{

}

int
main (int argc, char **argv)
{
  gtk_init (&argc, &argv);

  init ();
  gtk_main ();
  done ();

  return (0);
}
