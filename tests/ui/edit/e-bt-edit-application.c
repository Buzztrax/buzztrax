/* $Id$
 *
 * Buzztard
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

static void test_setup(void) {
  bt_edit_setup();
}

static void test_teardown(void) {
  bt_edit_teardown();
}

//-- helper

/*
static gboolean timeout(gpointer data) {
  GST_INFO("shutting down the gui");
  gtk_main_quit();
  //gtk_widget_destroy(GTK_WIDGET(data));
  return FALSE;
}
*/

//-- tests

// create app and then unconditionally destroy window
BT_START_TEST(test_create_app) {
  BtEditApplication *app;
  BtMainWindow *main_window;

  app=bt_edit_application_new();
  GST_INFO("back in test app=%p, app->ref_ct=%d",app,G_OBJECT(app)->ref_count);
  fail_unless(app != NULL, NULL);

  // get window
  g_object_get(app,"main-window",&main_window,NULL);
  fail_unless(main_window != NULL, NULL);
  GST_INFO("main_window->ref_ct=%d",G_OBJECT(main_window)->ref_count);

  // make screenshot
  check_make_widget_screenshot(GTK_WIDGET(main_window),NULL);

  // close window
  GST_INFO("main_window->ref_ct=%d",G_OBJECT(main_window)->ref_count);
  // needs a main-loop (version 1,2)
  gtk_widget_destroy(GTK_WIDGET(main_window));
  // version 1
  while(gtk_events_pending()) gtk_main_iteration();
  /*
  GST_INFO("mainlevel is %d",gtk_main_level());
  while(g_main_context_pending(NULL)) g_main_context_iteration(NULL,FALSE);
  // after this loop the window should be gone

  // version 2 (makes the window visible :( )
  g_timeout_add(2000,timeout,main_window);
  gtk_main();

  // version 3 (does not work)
  g_object_checked_unref(main_window);
  */

  // free application
  GST_INFO("app->ref_ct=%d",G_OBJECT(app)->ref_count);
  g_object_checked_unref(app);
}
BT_END_TEST

// create a new song
/* fails with gtk-2.4
 * -> when destroying menus one gets
 * (bt-edit:22237): Gtk-WARNING **: mnemonic "g" wasn't removed for widget (0x856cd40)
 */
BT_START_TEST(test_new1) {
  BtEditApplication *app;
  BtMainWindow *main_window;
  BtSong *song;
  gboolean unsaved;

  app=bt_edit_application_new();
  GST_INFO("back in test app=%p, app->ref_ct=%d",app,G_OBJECT(app)->ref_count);
  fail_unless(app != NULL, NULL);

  // create a new song
  bt_edit_application_new_song(app);
  g_object_get(app,"song",&song,NULL);
  fail_unless(song != NULL, NULL);
  // song should be unchanged
  g_object_get(song,"unsaved",&unsaved,NULL);
  fail_unless(unsaved == FALSE, NULL);
  g_object_unref(song);
  GST_INFO("song created");

  // get window
  g_object_get(app,"main-window",&main_window,NULL);
  fail_unless(main_window != NULL, NULL);

  // close window
  gtk_widget_destroy(GTK_WIDGET(main_window));
  while(gtk_events_pending()) gtk_main_iteration();
  //GST_INFO("mainlevel is %d",gtk_main_level());
  //while(g_main_context_pending(NULL)) g_main_context_iteration(/*context=*/NULL,/*may_block=*/FALSE);

  // free application
  GST_INFO("app->ref_ct=%d",G_OBJECT(app)->ref_count);
  g_object_checked_unref(app);
}
BT_END_TEST

// load a song
BT_START_TEST(test_load1) {
  BtEditApplication *app;
  BtMainWindow *main_window;
  BtSong *song;
  gboolean unsaved;

  app=bt_edit_application_new();
  GST_INFO("back in test app=%p, app->ref_ct=%d",app,G_OBJECT(app)->ref_count);
  fail_unless(app != NULL, NULL);

  bt_edit_application_load_song(app, check_get_test_song_path("melo3.xml"));
  g_object_get(app,"song",&song,NULL);
  fail_unless(song != NULL, NULL);
  // song should be unchanged
  g_object_get(song,"unsaved",&unsaved,NULL);
  fail_unless(unsaved == FALSE, NULL);
  g_object_unref(song);
  GST_INFO("song loaded");

  // get window
  g_object_get(app,"main-window",&main_window,NULL);
  fail_unless(main_window != NULL, NULL);

  // make screenshot
  check_make_widget_screenshot(GTK_WIDGET(main_window),"song");

  // close window
  gtk_widget_destroy(GTK_WIDGET(main_window));
  while(gtk_events_pending()) gtk_main_iteration();
  //GST_INFO("mainlevel is %d",gtk_main_level());
  //while(g_main_context_pending(NULL)) g_main_context_iteration(/*context=*/NULL,/*may_block=*/FALSE);

  // free application
  GST_INFO("app->ref_ct=%d",G_OBJECT(app)->ref_count);
  g_object_checked_unref(app);

}
BT_END_TEST

