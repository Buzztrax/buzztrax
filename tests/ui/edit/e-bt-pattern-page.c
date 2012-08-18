/* Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
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
  g_object_get (main_window, "pages", &pages, NULL);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (pages),
      BT_MAIN_PAGES_PATTERNS_PAGE);

  while (gtk_events_pending ())
    gtk_main_iteration ();
}

static void
test_teardown (void)
{
  g_object_unref (song);
  g_object_unref (pages);

  gtk_widget_destroy (GTK_WIDGET (main_window));
  while (gtk_events_pending ())
    gtk_main_iteration ();

  g_object_checked_unref (app);
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

static void
test_bt_main_page_patterns_focus (BT_TEST_ARGS)
{
  BT_TEST_START;
  BtMainPagePatterns *pattern_page;
  BtMachine *machine;
  BtPattern *pattern;

  /* arrange */
  machine =
      BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztard-test-mono-source", 0L, NULL));
  pattern = bt_pattern_new (song, "pattern-id", "pattern-name", 8L, machine);
  g_object_get (G_OBJECT (pages), "patterns-page", &pattern_page, NULL);
  bt_main_page_patterns_show_pattern (pattern_page, pattern);

  /* act */
  GtkWidget *widget = gtk_window_get_focus ((GtkWindow *) main_window);

  /* assert */
  ck_assert_str_eq ("pattern editor", gtk_widget_get_name (widget));

  /* cleanup */
  g_object_unref (pattern_page);
  g_object_unref (pattern);
  g_object_unref (machine);
  BT_TEST_END;
}

// test entering notes
static void
test_bt_main_page_patterns_enter_note (BT_TEST_ARGS)
{
  BT_TEST_START;
  BtMainPagePatterns *pattern_page;
  BtMachine *machine;
  BtPattern *pattern;
  GtkWidget *pattern_editor;
  gchar *str;

  /* arrange */
  machine =
      BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztard-test-mono-source", 0L, NULL));
  pattern = bt_pattern_new (song, "pattern-id", "pattern-name", 8L, machine);
  g_object_get (G_OBJECT (pages), "patterns-page", &pattern_page, NULL);
  bt_main_page_patterns_show_pattern (pattern_page, pattern);
  pattern_editor = gtk_window_get_focus ((GtkWindow *) main_window);
  move_cursor_to (pattern_editor, 0, 3, 0, 0);

  /* act */
  // send a 'q' key-press and check that it results in a 'c-' of any octave
  check_send_key (pattern_editor, 0, 'q', 0x18);

  /* assert */
  str = bt_pattern_get_global_event (pattern, 0, 3);
  fail_unless (str != NULL, NULL);
  str[2] = '0';
  ck_assert_str_eq_and_free (str, "c-0");

  /* cleanup */
  g_object_unref (pattern_page);
  g_object_unref (pattern);
  g_object_unref (machine);
  BT_TEST_END;
}

static void
test_bt_main_page_patterns_enter_note_off (BT_TEST_ARGS)
{
  BT_TEST_START;
  BtMainPagePatterns *pattern_page;
  BtMachine *machine;
  BtPattern *pattern;
  GtkWidget *pattern_editor;

  /* arrange */
  machine =
      BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztard-test-mono-source", 0L, NULL));
  pattern = bt_pattern_new (song, "pattern-id", "pattern-name", 8L, machine);
  g_object_get (G_OBJECT (pages), "patterns-page", &pattern_page, NULL);
  bt_main_page_patterns_show_pattern (pattern_page, pattern);
  pattern_editor = gtk_window_get_focus ((GtkWindow *) main_window);
  move_cursor_to (pattern_editor, 0, 3, 0, 0);

  /* act */
  check_send_key (pattern_editor, 0, '1', 0x0a);

  /* assert */
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern, 0, 3),
      "off");

  /* cleanup */
  g_object_unref (pattern_page);
  g_object_unref (pattern);
  g_object_unref (machine);
  BT_TEST_END;
}

// test entering notes
static void
test_bt_main_page_patterns_clear_note (BT_TEST_ARGS)
{
  BT_TEST_START;
  BtMainPagePatterns *pattern_page;
  BtMachine *machine;
  BtPattern *pattern;
  GtkWidget *pattern_editor;

  /* arrange */
  machine =
      BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztard-test-mono-source", 0L, NULL));
  pattern = bt_pattern_new (song, "pattern-id", "pattern-name", 8L, machine);
  g_object_get (G_OBJECT (pages), "patterns-page", &pattern_page, NULL);
  bt_main_page_patterns_show_pattern (pattern_page, pattern);
  pattern_editor = gtk_window_get_focus ((GtkWindow *) main_window);
  move_cursor_to (pattern_editor, 0, 3, 0, 0);
  check_send_key (pattern_editor, 0, 'q', 0x18);
  check_send_key (pattern_editor, 0, GDK_Page_Up, 0);

  /* act */
  // send a '.' key-press
  check_send_key (pattern_editor, 0, '.', 0x3c);

  /* assert */
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern, 0, 3), NULL);

  /* cleanup */
  g_object_unref (pattern_page);
  g_object_unref (pattern);
  g_object_unref (machine);
  BT_TEST_END;
}

