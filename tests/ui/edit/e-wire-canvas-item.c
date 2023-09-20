/* Buzztrax
 * Copyright (C) 2012 Buzztrax team <buzztrax-devel@buzztrax.org>
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
static BtSetup *setup;
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
  g_object_get (song, "setup", &setup, NULL);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (pages),
      BT_MAIN_PAGES_MACHINES_PAGE);

  flush_main_loop ();
}

static void
test_teardown (void)
{
  g_object_unref (setup);
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

//-- tests

START_TEST (test_bt_wire_canvas_item_create)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMainPageMachines *machines_page;

  g_object_get (pages, "machines-page", &machines_page, NULL);
  bt_main_page_machines_add_source_machine (machines_page, "beep1", "simsyn");
  BtMachine *machine1 = bt_setup_get_machine_by_id (setup, "beep1");
  BtMachine *machine2 = bt_setup_get_machine_by_id (setup, "master");
  BtMachineCanvasItem *item1 =
      bt_machine_canvas_item_new (machines_page, machine1, 100.0, 100.0, 1.0);
  BtMachineCanvasItem *item2 =
      bt_machine_canvas_item_new (machines_page, machine2, 150.0, 150.0, 1.0);
  BtWire *wire = bt_wire_new (song, machine1, machine2, NULL);

  GST_INFO ("-- act --");
  BtWireCanvasItem *item =
      bt_wire_canvas_item_new (machines_page, wire, item1, item2);

  GST_INFO ("-- assert --");
  ck_assert (item != NULL);

  GST_INFO ("-- cleanup --");
  flush_main_loop ();
  gst_object_unref (machine1);
  gst_object_unref (machine2);
  g_object_unref (machines_page);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_wire_canvas_item_show_analyzer)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMainPageMachines *machines_page;
  GtkWidget *dialog;

  g_object_get (pages, "machines-page", &machines_page, NULL);
  bt_main_page_machines_add_source_machine (machines_page, "beep1", "simsyn");
  BtMachine *machine1 = bt_setup_get_machine_by_id (setup, "beep1");
  BtMachine *machine2 = bt_setup_get_machine_by_id (setup, "master");
  BtMachineCanvasItem *item1 =
      bt_machine_canvas_item_new (machines_page, machine1, 100.0, 100.0, 1.0);
  BtMachineCanvasItem *item2 =
      bt_machine_canvas_item_new (machines_page, machine2, 150.0, 150.0, 1.0);
  BtWire *wire = bt_wire_new (song, machine1, machine2, NULL);
  BtWireCanvasItem *item =
      bt_wire_canvas_item_new (machines_page, wire, item1, item2);

  GST_INFO ("-- act --");
  bt_wire_show_analyzer_dialog (wire);

  GST_INFO ("-- assert --");
  g_object_get (item, "analysis-dialog", &dialog, NULL);
  ck_assert (dialog != NULL);

  GST_INFO ("-- cleanup --");
  flush_main_loop ();
  gtk_widget_destroy (dialog);
  gst_object_unref (machine1);
  gst_object_unref (machine2);
  g_object_unref (machines_page);
  BT_TEST_END;
}
END_TEST

TCase *
bt_wire_canvas_item_example_case (void)
{
  TCase *tc = tcase_create ("BtWireCanvasItemExamples");

  tcase_add_test (tc, test_bt_wire_canvas_item_create);
  tcase_add_test (tc, test_bt_wire_canvas_item_show_analyzer);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