// load a song, free it, load another
BT_START_TEST(test_load2) {
  BtEditApplication *app;
  BtMainWindow *main_window;
  BtSong *song1,*song2;
  gboolean unsaved;

  app=bt_edit_application_new();
  GST_INFO("back in test app=%p, app->ref_ct=%d",app,G_OBJECT(app)->ref_count);
  fail_unless(app != NULL, NULL);

  // load first song
  bt_edit_application_load_song(app, check_get_test_song_path("melo1.xml"));
  g_object_get(app,"song",&song1,NULL);
  fail_unless(song1 != NULL, NULL);
  // song should be unchanged
  g_object_get(song1,"unsaved",&unsaved,NULL);
  fail_unless(unsaved == FALSE, NULL);
  g_object_unref(song1);
  GST_INFO("song 1 loaded");

  // load second song
  bt_edit_application_load_song(app, check_get_test_song_path("melo3.xml"));
  g_object_get(app,"song",&song2,NULL);
  fail_unless(song2 != NULL, NULL);
  fail_unless(song2 != song1, NULL);
  // song should be unchanged
  g_object_get(song2,"unsaved",&unsaved,NULL);
  fail_unless(unsaved == FALSE, NULL);
  g_object_unref(song2);
  GST_INFO("song 2 loaded");

  // get window
  g_object_get(app,"main-window",&main_window,NULL);
  fail_unless(main_window != NULL, NULL);

  // close window
  gtk_widget_destroy(GTK_WIDGET(main_window));
  while(gtk_events_pending()) gtk_main_iteration();

  // free application
  GST_INFO("app->ref_ct=%d",G_OBJECT(app)->ref_count);
  g_object_checked_unref(app);

}
BT_END_TEST

// load a song, free it, load another
BT_START_TEST(test_load3) {
  BtEditApplication *app;
  BtMainWindow *main_window;
  BtSong *song;
  GstElement *bin;
  gboolean unsaved;
  guint num;

  app=bt_edit_application_new();
  GST_INFO("back in test app=%p, app->ref_ct=%d",app,G_OBJECT(app)->ref_count);
  fail_unless(app != NULL, NULL);
  g_object_get(app,"bin",&bin,NULL);
  GST_INFO("song.elements=%d",GST_BIN_NUMCHILDREN(bin));

  // load for first time
  bt_edit_application_load_song(app, check_get_test_song_path("melo3.xml"));
  g_object_get(app,"song",&song,NULL);
  fail_unless(song != NULL, NULL);
  // song should be unchanged
  g_object_get(song,"unsaved",&unsaved,NULL);
  fail_unless(unsaved == FALSE, NULL);
  g_object_unref(song);
  GST_INFO("song loaded");
  num=GST_BIN_NUMCHILDREN(bin);
  GST_INFO("song.elements=%d",num);

  // load song for second time
  bt_edit_application_load_song(app, check_get_test_song_path("melo3.xml"));
  g_object_get(app,"song",&song,NULL);
  fail_unless(song != NULL, NULL);
  // song should be unchanged
  g_object_get(song,"unsaved",&unsaved,NULL);
  fail_unless(unsaved == FALSE, NULL);
  g_object_unref(song);
  GST_INFO("song loaded again");
  GST_INFO("song.elements=%d",GST_BIN_NUMCHILDREN(bin));
  fail_unless(num == GST_BIN_NUMCHILDREN(bin), NULL);

  // get window
  g_object_get(app,"main-window",&main_window,NULL);
  fail_unless(main_window != NULL, NULL);

  // close window
  gtk_widget_destroy(GTK_WIDGET(main_window));
  while(gtk_events_pending()) gtk_main_iteration();

  // free application
  gst_object_unref(bin);
  GST_INFO("app->ref_ct=%d",G_OBJECT(app)->ref_count);
  g_object_checked_unref(app);

}
BT_END_TEST

