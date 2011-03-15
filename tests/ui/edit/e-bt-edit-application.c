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
BT_START_TEST(test_create) {
  BtEditApplication *app;
  BtMainWindow *main_window;

  app=bt_edit_application_new();
  GST_INFO("back in test app=%p, app->ref_ct=%d",app,G_OBJECT_REF_COUNT(app));
  fail_unless(app != NULL, NULL);

  // get window
  g_object_get(app,"main-window",&main_window,NULL);
  fail_unless(main_window != NULL, NULL);
  GST_INFO("main_window->ref_ct=%d",G_OBJECT_REF_COUNT(main_window));

  // make screenshot
  check_make_widget_screenshot(GTK_WIDGET(main_window),NULL);

  // close window
  GST_INFO("main_window->ref_ct=%d",G_OBJECT_REF_COUNT(main_window));
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
  GST_INFO("app->ref_ct=%d",G_OBJECT_REF_COUNT(app));
  g_object_checked_unref(app);
}
BT_END_TEST

static gboolean finish_main_loops(gpointer user_data) {
  // this does not work for dialogs (gtk_dialog_run)!
  gtk_main_quit();
  return FALSE;
}

// create app and then unconditionally destroy window
BT_START_TEST(test_run) {
  BtEditApplication *app;
  BtMainWindow *main_window;
  BtSettings *settings=bt_settings_make();

  app=bt_edit_application_new();
  fail_unless(app != NULL, NULL);

  // get window
  g_object_get(app,"main-window",&main_window,NULL);
  fail_unless(main_window != NULL, NULL);
  GST_INFO("main_window->ref_ct=%d",G_OBJECT_REF_COUNT(main_window));

  // avoid the about dialog
  g_object_set(settings,"news-seen",PACKAGE_VERSION_NUMBER,NULL);
  g_object_unref(settings);

  // run and quit
  g_idle_add(finish_main_loops,NULL);
  bt_edit_application_run(app);

  gtk_widget_destroy(GTK_WIDGET(main_window));

  // free application
  g_object_checked_unref(app);
}
BT_END_TEST


// create a new song
BT_START_TEST(test_new1) {
  BtEditApplication *app;
  BtMainWindow *main_window;
  BtSong *song;
  gboolean unsaved;

  app=bt_edit_application_new();
  GST_INFO("back in test app=%p, app->ref_ct=%d",app,G_OBJECT_REF_COUNT(app));
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
  GST_INFO("app->ref_ct=%d",G_OBJECT_REF_COUNT(app));
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
  GST_INFO("back in test app=%p, app->ref_ct=%d",app,G_OBJECT_REF_COUNT(app));
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
  GST_INFO("app->ref_ct=%d",G_OBJECT_REF_COUNT(app));
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
  GST_INFO("back in test app=%p, app->ref_ct=%d",app,G_OBJECT_REF_COUNT(app));
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

  // do events (normaly done by check_make_widget_screenshot())
  while(gtk_events_pending()) gtk_main_iteration();

  // get window
  g_object_get(app,"main-window",&main_window,NULL);
  fail_unless(main_window != NULL, NULL);

  // close window
  gtk_widget_destroy(GTK_WIDGET(main_window));
  while(gtk_events_pending()) gtk_main_iteration();

  // free application
  GST_INFO("app->ref_ct=%d",G_OBJECT_REF_COUNT(app));
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
  GST_INFO("back in test app=%p, app->ref_ct=%d",app,G_OBJECT_REF_COUNT(app));
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

  // do events (normaly done by check_make_widget_screenshot())
  while(gtk_events_pending()) gtk_main_iteration();

  // get window
  g_object_get(app,"main-window",&main_window,NULL);
  fail_unless(main_window != NULL, NULL);

  // close window
  gtk_widget_destroy(GTK_WIDGET(main_window));
  while(gtk_events_pending()) gtk_main_iteration();

  // free application
  gst_object_unref(bin);
  GST_INFO("app->ref_ct=%d",G_OBJECT_REF_COUNT(app));
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
  GST_INFO("back in test app=%p, app->ref_ct=%d",app,G_OBJECT_REF_COUNT(app));
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
  GST_INFO("app->ref_ct=%d",G_OBJECT_REF_COUNT(app));
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
  GST_INFO("back in test app=%p, app->ref_ct=%d",app,G_OBJECT_REF_COUNT(app));
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
  GST_INFO("app->ref_ct=%d",G_OBJECT_REF_COUNT(app));
  g_object_checked_unref(app);

}
BT_END_TEST

