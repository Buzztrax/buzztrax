/* port waveform-viewer to gtk3
 *
 * gcc -Wall -g -I. wave.c waveform-viewer.c -o wave -lm `pkg-config gtk+-3.0 --cflags --libs`
 */

#include <math.h>
#include <gtk/gtk.h>
#include <glib.h>
#include "waveform-viewer.h"

#define DATA_SIZE 500

static gboolean
move_playback_cursor (gpointer data)
{
  static gint64 pos = 0;

  g_object_set (data, "playback-cursor", pos, NULL);
  pos += 10;
  if (pos >= DATA_SIZE)
    pos = 0;
  return TRUE;
}

gint
main (gint argc, gchar ** argv)
{
  GtkWidget *window, *wave;
  gint16 data[DATA_SIZE];
  gint i;
  gdouble f, v;

  gtk_init (&argc, &argv);

  for (i = 0; i < DATA_SIZE; i++) {
    f = (gdouble) i;
    v = 32767.0 * (1.0 - (f / DATA_SIZE)) * sin (f / 50.0);
    data[i] = (gint16) CLAMP (v, -32768.0, 32767.0);
  }

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Waveform-Viewer");
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (gtk_main_quit),
      NULL);
  gtk_container_set_border_width (GTK_CONTAINER (window), 6);

  wave = bt_waveform_viewer_new ();
  bt_waveform_viewer_set_wave (BT_WAVEFORM_VIEWER (wave), data, 1, DATA_SIZE);
  g_object_set (wave, "loop-start", G_GINT64_CONSTANT (0), "loop-end",
      (gint64) DATA_SIZE, NULL);
  gtk_container_add (GTK_CONTAINER (window), wave);

  gtk_widget_show_all (window);
  g_timeout_add_seconds (1, move_playback_cursor, wave);
  gtk_main ();

  return 0;
}
