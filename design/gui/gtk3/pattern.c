/* port pattern-editor to gtk3
 *
 * gcc -Wall -g -I. pattern.c pattern-editor.c -o pattern -lm `pkg-config gtk+-3.0 gstreamer-1.0 --cflags --libs`
 */

#include <math.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gst/gst.h>
#include "pattern-editor.h"

static gboolean
move_playback_cursor (gpointer data)
{
  static gdouble pos = 0.0;

  g_object_set (data, "play-position", pos, NULL);
  pos += 0.05;
  if (pos >= 1.0)
    pos = 0.0;
  return TRUE;
}

static gfloat
get_data_at (gpointer pattern_data, gpointer column_data, guint row,
    guint track, guint param)
{
  return 1.0;
}

static void
set_data_at (gpointer pattern_data, gpointer column_data, guint row,
    guint track, guint param, guint digit, gfloat value)
{
}

gint
main (gint argc, gchar ** argv)
{
  GtkWidget *window, *scrolled_window, *pattern;
  const gint num_rows = 16;
  const gint num_groups = 3;
  BtPatternEditorColumnGroup groups[3] = { {0,}, };
  BtPatternEditorColumn columns[4] = { {0,}, };
  static BtPatternEditorCallbacks callbacks = { get_data_at, set_data_at };

  gtk_init (&argc, &argv);
  gst_init (&argc, &argv);

  /* prepare some pattern data */
  columns[0].type = PCT_SWITCH;
  columns[0].min = 0;
  columns[0].max = 1;
  columns[0].def = 0;
  columns[1].type = PCT_BYTE;
  columns[1].min = 0;
  columns[1].max = 0xff;
  columns[1].def = 0;
  columns[2].type = PCT_WORD;
  columns[2].min = 0;
  columns[2].max = 0xffff;
  columns[2].def = 0;
  columns[3].type = PCT_NOTE;
  columns[3].min = 0;
  columns[3].max = 0xff;
  columns[3].def = 0;

  groups[0].name = "Global";
  groups[0].num_columns = 2;
  groups[0].columns = columns;
  groups[1].name = "Voice 1";
  groups[1].num_columns = 4;
  groups[1].columns = columns;
  groups[2].name = "Voice 2";
  groups[2].num_columns = 4;
  groups[2].columns = columns;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Pattern-Editor");
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (gtk_main_quit),
      NULL);
  gtk_container_set_border_width (GTK_CONTAINER (window), 6);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
      GTK_SHADOW_NONE);
  gtk_container_add (GTK_CONTAINER (window), scrolled_window);

  pattern = bt_pattern_editor_new ();
  bt_pattern_editor_set_pattern (BT_PATTERN_EDITOR (pattern), NULL, num_rows,
      num_groups, groups, &callbacks);

  gtk_container_add (GTK_CONTAINER (scrolled_window), pattern);

  gtk_widget_show_all (window);
  g_timeout_add_seconds (1, move_playback_cursor, pattern);
  gtk_main ();

  return 0;
}