// test entering booleans
static void
test_bt_main_page_patterns_enter_switch (BT_TEST_ARGS)
{
  BT_TEST_START;
  BtMainPagePatterns *pattern_page;
  BtMachine *machine;
  BtPattern *pattern;
  GtkWidget *pattern_editor;

  /* arrange */
  machine =
      BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztard-test-mono-source", 0L, NULL));
  pattern = bt_pattern_new (song, "pattern-id", "pattern-name", 8L, machine);
  g_object_get (G_OBJECT (pages), "patterns-page", &pattern_page, NULL);
  bt_main_page_patterns_show_pattern (pattern_page, pattern);
  pattern_editor = gtk_window_get_focus ((GtkWindow *) main_window);
  move_cursor_to (pattern_editor, 0, 2, 0, 0);

  /* act */
  check_send_key (pattern_editor, 0, '1', 0x0a);

  /* assert */
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern, 0, 2), "1");

  /* cleanup */
  g_object_unref (pattern_page);
  g_object_unref (pattern);
  g_object_unref (machine);
  BT_TEST_END;
}

// test entering sparse enum
static void
test_bt_main_page_patterns_enter_sparse_enum (BT_TEST_ARGS)
{
  BT_TEST_START;
  BtMainPagePatterns *pattern_page;
  BtMachine *machine;
  BtPattern *pattern;
  GtkWidget *pattern_editor;

  /* arrange */
  machine =
      BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztard-test-mono-source", 0L, NULL));
  pattern = bt_pattern_new (song, "pattern-id", "pattern-name", 8L, machine);
  g_object_get (G_OBJECT (pages), "patterns-page", &pattern_page, NULL);
  bt_main_page_patterns_show_pattern (pattern_page, pattern);
  pattern_editor = gtk_window_get_focus ((GtkWindow *) main_window);
  move_cursor_to (pattern_editor, 0, 4, 1, 0);

  /* act */
  check_send_key (pattern_editor, 0, '1', 0x0a);

  /* assert */
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern, 0, 4), "1");

  /* cleanup */
  g_object_unref (pattern_page);
  g_object_unref (pattern);
  g_object_unref (machine);
  BT_TEST_END;
}

// test entering sparse enum
static void
test_bt_main_page_patterns_enter_invalid_sparse_enum (BT_TEST_ARGS)
{
  BT_TEST_START;
  BtMainPagePatterns *pattern_page;
  BtMachine *machine;
  BtPattern *pattern;
  GtkWidget *pattern_editor;

  /* arrange */
  machine =
      BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztard-test-mono-source", 0L, NULL));
  pattern = bt_pattern_new (song, "pattern-id", "pattern-name", 8L, machine);
  g_object_get (G_OBJECT (pages), "patterns-page", &pattern_page, NULL);
  bt_main_page_patterns_show_pattern (pattern_page, pattern);
  pattern_editor = gtk_window_get_focus ((GtkWindow *) main_window);
  move_cursor_to (pattern_editor, 0, 4, 0, 0);

  /* act */
  check_send_key (pattern_editor, 0, '1', 0x0a);

  /* assert */
  fail_if (bt_pattern_test_global_event (pattern, 0, 4));

  /* cleanup */
  g_object_unref (pattern_page);
  g_object_unref (pattern);
  g_object_unref (machine);
  BT_TEST_END;
}

// test entering sparse enum
static void
test_bt_main_page_patterns_enter_sparse_enum_in_2_steps (BT_TEST_ARGS)
{
  BT_TEST_START;
  BtMainPagePatterns *pattern_page;
  BtMachine *machine;
  BtPattern *pattern;
  GtkWidget *pattern_editor;

  /* arrange */
  machine =
      BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztard-test-mono-source", 0L, NULL));
  pattern = bt_pattern_new (song, "pattern-id", "pattern-name", 8L, machine);
  g_object_get (G_OBJECT (pages), "patterns-page", &pattern_page, NULL);
  bt_main_page_patterns_show_pattern (pattern_page, pattern);
  pattern_editor = gtk_window_get_focus ((GtkWindow *) main_window);
  move_cursor_to (pattern_editor, 0, 4, 0, 0);

  /* act */
  check_send_key (pattern_editor, 0, '1', 0x0a);
  move_cursor_to (pattern_editor, 0, 4, 1, 0);
  check_send_key (pattern_editor, 0, '4', 0x0d);

  /* assert */
  ck_assert_str_eq_and_free (bt_pattern_get_global_event (pattern, 0, 4), "20");

  /* cleanup */
  g_object_unref (pattern_page);
  g_object_unref (pattern);
  g_object_unref (machine);
  BT_TEST_END;
}


static void
test_bt_main_page_patterns_pattern_voices (BT_TEST_ARGS)
{
  BT_TEST_START;
  BtMainPagePatterns *pattern_page;
  BtMachine *machine;
  BtPattern *pattern;
  GtkWidget *pattern_editor;
  gulong voices;

  /* arrange */
  machine =
      BT_MACHINE (bt_source_machine_new (song, "gen",
          "buzztard-test-poly-source", 1L, NULL));
  pattern = bt_pattern_new (song, "pattern-id", "pattern-name", 8L, machine);
  g_object_get (G_OBJECT (pages), "patterns-page", &pattern_page, NULL);
  bt_main_page_patterns_show_pattern (pattern_page, pattern);
  pattern_editor = gtk_window_get_focus ((GtkWindow *) main_window);

  /* act */
  // change voices
  g_object_set (machine, "voices", 2L, NULL);
  g_object_get (pattern, "voices", &voices, NULL);

  /* assert */
  fail_unless (voices == 2, NULL);
  // send two tab keys to ensure the new voice is visible
  check_send_key (pattern_editor, 0, GDK_Tab, 0);
  check_send_key (pattern_editor, 0, GDK_Tab, 0);
  mark_point ();

  /* cleanup */
  g_object_unref (pattern_page);
  g_object_unref (pattern);
  g_object_unref (machine);
  BT_TEST_END;
}

TCase *
bt_pattern_page_example_case (void)
{
  TCase *tc = tcase_create ("BtPatternPageExamples");

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
  return (tc);
}
