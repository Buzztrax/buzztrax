/*
 * test to use clutter for a gnome-canvas replacement
 *
 * gcc -Wall -g cluttercanvas.c -o cluttercanvas `pkg-config clutter-1.0 clutter-gtk-1.0 gtk+-3.0 --cflags  --libs`
 *
 * DONE:
 * - scrollable canvas
 * - machine icon
 *   - can be dragged around
 *   - becomes transparent while dragging
 * - zooming
 *
 * TODO:
 * - zooming
 *   - needs to adjust scrollbars
 *   - pick better resolution for icon images
 * - draw wires
 *   - cairo?
 * - panning by dragging the background
 * - context menu on items/stage
 */

#include <gtk/gtk.h>
#include <clutter/clutter.h>
#include <clutter-gtk/clutter-gtk.h>
#include <stdlib.h>

ClutterActor *stage = NULL;
ClutterActor *canvas = NULL;
ClutterActor *icon = NULL;

#define WIDTH 640.0
#define HEIGHT 480.0

/* toolbar actions */
static gboolean
on_change_bg_color_clicked (GtkButton * button, gpointer user_data)
{
  static gboolean already_changed = FALSE;
  if (already_changed) {
    ClutterColor stage_color = { 0x00, 0x00, 0x00, 0xff };      /* Black */
    clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_color);
  } else {
    ClutterColor stage_color = { 0xA0, 0x20, 0x20, 0xff };      /* Red */
    clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_color);
  }
  already_changed = !already_changed;
  return TRUE;
}

static gdouble zoom_x = 1.0;
static gdouble zoom_y = 1.0;

static gboolean
on_zoom_in_clicked (GtkButton * button, gpointer user_data)
{
  zoom_x *= 1.5;
  zoom_y *= 1.5;
  clutter_actor_set_scale (canvas, zoom_x, zoom_y);
  return TRUE;
}

static gboolean
on_zoom_out_clicked (GtkButton * button, gpointer user_data)
{
  zoom_x /= 1.5;
  zoom_y /= 1.5;
  clutter_actor_set_scale (canvas, zoom_x, zoom_y);
  return TRUE;
}

/* actor actions */

static gboolean
on_stage_button_press (ClutterActor * actor, ClutterEvent * event,
    gpointer user_data)
{
  gfloat x = 0, y = 0;
  clutter_event_get_coords (event, &x, &y);

  g_print ("Stage clicked at (%f, %f)\n", x, y);
  return TRUE;
}

static void
on_icon_drag_begin (ClutterDragAction * action, ClutterActor * actor,
    gfloat event_x, gfloat event_y, ClutterModifierType modifiers,
    gpointer user_data)
{
  clutter_actor_set_opacity (actor, 200);
}

static void
on_icon_drag_end (ClutterDragAction * action, ClutterActor * actor,
    gfloat event_x, gfloat event_y, ClutterModifierType modifiers,
    gpointer user_data)
{
  clutter_actor_set_opacity (actor, 255);
}

static void
on_v_scroll (GtkAdjustment * adjustment, gpointer user_data)
{
  clutter_actor_set_y (canvas, -gtk_adjustment_get_value (adjustment));
}

static void
on_h_scroll (GtkAdjustment * adjustment, gpointer user_data)
{
  clutter_actor_set_x (canvas, -gtk_adjustment_get_value (adjustment));
}

