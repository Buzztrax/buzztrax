/* Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "m-bt-edit.h"

//-- globals

//-- fixtures

static void
test_setup (void)
{
  bt_edit_setup ();
}

static void
test_teardown (void)
{
  bt_edit_teardown ();
}

//-- helper

//-- tests

// test some key presses
static void test_editing (BT_TEST_ARGS)
{
  BT_TEST_START;
  BtEditApplication *app;
  BtMainWindow *main_window;
  BtMainPages *pages;
  BtMainPagePatterns *pattern_page;
  BtSong *song;
  BtSetup *setup;
  BtMachine *machine;
  BtCmdPattern *cmd_pattern;
  BtPattern *pattern;
  GtkWidget *widget;
  gchar *str;

  app = bt_edit_application_new ();
  GST_INFO ("back in test app=%p, app->ref_ct=%d", app,
      G_OBJECT_REF_COUNT (app));
  fail_unless (app != NULL, NULL);

  // load a song
  bt_edit_application_load_song (app, check_get_test_song_path ("melo3.xml"));
  g_object_get (app, "song", &song, NULL);
  fail_unless (song != NULL, NULL);
  g_object_get (song, "setup", &setup, NULL);
  g_object_unref (song);
  GST_INFO ("song loaded");

  // get window
  g_object_get (app, "main-window", &main_window, NULL);
  fail_unless (main_window != NULL, NULL);

  // make sure the pattern view shows something
  machine = bt_setup_get_machine_by_id (setup, "beep1");
  cmd_pattern = bt_machine_get_pattern_by_id (machine, "beep1::C-3_E-3_G-3");
  fail_unless (BT_IS_PATTERN (cmd_pattern));
  pattern = (BtPattern *) cmd_pattern;
  g_object_get (G_OBJECT (main_window), "pages", &pages, NULL);
  g_object_get (G_OBJECT (pages), "patterns-page", &pattern_page, NULL);

  // show page
  gtk_notebook_set_current_page (GTK_NOTEBOOK (pages),
      BT_MAIN_PAGES_PATTERNS_PAGE);
  while (gtk_events_pending ())
    gtk_main_iteration ();
  bt_main_page_patterns_show_pattern (pattern_page, pattern);

  widget = gtk_window_get_focus ((GtkWindow *) main_window);
  GST_INFO ("testing key-press handling on: '%s'",
      gtk_widget_get_name (widget));
  fail_unless (!strcmp ("pattern editor", gtk_widget_get_name (widget)), NULL);

  str = bt_pattern_get_global_event (pattern, 0, 0);
  fail_unless (str != NULL, NULL);
  fail_unless (!strcmp (str, "c-3"), NULL);
  g_free (str);

  // make sure we're at top left
  check_send_key ((GtkWidget *) pattern_page, GDK_Page_Up, 0);
  check_send_key ((GtkWidget *) pattern_page, GDK_Home, 0);

  // send a '.' key-press
  check_send_key ((GtkWidget *) pattern_page, '.', 0x3c);
  str = bt_pattern_get_global_event (pattern, 0, 0);
  GST_INFO ("data at 0,0: '%s'", str);
  fail_unless (str == NULL, NULL);

  // make sure we're at top left
  check_send_key ((GtkWidget *) pattern_page, GDK_Page_Up, 0);
  check_send_key ((GtkWidget *) pattern_page, GDK_Home, 0);

  // send a 'q' key-press and check that it results in a 'c-' of any octave
  check_send_key ((GtkWidget *) pattern_page, 'q', 0x18);
  str = bt_pattern_get_global_event (pattern, 0, 0);
  GST_INFO ("data at 0,0: '%s'", str);
  fail_unless (str != NULL, NULL);
  fail_unless (!strncmp (str, "c-", 2), NULL);
  g_free (str);

  g_object_unref (pattern_page);
  g_object_unref (pattern);
  g_object_unref (machine);
  g_object_unref (setup);
  g_object_unref (pages);

  // close window
  gtk_widget_destroy (GTK_WIDGET (main_window));
  while (gtk_events_pending ())
    gtk_main_iteration ();
  //GST_INFO("mainlevel is %d",gtk_main_level());
  //while(g_main_context_pending(NULL)) g_main_context_iteration(/*context=*/NULL,/*may_block=*/FALSE);

  // free application
  GST_INFO ("app->ref_ct=%d", G_OBJECT_REF_COUNT (app));
  g_object_checked_unref (app);

  BT_TEST_END;
}

