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

static void
test_bt_main_page_machines_machine_ref (BT_TEST_ARGS)
{
  BT_TEST_START;
  BtMainPageMachines *machines_page;
  BtSetup *setup;
  BtMachine *src_machine;

  /* arrange */
  g_object_get (song, "setup", &setup, NULL);

  g_object_get (G_OBJECT (pages), "machines-page", &machines_page, NULL);
  /* remove some other pages
     // (ev. run for all combinations - if a test using all pages fails?)
     gtk_notebook_remove_page(GTK_NOTEBOOK(pages),BT_MAIN_PAGES_INFO_PAGE);
     gtk_notebook_remove_page(GTK_NOTEBOOK(pages),BT_MAIN_PAGES_WAVES_PAGE);
     gtk_notebook_remove_page(GTK_NOTEBOOK(pages),BT_MAIN_PAGES_SEQUENCE_PAGE);
     gtk_notebook_remove_page(GTK_NOTEBOOK(pages),BT_MAIN_PAGES_PATTERNS_PAGE);
   */
  // show page
  gtk_notebook_set_current_page (GTK_NOTEBOOK (pages),
      BT_MAIN_PAGES_MACHINES_PAGE);

  // add and get a source machine
  bt_main_page_machines_add_source_machine (machines_page, "beep1", "simsyn");
  src_machine = bt_setup_get_machine_by_id (setup, "beep1");
  fail_unless (src_machine != NULL, NULL);

  while (gtk_events_pending ())
    gtk_main_iteration ();

  GST_INFO ("machine %p,ref_count=%d has been created", src_machine,
      G_OBJECT_REF_COUNT (src_machine));

  // remove the machine and check that it is disposed below
  bt_main_page_machines_delete_machine (machines_page, src_machine);
  g_object_unref (machines_page);

  while (gtk_events_pending ())
    gtk_main_iteration ();

  /* cleanup */
  g_object_checked_unref (src_machine);
  g_object_unref (setup);
  BT_TEST_END;
}


TCase *
bt_machine_page_example_case (void)
{
  TCase *tc = tcase_create ("BtMachinePageExamples");

  tcase_add_test (tc, test_bt_main_page_machines_machine_ref);
  // we *must* use a checked fixture, as only this runs in the same context
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  return (tc);
}