gint
main (gint argc, gchar * argv[])
{
  GtkWidget *button;
  gint i;

  if (gtk_clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
    return 1;

  GtkIconTheme *it = gtk_icon_theme_get_default ();

  /* Create the window and some child widgets: */
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_size_request (window, 200, 200);
  /* Stop the application when the window is closed: */
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  GtkWidget *vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  /* tool bar */
  GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  button = gtk_button_new_with_label ("Change BG Color");
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked", G_CALLBACK (on_change_bg_color_clicked),
      NULL);

  button = gtk_button_new_with_label ("Zoom In");
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked", G_CALLBACK (on_zoom_in_clicked), NULL);

  button = gtk_button_new_with_label ("Zoom Out");
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked", G_CALLBACK (on_zoom_out_clicked), NULL);

  /* Create a table to hold the scrollbars and the ClutterEmbed widget: */
  GtkWidget *table = gtk_table_new (2, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  /* Create the clutter widget: */
  GtkWidget *clutter_widget = gtk_clutter_embed_new ();
  gtk_table_attach (GTK_TABLE (table), clutter_widget,
      0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (clutter_widget);

  /* Get the stage and set its size and color: */
  stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (clutter_widget));
  ClutterColor stage_color = { 0x00, 0x00, 0x00, 0xff };        /* Black */
  clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_color);
  clutter_actor_set_size (stage, WIDTH, HEIGHT);
  clutter_actor_set_position (stage, 0.0, 0.0);
  /* Connect a signal handler to handle mouse clicks and key presses on the stage: */
  g_signal_connect (stage, "button-press-event",
      G_CALLBACK (on_stage_button_press), NULL);

  /* Add canvas (root group) 
   * we're using this for scrolling
   * in clutter 1.12 we shoud use ClutterScrollActor
   */
  canvas = clutter_group_new ();
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), canvas);

  /* Add boxes */
  for (i = 20; i < 250; i += 30) {
    ClutterColor actor_color = { 0, 255 - i, i, 128 };
    ClutterActor *rect = clutter_rectangle_new_with_color (&actor_color);
    clutter_container_add_actor (CLUTTER_CONTAINER (canvas), rect);
    clutter_actor_set_size (rect, 50.0, 50.0);
    clutter_actor_set_position (rect, (gfloat) i, (gfloat) i);
  }
  for (i = 20; i < 250; i += 30) {
    ClutterColor actor_color = { 255 - i, 0, i, 128 };
    ClutterActor *rect = clutter_rectangle_new_with_color (&actor_color);
    clutter_container_add_actor (CLUTTER_CONTAINER (canvas), rect);
    clutter_actor_set_size (rect, 50.0, 50.0);
    clutter_actor_set_position (rect, (gfloat) 305 - i, (gfloat) i);
  }

  /* Try an image */
  GdkPixbuf *pixbuf = gtk_icon_theme_load_icon (it,
      "buzztrax_generator", 64,
      GTK_ICON_LOOKUP_FORCE_SVG | GTK_ICON_LOOKUP_FORCE_SIZE, NULL);
  ClutterContent *image = clutter_image_new ();
  clutter_image_set_data (CLUTTER_IMAGE (image),
      gdk_pixbuf_get_pixels (pixbuf), gdk_pixbuf_get_has_alpha (pixbuf)
      ? COGL_PIXEL_FORMAT_RGBA_8888
      : COGL_PIXEL_FORMAT_RGB_888,
      gdk_pixbuf_get_width (pixbuf),
      gdk_pixbuf_get_height (pixbuf), gdk_pixbuf_get_rowstride (pixbuf), NULL);
  g_object_unref (pixbuf);
  icon = clutter_actor_new ();
  //clutter_actor_add_constraint (icon, clutter_bind_constraint_new (stage, CLUTTER_BIND_SIZE, 0.0));
  clutter_actor_set_size (icon, 64.0, 64.0);
  clutter_actor_set_content_scaling_filters (icon,
      CLUTTER_SCALING_FILTER_TRILINEAR, CLUTTER_SCALING_FILTER_LINEAR);
  clutter_actor_set_content (icon, image);
  clutter_actor_set_reactive (icon, TRUE);
  g_object_unref (image);
  clutter_container_add_actor (CLUTTER_CONTAINER (canvas), icon);
  ClutterAction *drag_action = clutter_drag_action_new ();
  g_signal_connect (drag_action, "drag-begin", G_CALLBACK (on_icon_drag_begin),
      NULL);
  g_signal_connect (drag_action, "drag-end", G_CALLBACK (on_icon_drag_end),
      NULL);
  clutter_actor_add_action (icon, drag_action);

  /* Show all content */
  clutter_actor_show_all (stage);

  /* Create scrollbars and connect them */
  GtkAdjustment *h_adjustment = NULL, *v_adjustment = NULL;
  GtkWidget *scrollbar;

  v_adjustment =
      gtk_adjustment_new (0.0, 0.0, HEIGHT, 1.0, 10.0, HEIGHT / 10.0);
  g_signal_connect (v_adjustment, "value-changed", G_CALLBACK (on_v_scroll),
      NULL);
  scrollbar = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL, v_adjustment);
  gtk_table_attach (GTK_TABLE (table), scrollbar,
      1, 2, 0, 1, 0, GTK_EXPAND | GTK_FILL, 0, 0);

  h_adjustment = gtk_adjustment_new (0.0, 0.0, WIDTH, 1.0, 10.0, WIDTH / 10.0);
  g_signal_connect (h_adjustment, "value-changed", G_CALLBACK (on_h_scroll),
      NULL);
  scrollbar = gtk_scrollbar_new (GTK_ORIENTATION_HORIZONTAL, h_adjustment);
  gtk_table_attach (GTK_TABLE (table), scrollbar,
      0, 1, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 0);

  /* Show the window: */
  gtk_widget_show_all (GTK_WIDGET (window));

  /* Start the main loop, so we can respond to events: */
  gtk_main ();

  return EXIT_SUCCESS;

}
