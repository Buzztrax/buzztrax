/* Buzztrax
 * Copyright (C) 2007 Buzztrax team <buzztrax-devel@buzztrax.org>
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

  flush_main_loop ();
}

static void
test_teardown (void)
{
  gtk_widget_destroy (GTK_WIDGET (main_window));
  flush_main_loop ();

  ck_g_object_final_unref (app);
  bt_edit_teardown ();
}

static void
case_teardown (void)
{
}

//-- helper

//-- tests

// load a song and show machine properties dialog
START_TEST (test_bt_signal_analysis_dialog_create)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSong *song;
  BtSetup *setup;
  BtMachine *machine;
  BtWire *wire;
  GtkWidget *dialog;

  bt_edit_application_load_song (app, check_get_test_song_path ("melo3.xml"),
      NULL);
  g_object_get (app, "song", &song, NULL);
  g_object_get (song, "setup", &setup, NULL);
  machine = bt_setup_get_machine_by_id (setup, "beep1");
  wire = bt_setup_get_wire_by_src_machine (setup, machine);

  GST_INFO ("-- act --");
  if ((dialog = GTK_WIDGET (bt_signal_analysis_dialog_new (GST_BIN (wire))))) {
    GST_INFO ("-- assert --");
    // if we miss gst-plugins like level or spectrum, we don't get the dialog
    mark_point ();
    gtk_widget_show_all (dialog);
    check_make_widget_screenshot (GTK_WIDGET (dialog), NULL);

    GST_INFO ("-- cleanup --");
    gtk_widget_destroy (dialog);
  }
  g_object_unref (wire);
  g_object_unref (machine);
  g_object_unref (setup);
  g_object_unref (song);
  BT_TEST_END;
}
END_TEST

TCase *
bt_signal_analysis_dialog_example_case (void)
{
  TCase *tc = tcase_create ("BtSignalAnalysisDialogExamples");

  tcase_add_test (tc, test_bt_signal_analysis_dialog_create);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