// view all tabs
BT_START_TEST(test_tabs1) {
  BtEditApplication *app;
  BtMainWindow *main_window;
  BtMainPages *pages;
  BtMainPagePatterns *pattern_page;
  BtSong *song;
  BtSetup *setup;
  BtWave *wave;
  BtMachine *src_machine;
  GtkWidget *child;
  GList *children;
  guint i,num_pages;

  app=bt_edit_application_new();
  GST_INFO("back in test app=%p, app->ref_ct=%d",app,G_OBJECT_REF_COUNT(app));
  fail_unless(app != NULL, NULL);

  // load a song and a sample
  bt_edit_application_load_song(app,check_get_test_song_path("melo3.xml"));
  g_object_get(app,"song",&song,NULL);
  fail_unless(song != NULL, NULL);
  g_object_get(song,"setup",&setup,NULL);
  wave=bt_wave_new(song,"test","file:///tmp/test.wav",1,1.0,BT_WAVE_LOOP_MODE_OFF,0);
  fail_unless(wave != NULL, NULL);
  // sample loading is async
  while(gtk_events_pending()) gtk_main_iteration();
  // stimulate ui update
  g_object_notify(G_OBJECT(app),"song");
  while(gtk_events_pending()) gtk_main_iteration();
  // free resources
  g_object_unref(wave);
  g_object_unref(song);
  GST_INFO("song loaded");

  // get window
  g_object_get(app,"main-window",&main_window,NULL);
  fail_unless(main_window != NULL, NULL);

  // view all tabs
  g_object_get(G_OBJECT(main_window),"pages",&pages,NULL);
  // make sure the pattern view shows something
  src_machine=bt_setup_get_machine_by_id(setup,"beep1");
  g_object_get(G_OBJECT(pages),"patterns-page",&pattern_page,NULL);
  bt_main_page_patterns_show_machine(pattern_page,src_machine);
  g_object_unref(pattern_page);
  g_object_unref(src_machine);
  g_object_unref(setup);

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
  GST_INFO("app->ref_ct=%d",G_OBJECT_REF_COUNT(app));
  g_object_checked_unref(app);

}
BT_END_TEST

// view all tabs
BT_START_TEST(test_tabs_playing) {
  BtEditApplication *app;
  BtMainWindow *main_window;
  BtMainPages *pages;
  BtMainPagePatterns *pattern_page;
  BtSong *song;
  BtSetup *setup;
  BtWave *wave;
  BtMachine *src_machine;
  GtkWidget *child;
  GList *children;
  guint i,num_pages;

  app=bt_edit_application_new();
  GST_INFO("back in test app=%p, app->ref_ct=%d",app,G_OBJECT_REF_COUNT(app));
  fail_unless(app != NULL, NULL);

  // load a song and a sample
  bt_edit_application_load_song(app,check_get_test_song_path("melo3.xml"));
  g_object_get(app,"song",&song,NULL);
  fail_unless(song != NULL, NULL);
  g_object_get(song,"setup",&setup,NULL);
  wave=bt_wave_new(song,"test","file:///tmp/test.wav",1,1.0,BT_WAVE_LOOP_MODE_OFF,0);
  fail_unless(wave != NULL, NULL);
  // sample loading is async
  while(gtk_events_pending()) gtk_main_iteration();
  // stimulate ui update
  g_object_notify(G_OBJECT(app),"song");
  while(gtk_events_pending()) gtk_main_iteration();
  // free resources
  g_object_unref(wave);
  g_object_unref(song);
  GST_INFO("song loaded");

  // get window
  g_object_get(app,"main-window",&main_window,NULL);
  fail_unless(main_window != NULL, NULL);

  // view all tabs
  g_object_get(G_OBJECT(main_window),"pages",&pages,NULL);
  // make sure the pattern view shows something
  src_machine=bt_setup_get_machine_by_id(setup,"beep1");
  g_object_get(G_OBJECT(pages),"patterns-page",&pattern_page,NULL);
  bt_main_page_patterns_show_machine(pattern_page,src_machine);
  g_object_unref(pattern_page);
  g_object_unref(src_machine);
  g_object_unref(setup);

  children=gtk_container_get_children(GTK_CONTAINER(pages));
  //num_pages=gtk_notebook_get_n_pages(GTK_NOTEBOOK(pages));
  num_pages=g_list_length(children);
  // play for a while to trigger screen updates
  bt_song_play(song);
  for(i=0;i<num_pages;i++) {
    bt_song_update_playback_position(song);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(pages),i);
    child=GTK_WIDGET(g_list_nth_data(children,i));

    while(gtk_events_pending()) gtk_main_iteration();
  }
  bt_song_stop(song);
  g_list_free(children);
  g_object_unref(pages);

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

