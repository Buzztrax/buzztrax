/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
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

START_TEST (test_bt_main_page_patterns_focus)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);
  BtMainPagePatterns *pattern_page;
  g_object_get (G_OBJECT (pages), "patterns-page", &pattern_page, NULL);
  bt_main_page_patterns_show_pattern (pattern_page, pattern);

  GST_INFO ("-- act --");
  GtkWidget *widget = gtk_window_get_focus ((GtkWindow *) main_window);

  GST_INFO ("-- assert --");
  ck_assert_str_eq ("pattern editor", gtk_widget_get_name (widget));

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern_page);
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST

// test entering notes
START_TEST (test_bt_main_page_patterns_enter_note)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);
  BtMainPagePatterns *pattern_page;
  GtkWidget *pattern_editor;
  gchar *str;

  g_object_get (G_OBJECT (pages), "patterns-page", &pattern_page, NULL);
  bt_main_page_patterns_show_pattern (pattern_page, pattern);
  pattern_editor = gtk_window_get_focus ((GtkWindow *) main_window);
  move_cursor_to (pattern_editor, 0, 3, 0, 0);

  GST_INFO ("-- act --");
  // send a 'q' key-press and check that it results in a 'c-' of any octave
  check_send_key (pattern_editor, 0, 'q', 0x18);

  GST_INFO ("-- assert --");
  str = bt_pattern_get_global_event (pattern, 0, 3);
  ck_assert (str != NULL);
  str[2] = '0';
  ck_assert_str_eq_and_free (str, "c-0");

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern_page);
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_main_page_patterns_enter_note_off)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);
  BtMainPagePatterns *pattern_page;
  GtkWidget *pattern_editor;

  g_object_get (G_OBJECT (pages), "patterns-page", &pattern_page, NULL);
  bt_main_page_patterns_show_pattern (pattern_page, pattern);
  pattern_editor = gtk_window_get_focus ((GtkWindow *) main_window);
  move_cursor_to (pattern_editor, 0, 3, 0, 0);

  GST_INFO ("-- act --");
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

// test entering notes
START_TEST (test_bt_main_page_patterns_clear_note)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);
  BtMainPagePatterns *pattern_page;
  GtkWidget *pattern_editor;

  g_object_get (G_OBJECT (pages), "patterns-page", &pattern_page, NULL);
  bt_main_page_patterns_show_pattern (pattern_page, pattern);
  pattern_editor = gtk_window_get_focus ((GtkWindow *) main_window);
  move_cursor_to (pattern_editor, 0, 3, 0, 0);
  check_send_key (pattern_editor, 0, 'q', 0x18);
  check_send_key (pattern_editor, 0, GDK_KEY_Page_Up, 0);

  GST_INFO ("-- act --");
  // send a '.' key-press
  check_send_key (pattern_editor, 0, '.', 0x3c);

  GST_INFO ("-- assert --");
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern, 0, 3), NULL);

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern_page);
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST

// test entering booleans
START_TEST (test_bt_main_page_patterns_enter_switch)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);
  BtMainPagePatterns *pattern_page;
  GtkWidget *pattern_editor;

  g_object_get (G_OBJECT (pages), "patterns-page", &pattern_page, NULL);
  bt_main_page_patterns_show_pattern (pattern_page, pattern);
  pattern_editor = gtk_window_get_focus ((GtkWindow *) main_window);
  move_cursor_to (pattern_editor, 0, 2, 0, 0);

  GST_INFO ("-- act --");
  check_send_key (pattern_editor, 0, '1', 0x0a);

  GST_INFO ("-- assert --");
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern, 0, 2), "1");

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern_page);
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST

// test entering sparse enum
START_TEST (test_bt_main_page_patterns_enter_sparse_enum)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);
  BtMainPagePatterns *pattern_page;
  GtkWidget *pattern_editor;

  g_object_get (G_OBJECT (pages), "patterns-page", &pattern_page, NULL);
  bt_main_page_patterns_show_pattern (pattern_page, pattern);
  pattern_editor = gtk_window_get_focus ((GtkWindow *) main_window);
  move_cursor_to (pattern_editor, 0, 4, 1, 0);

  GST_INFO ("-- act --");
  check_send_key (pattern_editor, 0, '1', 0x0a);

  GST_INFO ("-- assert --");
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern, 0, 4), "1");

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern_page);
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST

