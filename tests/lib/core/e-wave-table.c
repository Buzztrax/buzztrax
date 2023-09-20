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

START_TEST (test_bt_wave_table_default_empty)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtWavetable *wave_table =
      (BtWavetable *) check_gobject_get_object_property (song, "wavetable");

  GST_INFO ("-- act --");
  GList *list = (GList *) check_gobject_get_ptr_property (wave_table, "waves");

  GST_INFO ("-- assert --");
  ck_assert (list == NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (wave_table);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_wave_table_add_wave)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtWavetable *wave_table =
      (BtWavetable *) check_gobject_get_object_property (song, "wavetable");

  GST_INFO ("-- act --");
  BtWave *wave = bt_wave_new (song, "sample1", ext_data_uri, 1, 1.0,
      BT_WAVE_LOOP_MODE_OFF, 0);

  GST_INFO ("-- assert --");
  GList *list = (GList *) check_gobject_get_ptr_property (wave_table, "waves");
  ck_assert (list != NULL);
  ck_assert_int_eq (g_list_length (list), 1);

  GST_INFO ("-- cleanup --");
  g_list_free (list);
  g_object_unref (wave);
  g_object_unref (wave_table);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_wave_table_rem_wave)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtWavetable *wave_table =
      (BtWavetable *) check_gobject_get_object_property (song, "wavetable");
  BtWave *wave = bt_wave_new (song, "sample1", ext_data_uri, 1, 1.0,
      BT_WAVE_LOOP_MODE_OFF, 0);

  GST_INFO ("-- act --");
  bt_wavetable_remove_wave (wave_table, wave);

  GST_INFO ("-- assert --");
  GList *list = (GList *) check_gobject_get_ptr_property (wave_table, "waves");
  ck_assert (list == NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (wave);
  g_object_unref (wave_table);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_wave_table_get_wave_by_ix)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtWavetable *wave_table =
      (BtWavetable *) check_gobject_get_object_property (song, "wavetable");
  BtWave *wave = bt_wave_new (song, "sample1", ext_data_uri, 1, 1.0,
      BT_WAVE_LOOP_MODE_OFF, 0);

  GST_INFO ("-- act --");
  BtWave *wave_by_ix = bt_wavetable_get_wave_by_index (wave_table, 1);

  GST_INFO ("-- assert --");
  ck_assert (wave == wave_by_ix);

  GST_INFO ("-- cleanup --");
  g_object_unref (wave);
  g_object_unref (wave_by_ix);
  g_object_unref (wave_table);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_wave_table_replace_wave)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtWavetable *wave_table =
      (BtWavetable *) check_gobject_get_object_property (song, "wavetable");
  BtWave *wave1 = bt_wave_new (song, "sample1", ext_data_uri, 1, 1.0,
      BT_WAVE_LOOP_MODE_OFF, 0);

  GST_INFO ("-- act --");
  BtWave *wave2 = bt_wave_new (song, "sample2", ext_data_uri, 1, 1.0,
      BT_WAVE_LOOP_MODE_FORWARD, 0);
  BtWave *wave_by_ix = bt_wavetable_get_wave_by_index (wave_table, 1);

  GST_INFO ("-- assert --");
  ck_assert (wave2 == wave_by_ix);

  GST_INFO ("-- cleanup --");
  g_object_unref (wave1);
  g_object_unref (wave2);
  g_object_unref (wave_by_ix);
  g_object_unref (wave_table);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_wave_table_get_callbacks)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtWavetable *wave_table =
      (BtWavetable *) check_gobject_get_object_property (song, "wavetable");
  BtWave *wave = bt_wave_new (song, "sample1", ext_data_uri, 1, 1.0,
      BT_WAVE_LOOP_MODE_OFF, 0);

  GST_INFO ("-- act --");
  gpointer *cb = (gpointer *) bt_wavetable_get_callbacks (wave_table);

  GST_INFO ("-- assert --");
  ck_assert (cb != NULL);
  ck_assert (cb[0] != NULL);
  ck_assert (cb[1] != NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (wave);
  g_object_unref (wave_table);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_wave_table_callbacks_get_wave)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtWavetable *wave_table =
      (BtWavetable *) check_gobject_get_object_property (song, "wavetable");
  BtWave *wave = bt_wave_new (song, "sample1", ext_data_uri, 1, 1.0,
      BT_WAVE_LOOP_MODE_OFF, 0);
  gpointer *cb = (gpointer *) bt_wavetable_get_callbacks (wave_table);

  GST_INFO ("-- act --");
  GstStructure *(*get_wave_buffer) (gpointer, guint, guint);
  get_wave_buffer = cb[1];
  GstStructure *s = get_wave_buffer (cb[0], 1, 0);

  GST_INFO ("-- assert --");
  ck_assert (s != NULL);

  GST_INFO ("-- cleanup --");
  gst_structure_free (s);
  g_object_unref (wave);
  g_object_unref (wave_table);
  BT_TEST_END;
}
END_TEST

TCase *
bt_wave_table_example_case (void)
{
  TCase *tc = tcase_create ("BtWaveTableExamples");

  tcase_add_test (tc, test_bt_wave_table_default_empty);
  tcase_add_test (tc, test_bt_wave_table_add_wave);
  tcase_add_test (tc, test_bt_wave_table_rem_wave);
  tcase_add_test (tc, test_bt_wave_table_get_wave_by_ix);
  tcase_add_test (tc, test_bt_wave_table_replace_wave);
  tcase_add_test (tc, test_bt_wave_table_get_callbacks);
  tcase_add_test (tc, test_bt_wave_table_callbacks_get_wave);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
