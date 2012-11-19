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

  g_object_checked_unref (app);
  bt_edit_teardown ();
}

static void
case_teardown (void)
{
}

//-- helper

//-- tests

static void
test_bt_main_page_machines_focus (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */

  /* act */
  GtkWidget *widget = gtk_window_get_focus ((GtkWindow *) main_window);

  /* assert */
  ck_assert_str_eq ("machine-and-wire-editor", gtk_widget_get_name (widget));

  /* cleanup */
  BT_TEST_END;
}

static void
test_bt_main_page_machines_machine_create (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtMainPageMachines *machines_page;
  BtSetup *setup;

  g_object_get (song, "setup", &setup, NULL);
  g_object_get (pages, "machines-page", &machines_page, NULL);

  /* act */
  bt_main_page_machines_add_source_machine (machines_page, "beep1", "simsyn");
  BtMachine *machine = bt_setup_get_machine_by_id (setup, "beep1");

  /* assert */
  fail_unless (machine != NULL, NULL);
  flush_main_loop ();

  /* cleanup */
  gst_object_unref (machine);
  g_object_unref (machines_page);
  g_object_unref (setup);
  BT_TEST_END;
}

static void
test_bt_main_page_machines_machine_ref (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
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

  /* act */
  // remove the machine and check that it is disposed below
  bt_main_page_machines_delete_machine (machines_page, src_machine);
  flush_main_loop ();
  GST_INFO_OBJECT (src_machine, "machine[beep1]: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (src_machine));

  /* assert */
  g_object_checked_unref (src_machine);

  /* cleanup */
  g_object_unref (machines_page);
  g_object_unref (setup);
  BT_TEST_END;
}

// TODO(ensonic): do the same test for a wire, right now adding a wire is not
// exposed as public api though

// load a song and remove a machine
static void
test_bt_main_page_machines_remove_source_machine (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSong *song;
  BtSetup *setup;
  BtMainPageMachines *machines_page;

  bt_edit_application_load_song (app,
      check_get_test_song_path ("test-simple1.xml"));
  g_object_get (app, "song", &song, NULL);
  g_object_get (song, "setup", &setup, NULL);
  g_object_get (pages, "machines-page", &machines_page, NULL);
  BtMachine *machine = bt_setup_get_machine_by_id (setup, "sine1");
  flush_main_loop ();
  // core: 1 x pipeline, 1 x setup, 1 x wire, 1 x sequence
  // edit: 1 x machine-canvas-item, 1 x main-page-patterns, 1 x main-page-sequence
  // + 1 temp ref in bt_main_page_machines_delete_machine
  GST_INFO ("machine[sine1]: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));

  /* act */
  bt_main_page_machines_delete_machine (machines_page, machine);
  flush_main_loop ();
  GST_INFO ("machine[sine1]: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));

  /* assert */
  // !!! the wire is not released!
  // core: 1 x pipeline, 1 x setup
  // edit: 1 x wire-canvas-item
  g_object_checked_unref (machine);

  /* cleanup */
  g_object_unref (machines_page);
  g_object_unref (setup);
  g_object_unref (song);
  BT_TEST_END;
}

// load a song and remove machines
static void
test_bt_main_page_machines_remove_processor_machine (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSong *song;
  BtSetup *setup;
  BtMainPageMachines *machines_page;

  bt_edit_application_load_song (app,
      check_get_test_song_path ("test-simple2.xml"));
  g_object_get (app, "song", &song, NULL);
  g_object_get (song, "setup", &setup, NULL);
  g_object_get (pages, "machines-page", &machines_page, NULL);
  BtMachine *machine = bt_setup_get_machine_by_id (setup, "amp1");
  flush_main_loop ();
  GST_INFO ("machine[amp1]: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));

  /* act */
  bt_main_page_machines_delete_machine (machines_page, machine);
  flush_main_loop ();
  GST_INFO ("machine[amp1]: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine));

  /* assert */
  g_object_checked_unref (machine);

  /* cleanup */
  g_object_unref (machines_page);
  g_object_unref (setup);
  g_object_unref (song);
  BT_TEST_END;
}

static void
test_bt_main_page_machines_remove_wire (BT_TEST_ARGS)
{
  BT_TEST_START;
  /* arrange */
  BtSong *song;
  BtSetup *setup;

  bt_edit_application_load_song (app,
      check_get_test_song_path ("test-simple2.xml"));
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

  /* act */
  bt_setup_remove_wire (setup, wire);
  flush_main_loop ();
  GST_INFO ("wire[sine1->amp1]: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (wire));

  /* assert */
  g_object_checked_unref (wire);

  /* cleanup */
  gst_object_unref (machine1);
  gst_object_unref (machine2);
  g_object_unref (setup);
  g_object_unref (song);
  BT_TEST_END;
}

// load a song and remove machines & wires
// FIXME(ensonic): this test needs work, doing too many things at once
static void
test_bt_main_page_machines_edit (BT_TEST_ARGS)
{
  BT_TEST_START;
  BtMainPages *pages;
  BtSong *song;
  BtSetup *setup;

  // sine1 ! amp1 ! master + sine2 ! amp1
  //bt_edit_application_load_song(app, check_get_test_song_path("test-simple3.xml"));
  // sine1 ! amp1 ! master
  bt_edit_application_load_song (app,
      check_get_test_song_path ("test-simple2.xml"));
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
  g_object_checked_unref (wire);

  // remove a source
  GST_INFO ("machine[sine1]: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine1));
  bt_setup_remove_machine (setup, machine1);
  flush_main_loop ();
  GST_INFO ("machine[sine1]: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine1));
  // ref count should be 1 now
  g_object_checked_unref (machine1);

  // remove an effect
  GST_INFO ("machine[amp1]: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine2));
  bt_setup_remove_machine (setup, machine2);
  flush_main_loop ();
  GST_INFO ("machine[amp1]: %" G_OBJECT_REF_COUNT_FMT,
      G_OBJECT_LOG_REF_COUNT (machine2));
  // ref count should be 1 now
  g_object_checked_unref (machine2);

  /* cleanup */
  g_object_unref (setup);
  g_object_unref (song);
  BT_TEST_END;
}

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
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  tcase_add_unchecked_fixture (tc, case_setup, case_teardown);
  return (tc);
}
