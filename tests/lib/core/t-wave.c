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
  ck_g_object_final_unref (song);
  ck_g_object_final_unref (app);
}

static void
case_teardown (void)
{
}


//-- tests

START_TEST (test_bt_wave_properties)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GObject *wave = (GObject *) bt_wave_new (song, "sample1", ext_data_uri, 1,
      1.0, BT_WAVE_LOOP_MODE_OFF, 0);

  /* act & assert */
  ck_assert (check_gobject_properties (wave));

  GST_INFO ("-- cleanup --");
  g_object_unref (wave);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_wave_get_beyond_size)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtWave *wave = bt_wave_new (song, "sample1", ext_data_uri, 1, 1.0,
      BT_WAVE_LOOP_MODE_OFF, 0);

  /* act & assert */
  ck_assert (bt_wave_get_level_by_index (wave, 10) == NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (wave);
  BT_TEST_END;
}
END_TEST

TCase *
bt_wave_test_case (void)
{
  TCase *tc = tcase_create ("BtWaveTests");

  tcase_add_test (tc, test_bt_wave_properties);
  tcase_add_test (tc, test_bt_wave_get_beyond_size);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
