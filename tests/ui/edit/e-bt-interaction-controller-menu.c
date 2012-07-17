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

//-- fixtures

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

//-- helper

//-- tests

// view all tabs
static void test_create_menu (BT_TEST_ARGS)
{
  BT_TEST_START;
  BtEditApplication *app;
  BtMainWindow *main_window;
  GtkWidget *menu;

  app = bt_edit_application_new ();
  GST_INFO ("back in test app=%p, app->ref_ct=%d", app,
      G_OBJECT_REF_COUNT (app));
  fail_unless (app != NULL, NULL);

  // get window
  g_object_get (app, "main-window", &main_window, NULL);
  fail_unless (main_window != NULL, NULL);
  GST_INFO ("main_window->ref_ct=%d", G_OBJECT_REF_COUNT (main_window));

  // make menus
  menu =
      (GtkWidget *)
      bt_interaction_controller_menu_new (BT_INTERACTION_CONTROLLER_RANGE_MENU);
  fail_unless (menu != NULL, NULL);
  gtk_widget_destroy (menu);

  menu =
      (GtkWidget *)
      bt_interaction_controller_menu_new
      (BT_INTERACTION_CONTROLLER_TRIGGER_MENU);
  fail_unless (menu != NULL, NULL);
  gtk_widget_destroy (menu);

  gtk_widget_destroy (GTK_WIDGET (main_window));

  // free application
  GST_INFO ("app->ref_ct=%d", G_OBJECT_REF_COUNT (app));
  g_object_checked_unref (app);

  BT_TEST_END;
}
 TCase * bt_interaction_controller_menu_example_case (void)
{
  TCase *tc = tcase_create ("BtInteractionControllerMenuExamples");

  tcase_add_test (tc, test_create_menu);
  // we *must* use a checked fixture, as only this runs in the same context
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  return (tc);
}
