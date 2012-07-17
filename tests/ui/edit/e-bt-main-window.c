/* Buzztard
 * Copyright (C) 2009 Buzztard team <buzztard-devel@lists.sf.net>
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

//-- fixtures

static void
test_setup (void)
{
  bt_edit_setup ();
}

static void
test_teardown (void)
{
  bt_edit_teardown ();
}

//-- helper

/*
static gboolean timeout(gpointer data) {
  GST_INFO("shutting down the gui");
  gtk_main_quit();
  //gtk_widget_destroy(GTK_WIDGET(data));
  return FALSE;
}
*/

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

// test message dialog
static void
test_message_dialog1 (BT_TEST_ARGS)
{
  BT_TEST_START;
  BtEditApplication *app;
  BtMainWindow *main_window;

  app = bt_edit_application_new ();
  fail_unless (app != NULL, NULL);

  // get window
  g_object_get (app, "main-window", &main_window, NULL);
  fail_unless (main_window != NULL, NULL);

  dialog_data.dialog = NULL;
  dialog_data.response = GTK_RESPONSE_ACCEPT;
  g_idle_add (leave_dialog, (gpointer) main_window);
  bt_dialog_message (main_window, "title", "headline", "message");

  // close window
  gtk_widget_destroy (GTK_WIDGET (main_window));

  while (gtk_events_pending ())
    gtk_main_iteration ();

  // free application
  g_object_checked_unref (app);
  BT_TEST_END;
}

// test question dialog
static void
test_question_dialog1 (BT_TEST_ARGS)
{
  BT_TEST_START;
  BtEditApplication *app;
  BtMainWindow *main_window;
  gboolean res;

  app = bt_edit_application_new ();
  fail_unless (app != NULL, NULL);

  // get window
  g_object_get (app, "main-window", &main_window, NULL);
  fail_unless (main_window != NULL, NULL);

  dialog_data.dialog = NULL;
  dialog_data.response = GTK_RESPONSE_ACCEPT;
  g_idle_add (leave_dialog, (gpointer) main_window);
  res = bt_dialog_question (main_window, "title", "headline", "message");
  fail_unless (res == TRUE, NULL);

  // close window
  gtk_widget_destroy (GTK_WIDGET (main_window));

  while (gtk_events_pending ())
    gtk_main_iteration ();

  // free application
  g_object_checked_unref (app);
  BT_TEST_END;
}

// test open a song, but cancel
static void
test_open_song (BT_TEST_ARGS)
{
  BT_TEST_START;
  BtEditApplication *app;
  BtMainWindow *main_window;

  app = bt_edit_application_new ();
  fail_unless (app != NULL, NULL);

  // get window
  g_object_get (app, "main-window", &main_window, NULL);
  fail_unless (main_window != NULL, NULL);

  dialog_data.dialog = NULL;
  dialog_data.response = GTK_RESPONSE_CANCEL;
  g_idle_add (leave_dialog, (gpointer) main_window);
  bt_main_window_open_song (main_window);

  // close window
  gtk_widget_destroy (GTK_WIDGET (main_window));

  while (gtk_events_pending ())
    gtk_main_iteration ();

  // free application
  g_object_checked_unref (app);
  BT_TEST_END;
}

// test open a song, but cancel
static void
test_save_song_as (BT_TEST_ARGS)
{
  BT_TEST_START;
  BtEditApplication *app;
  BtMainWindow *main_window;

  app = bt_edit_application_new ();
  fail_unless (app != NULL, NULL);

  // create a new song
  bt_edit_application_new_song (app);

  // get window
  g_object_get (app, "main-window", &main_window, NULL);
  fail_unless (main_window != NULL, NULL);

  dialog_data.dialog = NULL;
  dialog_data.response = GTK_RESPONSE_CANCEL;
  g_idle_add (leave_dialog, (gpointer) main_window);
  bt_main_window_save_song_as (main_window);

  // close window
  gtk_widget_destroy (GTK_WIDGET (main_window));

  while (gtk_events_pending ())
    gtk_main_iteration ();

  // free everything
  g_object_checked_unref (app);
  BT_TEST_END;
}

TCase *
bt_main_window_example_case (void)
{
  TCase *tc = tcase_create ("BtMainWindowExamples");

  tcase_add_test (tc, test_message_dialog1);
  tcase_add_test (tc, test_question_dialog1);
  tcase_add_test (tc, test_open_song);
  tcase_add_test (tc, test_save_song_as);
  // we *must* use a checked fixture, as only this runs in the same context
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  return (tc);
}
