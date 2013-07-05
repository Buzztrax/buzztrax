/*
 * test buttons with shaded background
 *
 * gcc -Wall -g minibutton.c -o minibutton `pkg-config gtk+-3.0 --cflags  --libs`
 */

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>

static GtkWidget *
make_mini_button (const gchar * txt, const gchar * style)
{
  GtkWidget *button;
  GtkStyleContext *context;

  button = gtk_toggle_button_new_with_label (txt);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_widget_set_can_focus (button, FALSE);

  context = gtk_widget_get_style_context (button);
  gtk_style_context_add_class (context, "mini");
  gtk_style_context_add_class (context, style);

  return (button);
}

gint
main (gint argc, gchar * argv[])
{
  GtkWidget *window, *box, *button;
  GtkStyleProvider *provider;

  gtk_init (&argc, &argv);

  /* load custom style */
  provider = (GtkStyleProvider *) gtk_css_provider_new ();
  // prelight works, and color works
  // the selected state and the background color remains unchanged :/
  gtk_css_provider_load_from_data (GTK_CSS_PROVIDER (provider),
      ".mini {\n"
      "   margin:  0;\n"
      "   padding: 0;\n"
      "}\n"
      "@define-color mute-bg #f0a0a0;\n"
      ".mute:prelight, .mute:active {\n"
      "   background-color: @mute-bg;\n"
      "   background-image: -gtk-gradient (linear, 0 0, 0 1, from(darker(@mute-bg)), to(lighter(@mute-bg)));\n"
      "}\n"
      "@define-color solo-bg #a0a0f0;\n"
      ".solo:prelight, .solo:active {\n"
      "   background-color: @solo-bg;\n"
      "   background-image: -gtk-gradient (linear, 0 0, 0 1, from(darker(@solo-bg)), to(lighter(@solo-bg)));\n"
      "}\n"
      "@define-color bypass-bg #f0d090;\n"
      ".bypass:prelight, .bypass:active {\n"
      "   background-color: @bypass-bg;\n"
      "   background-image: -gtk-gradient (linear, 0 0, 0 1, from(darker(@bypass-bg)), to(lighter(@bypass-bg)));\n"
      "}\n" "", -1, NULL);

  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
      provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  /* create ui */

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "colored mini buttons");
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (gtk_main_quit),
      NULL);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add (GTK_CONTAINER (window), box);

  button = make_mini_button ("M", "mute");
  gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE, 0);

  button = make_mini_button ("S", "solo");
  gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE, 0);

  button = make_mini_button ("B", "bypass");
  gtk_box_pack_start (GTK_BOX (box), button, TRUE, TRUE, 0);

  gtk_widget_show_all (window);
  gtk_main ();

  return 0;
}
