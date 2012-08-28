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

static BtEditApplication *app;
static BtMainWindow *main_window;

//-- fixtures

static void
case_setup (void)
{
  GST_INFO
      ("================================================================================");
}

static void
test_setup (void)
{
  bt_edit_setup ();
  app = bt_edit_application_new ();
  g_object_get (app, "main-window", &main_window, NULL);

  flush_main_loop ();
}

static void
test_teardown (void)
{
  gtk_widget_destroy (GTK_WIDGET (main_window));
  flush_main_loop ();

  g_object_checked_unref (app);
  bt_edit_teardown ();
}

static void
case_teardown (void)
{
}

//-- helper

//-- tests

// create app and then unconditionally destroy window
static void
test_bt_edit_application_create (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */

  /* act */

  /* assert */
  fail_unless (app != NULL, NULL);
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

  /* cleanup */
  BT_TEST_END;
}

static gboolean
finish_main_loops (gpointer user_data)
{
  // this does not work for dialogs (gtk_dialog_run)!
  gtk_main_quit ();
  return FALSE;
}

// create app and then unconditionally destroy window
static void
test_bt_edit_application_run (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSettings *settings = bt_settings_make ();
  // avoid the about dialog
  g_object_set (settings, "news-seen", PACKAGE_VERSION_NUMBER, NULL);
  g_object_unref (settings);

  /* act */
  g_idle_add (finish_main_loops, NULL);
  bt_edit_application_run (app);

  /* assert */
  mark_point ();

  /* cleanup */
  BT_TEST_END;
}

// create a new song
static void
test_bt_edit_application_new_song (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */

  /* act */
  bt_edit_application_new_song (app);

  /* assert */
  ck_assert_gobject_object_ne (app, "song", NULL);

  /* cleanup */
  BT_TEST_END;
}

static void
test_bt_edit_application_new_song_is_saved (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */

  /* act */
  bt_edit_application_new_song (app);

  /* assert */
  ck_assert_gobject_boolean_eq (app, "unsaved", FALSE);
  check_make_widget_screenshot (GTK_WIDGET (main_window), "song");

  /* cleanup */
  BT_TEST_END;
}

// load a song
static void
test_bt_edit_application_load_song (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */

  /* act */
  bt_edit_application_load_song (app, check_get_test_song_path ("melo3.xml"));

  /* assert */
  ck_assert_gobject_object_ne (app, "song", NULL);

  /* cleanup */
  BT_TEST_END;
}

static void
test_bt_edit_application_load_song_is_saved (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */

  /* act */
  bt_edit_application_load_song (app, check_get_test_song_path ("melo3.xml"));

  /* assert */
  ck_assert_gobject_boolean_eq (app, "unsaved", FALSE);

  /* cleanup */
  BT_TEST_END;
}

// load a song, free it, load another
static void
test_bt_edit_application_load_songs (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSong *song1, *song2;
  bt_edit_application_load_song (app, check_get_test_song_path ("melo1.xml"));
  g_object_get (app, "song", &song1, NULL);
  g_object_unref (song1);

  /* act */
  bt_edit_application_load_song (app, check_get_test_song_path ("melo3.xml"));

  /* assert */
  g_object_get (app, "song", &song2, NULL);
  fail_unless (song2 != NULL, NULL);
  fail_unless (song2 != song1, NULL);

  /* cleanup */
  g_object_unref (song2);
  BT_TEST_END;
}

// load a song, free it, load again
static void
test_bt_edit_application_num_children (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  guint num;
  GstElement *bin;
  g_object_get (app, "bin", &bin, NULL);
  GST_INFO ("song.elements=%d", GST_BIN_NUMCHILDREN (bin));

  /* act */
  // load for first time
  bt_edit_application_load_song (app, check_get_test_song_path ("melo3.xml"));
  num = GST_BIN_NUMCHILDREN (bin);
  GST_INFO ("song.elements=%d", num);
  // load song for second time
  bt_edit_application_load_song (app, check_get_test_song_path ("melo3.xml"));
  GST_INFO ("song.elements=%d", GST_BIN_NUMCHILDREN (bin));

  /* assert */
  fail_unless (num == GST_BIN_NUMCHILDREN (bin), NULL);

  /* cleanup */
  gst_object_unref (bin);
  BT_TEST_END;
}

// load a song with a view disabled
static void
test_bt_edit_application_load_song_with_view_disabled (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtMainPages *pages;
  g_object_get (G_OBJECT (main_window), "pages", &pages, NULL);
  gtk_notebook_remove_page (GTK_NOTEBOOK (pages), _i);
  g_object_unref (pages);
  GST_INFO ("removed page number %d", _i);

  /* act */
  bt_edit_application_load_song (app, check_get_test_song_path ("melo3.xml"));

  /* assert */
  mark_point ();

  /* cleanup */
  BT_TEST_END;
}

// load a song and play it
static void
test_bt_edit_application_load_and_play (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSong *song;

  bt_edit_application_load_song (app, check_get_test_song_path ("melo1.xml"));
  g_object_get (app, "song", &song, NULL);

  /* act */
  gboolean playing = bt_song_play (song);

  /* assert */
  fail_unless (playing == TRUE, NULL);

  /* cleanup */
  flush_main_loop ();
  bt_song_stop (song);
  g_object_unref (song);
  BT_TEST_END;
}

// load a song, play it and load another song while the former is playing
static void
test_bt_edit_application_load_while_playing (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSong *song;

  bt_edit_application_load_song (app, check_get_test_song_path ("melo1.xml"));
  g_object_get (app, "song", &song, NULL);
  bt_song_play (song);
  g_object_unref (song);

  /* act */
  bt_edit_application_load_song (app, check_get_test_song_path ("melo2.xml"));

  /* assert */
  mark_point ();

  /* cleanup */
  BT_TEST_END;
}

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
  return (tc);
}
