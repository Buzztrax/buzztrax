/* Buzztard
 * Copyright (C) 2010 Buzztard team <buzztard-devel@lists.sf.net>
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

//-- tests

// this tests triggering certain dialog resposes
static struct
{
  GtkDialog *dialog;
  gint response;
} dialog_data;

static gboolean
leave_dialog (gpointer user_data)
{
  if (dialog_data.dialog) {
    gtk_dialog_response (dialog_data.dialog, dialog_data.response);
    return FALSE;
  } else {
    BtMainWindow *main_window = BT_MAIN_WINDOW (user_data);

    // need to get get the dialog here
    g_object_get (main_window, "dialog", &dialog_data.dialog, NULL);
    return TRUE;
  }
}

// create app and then unconditionally destroy window
static void
test_bt_machine_actions_about_dialog (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSong *song;
  BtSetup *setup;
  BtMachine *machine;

  bt_edit_application_new_song (app);
  g_object_get (app, "song", &song, NULL);
  g_object_get (song, "setup", &setup, NULL);
  machine = bt_setup_get_machine_by_id (setup, "master");

  dialog_data.dialog = NULL;
  dialog_data.response = GTK_RESPONSE_ACCEPT;
  g_idle_add (leave_dialog, (gpointer) main_window);

  /* act */
  bt_machine_action_about (machine, main_window);

  /* assert */
  fail_unless (dialog_data.dialog != NULL, NULL);

  /* cleanup */
  gst_object_unref (machine);
  g_object_unref (setup);
  g_object_unref (song);
  BT_TEST_END;
}

TCase *
bt_machine_actions_example_case (void)
{
  TCase *tc = tcase_create ("BtMachineActionsExamples");

  tcase_add_test (tc, test_bt_machine_actions_about_dialog);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
