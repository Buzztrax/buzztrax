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
static BtMainWindow *main_window;

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
  g_object_get (app, "main-window", &main_window, NULL);

  while (gtk_events_pending ())
    gtk_main_iteration ();
}

static void
test_teardown (void)
{
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

//-- tests

static void
test_bt_render_dialog_create (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  GtkWidget *dialog;

  bt_edit_application_new_song (app);

  /* act */
  dialog = GTK_WIDGET (bt_render_dialog_new ());

  /* assert */
  fail_unless (dialog != NULL, NULL);
  gtk_widget_show_all (dialog);
  check_make_widget_screenshot (GTK_WIDGET (dialog), NULL);

  /* cleanup */
  gtk_widget_destroy (dialog);
  BT_TEST_END;
}

TCase *
bt_render_dialog_example_case (void)
{
  TCase *tc = tcase_create ("BtRenderDialogExamples");

  tcase_add_test (tc, test_bt_render_dialog_create);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
