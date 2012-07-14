/* Buzztard
 * Copyright (C) 2011 Buzztard team <buzztard-devel@lists.sf.net>
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

// activate tracks
BT_START_TEST (test_active_machine)
{
  BtEditApplication *app;
  BtMainWindow *main_window;
  BtMainPages *pages;
  BtMainPageSequence *sequence_page;
  BtSong *song;
  BtSetup *setup;
  BtMachine *src_machine1;
  BtMachine *src_machine2;

  app = bt_edit_application_new ();
  GST_INFO ("back in test app=%p, app->ref_ct=%d", app,
      G_OBJECT_REF_COUNT (app));
  fail_unless (app != NULL, NULL);

  // load a song
  bt_edit_application_load_song (app,
      check_get_test_song_path ("test-simple3.xml"));
  g_object_get (app, "song", &song, NULL);
  fail_unless (song != NULL, NULL);
  g_object_get (song, "setup", &setup, NULL);
  g_object_unref (song);
  GST_INFO ("song loaded");

  // get window and page
  g_object_get (app, "main-window", &main_window, NULL);
  fail_unless (main_window != NULL, NULL);
  g_object_get (G_OBJECT (main_window), "pages", &pages, NULL);
  g_object_get (G_OBJECT (pages), "sequence-page", &sequence_page, NULL);

  src_machine1 = bt_setup_get_machine_by_id (setup, "sine1");
  fail_unless (src_machine1 != NULL, NULL);
  GST_INFO ("sine1 %p,ref_count=%d", src_machine1,
      G_OBJECT_REF_COUNT (src_machine1));
  src_machine2 = bt_setup_get_machine_by_id (setup, "sine2");
  fail_unless (src_machine2 != NULL, NULL);
  GST_INFO ("sine2 %p,ref_count=%d", src_machine2,
      G_OBJECT_REF_COUNT (src_machine2));
  g_object_unref (setup);

  // show page
  gtk_notebook_set_current_page (GTK_NOTEBOOK (pages),
      BT_MAIN_PAGES_SEQUENCE_PAGE);
  while (gtk_events_pending ())
    gtk_main_iteration ();

  // need to focus the sequence page to make the key events work
  gtk_widget_grab_focus ((GtkWidget *) sequence_page);
  while (gtk_events_pending ())
    gtk_main_iteration ();

  GST_INFO ("sine1 %p,ref_count=%d", src_machine1,
      G_OBJECT_REF_COUNT (src_machine1));
  GST_INFO ("sine2 %p,ref_count=%d", src_machine2,
      G_OBJECT_REF_COUNT (src_machine2));

  // make screenshot
  check_make_widget_screenshot ((GtkWidget *) sequence_page, "0");

  // emit key presses to go though the tracks
  // send a '->' key
  check_send_key ((GtkWidget *) sequence_page, GDK_Right, 0);

  // make screenshot
  check_make_widget_screenshot ((GtkWidget *) sequence_page, "1");
  GST_INFO ("sine1 %p,ref_count=%d", src_machine1,
      G_OBJECT_REF_COUNT (src_machine1));
  GST_INFO ("sine2 %p,ref_count=%d", src_machine2,
      G_OBJECT_REF_COUNT (src_machine2));

  // send a '<-' key-press twice
  check_send_key ((GtkWidget *) sequence_page, GDK_Left, 0);
  check_send_key ((GtkWidget *) sequence_page, GDK_Left, 0);

  // make screenshot
  check_make_widget_screenshot ((GtkWidget *) sequence_page, "2");
  GST_INFO ("sine1 %p,ref_count=%d", src_machine1,
      G_OBJECT_REF_COUNT (src_machine1));
  GST_INFO ("sine2 %p,ref_count=%d", src_machine2,
      G_OBJECT_REF_COUNT (src_machine2));

  g_object_unref (src_machine1);
  g_object_unref (src_machine2);
  g_object_unref (sequence_page);
  g_object_unref (pages);

  // close window
  gtk_widget_destroy (GTK_WIDGET (main_window));
  while (gtk_events_pending ())
    gtk_main_iteration ();
  //GST_INFO("mainlevel is %d",gtk_main_level());
  //while(g_main_context_pending(NULL)) g_main_context_iteration(/*context=*/NULL,/*may_block=*/FALSE);

  // free application
  GST_INFO ("app->ref_ct=%d", G_OBJECT_REF_COUNT (app));
  g_object_checked_unref (app);

}

BT_END_TEST TCase * bt_sequence_page_example_case (void)
{
  TCase *tc = tcase_create ("BtSequencePageExamples");

  tcase_add_test (tc, test_active_machine);
  // we *must* use a checked fixture, as only this runs in the same context
  tcase_add_checked_fixture (tc, test_setup, test_teardown);
  return (tc);
}
