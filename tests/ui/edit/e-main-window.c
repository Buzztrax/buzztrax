/* Buzztrax
 * Copyright (C) 2009 Buzztrax team <buzztrax-devel@buzztrax.org>
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

  g_object_checked_unref (app);
  bt_edit_teardown ();
}

static void
case_teardown (void)
{
}

//-- helper

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
test_bt_main_window_message (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  dialog_data.dialog = NULL;
  dialog_data.response = GTK_RESPONSE_ACCEPT;
  g_idle_add (leave_dialog, (gpointer) main_window);

  GST_INFO ("-- act --");
  bt_dialog_message (main_window, "title", "headline", "message");

  GST_INFO ("-- assert --");
  fail_unless (dialog_data.dialog != NULL, NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}

// test question dialog
static void
test_bt_main_window_question (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  dialog_data.dialog = NULL;
  dialog_data.response = GTK_RESPONSE_ACCEPT;
  g_idle_add (leave_dialog, (gpointer) main_window);

  GST_INFO ("-- act --");
  gboolean res =
      bt_dialog_question (main_window, "title", "headline", "message");

  GST_INFO ("-- assert --");
  fail_unless (res == TRUE, NULL);
  fail_unless (dialog_data.dialog != NULL, NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}

// test open a song, but cancel
static void
test_bt_main_window_open_song (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  dialog_data.dialog = NULL;
  dialog_data.response = GTK_RESPONSE_CANCEL;
  g_idle_add (leave_dialog, (gpointer) main_window);

  GST_INFO ("-- act --");
  bt_main_window_open_song (main_window);

  GST_INFO ("-- assert --");
  fail_unless (dialog_data.dialog != NULL, NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}

// test open a song, but cancel
static void
test_bt_main_window_save_song_as (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  bt_edit_application_new_song (app);
  dialog_data.dialog = NULL;
  dialog_data.response = GTK_RESPONSE_CANCEL;
  g_idle_add (leave_dialog, (gpointer) main_window);

  GST_INFO ("-- act --");
  bt_main_window_save_song_as (main_window);

  GST_INFO ("-- assert --");
  fail_unless (dialog_data.dialog != NULL, NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}

static void
test_bt_main_window_check_unsaved (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  bt_edit_application_new_song (app);
  GST_INFO ("before act");

  GST_INFO ("-- act --");
  gboolean res = bt_main_window_check_unsaved_song (main_window, "X", "X");

  GST_INFO ("-- assert --");
  fail_unless (res == TRUE, NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}

TCase *
bt_main_window_example_case (void)
{
  TCase *tc = tcase_create ("BtMainWindowExamples");

  tcase_add_test (tc, test_bt_main_window_message);
  tcase_add_test (tc, test_bt_main_window_question);
  tcase_add_test (tc, test_bt_main_window_open_song);
  tcase_add_test (tc, test_bt_main_window_save_song_as);
  tcase_add_test (tc, test_bt_main_window_check_unsaved);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
