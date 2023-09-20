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
      BT_MAIN_PAGES_MACHINES_PAGE);

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

//-- tests

START_TEST (test_bt_main_page_machines_focus)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");

  GST_INFO ("-- act --");
  GtkWidget *widget = gtk_window_get_focus ((GtkWindow *) main_window);

  GST_INFO ("-- assert --");
  ck_assert_str_eq ("machine-and-wire-editor", gtk_widget_get_name (widget));

  GST_INFO ("-- cleanup --");
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_main_page_machines_machine_create)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMainPageMachines *machines_page;
  BtSetup *setup;

  g_object_get (song, "setup", &setup, NULL);
  g_object_get (pages, "machines-page", &machines_page, NULL);

  GST_INFO ("-- act --");
  bt_main_page_machines_add_source_machine (machines_page, "beep1", "simsyn");
  BtMachine *machine = bt_setup_get_machine_by_id (setup, "beep1");

  GST_INFO ("-- assert --");
  ck_assert (machine != NULL);
  flush_main_loop ();

  GST_INFO ("-- cleanup --");
  gst_object_unref (machine);
  g_object_unref (machines_page);
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_main_page_machines_machine_ref)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMainPageMachines *machines_page;
  BtSetup *setup;
  BtMachine *src_machine;

  g_object_get (song, "setup", &setup, NULL);
  g_object_get (pages, "machines-page", &machines_page, NULL);
  /* remove some pages to narrow down ref leaks
   * BT_EDIT_UI_CFG="no-pattern-page,no-sequence-page" ...
   */

  // add and get a source machine
  bt_main_page_machines_add_source_machine (machines_page, "beep1", "simsyn");
  src_machine = bt_setup_get_machine_by_id (setup, "beep1");
  flush_main_loop ();
  GST_INFO_OBJECT (src_machine, "machine[beep1]: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (src_machine));

  GST_INFO ("-- act --");
  // remove the machine and check that it is disposed below
  bt_main_page_machines_delete_machine (machines_page, src_machine);
  flush_main_loop ();
  GST_INFO_OBJECT (src_machine, "machine[beep1]: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (src_machine));

  GST_INFO ("-- assert --");
  ck_g_object_final_unref (src_machine);

  GST_INFO ("-- cleanup --");
  g_object_unref (machines_page);
  g_object_unref (setup);
  BT_TEST_END;
}
END_TEST

// TODO(ensonic): do the same test for a wire, right now adding a wire is not
// exposed as public api though

// load a song and remove a machine
START_TEST (test_bt_main_page_machines_remove_source_machine)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSong *song;
  BtSetup *setup;
  BtMainPageMachines *machines_page;

  bt_edit_application_load_song (app,
      check_get_test_song_path ("test-simple1.xml"), NULL);
  g_object_get (app, "song", &song, NULL);
  g_object_get (song, "setup", &setup, NULL);
  g_object_get (pages, "machines-page", &machines_page, NULL);
  BtMachine *machine = bt_setup_get_machine_by_id (setup, "sine1");
  flush_main_loop ();
  // core: 1 x pipeline, 1 x setup, 1 x wire, 1 x sequence
  // edit: 1 x machine-canvas-item, 1 x main-page-patterns, 1 x main-page-sequence
  GST_INFO ("machine[sine1]: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));

  GST_INFO ("-- act --");
  bt_main_page_machines_delete_machine (machines_page, machine);
  flush_main_loop ();
  GST_INFO ("machine[sine1]: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));

  gchar buf[200];
  // counts for 2 refs, pipeline + setup
  //"bt_setup_remove_machine:<sine1,0xc0cc30,ref_ct=3> unparented machine: 0xc0cc30,ref_ct=3"
  sprintf (buf,
      ":bt_machine_canvas_item_dispose: release the machine %p,", machine);
  check_log_contains (buf, "no unref from BtMachineCanvasItem");
  sprintf (buf,
      ":update_after_track_changed: unref old cur-machine %p,", machine);
  check_log_contains (buf, "no unref from BtMainPageSequence");

  // grep "0xc0cc30" /tmp/bt_edit/e-main-page-machines/test_bt_main_page_machines_remove_source_machine.log | grep "wire.c
  // wire-canvas-item is getting disposed, but something still has a ref on the wire

  GST_INFO ("-- assert --");
  ck_g_object_final_unref (machine);

  GST_INFO ("-- cleanup --");
  g_object_unref (machines_page);
  g_object_unref (setup);
  g_object_unref (song);
  BT_TEST_END;
}
END_TEST

// load a song and remove machines
START_TEST (test_bt_main_page_machines_remove_processor_machine)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSong *song;
  BtSetup *setup;
  BtMainPageMachines *machines_page;

  bt_edit_application_load_song (app,
      check_get_test_song_path ("test-simple2.xml"), NULL);
  g_object_get (app, "song", &song, NULL);
  g_object_get (song, "setup", &setup, NULL);
  g_object_get (pages, "machines-page", &machines_page, NULL);
  BtMachine *machine = bt_setup_get_machine_by_id (setup, "amp1");
  flush_main_loop ();
  GST_INFO ("machine[amp1]: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));

  GST_INFO ("-- act --");
  bt_main_page_machines_delete_machine (machines_page, machine);
  flush_main_loop ();
  GST_INFO ("machine[amp1]: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));
  flush_main_loop ();

  GST_INFO ("-- assert --");
  ck_g_object_final_unref (machine);

  GST_INFO ("-- cleanup --");
  g_object_unref (machines_page);
  g_object_unref (setup);
  g_object_unref (song);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_main_page_machines_remove_wire)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtSong *song;
  BtSetup *setup;

  bt_edit_application_load_song (app,
      check_get_test_song_path ("test-simple2.xml"), NULL);
  g_object_get (app, "song", &song, NULL);
  g_object_get (song, "setup", &setup, NULL);
  BtMachine *machine1 = bt_setup_get_machine_by_id (setup, "sine1");
  GST_INFO ("machine[sine1]: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine1));
  BtMachine *machine2 = bt_setup_get_machine_by_id (setup, "amp1");
  GST_INFO ("machine[amp1]: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine2));
  BtWire *wire = bt_setup_get_wire_by_machines (setup, machine1, machine2);
  GST_INFO ("wire[sine1->amp1]: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (wire));

  GST_INFO ("-- act --");
  bt_setup_remove_wire (setup, wire);
  flush_main_loop ();
  GST_INFO ("wire[sine1->amp1]: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (wire));

  GST_INFO ("-- assert --");
  ck_g_object_final_unref (wire);

  GST_INFO ("-- cleanup --");
  gst_object_unref (machine1);
  gst_object_unref (machine2);
  g_object_unref (setup);
  g_object_unref (song);
  BT_TEST_END;
}
END_TEST

// load a song and remove machines & wires
// FIXME(ensonic): this test needs work, doing too many things at once
START_TEST (test_bt_main_page_machines_edit)
{
  BT_TEST_START;
  BtMainPages *pages;
  BtSong *song;
  BtSetup *setup;

  // sine1 ! amp1 ! master + sine2 ! amp1
  //bt_edit_application_load_song(app, check_get_test_song_path("test-simple3.xml"));
  // sine1 ! amp1 ! master
  bt_edit_application_load_song (app,
      check_get_test_song_path ("test-simple2.xml"), NULL);
  g_object_get (app, "song", &song, NULL);
  g_object_get (song, "setup", &setup, NULL);

  g_object_get (G_OBJECT (main_window), "pages", &pages, NULL);
  // remove some other pages
  // (ev. run for all combinations - if a test using all pages fails?)
  gtk_notebook_remove_page (GTK_NOTEBOOK (pages), BT_MAIN_PAGES_INFO_PAGE);
  gtk_notebook_remove_page (GTK_NOTEBOOK (pages), BT_MAIN_PAGES_WAVES_PAGE);
  // FIXME(ensonic): having the sequence page enabled/disabled makes a difference
  //  between ref-leak and too many unrefs
  //gtk_notebook_remove_page(GTK_NOTEBOOK(pages),BT_MAIN_PAGES_SEQUENCE_PAGE);
  //gtk_notebook_remove_page(GTK_NOTEBOOK(pages),BT_MAIN_PAGES_PATTERNS_PAGE);
  g_object_unref (pages);

  // remove wire
  BtMachine *machine1 = bt_setup_get_machine_by_id (setup, "sine1");
  GST_INFO ("machine[sine1]: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine1));
  BtMachine *machine2 = bt_setup_get_machine_by_id (setup, "amp1");
  GST_INFO ("machine[amp1]: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine2));
  BtWire *wire = bt_setup_get_wire_by_machines (setup, machine1, machine2);
  GST_INFO ("wire[sine1->amp1]: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (wire));
  bt_setup_remove_wire (setup, wire);
  flush_main_loop ();
  GST_INFO ("wire[sine1->amp1]: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (wire));
  // ref count should be 1 now
  ck_g_object_final_unref (wire);

  // remove a source
  GST_INFO ("machine[sine1]: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine1));
  bt_setup_remove_machine (setup, machine1);
  flush_main_loop ();
  GST_INFO ("machine[sine1]: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine1));
  // ref count should be 1 now
  ck_g_object_final_unref (machine1);

  // remove an effect
  GST_INFO ("machine[amp1]: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine2));
  bt_setup_remove_machine (setup, machine2);
  flush_main_loop ();
  GST_INFO ("machine[amp1]: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine2));
  // ref count should be 1 now
  ck_g_object_final_unref (machine2);

  GST_INFO ("-- cleanup --");
  g_object_unref (setup);
  g_object_unref (song);
  BT_TEST_END;
}
END_TEST

START_TEST (test_bt_main_page_machines_convert_coordinates)
{
  BT_TEST_START;
  GST_INFO ("-- arrange --");
  BtMainPageMachines *machines_page;
  gdouble xr = 0.5, yr = -0.5;
  gdouble xc, yc, xr2, yr2;

  g_object_get (pages, "machines-page", &machines_page, NULL);
  flush_main_loop ();

  GST_INFO ("-- act --");
  bt_main_page_machines_relative_coords_to_canvas (machines_page, xr, yr, &xc,
      &yc);
  bt_main_page_machines_canvas_coords_to_relative (machines_page, xc, yc, &xr2,
      &yr2);

  GST_INFO ("-- assert --");
  ck_assert_float_eq (xr, xr2);
  ck_assert_float_eq (yr, yr2);

  GST_INFO ("-- cleanup --");
  g_object_unref (machines_page);
  BT_TEST_END;
}
END_TEST

TCase *
bt_main_page_machines_example_case (void)
{
  TCase *tc = tcase_create ("BtMainPageMachinesExamples");

  tcase_add_test (tc, test_bt_main_page_machines_focus);
  tcase_add_test (tc, test_bt_main_page_machines_machine_create);
  tcase_add_test (tc, test_bt_main_page_machines_machine_ref);
  tcase_add_test (tc, test_bt_main_page_machines_remove_source_machine);
  tcase_add_test (tc, test_bt_main_page_machines_remove_processor_machine);
  tcase_add_test (tc, test_bt_main_page_machines_remove_wire);
  tcase_add_test (tc, test_bt_main_page_machines_edit);
  tcase_add_test (tc, test_bt_main_page_machines_convert_coordinates);
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return tc;
}