// load a song and remove a machine
BT_START_TEST(test_machine_view_edit0) {
  BtEditApplication *app;
  BtMainWindow *main_window;
  BtSong *song;
  BtSetup *setup;
  BtMachine *machine;

  app=bt_edit_application_new();
  GST_INFO("back in test app=%p, app->ref_ct=%d",app,G_OBJECT_REF_COUNT(app));
  fail_unless(app != NULL, NULL);

  bt_edit_application_load_song(app, check_get_test_song_path("test-simple1.xml"));
  g_object_get(app,"song",&song,NULL);
  fail_unless(song != NULL, NULL);
  GST_INFO("song loaded");
  g_object_get(song,"setup",&setup,NULL);

  // get window
  g_object_get(app,"main-window",&main_window,NULL);
  fail_unless(main_window != NULL, NULL);

  // remove a source
  machine=bt_setup_get_machine_by_id(setup,"sine1");
  GST_INFO("setup.machine[sine2].ref_count=%d",G_OBJECT_REF_COUNT(machine));
  bt_setup_remove_machine(setup,machine);
  while(gtk_events_pending()) gtk_main_iteration();
  GST_INFO("setup.machine[sine2].ref_count=%d",G_OBJECT_REF_COUNT(machine));
  // ref count should be 1 now
  fail_unless(G_OBJECT_REF_COUNT(machine)==1,NULL);
  g_object_unref(machine);

  g_object_unref(setup);
  g_object_unref(song);

  // close window
  gtk_widget_destroy(GTK_WIDGET(main_window));
  while(gtk_events_pending()) gtk_main_iteration();

  // free application
  GST_INFO("app->ref_ct=%d",G_OBJECT_REF_COUNT(app));
  g_object_checked_unref(app);
}
BT_END_TEST

// load a song and remove machines
BT_START_TEST(test_machine_view_edit1) {
  BtEditApplication *app;
  BtMainWindow *main_window;
  BtSong *song;
  BtSetup *setup;
  BtMachine *machine;

  app=bt_edit_application_new();
  GST_INFO("back in test app=%p, app->ref_ct=%d",app,G_OBJECT_REF_COUNT(app));
  fail_unless(app != NULL, NULL);

  bt_edit_application_load_song(app, check_get_test_song_path("test-simple2.xml"));
  g_object_get(app,"song",&song,NULL);
  fail_unless(song != NULL, NULL);
  GST_INFO("song loaded");
  g_object_get(song,"setup",&setup,NULL);

  // get window
  g_object_get(app,"main-window",&main_window,NULL);
  fail_unless(main_window != NULL, NULL);

  // remove a source
  machine=bt_setup_get_machine_by_id(setup,"sine1");
  GST_INFO("setup.machine[sine1].ref_count=%d",G_OBJECT_REF_COUNT(machine));
  bt_setup_remove_machine(setup,machine);
  while(gtk_events_pending()) gtk_main_iteration();
  GST_INFO("setup.machine[sine1].ref_count=%d",G_OBJECT_REF_COUNT(machine));
  // ref count should be 1 now
  fail_unless(G_OBJECT_REF_COUNT(machine)==1,NULL);
  g_object_unref(machine);

  // remove an effect
  machine=bt_setup_get_machine_by_id(setup,"amp1");
  GST_INFO("setup.machine[amp1].ref_count=%d",G_OBJECT_REF_COUNT(machine));
  bt_setup_remove_machine(setup,machine);
  while(gtk_events_pending()) gtk_main_iteration();
  GST_INFO("setup.machine[amp1].ref_count=%d",G_OBJECT_REF_COUNT(machine));
  // ref count should be 1 now
  fail_unless(G_OBJECT_REF_COUNT(machine)==1,NULL);
  g_object_unref(machine);

  g_object_unref(setup);
  g_object_unref(song);

  // close window
  gtk_widget_destroy(GTK_WIDGET(main_window));
  while(gtk_events_pending()) gtk_main_iteration();

  // free application
  GST_INFO("app->ref_ct=%d",G_OBJECT_REF_COUNT(app));
  g_object_checked_unref(app);
}
BT_END_TEST

