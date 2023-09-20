/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
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
static BtMainWindow *main_window;

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

  bt_edit_setup ();
  app = bt_edit_application_new ();
  g_object_get (app, "main-window", &main_window, NULL);
  // no beeps please
  settings = bt_settings_make ();
  g_object_set (settings, "audiosink", "fakesink", NULL);
  g_object_unref (settings);

  flush_main_loop ();
}

static void
test_teardown (void)
{
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

// create app and then unconditionally destroy window
START_TEST (test_bt_edit_application_create)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");

  GST_INFO ("-- assert --");
  ck_assert (app != NULL);
  check_make_widget_screenshot (GTK_WIDGET (main_window), NULL);
  {
    BtCheckWidgetScreenshotRegions regions[] = {
      {BT_CHECK_WIDGET_SCREENSHOT_REGION_MATCH_TYPE, NULL, NULL,
          BT_TYPE_MAIN_MENU, GTK_POS_LEFT},
      {BT_CHECK_WIDGET_SCREENSHOT_REGION_MATCH_TYPE, NULL, NULL,
          BT_TYPE_MAIN_TOOLBAR, GTK_POS_LEFT},
      {BT_CHECK_WIDGET_SCREENSHOT_REGION_MATCH_TYPE, NULL, NULL,
          BT_TYPE_MAIN_STATUSBAR, GTK_POS_LEFT},
      {BT_CHECK_WIDGET_SCREENSHOT_REGION_MATCH_LABEL, NULL, "Grid",
          G_TYPE_INVALID, GTK_POS_RIGHT},
      {BT_CHECK_WIDGET_SCREENSHOT_REGION_MATCH_NONE, NULL, NULL, G_TYPE_INVALID,
          0}
    };
    check_make_widget_screenshot_with_highlight (GTK_WIDGET (main_window),
        "highlight", regions);
  }

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

static gboolean
finish_main_loops (gpointer user_data)
{
  // this does not work for dialogs (gtk_dialog_run)!
  gtk_main_quit ();
  return FALSE;
}

// create app and then unconditionally destroy window
START_TEST (test_bt_edit_application_run)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSettings *settings = bt_settings_make ();
  // avoid the about dialog
  g_object_set (settings, "news-seen", PACKAGE_VERSION_NUMBER, NULL);
  g_object_unref (settings);

  GST_INFO ("-- act --");
  g_idle_add (finish_main_loops, NULL);
  bt_edit_application_run (app);

  GST_INFO ("-- assert --");
  mark_point ();

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

// create a new song
START_TEST (test_bt_edit_application_new_song)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  bt_edit_application_new_song (app);

  GST_INFO ("-- assert --");
  ck_assert_gobject_object_ne (app, "song", NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_edit_application_new_song_is_saved)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  bt_edit_application_new_song (app);

  GST_INFO ("-- assert --");
  ck_assert_gobject_gboolean_eq (app, "unsaved", FALSE);
  check_make_widget_screenshot (GTK_WIDGET (main_window), "song");

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

// load a song
START_TEST (test_bt_edit_application_load_song)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  bt_edit_application_load_song (app, check_get_test_song_path ("melo3.xml"),
      NULL);

  GST_INFO ("-- assert --");
  ck_assert_gobject_object_ne (app, "song", NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_edit_application_load_song_is_saved)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  bt_edit_application_load_song (app, check_get_test_song_path ("melo3.xml"),
      NULL);

  GST_INFO ("-- assert --");
  ck_assert_gobject_gboolean_eq (app, "unsaved", FALSE);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

// load a song, free it, load another
START_TEST (test_bt_edit_application_load_songs)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSong *song1, *song2;
  bt_edit_application_load_song (app, check_get_test_song_path ("melo1.xml"),
      NULL);
  g_object_get (app, "song", &song1, NULL);
  g_object_unref (song1);

  GST_INFO ("-- act --");
  bt_edit_application_load_song (app, check_get_test_song_path ("melo3.xml"),
      NULL);

  GST_INFO ("-- assert --");
  g_object_get (app, "song", &song2, NULL);
  ck_assert (song2 != NULL);
  ck_assert (song2 != song1);

  GST_INFO ("-- cleanup --");
  g_object_unref (song2);
  BT_TEST_END;
}
END_TEST

