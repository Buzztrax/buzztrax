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

  ck_g_object_final_unref (app);
  bt_edit_teardown ();
}

static void
case_teardown (void)
{
}

//-- tests

static void
test_bt_settings_dialog_create (BT_TEST_ARGS)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  GEnumClass *enum_class = g_type_class_ref (BT_TYPE_SETTINGS_PAGE);
  GEnumValue *enum_value = g_enum_get_value (enum_class, _i);

  GST_INFO ("-- act --");
  GtkWidget *dialog = GTK_WIDGET (bt_settings_dialog_new ());

  GST_INFO ("-- assert --");
  fail_unless (dialog != NULL, NULL);
  g_object_set (G_OBJECT (dialog), "page", _i, NULL);
  gtk_widget_show_all (dialog);
  check_make_widget_screenshot (dialog, enum_value->value_nick);

  GST_INFO ("-- cleanup --");
  gtk_widget_destroy (dialog);
  g_type_class_unref (enum_class);
  BT_TEST_END;
}

TCase *
bt_settings_dialog_example_case (void)
{
  TCase *tc = tcase_create ("BtSettingsDialogExamples");

  tcase_add_loop_test (tc, test_bt_settings_dialog_create, 0,
      BT_SETTINGS_PAGES_COUNT);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