// load a song and play it
BT_START_TEST(test_load_and_play1) {
  BtEditApplication *app;
  BtMainWindow *main_window;
  BtSong *song;
  gboolean unsaved;
  gboolean playing;

  app=bt_edit_application_new();
  GST_INFO("back in test app=%p, app->ref_ct=%d",app,G_OBJECT(app)->ref_count);
  fail_unless(app != NULL, NULL);

  bt_edit_application_load_song(app, check_get_test_song_path("test-simple1.xml"));
  g_object_get(app,"song",&song,NULL);
  fail_unless(song != NULL, NULL);
  // song should be unchanged
  g_object_get(song,"unsaved",&unsaved,NULL);
  fail_unless(unsaved == FALSE, NULL);
  GST_INFO("song loaded");

  // get window
  g_object_get(app,"main-window",&main_window,NULL);
  fail_unless(main_window != NULL, NULL);

  playing=bt_song_play(song);
  fail_unless(playing == TRUE, NULL);

  while(gtk_events_pending()) gtk_main_iteration();
  bt_song_stop(song);

  // close window
  gtk_widget_destroy(GTK_WIDGET(main_window));
  while(gtk_events_pending()) gtk_main_iteration();
  //GST_INFO("mainlevel is %d",gtk_main_level());
  //while(g_main_context_pending(NULL)) g_main_context_iteration(/*context=*/NULL,/*may_block=*/FALSE);

  g_object_unref(song);
  // free application
  GST_INFO("app->ref_ct=%d",G_OBJECT(app)->ref_count);
  g_object_checked_unref(app);

}
BT_END_TEST

// load a song, free it, load another song and play it
BT_START_TEST(test_load_and_play2) {
  BtEditApplication *app;
  BtMainWindow *main_window;
  BtSong *song1,*song2;
  gboolean unsaved;
  gboolean playing;

  app=bt_edit_application_new();
  GST_INFO("back in test app=%p, app->ref_ct=%d",app,G_OBJECT(app)->ref_count);
  fail_unless(app != NULL, NULL);

  // load first song
  bt_edit_application_load_song(app, check_get_test_song_path("melo1.xml"));
  g_object_get(app,"song",&song1,NULL);
  fail_unless(song1 != NULL, NULL);
  // song should be unchanged
  g_object_get(song1,"unsaved",&unsaved,NULL);
  fail_unless(unsaved == FALSE, NULL);
  g_object_unref(song1);
  GST_INFO("song 1 loaded");

  // load second song
  bt_edit_application_load_song(app, check_get_test_song_path("melo2.xml"));
  g_object_get(app,"song",&song2,NULL);
  fail_unless(song2 != NULL, NULL);
  fail_unless(song2 != song1, NULL);
  // song should be unchanged
  g_object_get(song2,"unsaved",&unsaved,NULL);
  fail_unless(unsaved == FALSE, NULL);
  GST_INFO("song 2 loaded");

  // get window
  g_object_get(app,"main-window",&main_window,NULL);
  fail_unless(main_window != NULL, NULL);

  playing=bt_song_play(song2);
  fail_unless(playing == TRUE, NULL);

  while(gtk_events_pending()) gtk_main_iteration();
  bt_song_stop(song2);

  // close window
  gtk_widget_destroy(GTK_WIDGET(main_window));
  while(gtk_events_pending()) gtk_main_iteration();
  //GST_INFO("mainlevel is %d",gtk_main_level());
  //while(g_main_context_pending(NULL)) g_main_context_iteration(/*context=*/NULL,/*may_block=*/FALSE);

  g_object_unref(song2);
  // free application
  GST_INFO("app->ref_ct=%d",G_OBJECT(app)->ref_count);
  g_object_checked_unref(app);

}
BT_END_TEST

