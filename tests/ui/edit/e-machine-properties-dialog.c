/* Buzztard
 * Copyright (C) 2007 Buzztard team <buzztard-devel@lists.sf.net>
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
  GST_INFO
      ("================================================================================");
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

  g_object_checked_unref (app);
  bt_edit_teardown ();
}

static void
case_teardown (void)
{
}

//-- helper

//-- tests

// show machine properties dialog
static gchar *element_names[] = {
  "buzztard-test-no-arg-mono-source", "buzztard-test-mono-source",
  "buzztard-test-poly-source", "buzztard-test-poly-source"
};
static gulong element_voices[] = { 0, 0, 0, 1 };

static void
test_bt_machine_properties_dialog_create (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSong *song;
  BtMachine *machine;
  GtkWidget *dialog;

  bt_edit_application_new_song (app);
  g_object_get (app, "song", &song, NULL);
  machine = BT_MACHINE (bt_source_machine_new (song, "gen", element_names[_i],
          element_voices[_i], NULL));

  /* act */
  dialog = GTK_WIDGET (bt_machine_properties_dialog_new (machine));

  /* assert */
  fail_unless (dialog != NULL, NULL);
  gtk_widget_show_all (dialog);
  check_make_widget_screenshot (GTK_WIDGET (dialog), NULL);

  /* cleanup */
  gtk_widget_destroy (dialog);
  g_object_unref (song);
  BT_TEST_END;
}


// show machine properties dialog while playing to test parameter updates
static gchar *machine_ids[] = { "beep1", "echo1", "audio_sink" };

static void
test_bt_machine_properties_dialog_update (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSong *song;
  BtSetup *setup;
  BtMachine *machine;
  GtkWidget *dialog;

  bt_edit_application_load_song (app, check_get_test_song_path ("melo3.xml"));
  g_object_get (app, "song", &song, NULL);
  g_object_get (song, "setup", &setup, NULL);
  machine = bt_setup_get_machine_by_id (setup, machine_ids[_i]);
  dialog = GTK_WIDGET (bt_machine_properties_dialog_new (machine));
  gtk_widget_show_all (dialog);

  /* act */
  // play for a while to trigger dialog updates
  bt_song_play (song);
  bt_song_update_playback_position (song);
  flush_main_loop ();
  bt_song_stop (song);

  /* assert */
  mark_point ();

  /* cleanup */
  gtk_widget_destroy (dialog);
  g_object_unref (machine);
  g_object_unref (setup);
  g_object_unref (song);
  BT_TEST_END;
}

// TODO(ensonic): test adding/removing voices/wires while dialog is shown

TCase *
bt_machine_properties_dialog_example_case (void)
{
  TCase *tc = tcase_create ("BtMachinePropertiesDialogExamples");

  tcase_add_loop_test (tc, test_bt_machine_properties_dialog_create, 0,
      G_N_ELEMENTS (element_names));
  tcase_add_loop_test (tc, test_bt_machine_properties_dialog_update, 0,
      G_N_ELEMENTS (machine_ids));
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
