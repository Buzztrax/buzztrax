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

#include "m-bt-core.h"

//-- globals

static BtApplication *app;
#if 0
static gboolean play_signal_invoked = FALSE;
#endif

//-- fixtures

static void
case_setup (void)
{
  BT_CASE_START;
}

static void
test_setup (void)
{
  app = bt_test_application_new ();
}

static void
test_teardown (void)
{
  g_object_checked_unref (app);
}

static void
case_teardown (void)
{
}


//-- tests

static void
test_bt_song_properties (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  GObject *song = (GObject *) bt_song_new (app);

  /* act & assert */
  fail_unless (check_gobject_properties (song));

  /* cleanup */
  g_object_checked_unref (song);
  BT_TEST_END;
}

// test if the default constructor handles NULL
static void
test_bt_song_new_null_app (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  check_init_error_trapp ("bt_song_", "BT_IS_APPLICATION (self->priv->app)");

  /* act */
  BtSong *song = bt_song_new (NULL);

  /* assert */
  fail_unless (check_has_error_trapped (), NULL);
  fail_unless (song != NULL, NULL);

  /* cleanup */
  g_object_unref (song);
  BT_TEST_END;
}

// play without loading a song (means don't play anything audible)
static void
test_bt_song_play_empty (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSong *song = bt_song_new (app);

  /* act & assert */
  fail_unless (bt_song_play (song), NULL);

  /* cleanup */
  bt_song_stop (song);
  g_object_checked_unref (song);
  BT_TEST_END;
}

// song is null
static void
test_bt_song_play_null (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSong *song = bt_song_new (app);
  check_init_error_trapp ("bt_song_play", "BT_IS_SONG (self)");

  /* act */
  bt_song_play (NULL);

  /* assert */
  fail_unless (check_has_error_trapped (), NULL);

  /* cleanup */
  bt_song_stop (song);
  g_object_checked_unref (song);
  BT_TEST_END;
}

// load a new song while the first plays
static void
test_bt_song_play_and_load_new (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSong *song = bt_song_new (app);
  BtSongIO *loader =
      bt_song_io_from_file (check_get_test_song_path ("test-simple1.xml"));
  bt_song_io_load (loader, song);
  g_object_checked_unref (loader);
  bt_song_play (song);
  check_run_main_loop_for_usec (G_USEC_PER_SEC / 5);

  /* act */
  loader = bt_song_io_from_file (check_get_test_song_path ("test-simple2.xml"));

  /* assert */
  fail_unless (bt_song_io_load (loader, song), NULL);

  /* cleanup */
  bt_song_stop (song);
  g_object_checked_unref (loader);
  g_object_checked_unref (song);
  BT_TEST_END;
}

TCase *
bt_song_test_case (void)
{
  TCase *tc = tcase_create ("BtSongTests");

  tcase_add_test (tc, test_bt_song_properties);
  tcase_add_test (tc, test_bt_song_new_null_app);
  tcase_add_test (tc, test_bt_song_play_empty);
  tcase_add_test (tc, test_bt_song_play_null);
  tcase_add_test (tc, test_bt_song_play_and_load_new);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
