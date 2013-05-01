/* Buzztrax
 * Copyright (C) 2012 Buzztrax team <buzztrax-devel@lists.sf.net>
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
  bt_edit_setup ();
  app = bt_edit_application_new ();
  g_object_get (app, "main-window", &main_window, NULL);
  ext_data_path = g_build_filename (g_get_tmp_dir (), "test.wav", NULL);
  ext_data_uri = g_strconcat ("file://", ext_data_path, NULL);

  flush_main_loop ();
}

static void
test_teardown (void)
{
  gtk_widget_destroy (GTK_WIDGET (main_window));
  flush_main_loop ();

  g_free (ext_data_uri);
  g_free (ext_data_path);

  g_object_checked_unref (app);
  bt_edit_teardown ();
}

static void
case_teardown (void)
{
}

//-- tests

static void
test_bt_wavelevel_list_model_create_null (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */

  bt_edit_application_new_song (app);

  /* act */
  BtWavelevelListModel *model = bt_wavelevel_list_model_new (NULL);

  /* assert */
  fail_unless (model != NULL, NULL);

  /* cleanup */
  g_object_unref (model);
  BT_TEST_END;
}

static void
test_bt_wavelevel_list_model_create (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSong *song;

  bt_edit_application_new_song (app);
  g_object_get (app, "song", &song, NULL);
  BtWave *wave = bt_wave_new (song, "sample1", ext_data_uri, 1, 1.0,
      BT_WAVE_LOOP_MODE_OFF, 0);

  /* act */
  BtWavelevelListModel *model = bt_wavelevel_list_model_new (wave);

  /* assert */
  fail_unless (model != NULL, NULL);

  /* cleanup */
  g_object_unref (model);
  g_object_unref (wave);
  g_object_unref (song);
  BT_TEST_END;
}

// load a wave and check the wavelevel
static void
test_bt_wavelevel_list_model_get_wavelevel (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSong *song;
  GtkTreeIter iter;

  bt_edit_application_new_song (app);
  g_object_get (app, "song", &song, NULL);
  BtWave *wave = bt_wave_new (song, "sample1", ext_data_uri, 1, 1.0,
      BT_WAVE_LOOP_MODE_OFF, 0);
  BtWavelevelListModel *model = bt_wavelevel_list_model_new (wave);
  gtk_tree_model_get_iter_first ((GtkTreeModel *) model, &iter);

  /* act */
  BtWavelevel *wavelevel = bt_wavelevel_list_model_get_object (model, &iter);

  /* assert */
  fail_unless (wavelevel != NULL, NULL);

  /* cleanup */
  g_object_unref (wavelevel);
  g_object_unref (model);
  g_object_unref (song);
  BT_TEST_END;
}

TCase *
bt_wavelevel_list_model_example_case (void)
{
  TCase *tc = tcase_create ("BtWavelevelListModelExamples");

  tcase_add_test (tc, test_bt_wavelevel_list_model_create_null);
  tcase_add_test (tc, test_bt_wavelevel_list_model_create);
  tcase_add_test (tc, test_bt_wavelevel_list_model_get_wavelevel);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
