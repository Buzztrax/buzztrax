/* Buzztrax
 * Copyright (C) 2015 Buzztrax team <buzztrax-devel@buzztrax.org>
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
}

static void
test_teardown (void)
{
  bt_edit_teardown ();
}

static void
case_teardown (void)
{
}

//-- helper

//-- tests
START_TEST (test_bt_main_page_wave_missing_playbin)
{
  BT_TEST_START;
  BtEditApplication *app;
  BtMainWindow *main_window;
  BtMainPages *pages;

  GST_INFO ("-- arrange --");
  check_remove_gst_feature ("playbin");

  GST_INFO ("-- act --");
  app = bt_edit_application_new ();
  bt_edit_application_new_song (app);
  g_object_get (app, "main-window", &main_window, NULL);
  g_object_get (main_window, "pages", &pages, NULL);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (pages),
      BT_MAIN_PAGES_WAVES_PAGE);
  flush_main_loop ();

  GST_INFO ("-- assert --");
  mark_point ();

  GST_INFO ("-- cleanup --");
  g_object_unref (pages);

  gtk_widget_destroy (GTK_WIDGET (main_window));
  flush_main_loop ();

  ck_g_object_final_unref (app);
  BT_TEST_END;
}
END_TEST



TCase *
bt_main_page_waves_test_case (void)
{
  TCase *tc = tcase_create ("BtMainPageWavesTests");

  tcase_add_test (tc, test_bt_main_page_wave_missing_playbin);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
