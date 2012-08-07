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
static BtSong *song;
static BtMainWindow *main_window;
static BtMainPages *pages;

//-- fixtures

static void
test_setup (void)
{
  bt_edit_setup ();
  app = bt_edit_application_new ();
  bt_edit_application_new_song (app);
  g_object_get (app, "song", &song, "main-window", &main_window, NULL);
  g_object_get (G_OBJECT (main_window), "pages", &pages, NULL);

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

//-- helper

//-- tests

// show pattern page with empty pattern and emit key-presses
static void
test_bt_main_page_patterns_key_press_in_empty_pattern (BT_TEST_ARGS)
{
  BT_TEST_START;
  BtMainPagePatterns *pattern_page;
  BtMachine *machine;

  /* arrange */
  machine =
      BT_MACHINE (bt_source_machine_new (song, "gen", "fakesrc", 0, NULL));
  g_object_get (G_OBJECT (pages), "patterns-page", &pattern_page, NULL);
  bt_main_page_patterns_show_machine (pattern_page, machine);

  /* act */
  check_send_key ((GtkWidget *) pattern_page, 0, '.', 0x3c);
  check_send_key ((GtkWidget *) pattern_page, 0, '0', 0x13);

  /* assert */
  mark_point ();

  /* cleanup */
  g_object_unref (pattern_page);
  g_object_unref (machine);
  BT_TEST_END;
}

// show pattern page with empty pattern and emit mouse clicks
static void
test_bt_main_page_patterns_mouse_click_in_empty_pattern (BT_TEST_ARGS)
{
  BT_TEST_START;
  BtMainPagePatterns *pattern_page;
  BtMachine *machine;
  GtkWidget *widget;

  /* arrange */
  machine =
      BT_MACHINE (bt_source_machine_new (song, "gen", "fakesrc", 0, NULL));
  g_object_get (G_OBJECT (pages), "patterns-page", &pattern_page, NULL);
  bt_main_page_patterns_show_machine (pattern_page, machine);
  widget = gtk_window_get_focus ((GtkWindow *) main_window);

  /* act */
  check_send_click (widget, 1, 10.0, 100.0);

  /* assert */
  mark_point ();

  /* cleanup */
  g_object_unref (pattern_page);
  g_object_unref (machine);
  BT_TEST_END;
}

TCase *
bt_pattern_page_test_case (void)
{
  TCase *tc = tcase_create ("BtPatternPageTests");

  tcase_add_test (tc, test_bt_main_page_patterns_key_press_in_empty_pattern);
  tcase_add_test (tc, test_bt_main_page_patterns_mouse_click_in_empty_pattern);
  // we *must* use a checked fixture, as only this runs in the same context
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  return (tc);
}