// load a song and remove machines
// (same as above with order in which we remove the machines swapped)
BT_START_TEST(test_machine_view_edit2) {
  BtEditApplication *app;
  BtMainWindow *main_window;
  BtSong *song;
  BtSetup *setup;
  BtMachine *machine;

  app=bt_edit_application_new();
  GST_INFO("back in test app=%p, app->ref_ct=%d",app,G_OBJECT_REF_COUNT(app));
  fail_unless(app != NULL, NULL);

  bt_edit_application_load_song(app, check_get_test_song_path("test-simple3.xml"));
  g_object_get(app,"song",&song,NULL);
  fail_unless(song != NULL, NULL);
  GST_INFO("song loaded");
  g_object_get(song,"setup",&setup,NULL);

  // get window
  g_object_get(app,"main-window",&main_window,NULL);
  fail_unless(main_window != NULL, NULL);

  // remove an effect (this remes the wires)
  machine=bt_setup_get_machine_by_id(setup,"amp1");
  GST_INFO("setup.machine[amp1].ref_count=%d",G_OBJECT_REF_COUNT(machine));
  bt_setup_remove_machine(setup,machine);
  while(gtk_events_pending()) gtk_main_iteration();
  GST_INFO("setup.machine[amp1].ref_count=%d",G_OBJECT_REF_COUNT(machine));
  // ref count should be 1 now
  fail_unless(G_OBJECT_REF_COUNT(machine)==1,NULL);
  g_object_unref(machine);

  // check a source
  machine=bt_setup_get_machine_by_id(setup,"sine1");
  GST_INFO("setup.machine[sine1].ref_count=%d",G_OBJECT_REF_COUNT(machine));
  g_object_unref(machine);

  // remove a source
  machine=bt_setup_get_machine_by_id(setup,"sine2");
  GST_INFO("setup.machine[sine2].ref_count=%d",G_OBJECT_REF_COUNT(machine));
  bt_setup_remove_machine(setup,machine);
  while(gtk_events_pending()) gtk_main_iteration();
  GST_INFO("setup.machine[sine2].ref_count=%d",G_OBJECT_REF_COUNT(machine));
  // ref count should be 1 now
  fail_unless(G_OBJECT_REF_COUNT(machine)==1,NULL);
  g_object_unref(machine);

  g_object_unref(setup);
  g_object_unref(song);

  // close window
  gtk_widget_destroy(GTK_WIDGET(main_window));
  while(gtk_events_pending()) gtk_main_iteration();

  // free application
  GST_INFO("app->ref_ct=%d",G_OBJECT_REF_COUNT(app));
  g_object_checked_unref(app);
}
BT_END_TEST

