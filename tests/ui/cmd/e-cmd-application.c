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

#include <glib/gstdio.h>

#include "m-bt-cmd.h"

//-- globals

//-- fixtures

static void
case_setup (void)
{
  BT_CASE_START;
}

static void
test_setup (void)
{
}

static void
test_teardown (void)
{
}

static void
case_teardown (void)
{
}

//-- tests

START_TEST (test_bt_cmd_application_create)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  BtCmdApplication *app = bt_cmd_application_new (TRUE);

  GST_INFO ("-- assert --");
  ck_assert (app != NULL);
  ck_assert_int_eq (G_OBJECT_REF_COUNT (app), 1);

  GST_INFO ("-- cleanup --");
  ck_g_object_final_unref (app);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_cmd_application_play)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtCmdApplication *app = bt_cmd_application_new (TRUE);

  GST_INFO ("-- act --");
  gboolean ret = bt_cmd_application_play (app,
      check_get_test_song_path ("test-simple1.xml"));

  GST_INFO ("-- assert --");
  ck_assert (ret == TRUE);

  GST_INFO ("-- cleanup --");
  ck_g_object_final_unref (app);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_cmd_application_play_two_files)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtCmdApplication *app = bt_cmd_application_new (TRUE);

  GST_INFO ("-- act --");
  bt_cmd_application_play (app, check_get_test_song_path ("test-simple1.xml"));
  gboolean ret = bt_cmd_application_play (app,
      check_get_test_song_path ("test-simple2.xml"));

  GST_INFO ("-- assert --");
  ck_assert (ret == TRUE);

  GST_INFO ("-- cleanup --");
  ck_g_object_final_unref (app);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_cmd_application_play_incomplete_file)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtCmdApplication *app = bt_cmd_application_new (TRUE);

  GST_INFO ("-- act --");
  gboolean ret = bt_cmd_application_play (app,
      check_get_test_song_path ("broken2.xml"));

  GST_INFO ("-- assert --");
  // we should still attempt to play it
  ck_assert (ret == TRUE);

  GST_INFO ("-- cleanup --");
  ck_g_object_final_unref (app);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_cmd_application_info)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtCmdApplication *app = bt_cmd_application_new (TRUE);
  gchar *tmp_file_name =
      g_build_filename (g_get_tmp_dir (), "test-simple1.xml.txt", NULL);

  GST_INFO ("-- act --");
  gboolean ret = bt_cmd_application_info (app,
      check_get_test_song_path ("test-simple1.xml"), tmp_file_name);

  GST_INFO ("-- assert --");
  ck_assert (ret == TRUE);
  ck_assert (check_file_contains_str (NULL, tmp_file_name,
          "song.song_info.name: \"test simple 1\""));

  GST_INFO ("-- cleanup --");
  g_unlink (tmp_file_name);
  g_free (tmp_file_name);
  ck_g_object_final_unref (app);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_cmd_application_info_for_incomplete_file)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtCmdApplication *app = bt_cmd_application_new (TRUE);
  gchar *tmp_file_name =
      g_build_filename (g_get_tmp_dir (), "broken2.xml.txt", NULL);

  GST_INFO ("-- act --");
  gboolean ret = bt_cmd_application_info (app,
      check_get_test_song_path ("broken2.xml"), tmp_file_name);

  GST_INFO ("-- assert --");
  ck_assert (ret == TRUE);
  ck_assert (check_file_contains_str (NULL, tmp_file_name,
          "song.song_info.name: \"broken 2\""));
  ck_assert (check_file_contains_str (NULL, tmp_file_name,
          "song.setup.number_of_missing_machines: 1"));
  ck_assert (check_file_contains_str (NULL, tmp_file_name,
          "song.wavetable.number_of_missing_waves: 2"));

  GST_INFO ("-- cleanup --");
  g_unlink (tmp_file_name);
  g_free (tmp_file_name);
  ck_g_object_final_unref (app);
  BT_TEST_END;
}
END_TEST

TCase *
bt_cmd_application_example_case (void)
{
  TCase *tc = tcase_create ("BtCmdApplicationExamples");

  tcase_add_test (tc, test_bt_cmd_application_create);
  tcase_add_test (tc, test_bt_cmd_application_play);
  tcase_add_test (tc, test_bt_cmd_application_play_two_files);
  tcase_add_test (tc, test_bt_cmd_application_play_incomplete_file);
  tcase_add_test (tc, test_bt_cmd_application_info);
  tcase_add_test (tc, test_bt_cmd_application_info_for_incomplete_file);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
