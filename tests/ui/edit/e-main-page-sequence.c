/* Buzztard
 * Copyright (C) 2011 Buzztard team <buzztard-devel@lists.sf.net>
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
static BtSong *song;
static BtSequence *sequence;
static BtMainWindow *main_window;
static BtMainPages *pages;

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
  bt_edit_application_new_song (app);
  g_object_get (app, "song", &song, "main-window", &main_window, NULL);
  g_object_get (song, "sequence", &sequence, NULL);
  g_object_get (main_window, "pages", &pages, NULL);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (pages),
      BT_MAIN_PAGES_SEQUENCE_PAGE);

  flush_main_loop ();

  // TODO(ensonic): why is gtk not invoking focus()?
  {
    GtkWidget *page;
    g_object_get (pages, "sequence-page", &page, NULL);
    GTK_WIDGET_GET_CLASS (page)->focus (page, GTK_DIR_TAB_FORWARD);
    g_object_unref (page);
  }
}

static void
test_teardown (void)
{
  g_object_unref (sequence);
  g_object_unref (song);
  g_object_unref (pages);

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

static void
test_bt_main_page_sequence_focus (BT_TEST_ARGS)
{
  BT_TEST_START;

  /* arrange */

  /* act */
  GtkWidget *widget = gtk_window_get_focus ((GtkWindow *) main_window);

  /* assert */
  ck_assert_str_eq ("sequence editor", gtk_widget_get_name (widget));

  /* cleanup */
  BT_TEST_END;
}

// activate tracks
static void
test_bt_main_page_sequence_active_machine (BT_TEST_ARGS)
{
  BT_TEST_START;
  BtMainPageSequence *sequence_page;
  BtMachine *machine1;
  BtMachine *machine2;
  GtkWidget *sequence_view;

  /* arrange */
  // We need to add the fx first as we can't trigger a refresh of the view
  // (main-page-sequence is not listening for "track-added" signal).
  // When adding a src, a track is added automatically and this updates the
  // view though.
  machine1 =
      BT_MACHINE (bt_processor_machine_new (song, "fx", "volume", 0L, NULL));
  bt_sequence_add_track (sequence, machine1, -1);
  machine2 =
      BT_MACHINE (bt_source_machine_new (song, "gen", "simsyn", 0L, NULL));
  g_object_get (G_OBJECT (pages), "sequence-page", &sequence_page, NULL);
  sequence_view = gtk_window_get_focus ((GtkWindow *) main_window);

  // make screenshot, focus on track=gen
  check_make_widget_screenshot ((GtkWidget *) sequence_page, "0");

  // emit key presses to go though the tracks
  // for unknown reasons the cursor is not moving when sending key presses, this
  // could be releated to how key bindings work, using the action signal works

  //check_send_key ((GtkWidget *) sequence_page, 0, GDK_Left, 0x71);
  g_signal_emit_by_name (sequence_view, "move-cursor",
      GTK_MOVEMENT_VISUAL_POSITIONS, -1, NULL);

  // make screenshot, focus on track=gen
  check_make_widget_screenshot ((GtkWidget *) sequence_page, "1");

  //check_send_key ((GtkWidget *) sequence_page, 0, GDK_Left, 0x71);
  g_signal_emit_by_name (sequence_view, "move-cursor",
      GTK_MOVEMENT_VISUAL_POSITIONS, -1, NULL);

  // make screenshot, focus on track=labels
  check_make_widget_screenshot ((GtkWidget *) sequence_page, "2");

  /* cleanup */
  g_object_unref (sequence_page);
  g_object_unref (machine1);
  g_object_unref (machine2);
  BT_TEST_END;
}

// activate tracks
static void
test_bt_main_page_sequence_enter_pattern (BT_TEST_ARGS)
{
  BT_TEST_START;
  BtMainPageSequence *sequence_page;
  BtMachine *machine;
  BtPattern *pattern1;
  BtCmdPattern *pattern2;

  /* arrange */
  machine =
      BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztard-test-mono-source", 0L, NULL));
  pattern1 = bt_pattern_new (song, "pattern-id", "pattern-name", 8L, machine);
  g_object_get (G_OBJECT (pages), "sequence-page", &sequence_page, NULL);

  /* act */
  // send a '1' key to insert the pattern we created (not the default)
  check_send_key ((GtkWidget *) sequence_page, 0, '1', 0);

  /* assert */
  pattern2 = bt_sequence_get_pattern (sequence, 0, 0);
  fail_unless ((BtCmdPattern *) pattern1 == pattern2);

  /* cleanup */
  g_object_unref (sequence_page);
  g_object_unref (pattern2);
  g_object_unref (pattern1);
  g_object_unref (machine);
  BT_TEST_END;
}

TCase *
bt_sequence_page_example_case (void)
{
  TCase *tc = tcase_create ("BtSequencePageExamples");

  tcase_add_test (tc, test_bt_main_page_sequence_focus);
  tcase_add_test (tc, test_bt_main_page_sequence_active_machine);
  tcase_add_test (tc, test_bt_main_page_sequence_enter_pattern);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
