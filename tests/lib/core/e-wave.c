/* Buzztard
 * Copyright (C) 2012 Buzztard team <buzztard-devel@lists.sf.net>
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

#include "m-bt-core.h"

//-- globals

static BtApplication *app;
static BtSong *song;
static gchar *ext_data_path, *ext_data_uri;

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
  song = bt_song_new (app);
  ext_data_path = g_build_filename (g_get_tmp_dir (), "test.wav", NULL);
  ext_data_uri = g_strconcat ("file://", ext_data_path, NULL);
}

static void
test_teardown (void)
{
  g_free (ext_data_uri);
  g_free (ext_data_path);
  g_object_checked_unref (song);
  g_object_checked_unref (app);
}

static void
case_teardown (void)
{
}


//-- tests

static void
test_bt_wave_default_levels (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtWave *wave = bt_wave_new (song, "sample1", ext_data_uri, 1, 1.0,
      BT_WAVE_LOOP_MODE_OFF, 0);

  /* act */
  GList *list = (GList *) check_gobject_get_ptr_property (wave, "wavelevels");

  /* assert */
  fail_unless (list != NULL, NULL);
  ck_assert_int_eq (g_list_length (list), 1);

  /* cleanup */
  g_object_unref (wave);
  BT_TEST_END;
}

TCase *
bt_wave_example_case (void)
{
  TCase *tc = tcase_create ("BtWaveExamples");

  tcase_add_test (tc, test_bt_wave_default_levels);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
