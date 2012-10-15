/*
 * test to use clutter for a gnome-canvas replacement
 *
 * gcc -Wall -g cluttercanvas.c -o cluttercanvas `pkg-config clutter-1.0 clutter-gtk-0.10 gtk+-2.0 --cflags  --libs`
 */

#include <gtk/gtk.h>
#include <clutter/clutter.h>
#include <clutter-gtk/clutter-gtk.h>
#include <stdlib.h>

ClutterActor *stage = NULL;

static gboolean
on_button_clicked (GtkButton * button, gpointer user_data)
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
  return TRUE;                  /* Stop further handling of this event. */
}

static gboolean
on_stage_button_press (ClutterStage * stage, ClutterEvent * event,
    gpointer user_data)
{
  gfloat x = 0, y = 0;
  clutter_event_get_coords (event, &x, &y);

  g_print ("Stage clicked at (%f, %f)\n", x, y);
  return TRUE;                  /* Stop further handling of this event. */
}

int
main (int argc, char *argv[])
{
  gint i;

  gtk_clutter_init (&argc, &argv);

  /* Create the window and some child widgets: */
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_size_request (window, 200, 200);
  /* Stop the application when the window is closed: */
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  GtkWidget *vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  GtkWidget *button = gtk_button_new_with_label ("Change Color");
  gtk_box_pack_end (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  g_signal_connect (button, "clicked", G_CALLBACK (on_button_clicked), NULL);

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
  clutter_actor_set_size (stage, 640.0, 480.0);
  clutter_actor_set_position (stage, 0.0, 0.0);
  /* Connect a signal handler to handle mouse clicks and key presses on the stage: */
  g_signal_connect (stage, "button-press-event",
      G_CALLBACK (on_stage_button_press), NULL);

  /* Create a viewport actor to be able to scroll actor. By passing NULL it
   * will create new GtkAdjustments. */
  ClutterActor *viewport = gtk_clutter_viewport_new (NULL, NULL, NULL);
  clutter_actor_set_position (viewport, 0.0, 0.0);
  clutter_actor_set_size (viewport, 640.0, 480.0);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), viewport);

  /* Add boxes */
  for (i = 20; i < 250; i += 30) {
    ClutterColor actor_color = { 0, 255 - i, i, 128 };
    ClutterActor *rect = clutter_rectangle_new_with_color (&actor_color);
    // FIXME: objects are all at top left
    //clutter_container_add_actor(CLUTTER_CONTAINER(viewport), rect);
    clutter_container_add_actor (CLUTTER_CONTAINER (stage), rect);
    clutter_actor_set_size (rect, 50.0, 50.0);
    clutter_actor_set_position (rect, (gfloat) i, (gfloat) i);
  }
  clutter_actor_show_all (stage);

  /* Create scrollbars and connect them to viewport: */
  GtkAdjustment *h_adjustment = NULL, *v_adjustment = NULL;
  gtk_clutter_scrollable_get_adjustments (GTK_CLUTTER_SCROLLABLE (viewport),
      &h_adjustment, &v_adjustment);
  GtkWidget *scrollbar = gtk_vscrollbar_new (v_adjustment);
  gtk_table_attach (GTK_TABLE (table), scrollbar,
      1, 2, 0, 1, 0, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_adjustment_set_page_size (v_adjustment,
      gtk_adjustment_get_page_size (v_adjustment) / 2.0);
  g_print ("v_adj: %lf ... %lf ... %lf : %lf\n",
      gtk_adjustment_get_lower (v_adjustment),
      gtk_adjustment_get_value (v_adjustment),
      gtk_adjustment_get_upper (v_adjustment),
      gtk_adjustment_get_page_size (v_adjustment));

  scrollbar = gtk_hscrollbar_new (h_adjustment);
  gtk_table_attach (GTK_TABLE (table), scrollbar,
      0, 1, 1, 2, GTK_EXPAND | GTK_FILL, 0, 0, 0);
  gtk_adjustment_set_page_size (h_adjustment,
      gtk_adjustment_get_page_size (h_adjustment) / 2.0);
  g_print ("h_adj: %lf ... %lf ... %lf : %lf\n",
      gtk_adjustment_get_lower (h_adjustment),
      gtk_adjustment_get_value (h_adjustment),
      gtk_adjustment_get_upper (h_adjustment),
      gtk_adjustment_get_page_size (h_adjustment));

  /* Show the window: */
  gtk_widget_show_all (GTK_WIDGET (window));

  /* Start the main loop, so we can respond to events: */
  gtk_main ();

  return EXIT_SUCCESS;

}
