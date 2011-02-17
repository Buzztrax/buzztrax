/* $Id$
 *
 * Buzztard
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

//-- fixtures

static void test_setup(void) {
  bt_edit_setup();
}

static void test_teardown(void) {
  bt_edit_teardown();
}

//-- helper

//-- tests

// view all tabs
BT_START_TEST(test_machine_ref) {
  BtEditApplication *app;
  BtMainWindow *main_window;
  BtMainPages *pages;
  BtMainPageMachines *machines_page;
  BtSong *song;
  BtSetup *setup;
  BtMachine *src_machine;

  app=bt_edit_application_new();
  GST_INFO("back in test app=%p, app->ref_ct=%d",app,G_OBJECT_REF_COUNT(app));
  fail_unless(app != NULL, NULL);

  // create a new song
  bt_edit_application_new_song(app);

  // get the empty song
  g_object_get(app,"song",&song,NULL);
  fail_unless(song != NULL, NULL);
  g_object_get(song,"setup",&setup,NULL);
  g_object_unref(song);

  // get window
  g_object_get(app,"main-window",&main_window,NULL);
  fail_unless(main_window != NULL, NULL);
  g_object_get(G_OBJECT(main_window),"pages",&pages,NULL);
  fail_unless(pages != NULL, NULL);
  g_object_get(G_OBJECT(pages),"machines-page",&machines_page,NULL);
  fail_unless(machines_page != NULL, NULL);
  /* remove some other pages
  // (ev. run for all combinations - if a test using all pages fails?)
  gtk_notebook_remove_page(GTK_NOTEBOOK(pages),BT_MAIN_PAGES_INFO_PAGE);
  gtk_notebook_remove_page(GTK_NOTEBOOK(pages),BT_MAIN_PAGES_WAVES_PAGE);
  gtk_notebook_remove_page(GTK_NOTEBOOK(pages),BT_MAIN_PAGES_SEQUENCE_PAGE);
  gtk_notebook_remove_page(GTK_NOTEBOOK(pages),BT_MAIN_PAGES_PATTERNS_PAGE);
  */
  // show page
  gtk_notebook_set_current_page(GTK_NOTEBOOK(pages),BT_MAIN_PAGES_MACHINES_PAGE);
  g_object_unref(pages);

  // add and get a source machine
  bt_main_page_machines_add_source_machine(machines_page,"beep1","simsyn");
  src_machine=bt_setup_get_machine_by_id(setup,"beep1");
  fail_unless(src_machine != NULL, NULL);
  g_object_unref(setup);

  while(gtk_events_pending()) gtk_main_iteration();

  GST_INFO("machine %p,ref_count=%d has been created",src_machine,G_OBJECT_REF_COUNT(src_machine));

  // remove the machine and check that it is disposed
  bt_main_page_machines_delete_machine(machines_page,src_machine);
  g_object_unref(machines_page);

  while(gtk_events_pending()) gtk_main_iteration();

  g_object_checked_unref(src_machine);

  // close window
  gtk_widget_destroy(GTK_WIDGET(main_window));
  while(gtk_events_pending()) gtk_main_iteration();
  //GST_INFO("mainlevel is %d",gtk_main_level());
  //while(g_main_context_pending(NULL)) g_main_context_iteration(/*context=*/NULL,/*may_block=*/FALSE);

  // free application
  GST_INFO("app->ref_ct=%d",G_OBJECT_REF_COUNT(app));
  g_object_checked_unref(app);

}
BT_END_TEST

TCase *bt_machine_page_example_case(void) {
  TCase *tc = tcase_create("BtMachinePageExamples");

  tcase_add_test(tc,test_machine_ref);
  // we *must* use a checked fixture, as only this runs in the same context
  tcase_add_checked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