// view all tabs
BT_START_TEST(test_tabs1) {
  BtEditApplication *app;
  BtMainWindow *main_window;
  BtMainPages *pages;
  BtSong *song;
  GtkWidget *child;
  GList *children;
  guint i,num_pages;

  app=bt_edit_application_new();
  GST_INFO("back in test app=%p, app->ref_ct=%d",app,G_OBJECT(app)->ref_count);
  fail_unless(app != NULL, NULL);

  bt_edit_application_load_song(app,check_get_test_song_path("melo3.xml"));
  g_object_get(app,"song",&song,NULL);
  fail_unless(song != NULL, NULL);
  g_object_unref(song);
  GST_INFO("song loaded");

  // get window
  g_object_get(app,"main-window",&main_window,NULL);
  fail_unless(main_window != NULL, NULL);

  // view all tabs
  g_object_get(G_OBJECT(main_window),"pages",&pages,NULL);
  /* make sure the pattern view shows something
  BtMainPagePatterns *pattern_page;
  g_object_get(G_OBJECT(pages),"patterns-page",&pattern_page,NULL);
  bt_main_page_patterns_show_machine(pattern_page,src_machine);
  */ 
  children=gtk_container_get_children(GTK_CONTAINER(pages));
  //num_pages=gtk_notebook_get_n_pages(GTK_NOTEBOOK(pages));
  num_pages=g_list_length(children);
  for(i=0;i<num_pages;i++) {
    gtk_notebook_set_current_page(GTK_NOTEBOOK(pages),i);
    child=GTK_WIDGET(g_list_nth_data(children,i));

    // make screenshot
    check_make_widget_screenshot(GTK_WIDGET(pages),gtk_widget_get_name(child));
    while(gtk_events_pending()) gtk_main_iteration();
  }
  g_list_free(children);
  g_object_unref(pages);

  // close window
  gtk_widget_destroy(GTK_WIDGET(main_window));
  while(gtk_events_pending()) gtk_main_iteration();
  //GST_INFO("mainlevel is %d",gtk_main_level());
  //while(g_main_context_pending(NULL)) g_main_context_iteration(/*context=*/NULL,/*may_block=*/FALSE);

  // free application
  GST_INFO("app->ref_ct=%d",G_OBJECT(app)->ref_count);
  g_object_checked_unref(app);

}
BT_END_TEST

// load a song and remove a machine
BT_START_TEST(test_machine_view_edit) {
  BtEditApplication *app;
  BtMainWindow *main_window;
  BtSong *song;
  BtSetup *setup;
  BtMachine *machine;

  app=bt_edit_application_new();
  GST_INFO("back in test app=%p, app->ref_ct=%d",app,G_OBJECT(app)->ref_count);
  fail_unless(app != NULL, NULL);

  bt_edit_application_load_song(app, check_get_test_song_path("test-simple3.xml"));
  g_object_get(app,"song",&song,NULL);
  fail_unless(song != NULL, NULL);
  GST_INFO("song loaded");
  g_object_get(song,"setup",&setup,NULL);
  
  // remove a source
  machine=bt_setup_get_machine_by_id(setup,"sine2");
  bt_setup_remove_machine(setup,machine);
  GST_INFO("setup.machine[sine2].ref-count=%d",G_OBJECT(machine)->ref_count);
  // ref count should be 1 now
  fail_unless(G_OBJECT(machine)->ref_count==1,NULL);
  g_object_unref(machine);

  // remove an effect
  machine=bt_setup_get_machine_by_id(setup,"amp1");
  bt_setup_remove_machine(setup,machine);
  GST_INFO("setup.machine[amp1].ref-count=%d",G_OBJECT(machine)->ref_count);
  // ref count should be 1 now
  fail_unless(G_OBJECT(machine)->ref_count==1,NULL);
  g_object_unref(machine);

  g_object_unref(setup);
  g_object_unref(song);

  // get window
  g_object_get(app,"main-window",&main_window,NULL);
  fail_unless(main_window != NULL, NULL);

  // close window
  gtk_widget_destroy(GTK_WIDGET(main_window));
  while(gtk_events_pending()) gtk_main_iteration();

  // free application
  GST_INFO("app->ref_ct=%d",G_OBJECT(app)->ref_count);
  g_object_checked_unref(app);
}
BT_END_TEST
  
TCase *bt_edit_application_example_case(void) {
  TCase *tc = tcase_create("BtEditApplicationExamples");

  tcase_add_test(tc,test_create_app);
  tcase_add_test(tc,test_new1);
  tcase_add_test(tc,test_load1);
  tcase_add_test(tc,test_load2);
  tcase_add_test(tc,test_load3);
  tcase_add_test(tc,test_load_and_play1);
  tcase_add_test(tc,test_load_and_play2);
  tcase_add_test(tc,test_tabs1);
  tcase_add_test(tc,test_machine_view_edit);
  // we *must* use a checked fixture, as only this runs in the same context
  tcase_add_checked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
