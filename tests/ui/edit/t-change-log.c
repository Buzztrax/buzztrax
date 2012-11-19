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
  BT_CASE_START;
}

static void
test_setup (void)
{
  bt_edit_setup ();
  app = bt_edit_application_new ();
  bt_edit_application_new_song (app);
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

//-- tests

static void
test_bt_change_log_recover_while_missing_machine (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtChangeLog *cl = bt_change_log_new ();
  gchar *log_name = g_build_filename (g_get_tmp_dir (), "bt-crash.log", NULL);
  gchar content[] =
      PACKAGE " edit journal : " PACKAGE_VERSION "\n"
      "\n"
      "BtMainPageMachines::add_machine 0,\"synth\",\"-DOES-NOT-EXIST-\"\n"
      "BtMainPageMachines::set_machine_property \"synth\",\"ypos\",\"-0.3\"\n"
      "BtMainPageMachines::set_machine_property \"synth\",\"xpos\",\"-0.4\"\n";
  g_file_set_contents (log_name, content, strlen (content), NULL);

  /* act */
  gboolean res = bt_change_log_recover (cl, log_name);

  /* assert */
  fail_if (res, NULL);

  /* cleanup */
  flush_main_loop ();
  g_unlink (log_name);
  g_object_unref (cl);
  BT_TEST_END;
}

TCase *
bt_change_log_test_case (void)
{
  TCase *tc = tcase_create ("BtChangeLogTests");

  tcase_add_test (tc, test_bt_change_log_recover_while_missing_machine);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
