/*
 * test buttons with shaded background
 *
 * gcc -Wall -g minibutton.c -o minibutton `pkg-config gtk+-3.0 --cflags  --libs`
 */

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>

static void
widget_shade_bg_color (GtkWidget * widget, GtkStateFlags state, gfloat rf,
    gfloat gf, gfloat bf)
{
  GtkStyle *style = gtk_widget_get_style (widget);
  GdkColor src = style->bg[state];
  GdkRGBA dst;
  gfloat c;

  dst.alpha = 1.0;
  c = ((gfloat) src.red * rf) / 65535.0;
  dst.red = MIN (c, 1.0);
  c = ((gfloat) src.green * gf) / 65535.0;
  dst.green = MIN (c, 1.0);
  c = ((gfloat) src.blue * bf) / 65535.0;
  dst.blue = MIN (c, 1.0);
  // TODO(ensonic): gtk+3 ignores the background color :/
  gtk_widget_override_background_color (widget, state, &dst);
  gtk_widget_override_color (widget, state, &dst);
}

static GtkWidget *
make_mini_button (const gchar * txt, gfloat rf, gfloat gf, gfloat bf,
    gboolean toggled)
{
  GtkWidget *button;

  button = gtk_toggle_button_new_with_label (txt);
  gtk_widget_set_name (button, "mini-button");
  widget_shade_bg_color (button,
      GTK_STATE_FLAG_ACTIVE | GTK_STATE_FLAG_PRELIGHT, rf, gf, bf);
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);

  if (toggled)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), toggled);

  return (button);
}


gint
main (gint argc, gchar * argv[])
{
  GtkWidget *window, *box, *button;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "colored mini buttons");
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (gtk_main_quit),
      NULL);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add (GTK_CONTAINER (window), box);

  button = make_mini_button ("M", 1.2, 1.0 / 1.25, 1.0 / 1.25, TRUE);   // red
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);

  button = make_mini_button ("S", 1.0 / 1.2, 1.0 / 1.2, 1.1, TRUE);     // blue
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);

  button = make_mini_button ("B", 1.2, 1.0 / 1.1, 1.0 / 1.4, TRUE);     // orange
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);

  gtk_widget_show_all (window);
  gtk_main ();

  return 0;
}
