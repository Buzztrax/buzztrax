/* Buzztrax
 * Copyright (C) 2011 Buzztrax team <buzztrax-devel@buzztrax.org>
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
static BtSequence *sequence;
static BtMainWindow *main_window;
static BtMainPages *pages;

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
  g_object_get (song, "sequence", &sequence, NULL);
  g_object_get (main_window, "pages", &pages, NULL);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (pages),
      BT_MAIN_PAGES_SEQUENCE_PAGE);

  flush_main_loop ();
}

static void
test_teardown (void)
{
  g_object_unref (sequence);
  g_object_unref (song);
  g_object_unref (pages);

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

static void
test_bt_main_page_sequence_focus (BT_TEST_ARGS)
{
  BT_TEST_START;

  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  GtkWidget *widget = gtk_window_get_focus ((GtkWindow *) main_window);

  GST_INFO ("-- assert --");
  ck_assert_str_eq ("sequence editor", gtk_widget_get_name (widget));

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}

// activate tracks
static void
test_bt_main_page_sequence_active_machine (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  // We need to add the fx first as we can't trigger a refresh of the view
  // (main-page-sequence is not listening for "track-added" signal).
  // When adding a src, a track is added automatically and this updates the
  // view though.
  bt_sequence_add_track (sequence,
      BT_MACHINE (bt_processor_machine_new (song, "fx", "volume", 0L, NULL)),
      -1);
  bt_source_machine_new (song, "gen", "simsyn", 0L, NULL);
  BtMainPageSequence *sequence_page;
  GtkWidget *sequence_view;

  g_object_get (G_OBJECT (pages), "sequence-page", &sequence_page, NULL);
  sequence_view = gtk_window_get_focus ((GtkWindow *) main_window);

  // make screenshot, focus on track=gen
  check_make_widget_screenshot ((GtkWidget *) sequence_page, "0");

  // emit key presses to go though the tracks
  // for unknown reasons the cursor is not moving when sending key presses, this
  // could be releated to how key bindings work, using the action signal works

  //check_send_key ((GtkWidget *) sequence_page, 0, GDK_Left, 0x71);
  gboolean sig_res;
  g_signal_emit_by_name (sequence_view, "move-cursor",
      GTK_MOVEMENT_VISUAL_POSITIONS, -1, &sig_res);

  // make screenshot, focus on track=gen
  check_make_widget_screenshot ((GtkWidget *) sequence_page, "1");

  //check_send_key ((GtkWidget *) sequence_page, 0, GDK_Left, 0x71);
  g_signal_emit_by_name (sequence_view, "move-cursor",
      GTK_MOVEMENT_VISUAL_POSITIONS, -1, &sig_res);

  // make screenshot, focus on track=labels
  check_make_widget_screenshot ((GtkWidget *) sequence_page, "2");

  GST_INFO ("-- cleanup --");
  g_object_unref (sequence_page);
  BT_TEST_END;
}

// activate tracks
static void
test_bt_main_page_sequence_enter_pattern (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztrax-test-mono-source", 0L, NULL));
  BtPattern *pattern1 = bt_pattern_new (song, "pattern-name", 8L, machine);
  BtMainPageSequence *sequence_page;
  BtCmdPattern *pattern2;

  g_object_get (G_OBJECT (pages), "sequence-page", &sequence_page, NULL);

  GST_INFO ("-- act --");
  // send a '1' key to insert the pattern we created (not the default)
  check_send_key ((GtkWidget *) sequence_page, 0, '1', 0);

  GST_INFO ("-- assert --");
  pattern2 = bt_sequence_get_pattern (sequence, 0, 0);
  fail_unless ((BtCmdPattern *) pattern1 == pattern2);

  GST_INFO ("-- cleanup --");
  g_object_unref (sequence_page);
  g_object_unref (pattern2);
  g_object_unref (pattern1);
  BT_TEST_END;
}

TCase *
bt_main_page_sequence_example_case (void)
{
  TCase *tc = tcase_create ("BtMainPageSequenceExamples");

  tcase_add_test (tc, test_bt_main_page_sequence_focus);
  tcase_add_test (tc, test_bt_main_page_sequence_active_machine);
  tcase_add_test (tc, test_bt_main_page_sequence_enter_pattern);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