// load a song, free it, load again
START_TEST (test_bt_edit_application_num_children)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  guint num;
  GstElement *bin;
  g_object_get (app, "bin", &bin, NULL);
  GST_INFO ("song.elements=%d", GST_BIN_NUMCHILDREN (bin));

  GST_INFO ("-- act --");
  // load for first time
  bt_edit_application_load_song (app, check_get_test_song_path ("melo3.xml"),
      NULL);
  num = GST_BIN_NUMCHILDREN (bin);
  GST_INFO ("song.elements=%d", num);
  // load song for second time
  bt_edit_application_load_song (app, check_get_test_song_path ("melo3.xml"),
      NULL);
  GST_INFO ("song.elements=%d", GST_BIN_NUMCHILDREN (bin));

  GST_INFO ("-- assert --");
  ck_assert (num == GST_BIN_NUMCHILDREN (bin));

  GST_INFO ("-- cleanup --");
  gst_object_unref (bin);
  BT_TEST_END;
}
END_TEST

// load a song with a view disabled
START_TEST (test_bt_edit_application_load_song_with_view_disabled)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMainPages *pages;
  g_object_get (G_OBJECT (main_window), "pages", &pages, NULL);
  gtk_notebook_remove_page (GTK_NOTEBOOK (pages), _i);
  g_object_unref (pages);
  GST_INFO ("removed page number %d", _i);

  GST_INFO ("-- act --");
  bt_edit_application_load_song (app, check_get_test_song_path ("melo3.xml"),
      NULL);

  GST_INFO ("-- assert --");
  mark_point ();

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

// load a song and play it
START_TEST (test_bt_edit_application_load_and_play)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSong *song;

  bt_edit_application_load_song (app, check_get_test_song_path ("melo1.xml"),
      NULL);
  g_object_get (app, "song", &song, NULL);

  GST_INFO ("-- act --");
  gboolean playing = bt_song_play (song);

  GST_INFO ("-- assert --");
  ck_assert (playing == TRUE);

  GST_INFO ("-- cleanup --");
  flush_main_loop ();
  bt_song_stop (song);
  g_object_unref (song);
  BT_TEST_END;
}
END_TEST

// load a song, play it and load another song while the former is playing
START_TEST (test_bt_edit_application_load_while_playing)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSong *song;

  bt_edit_application_load_song (app, check_get_test_song_path ("melo1.xml"),
      NULL);
  g_object_get (app, "song", &song, NULL);
  bt_song_play (song);
  g_object_unref (song);

  GST_INFO ("-- act --");
  bt_edit_application_load_song (app, check_get_test_song_path ("melo2.xml"),
      NULL);

  GST_INFO ("-- assert --");
  mark_point ();

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

TCase *
bt_edit_application_example_case (void)
{
  TCase *tc = tcase_create ("BtEditApplicationExamples");

  tcase_add_test (tc, test_bt_edit_application_create);
  tcase_add_test (tc, test_bt_edit_application_run);
  tcase_add_test (tc, test_bt_edit_application_new_song);
  tcase_add_test (tc, test_bt_edit_application_new_song_is_saved);
  tcase_add_test (tc, test_bt_edit_application_load_song);
  tcase_add_test (tc, test_bt_edit_application_load_song_is_saved);
  tcase_add_test (tc, test_bt_edit_application_load_songs);
  tcase_add_test (tc, test_bt_edit_application_num_children);
  tcase_add_loop_test (tc,
      test_bt_edit_application_load_song_with_view_disabled,
      BT_MAIN_PAGES_MACHINES_PAGE, BT_MAIN_PAGES_COUNT);
  tcase_add_test (tc, test_bt_edit_application_load_and_play);
  tcase_add_test (tc, test_bt_edit_application_load_while_playing);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