static void test_pattern_voices (BT_TEST_ARGS)
{
  BT_TEST_START;
  BtEditApplication *app;
  GError *err = NULL;
  BtMainWindow *main_window;
  BtMainPages *pages;
  BtMainPagePatterns *pattern_page;
  BtSong *song;
  BtSetup *setup;
  BtMachine *machine;
  BtPattern *pattern;
  GtkWidget *widget;
  gulong voices;

  // prepare
  app = bt_edit_application_new ();
  GST_INFO ("back in test app=%p, app->ref_ct=%d", app,
      G_OBJECT_REF_COUNT (app));
  bt_edit_application_new_song (app);

  g_object_get (app, "song", &song, "main-window", &main_window, NULL);
  g_object_get (song, "setup", &setup, NULL);

  // make sure the pattern view shows something
  machine =
      BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztard-test-poly-source", 1L, &err));
  fail_unless (machine != NULL, NULL);
  fail_unless (err == NULL, NULL);
  pattern = bt_pattern_new (song, "pattern-id", "pattern-name", 8L, machine);
  fail_unless (pattern != NULL, NULL);
  g_object_get (G_OBJECT (main_window), "pages", &pages, NULL);
  g_object_get (G_OBJECT (pages), "patterns-page", &pattern_page, NULL);

  // show page
  gtk_notebook_set_current_page (GTK_NOTEBOOK (pages),
      BT_MAIN_PAGES_PATTERNS_PAGE);
  while (gtk_events_pending ())
    gtk_main_iteration ();
  bt_main_page_patterns_show_pattern (pattern_page, pattern);

  widget = gtk_window_get_focus ((GtkWindow *) main_window);
  GST_INFO ("testing key-press handling on: '%s'",
      gtk_widget_get_name (widget));
  fail_unless (!strcmp ("pattern editor", gtk_widget_get_name (widget)), NULL);

  // change voices
  g_object_set (machine, "voices", 2L, NULL);
  g_object_get (pattern, "voices", &voices, NULL);
  fail_unless (voices == 2, NULL);

  // send two tab keys to ensure the new voice is visible
  check_send_key ((GtkWidget *) pattern_page, GDK_Tab, 0);
  check_send_key ((GtkWidget *) pattern_page, GDK_Tab, 0);

  // cleanup
  g_object_unref (pattern_page);
  g_object_unref (pattern);
  g_object_unref (machine);
  g_object_unref (setup);
  g_object_unref (song);
  g_object_unref (pages);

  // close window
  gtk_widget_destroy (GTK_WIDGET (main_window));
  while (gtk_events_pending ())
    gtk_main_iteration ();
  //GST_INFO("mainlevel is %d",gtk_main_level());
  //while(g_main_context_pending(NULL)) g_main_context_iteration(/*context=*/NULL,/*may_block=*/FALSE);

  // free application
  GST_INFO ("app->ref_ct=%d", G_OBJECT_REF_COUNT (app));
  g_object_checked_unref (app);

  BT_TEST_END;
}
 TCase * bt_pattern_page_example_case (void)
{
  TCase *tc = tcase_create ("BtPatternPageExamples");

  tcase_add_test (tc, test_editing);
  tcase_add_test (tc, test_pattern_voices);
  // we *must* use a checked fixture, as only this runs in the same context
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  return (tc);
}
