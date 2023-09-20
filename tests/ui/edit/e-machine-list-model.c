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

#include "m-bt-edit.h"

//-- globals

static BtEditApplication *app;
static BtSong *song;
static BtSetup *setup;
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
  g_object_get (app, "song", &song, "main-window", &main_window, NULL);
  g_object_get (song, "setup", &setup, NULL);

  flush_main_loop ();
}

static void
test_teardown (void)
{
  g_object_unref (setup);
  g_object_unref (song);

  flush_main_loop ();

  gtk_widget_destroy (GTK_WIDGET (main_window));
  flush_main_loop ();

  ck_g_object_final_unref (app);
  bt_edit_teardown ();
}

static void
case_teardown (void)
{
}

//-- tests

START_TEST (test_bt_machine_list_model_create)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  BtMachineListModel *model = bt_machine_list_model_new (setup);

  GST_INFO ("-- assert --");
  ck_assert (model != NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (model);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_machine_list_model_get_machine)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GtkTreeIter iter;
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  BtMachine *machine1 = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0, NULL));
  BtMachineListModel *model = bt_machine_list_model_new (setup);
  gtk_tree_model_get_iter_first ((GtkTreeModel *) model, &iter);

  GST_INFO ("-- act --");
  BtMachine *machine2 = bt_machine_list_model_get_object (model, &iter);

  GST_INFO ("-- assert --");
  ck_assert (machine1 == machine2);

  GST_INFO ("-- cleanup --");
  g_object_unref (model);
  BT_TEST_END;
}
END_TEST

TCase *
bt_machine_list_model_example_case (void)
{
  TCase *tc = tcase_create ("BtMachineListModelExamples");

  tcase_add_test (tc, test_bt_machine_list_model_create);
  tcase_add_test (tc, test_bt_machine_list_model_get_machine);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
