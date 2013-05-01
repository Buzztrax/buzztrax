/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@lists.sf.net>
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

static void
test_bt_cmd_application_create (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */

  /* act */
  BtCmdApplication *app = bt_cmd_application_new (TRUE);

  /* assert */
  fail_unless (app != NULL, NULL);
  ck_assert_int_eq (G_OBJECT_REF_COUNT (app), 1);

  /* cleanup */
  g_object_checked_unref (app);
  BT_TEST_END;
}

static void
test_bt_cmd_application_play (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtCmdApplication *app = bt_cmd_application_new (TRUE);

  /* act */
  gboolean ret = bt_cmd_application_play (app,
      check_get_test_song_path ("test-simple1.xml"));

  /* assert */
  fail_unless (ret == TRUE, NULL);

  /* cleanup */
  g_object_checked_unref (app);
  BT_TEST_END;
}

static void
test_bt_cmd_application_play_two_files (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtCmdApplication *app = bt_cmd_application_new (TRUE);

  /* act */
  bt_cmd_application_play (app, check_get_test_song_path ("test-simple1.xml"));
  gboolean ret = bt_cmd_application_play (app,
      check_get_test_song_path ("test-simple2.xml"));

  /* assert */
  fail_unless (ret == TRUE, NULL);

  /* cleanup */
  g_object_checked_unref (app);
  BT_TEST_END;
}

static void
test_bt_cmd_application_info (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtCmdApplication *app = bt_cmd_application_new (TRUE);
  gchar *tmp_file_name =
      g_build_filename (g_get_tmp_dir (), "test-simple1.xml.txt", NULL);

  /* act */
  gboolean ret = bt_cmd_application_info (app,
      check_get_test_song_path ("test-simple1.xml"), tmp_file_name);

  /* assert */
  fail_unless (ret == TRUE, NULL);
  fail_unless (check_file_contains_str (NULL, tmp_file_name,
          "song.song_info.name: \"test simple 1\""), NULL);

  /* cleanup */
  g_unlink (tmp_file_name);
  g_free (tmp_file_name);
  g_object_checked_unref (app);
  BT_TEST_END;
}

TCase *
bt_cmd_application_example_case (void)
{
  TCase *tc = tcase_create ("BtCmdApplicationExamples");

  tcase_add_test (tc, test_bt_cmd_application_create);
  tcase_add_test (tc, test_bt_cmd_application_play);
  tcase_add_test (tc, test_bt_cmd_application_play_two_files);
  tcase_add_test (tc, test_bt_cmd_application_info);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
