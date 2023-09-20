/* Buzztrax
 * Copyright (C) 2012 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "m-bt-edit.h"

//-- globals

static BtEditApplication *app;
static BtSong *song;
static BtSetup *setup;
static BtMainWindow *main_window;
static BtMainPages *pages;

//-- fixtures

static void
case_setup (void)
{
  BT_CASE_START;
}

static void
test_setup (void)
{
  BtSettings *settings;
  BtWave *wave;

  bt_edit_setup ();
  app = bt_edit_application_new ();

  // no beeps please
  settings = bt_settings_make ();
  g_object_set (settings, "audiosink", "fakesink", NULL);
  g_object_unref (settings);

  // FIXME(ensonic): have a test song with that sample
  bt_edit_application_load_song (app, check_get_test_song_path ("melo3.xml"),
      NULL);
  g_object_get (app, "song", &song, "main-window", &main_window, NULL);
  g_object_get (main_window, "pages", &pages, NULL);
  g_object_get (song, "setup", &setup, NULL);
  wave =
      bt_wave_new (song, "test", "file:///tmp/test.wav", 1, 1.0,
      BT_WAVE_LOOP_MODE_OFF, 0);
  // sample loading is async
  flush_main_loop ();
  // stimulate ui update
  g_object_notify (G_OBJECT (app), "song");
  flush_main_loop ();

  // free resources
  g_object_unref (wave);

}

static void
test_teardown (void)
{
  g_object_unref (setup);
  g_object_unref (song);
  g_object_unref (pages);

  gtk_widget_destroy (GTK_WIDGET (main_window));
  flush_main_loop ();

  ck_g_object_final_unref (app);
  bt_edit_teardown ();
}

static void
case_teardown (void)
{
}

//-- helper

//-- tests

// view all tabs
START_TEST (test_bt_main_pages_view_all_tabs)
{
  BT_TEST_START;
  BtMainPagePatterns *pattern_page;
  GList *children;
  guint i;

  // make sure the pattern view shows something
  BtMachine *machine = bt_setup_get_machine_by_id (setup, "beep1");
  g_object_get (pages, "patterns-page", &pattern_page, NULL);
  bt_main_page_patterns_show_machine (pattern_page, machine);
  g_object_unref (pattern_page);
  gst_object_unref (machine);

  children = gtk_container_get_children (GTK_CONTAINER (pages));
  for (i = 0; i < BT_MAIN_PAGES_COUNT; i++) {
    GtkWidget *child = GTK_WIDGET (g_list_nth_data (children, i));
    gtk_notebook_set_current_page (GTK_NOTEBOOK (pages), i);

    // make screenshot
    check_make_widget_screenshot (GTK_WIDGET (pages),
        gtk_widget_get_name (child));
    flush_main_loop ();
  }
  g_list_free (children);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

// view all tabs
START_TEST (test_bt_main_pages_view_all_tabs_playing)
{
  BT_TEST_START;
  BtMainPagePatterns *pattern_page;
  guint i;

  // make sure the pattern view shows something
  BtMachine *machine = bt_setup_get_machine_by_id (setup, "beep1");
  g_object_get (pages, "patterns-page", &pattern_page, NULL);
  bt_main_page_patterns_show_machine (pattern_page, machine);
  g_object_unref (pattern_page);
  gst_object_unref (machine);

  // play for a while to trigger screen updates
  bt_song_play (song);
  for (i = 0; i < BT_MAIN_PAGES_COUNT; i++) {
    bt_song_update_playback_position (song);

    gtk_notebook_set_current_page (GTK_NOTEBOOK (pages), i);
  }
  bt_song_stop (song);

  GST_INFO ("-- assert --");
  mark_point ();

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

TCase *
bt_main_pages_example_case (void)
{
  TCase *tc = tcase_create ("BtMainPagesExamples");

  tcase_add_test (tc, test_bt_main_pages_view_all_tabs);
  tcase_add_test (tc, test_bt_main_pages_view_all_tabs_playing);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