// test entering sparse enum
START_TEST (test_bt_main_page_patterns_enter_invalid_sparse_enum)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);
  BtMainPagePatterns *pattern_page;
  GtkWidget *pattern_editor;

  g_object_get (G_OBJECT (pages), "patterns-page", &pattern_page, NULL);
  bt_main_page_patterns_show_pattern (pattern_page, pattern);
  pattern_editor = gtk_window_get_focus ((GtkWindow *) main_window);
  move_cursor_to (pattern_editor, 0, 4, 0, 0);

  GST_INFO ("-- act --");
  check_send_key (pattern_editor, 0, '1', 0x0a);

  GST_INFO ("-- assert --");
  fail_if (bt_pattern_test_global_event (pattern, 0, 4));

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern_page);
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST

// test entering sparse enum
START_TEST (test_bt_main_page_patterns_enter_sparse_enum_in_2_steps)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-mono-source", 0L, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);
  BtMainPagePatterns *pattern_page;
  GtkWidget *pattern_editor;

  g_object_get (G_OBJECT (pages), "patterns-page", &pattern_page, NULL);
  bt_main_page_patterns_show_pattern (pattern_page, pattern);
  pattern_editor = gtk_window_get_focus ((GtkWindow *) main_window);
  move_cursor_to (pattern_editor, 0, 4, 0, 0);

  GST_INFO ("-- act --");
  check_send_key (pattern_editor, 0, '1', 0x0a);
  move_cursor_to (pattern_editor, 0, 4, 1, 0);
  check_send_key (pattern_editor, 0, '4', 0x0d);

  GST_INFO ("-- assert --");
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern, 0, 4), "20");

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern_page);
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST


START_TEST (test_bt_main_page_patterns_pattern_voices)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMachineConstructorParams cparams;
  cparams.song = song;
  cparams.id = "gen";
  BtMachine *machine = BT_MACHINE (bt_source_machine_new (&cparams,
          "buzztrax-test-poly-source", 1L, NULL));
  BtPattern *pattern = bt_pattern_new (song, "pattern-name", 8L, machine);
  BtMainPagePatterns *pattern_page;
  GtkWidget *pattern_editor;
  gulong voices;

  g_object_get (G_OBJECT (pages), "patterns-page", &pattern_page, NULL);
  bt_main_page_patterns_show_pattern (pattern_page, pattern);
  pattern_editor = gtk_window_get_focus ((GtkWindow *) main_window);

  GST_INFO ("-- act --");
  // change voices
  g_object_set (machine, "voices", 2L, NULL);
  g_object_get (pattern, "voices", &voices, NULL);

  GST_INFO ("-- assert --");
  ck_assert (voices == 2);
  // send two tab keys to ensure the new voice is visible
  check_send_key (pattern_editor, 0, GDK_KEY_Tab, 0);
  check_send_key (pattern_editor, 0, GDK_KEY_Tab, 0);
  mark_point ();

  GST_INFO ("-- cleanup --");
  g_object_unref (pattern_page);
  g_object_unref (pattern);
  BT_TEST_END;
}
END_TEST

TCase *
bt_main_page_patterns_example_case (void)
{
  TCase *tc = tcase_create ("BtMainPagePatternsExamples");

  tcase_add_test (tc, test_bt_main_page_patterns_focus);
  tcase_add_test (tc, test_bt_main_page_patterns_enter_note);
  tcase_add_test (tc, test_bt_main_page_patterns_enter_note_off);
  tcase_add_test (tc, test_bt_main_page_patterns_clear_note);
  tcase_add_test (tc, test_bt_main_page_patterns_enter_switch);
  tcase_add_test (tc, test_bt_main_page_patterns_enter_sparse_enum);
  tcase_add_test (tc, test_bt_main_page_patterns_enter_invalid_sparse_enum);
  tcase_add_test (tc, test_bt_main_page_patterns_enter_sparse_enum_in_2_steps);
  tcase_add_test (tc, test_bt_main_page_patterns_pattern_voices);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
