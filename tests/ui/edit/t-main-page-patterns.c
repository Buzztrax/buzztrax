/* Buzztrax
 * Copyright (C) 2010 Buzztrax team <buzztrax-devel@buzztrax.org>
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
  g_object_get (main_window, "pages", &pages, NULL);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (pages),
      BT_MAIN_PAGES_PATTERNS_PAGE);

  flush_main_loop ();
}

static void
test_teardown (void)
{
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

static void
move_cursor_to (GtkWidget * w, guint group, guint param, guint digit, guint row)
{
  g_object_set (w, "cursor-group", group, "cursor-param", param, "cursor-digit",
      digit, "cursor-row", row, NULL);
}

//-- tests

// show pattern page with empty pattern and emit key-presses
START_TEST (test_bt_main_page_patterns_key_press_in_empty_pattern)
{
  BT_TEST_START;
  BtMainPagePatterns *pattern_page;
  BtMachine *machine;
  GtkWidget *pattern_editor;

  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  machine =
      BT_MACHINE (bt_source_machine_new (&cparams, "fakesrc", 0, NULL));
  g_object_get (G_OBJECT (pages), "patterns-page", &pattern_page, NULL);
  bt_main_page_patterns_show_machine (pattern_page, machine);
  pattern_editor = gtk_window_get_focus ((GtkWindow *) main_window);

  GST_INFO ("-- act --");
  check_send_key (pattern_editor, 0, '.', 0x3c);
  check_send_key (pattern_editor, 0, '0', 0x13);

  GST_INFO ("-- assert --");
  mark_point ();

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern_page);
  BT_TEST_END;
}
END_TEST
END_TEST

// show pattern page with empty pattern and emit mouse clicks
START_TEST (test_bt_main_page_patterns_mouse_click_in_empty_pattern)
{
  BT_TEST_START;
  BtMainPagePatterns *pattern_page;
  BtMachine *machine;
  GtkWidget *pattern_editor;

  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  machine =
      BT_MACHINE (bt_source_machine_new (&cparams, "fakesrc", 0, NULL));
  g_object_get (G_OBJECT (pages), "patterns-page", &pattern_page, NULL);
  bt_main_page_patterns_show_machine (pattern_page, machine);
  pattern_editor = gtk_window_get_focus ((GtkWindow *) main_window);

  GST_INFO ("-- act --");
  check_send_click (pattern_editor, 1, 10.0, 100.0);

  GST_INFO ("-- assert --");
  mark_point ();

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern_page);
  BT_TEST_END;
}
END_TEST
END_TEST

// test entering non note key
START_TEST (test_bt_main_page_patterns_non_note_key_press)
{
  BT_TEST_START;
  BtMainPagePatterns *pattern_page;

  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);
  g_object_get (G_OBJECT (pages), "patterns-page", &pattern_page, NULL);
  bt_main_page_patterns_show_pattern (pattern_page, pattern);
  GtkWidget *pattern_editor = gtk_window_get_focus ((GtkWindow *) main_window);
  move_cursor_to (pattern_editor, 0, 3, 0, 0);

  GST_INFO ("-- act --");
  // send a '4' key-press
  check_send_key (pattern_editor, 0, '4', 0x0d);

  GST_INFO ("-- assert --");
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern, 0, 3), NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern_page);
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST
END_TEST

// test that cursor pos stays unchanged on invalid key presses
START_TEST (test_bt_main_page_patterns_cursor_pos_on_non_note_key)
{
  BT_TEST_START;
  BtMainPagePatterns *pattern_page;

  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);
  g_object_get (G_OBJECT (pages), "patterns-page", &pattern_page, NULL);
  bt_main_page_patterns_show_pattern (pattern_page, pattern);
  GtkWidget *pattern_editor = gtk_window_get_focus ((GtkWindow *) main_window);
  move_cursor_to (pattern_editor, 0, 3, 0, 0);

  GST_INFO ("-- act --");
  // send a '4' key-press
  check_send_key (pattern_editor, 0, '4', 0x0d);
  check_send_key (pattern_editor, 0, '1', 0x0a);

  GST_INFO ("-- assert --");
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern, 0, 3),
      "off");

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern_page);
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST
END_TEST


TCase *
bt_main_page_patterns_test_case (void)
{
  TCase *tc = tcase_create ("BtMainPagePatternsTests");

  tcase_add_test (tc, test_bt_main_page_patterns_key_press_in_empty_pattern);
  tcase_add_test (tc, test_bt_main_page_patterns_mouse_click_in_empty_pattern);
  tcase_add_test (tc, test_bt_main_page_patterns_non_note_key_press);
  tcase_add_test (tc, test_bt_main_page_patterns_cursor_pos_on_non_note_key);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
