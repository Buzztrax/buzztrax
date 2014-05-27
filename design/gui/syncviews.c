/* test synchonized scrolling of two treeviews
 *
 * gcc -Wall -g syncviews.c -o syncviews `pkg-config gtk+-3.0 --cflags --libs` -lm
 */

#include <math.h>
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

// this is a copy from gtk-2.X::gtk_scrolled_window_scroll_event() with a small
// modification to also handle scroll events if the scrollbar is hidden (we're
// not checking gtk_widget_get_visible()
static gboolean
on_scroll_event (GtkWidget * widget, GdkEventScroll * event, gpointer user_data)
{
  GtkWidget *range = NULL;
  GdkScrollDirection direction = event->direction;
  gchar *dbg;

  if (direction == GDK_SCROLL_UP || direction == GDK_SCROLL_DOWN) {
    range = gtk_scrolled_window_get_vscrollbar (GTK_SCROLLED_WINDOW (widget));
    dbg = "vadj";
  } else if (direction == GDK_SCROLL_LEFT || direction == GDK_SCROLL_RIGHT) {
    range = gtk_scrolled_window_get_hscrollbar (GTK_SCROLLED_WINDOW (widget));
    dbg = "hadj";
  } else {
    printf ("unknown direction: %d\n", direction);      // GTK_SROLL_SMOOTH
  }

  if (range) {
    GtkAdjustment *adj = gtk_range_get_adjustment (GTK_RANGE (range));
    gdouble delta, new_value;
    gdouble value, lower, upper, page_size;

    g_object_get (adj, "value", &value, "upper", &upper, "lower", &lower,
        "page-size", &page_size, NULL);

    // from _gtk_range_get_wheel_delta()
    delta = pow (page_size, 2.0 / 3.0);
    if (direction == GDK_SCROLL_UP || direction == GDK_SCROLL_LEFT)
      delta = -delta;
    if (gtk_range_get_inverted ((GtkRange *) range))
      delta = -delta;

    new_value = CLAMP (value + delta, lower, upper - page_size);

    printf ("scrolling: %s=%p, lower=%lf < value=%lf + delta=%lf < upper=%lf\n",
        dbg, adj, lower, value, delta, (upper - page_size));

    gtk_adjustment_set_value (adj, new_value);
    return TRUE;
  }
  return FALSE;
}

static void
on_scrollbar_visibility_changed (GObject * sb, GParamSpec * property,
    gpointer user_data)
{
  gboolean visible;

  g_object_get (sb, "visible", &visible, NULL);
  if (visible) {
    printf ("hiding scrollbar again\n");
    g_object_set (sb, "visible", FALSE, NULL);
  }
}

static void
on_scrolled_window_realize (GtkWidget * widget, gpointer user_data)
{
  GtkWidget *sb;

  printf ("realized\n");
  sb = gtk_scrolled_window_get_vscrollbar (GTK_SCROLLED_WINDOW (widget));
  if (sb) {
    printf ("hiding scrollbar\n");
    g_object_set (sb, "visible", FALSE, "no-show-all", TRUE, NULL);
    g_signal_connect (sb, "notify::visible",
        G_CALLBACK (on_scrollbar_visibility_changed), NULL);
  } else {
    printf ("no v-scrollbar found\n");
  }
}

static void
on_scrolled_window_size_changed (GtkWidget * widget, GdkRectangle * allocation,
    gpointer user_data)
{
  printf ("size-changed\n");
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

  // TODO(ensonic): bug726795: setting the policy should not affect size
  GtkPolicyType policy = GTK_POLICY_NEVER;
  if (g_getenv ("POLICY_AUTO")) {
    policy = GTK_POLICY_AUTOMATIC;
  }
  if (g_getenv ("POLICY_ALWAYS")) {
    policy = GTK_POLICY_ALWAYS;
  }

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Syncronized views");
  //gtk_widget_set_size_request(GTK_WIDGET(window),300,200);
  //gtk_widget_set_usize(GTK_WIDGET(window), 300, 200);
  // if we use this, that sizing does not work as expected
  //gtk_window_set_default_size(GTK_WINDOW(window),300, 250);
  gtk_window_resize (GTK_WINDOW (window), 300, 200);
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (destroy), NULL);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add (GTK_CONTAINER (window), box);

  // first view (slave)
  scrolled_window1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window1),
      GTK_POLICY_NEVER, policy);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window1),
      GTK_SHADOW_NONE);
  switch (policy) {
    case GTK_POLICY_NEVER:
      g_signal_connect (scrolled_window1, "scroll-event",
          G_CALLBACK (on_scroll_event), NULL);
      break;
    case GTK_POLICY_ALWAYS:
      g_signal_connect (scrolled_window1, "realize",
          G_CALLBACK (on_scrolled_window_realize), NULL);
      g_signal_connect (scrolled_window1, "size-allocate",
          G_CALLBACK (on_scrolled_window_size_changed), NULL);
      break;
    default:
      break;
  }
  tree_view1 = gtk_tree_view_new ();
  g_object_set (GTK_TREE_VIEW (tree_view1), "enable-search", FALSE,
      "rules-hint", TRUE, NULL);
  gtk_widget_set_size_request (tree_view1, 100, 40);
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
  gtk_box_pack_start (GTK_BOX (box), scrolled_window1, FALSE, FALSE, 0);

  // add vertical separator
  gtk_box_pack_start (GTK_BOX (box),
      gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 0);

  // second view (master)
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
  gtk_box_pack_start (GTK_BOX (box), scrolled_window2, TRUE, TRUE, 0);

  // shared list store
  store = gtk_list_store_new (2, G_TYPE_INT, G_TYPE_INT);

  for (i = 0; i < 15; i++) {
    gtk_list_store_append (store, &tree_iter);
    gtk_list_store_set (store, &tree_iter, 0, i, 1, i * i, -1);
  }
  gtk_tree_view_set_model (GTK_TREE_VIEW (tree_view1), GTK_TREE_MODEL (store));
  gtk_tree_view_set_model (GTK_TREE_VIEW (tree_view2), GTK_TREE_MODEL (store));
  g_object_unref (store);       // drop with treeview

  // make first scrolled-window also use the horiz-scrollbar of the second 
  // scrolled-window
  vadjust =
      gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW
      (scrolled_window2));
  gtk_scrolled_window_set_vadjustment (GTK_SCROLLED_WINDOW (scrolled_window1),
      vadjust);
  printf ("shared adjustment=%p\n", vadjust);

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