// load a song and remove machines
BT_START_TEST(test_machine_view_edit3) {
  BtEditApplication *app;
  BtMainWindow *main_window;
  BtMainPages *pages;
  BtSong *song;
  BtSetup *setup;
  BtWire *wire;
  BtMachine *machine1,*machine2;

  app=bt_edit_application_new();
  GST_INFO("back in test app=%p, app->ref_ct=%d",app,G_OBJECT_REF_COUNT(app));
  fail_unless(app != NULL, NULL);

  // sine1 ! amp1 ! master + sine2 ! amp1
  //bt_edit_application_load_song(app, check_get_test_song_path("test-simple3.xml"));
  // sine1 ! amp1 ! master
  bt_edit_application_load_song(app, check_get_test_song_path("test-simple2.xml"));
  g_object_get(app,"song",&song,NULL);
  fail_unless(song != NULL, NULL);
  GST_INFO("song loaded");
  g_object_get(song,"setup",&setup,NULL);

  // get window
  g_object_get(app,"main-window",&main_window,NULL);
  fail_unless(main_window != NULL, NULL);
  g_object_get(G_OBJECT(main_window),"pages",&pages,NULL);
  fail_unless(pages != NULL, NULL);
  // remove some other pages
  // (ev. run for all combinations - if a test using all pages fails?)
  gtk_notebook_remove_page(GTK_NOTEBOOK(pages),BT_MAIN_PAGES_INFO_PAGE);
  gtk_notebook_remove_page(GTK_NOTEBOOK(pages),BT_MAIN_PAGES_WAVES_PAGE);
  // FIXME: having the sequence page enabled/disabled makes a difference
  //  between ref-leak and too many unrefs
  //gtk_notebook_remove_page(GTK_NOTEBOOK(pages),BT_MAIN_PAGES_SEQUENCE_PAGE);
  //gtk_notebook_remove_page(GTK_NOTEBOOK(pages),BT_MAIN_PAGES_PATTERNS_PAGE);
  g_object_unref(pages);

  // remove wire
  machine1=bt_setup_get_machine_by_id(setup,"sine1");
  GST_INFO("setup.machine[sine1].ref_count=%d",G_OBJECT_REF_COUNT(machine1));
  machine2=bt_setup_get_machine_by_id(setup,"amp1");
  GST_INFO("setup.machine[amp1].ref_count=%d",G_OBJECT_REF_COUNT(machine2));
  wire=bt_setup_get_wire_by_machines(setup,machine1,machine2);
  GST_INFO("setup.wire[sine1->amp1].ref_count=%d",G_OBJECT_REF_COUNT(wire));
  bt_setup_remove_wire(setup,wire);
  while(gtk_events_pending()) gtk_main_iteration();
  GST_INFO("setup.wire[sine1->amp1].ref_count=%d",G_OBJECT_REF_COUNT(wire));
  // ref count should be 1 now
  fail_unless(G_OBJECT_REF_COUNT(wire)==1,NULL);
  g_object_unref(wire);

  // remove a source
  GST_INFO("setup.machine[sine1].ref_count=%d",G_OBJECT_REF_COUNT(machine1));
  bt_setup_remove_machine(setup,machine1);
  while(gtk_events_pending()) gtk_main_iteration();
  GST_INFO("setup.machine[sine1].ref_count=%d",G_OBJECT_REF_COUNT(machine1));
  // ref count should be 1 now
  fail_unless(G_OBJECT_REF_COUNT(machine1)==1,NULL);
  g_object_unref(machine1);

  // remove an effect
  GST_INFO("setup.machine[amp1].ref_count=%d",G_OBJECT_REF_COUNT(machine2));
  bt_setup_remove_machine(setup,machine2);
  while(gtk_events_pending()) gtk_main_iteration();
  GST_INFO("setup.machine[amp1].ref_count=%d",G_OBJECT_REF_COUNT(machine2));
  // ref count should be 1 now
  fail_unless(G_OBJECT_REF_COUNT(machine2)==1,NULL);
  g_object_unref(machine2);

  g_object_unref(setup);
  g_object_unref(song);

  // close window
  gtk_widget_destroy(GTK_WIDGET(main_window));
  while(gtk_events_pending()) gtk_main_iteration();

  // free application
  GST_INFO("app->ref_ct=%d",G_OBJECT_REF_COUNT(app));
  g_object_checked_unref(app);
}
BT_END_TEST


TCase *bt_edit_application_example_case(void) {
  TCase *tc = tcase_create("BtEditApplicationExamples");

  tcase_add_test(tc,test_create);
  tcase_add_test(tc,test_run);
  tcase_add_test(tc,test_new1);
  tcase_add_test(tc,test_load1);
  tcase_add_test(tc,test_load2);
  tcase_add_test(tc,test_load3);
  tcase_add_test(tc,test_load_and_play1);
  tcase_add_test(tc,test_load_and_play2);
  tcase_add_test(tc,test_tabs1);
  tcase_add_test(tc,test_tabs_playing);
  tcase_add_test(tc,test_machine_view_edit0);
  tcase_add_test(tc,test_machine_view_edit1);
  tcase_add_test(tc,test_machine_view_edit2);
  tcase_add_test(tc,test_machine_view_edit3);
  // we *must* use a checked fixture, as only this runs in the same context
  tcase_add_checked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
