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

// this tests triggering certain dialog resposes
static struct
{
  GtkDialog *dialog;
  gint response;
  gchar *current_name;
} dialog_data;

static void
case_setup (void)
{
  BT_CASE_START;
}

static void
test_setup (void)
{
  dialog_data.dialog = NULL;
  dialog_data.current_name = NULL;
  
  bt_edit_setup ();
  app = bt_edit_application_new ();
  g_object_get (app, "main-window", &main_window, NULL);

  flush_main_loop ();
}

static void
test_teardown (void)
{
  g_clear_pointer(&dialog_data.current_name, g_free);
  
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
    
    if (GTK_IS_FILE_CHOOSER (dialog_data.dialog)) {
      dialog_data.current_name = gtk_file_chooser_get_current_name(
        GTK_FILE_CHOOSER (dialog_data.dialog));
    }
    
    return TRUE;
  }
}

// test message dialog
START_TEST (test_bt_main_window_message)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  dialog_data.response = GTK_RESPONSE_ACCEPT;
  g_idle_add (leave_dialog, (gpointer) main_window);

  GST_INFO ("-- act --");
  bt_dialog_message (main_window, "title", "headline", "message");

  GST_INFO ("-- assert --");
  ck_assert (dialog_data.dialog != NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

// test question dialog
START_TEST (test_bt_main_window_question)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  dialog_data.response = GTK_RESPONSE_ACCEPT;
  g_idle_add (leave_dialog, (gpointer) main_window);

  GST_INFO ("-- act --");
  gboolean res =
      bt_dialog_question (main_window, "title", "headline", "message");

  GST_INFO ("-- assert --");
  ck_assert (res == TRUE);
  ck_assert (dialog_data.dialog != NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

// test open a song, but cancel
START_TEST (test_bt_main_window_open_song)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  dialog_data.response = GTK_RESPONSE_CANCEL;
  g_idle_add (leave_dialog, (gpointer) main_window);

  GST_INFO ("-- act --");
  bt_main_window_open_song (main_window);

  GST_INFO ("-- assert --");
  ck_assert (dialog_data.dialog != NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

// test open a song, but cancel
START_TEST (test_bt_main_window_save_song_as)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  bt_edit_application_new_song (app);
  dialog_data.response = GTK_RESPONSE_CANCEL;
  g_idle_add (leave_dialog, (gpointer) main_window);

  GST_INFO ("-- act --");
  bt_main_window_save_song_as (main_window);

  GST_INFO ("-- assert --");
  ck_assert (dialog_data.dialog != NULL);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_main_window_check_unsaved)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  bt_edit_application_new_song (app);
  GST_INFO ("before act");

  GST_INFO ("-- act --");
  gboolean res = bt_main_window_check_unsaved_song (main_window, "X", "X");

  GST_INFO ("-- assert --");
  ck_assert (res == TRUE);

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

// ensure window title is correct without a song title explicitly set
START_TEST (test_bt_main_window_song_title_untitled)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  bt_edit_application_new_song (app);

  GST_INFO ("-- assert --");
  const gchar *title =
      gtk_window_get_title (GTK_WINDOW (main_window));
  
  ck_assert_msg (
      g_str_has_prefix (title, "untitled song"),
      "Window title for untitled song must start with 'untitled song', "
      "but was '%s'",
      title
    );

  GST_INFO ("-- cleanup --");
  
  BT_TEST_END;
}
END_TEST

// ensure window title is correct with a song title explicitly set
START_TEST (test_bt_main_window_song_title_titled)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  bt_edit_application_new_song (app);

  GST_INFO ("-- assert --");
  BtSong *song;
  g_object_get (app, "song", &song, NULL);
  
  BtSongInfo *song_info;
  g_object_get (G_OBJECT (song), "song-info", &song_info, NULL);
  g_object_set (G_OBJECT (song_info), "name", "songtitle", NULL);
  
  const gchar *title =
      gtk_window_get_title (GTK_WINDOW (main_window));
  
  ck_assert_msg (
      g_str_has_prefix (title, "songtitle"),
      "Window title for titled song must include song title, but was '%s'",
      title
    );
  
  GST_INFO ("-- cleanup --");
  g_clear_object (&song);
  g_clear_object (&song_info);
  
  BT_TEST_END;
}
END_TEST

// ensure that the filename chosen for an untitled song is correct
START_TEST (test_bt_main_window_song_title_save)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  dialog_data.response = GTK_RESPONSE_CANCEL;
  g_idle_add (leave_dialog, (gpointer) main_window);

  GST_INFO ("-- act --");
  bt_edit_application_new_song (app);
  bt_main_window_save_song_as (main_window);

  GST_INFO ("-- assert --");
  g_idle_add (leave_dialog, (gpointer) main_window);
  
  ck_assert_msg (
      g_strcmp0 (dialog_data.current_name, "untitled song.xml") == 0,
      "Basename for untitled song must be 'untitled song.xml', but was '%s'",
      dialog_data.current_name
    );
  
  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

TCase *
bt_main_window_example_case (void)
{
  TCase *tc = tcase_create ("BtMainWindowExamples");

  tcase_add_test (tc, test_bt_main_window_message);
  tcase_add_test (tc, test_bt_main_window_question);
  tcase_add_test (tc, test_bt_main_window_open_song);
  tcase_add_test (tc, test_bt_main_window_save_song_as);
  tcase_add_test (tc, test_bt_main_window_check_unsaved);
  tcase_add_test (tc, test_bt_main_window_song_title_titled);
  tcase_add_test (tc, test_bt_main_window_song_title_untitled);
  tcase_add_test (tc, test_bt_main_window_song_title_save);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
