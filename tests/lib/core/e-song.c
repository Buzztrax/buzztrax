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

#include "m-bt-core.h"

//-- globals

#if 0
static gboolean play_signal_invoked=FALSE;
#endif

//-- fixtures

static void test_setup(void) {
  bt_core_setup();
  GST_INFO("================================================================================");
}

static void test_teardown(void) {
  bt_core_teardown();
  //puts(__FILE__":teardown");
}

//-- tests

// helper method to test the play signal
#if 0
static void on_song_is_playing_notify(const BtSong *song,GParamSpec *arg,gpointer user_data) {
  play_signal_invoked=TRUE;
}
#endif

// test if the default constructor works as expected
BT_START_TEST(test_btsong_obj1) {
  BtApplication *app=NULL;
  BtSong *song;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  fail_unless(song != NULL, NULL);

  g_object_checked_unref(song);

  g_object_checked_unref(app);
}
BT_END_TEST

// test if the song loading works without failure
BT_START_TEST(test_btsong_load1) {
  BtApplication *app=NULL;
  BtSong *song;
  BtSongIO *loader;
  gboolean load_ret = FALSE;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  fail_unless(song != NULL, NULL);
  //gst_debug_set_threshold_for_name("bt*",GST_LEVEL_DEBUG);
  loader=bt_song_io_from_file(check_get_test_song_path("test-simple1.xml"));
  mark_point();
  fail_unless(loader != NULL, NULL);
  //gst_debug_set_threshold_for_name("bt*",GST_LEVEL_WARNING);
  load_ret = bt_song_io_load(loader,song);
  mark_point();
  fail_unless(load_ret, NULL);

  mark_point();
  g_object_checked_unref(loader);
  mark_point();
  g_object_checked_unref(song);
  mark_point();
  g_object_checked_unref(app);
}
BT_END_TEST

// test if subsequent song loading works without failure
BT_START_TEST(test_btsong_load2) {
  BtApplication *app=NULL;
  BtSong *song;
  BtSongIO *loader;
  gboolean load_ret = FALSE;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  fail_unless(song != NULL, NULL);
  loader=bt_song_io_from_file(check_get_test_song_path("test-simple1.xml"));
  fail_unless(loader != NULL, NULL);
  load_ret = bt_song_io_load(loader,song);
  fail_unless(load_ret, NULL);
  g_object_checked_unref(loader);
  g_object_checked_unref(song);

  song=bt_song_new(app);
  fail_unless(song != NULL, NULL);
  loader=bt_song_io_from_file(check_get_test_song_path("test-simple2.xml"));
  fail_unless(loader != NULL, NULL);
  load_ret = bt_song_io_load(loader,song);
  fail_unless(load_ret, NULL);
  g_object_checked_unref(loader);
  g_object_checked_unref(song);

  g_object_checked_unref(app);
}
BT_END_TEST

// test if after the song loading everything is there
BT_START_TEST(test_btsong_load3) {
  BtApplication *app=NULL;
  BtSong *song;
  BtSongIO *loader;
  gboolean load_ret = FALSE;
  BtSinkMachine *master;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  fail_unless(song != NULL, NULL);
  //gst_debug_set_threshold_for_name("bt*",GST_LEVEL_DEBUG);
  loader=bt_song_io_from_file(check_get_test_song_path("test-simple1.xml"));
  mark_point();
  fail_unless(loader != NULL, NULL);
  //gst_debug_set_threshold_for_name("bt*",GST_LEVEL_WARNING);
  load_ret = bt_song_io_load(loader,song);
  mark_point();
  fail_unless(load_ret, NULL);

  g_object_get(song,"master",&master,NULL);
  fail_unless(master != NULL, NULL);
  g_object_unref(master);

  mark_point();
  g_object_checked_unref(loader);
  mark_point();
  g_object_checked_unref(song);
  mark_point();
  g_object_checked_unref(app);
}
BT_END_TEST

// test if the song play routine works without failure
BT_START_TEST(test_btsong_play1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSongIO *loader=NULL;
  gboolean load_ret = FALSE;
  gboolean res;
  gboolean is_playing;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  fail_unless(song != NULL, NULL);
  loader=bt_song_io_from_file(check_get_test_song_path("test-simple1.xml"));
  fail_unless(loader != NULL, NULL);
  load_ret = bt_song_io_load(loader,song);
  fail_unless(load_ret, NULL);

  //play_signal_invoked=FALSE;
  //g_signal_connect(G_OBJECT(song),"notify::is-playing",G_CALLBACK(on_song_is_playing_notify),NULL);

  res=bt_song_play(song);
  fail_unless(res, NULL);
  
  // TODO(ensonic): this needs a mainloop!
  g_usleep(G_USEC_PER_SEC/10);
  // this song is very short!
  //g_object_get(G_OBJECT(song),"is-playing",&is_playing,NULL);
  //fail_unless(is_playing, NULL);

  //fail_unless(play_signal_invoked, NULL);
  bt_song_stop(song);

  // TODO(ensonic): this needs a mainloop!
  g_usleep(G_USEC_PER_SEC/10);

  g_object_get(G_OBJECT(song),"is-playing",&is_playing,NULL);
  fail_unless(!is_playing, NULL);

  g_object_checked_unref(loader);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

// play, wait a little, stop, play again
BT_START_TEST(test_btsong_play2) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSongIO *loader=NULL;
  gboolean load_ret = FALSE;
  gboolean res;
  gint i;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  fail_unless(song != NULL, NULL);
  loader=bt_song_io_from_file(check_get_test_song_path("test-simple1.xml"));
  fail_unless(loader != NULL, NULL);
  load_ret = bt_song_io_load(loader,song);
  fail_unless(load_ret, NULL);

  //play_signal_invoked=FALSE;
  //g_signal_connect(G_OBJECT(song),"notify::is-playing",G_CALLBACK(on_song_is_playing_notify),NULL);

  for(i=0;i<2;i++) {
    res=bt_song_play(song);
    fail_unless(res, NULL);
  
    // TODO(ensonic): this needs a mainloop!
    g_usleep(G_USEC_PER_SEC/10);
    //fail_unless(play_signal_invoked, NULL);
    bt_song_stop(song);
    //play_signal_invoked=FALSE;
  }

  g_object_checked_unref(loader);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

// load a new song, play, change audiosink to fakesink
BT_START_TEST(test_btsong_play3) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSongIO *loader=NULL;
  gboolean load_ret = FALSE;
  gboolean res;
  BtSettings *settings=bt_settings_make();

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  fail_unless(song != NULL, NULL);
  loader=bt_song_io_from_file(check_get_test_song_path("test-simple1.xml"));
  fail_unless(loader != NULL, NULL);
  load_ret = bt_song_io_load(loader,song);
  fail_unless(load_ret, NULL);

  //play_signal_invoked=FALSE;
  //g_signal_connect(G_OBJECT(song),"notify::is-playing",G_CALLBACK(on_song_is_playing_notify),NULL);

  res=bt_song_play(song);
  fail_unless(res, NULL);

  // TODO(ensonic): this needs a mainloop!
  g_usleep(G_USEC_PER_SEC/10);
  
  // change audiosink
  g_object_set(settings,"audiosink","fakesink",NULL);
  
  //fail_unless(play_signal_invoked, NULL);
  bt_song_stop(song);

  g_object_checked_unref(loader);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

// change audiosink to NULL, load and play a song
BT_START_TEST(test_btsong_play4) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSongIO *loader=NULL;
  gboolean load_ret = FALSE;
  gboolean res;
  BtSettings *settings=bt_settings_make();

  // change audiosink
  g_object_set(settings,"audiosink",NULL,"system-audiosink",NULL,NULL);

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  fail_unless(song != NULL, NULL);
  loader=bt_song_io_from_file(check_get_test_song_path("test-simple1.xml"));
  fail_unless(loader != NULL, NULL);
  load_ret = bt_song_io_load(loader,song);
  fail_unless(load_ret, NULL);

  //play_signal_invoked=FALSE;
  //g_signal_connect(G_OBJECT(song),"notify::is-playing",G_CALLBACK(on_song_is_playing_notify),NULL);

  res=bt_song_play(song);
  fail_unless(res, NULL);

  // TODO(ensonic): this needs a mainloop!
  g_usleep(G_USEC_PER_SEC/10);
  //fail_unless(play_signal_invoked, NULL);
  bt_song_stop(song);

  g_object_checked_unref(loader);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

// test, if a newly created song contains empty setup, sequence, song-info and
// wavetable
BT_START_TEST(test_btsong_new1){
  BtApplication *app=NULL,*app2;
  BtSong *song=NULL,*song2;
  BtSetup *setup=NULL;
  BtSequence *sequence=NULL;
  BtSongInfo *songinfo=NULL;
  BtWavetable *wavetable=NULL;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  fail_unless(song != NULL, NULL);
  g_object_get(song,"app",&app2,NULL);
  fail_unless(app2==app,NULL);
  g_object_unref(app2);

  // get the setup property
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL,NULL);
  g_object_get(setup,"song",&song2,NULL);
  fail_unless(song2==song,NULL);
  g_object_unref(song2);
  g_object_unref(setup);

  // get the sequence property
  g_object_get(song,"sequence",&sequence,NULL);
  fail_unless(sequence!=NULL,NULL);
  g_object_get(sequence,"song",&song2,NULL);
  fail_unless(song2==song,NULL);
  g_object_unref(song2);
  g_object_unref(sequence);

  // get the song-info property
  g_object_get(song,"song-info",&songinfo,NULL);
  fail_unless(songinfo!=NULL,NULL);
  g_object_get(songinfo,"song",&song2,NULL);
  fail_unless(song2==song,NULL);
  g_object_unref(song2);
  g_object_unref(songinfo);

  // get the wavetable property
  g_object_get(song,"wavetable",&wavetable,NULL);
  fail_unless(wavetable!=NULL,NULL);
  g_object_get(wavetable,"song",&song2,NULL);
  fail_unless(song2==song,NULL);
  g_object_unref(song2);
  g_object_unref(wavetable);

  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

// test the idle looper
BT_START_TEST(test_btsong_idle1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSongIO *loader=NULL;
  gboolean load_ret = FALSE;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  fail_unless(song != NULL, NULL);
  loader=bt_song_io_from_file(check_get_test_song_path("test-simple1.xml"));
  fail_unless(loader != NULL, NULL);
  load_ret = bt_song_io_load(loader,song);
  fail_unless(load_ret, NULL);

  //play_signal_invoked=FALSE;
  //g_signal_connect(G_OBJECT(song),"notify::is-playing",G_CALLBACK(on_song_is_playing_notify),NULL);

  g_object_set(G_OBJECT(song),"is-idle",TRUE,NULL);
  
  // TODO(ensonic): this needs a mainloop!
  g_usleep(G_USEC_PER_SEC/10);
  g_object_set(G_OBJECT(song),"is-idle",FALSE,NULL);

  g_object_checked_unref(loader);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

// test the idle looper and playing transition
BT_START_TEST(test_btsong_idle2) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSongIO *loader=NULL;
  gboolean load_ret = FALSE;
  gboolean res;
  gboolean is_idle,is_playing;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  fail_unless(song != NULL, NULL);
  loader=bt_song_io_from_file(check_get_test_song_path("test-simple1.xml"));
  fail_unless(loader != NULL, NULL);
  load_ret = bt_song_io_load(loader,song);
  fail_unless(load_ret, NULL);

  g_object_set(G_OBJECT(song),"is-idle",TRUE,NULL);

  // TODO(ensonic): this needs a mainloop!
  g_usleep(G_USEC_PER_SEC/10);
  
  // start regular playback, this should stop the idle loop
  res=bt_song_play(song);
  fail_unless(res, NULL);

  // TODO(ensonic): this needs a mainloop!
  g_usleep(G_USEC_PER_SEC/10);

  // this song is very short!
  //g_object_get(G_OBJECT(song),"is-playing",&is_playing,"is-idle",&is_idle,NULL);
  //fail_unless(is_playing, NULL);
  //fail_unless(!is_idle, NULL);

  bt_song_stop(song);

  // TODO(ensonic): this needs a mainloop!
  g_usleep(G_USEC_PER_SEC/10);
  
  g_object_get(G_OBJECT(song),"is-playing",&is_playing,"is-idle",&is_idle,NULL);
  fail_unless(!is_playing, NULL);
  fail_unless(is_idle, NULL);
  
  g_object_set(G_OBJECT(song),"is-idle",FALSE,NULL);

  g_object_checked_unref(loader);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/*
 * check if we can connect a sine machine to a sink. Also try to play after
 * connecting the machines.
 */
BT_START_TEST(test_btsong_static1) {
  BtApplication *app=NULL;
  GError *err=NULL;
  // the song
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  BtSequence *sequence=NULL;
  // machines
  BtMachine *gen1=NULL;
  BtMachine *sink=NULL;
  BtMachine *machine;
  // wires
  BtWire *wire, *wire1=NULL;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);

  /* get the song setup and sequence */
  g_object_get(song,"setup",&setup,"sequence",&sequence,NULL);
  fail_unless(setup!=NULL, NULL);

  /* try to create the sink */
  sink=BT_MACHINE(bt_sink_machine_new(song,"master",&err));
  fail_unless(sink!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create generator1 */
  gen1=BT_MACHINE(bt_source_machine_new(song,"generator1","audiotestsrc",0,&err));
  fail_unless(gen1!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* check if we can retrieve the machine via the id */
  machine=bt_setup_get_machine_by_id(setup,"master");
  fail_unless(machine==sink, NULL);
  g_object_unref(machine);

  machine=bt_setup_get_machine_by_id(setup,"generator1");
  fail_unless(machine==gen1, NULL);
  g_object_unref(machine);

  /* try to link machines */
  wire1=bt_wire_new(song, gen1, sink,&err);
  fail_unless(wire1!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* check if we can retrieve the wire via the source machine */
  wire=bt_setup_get_wire_by_src_machine(setup,gen1);
  fail_unless(wire==wire1, NULL);
  g_object_try_unref(wire);

  /* check if we can retrieve the wire via the dest machine */
  wire=bt_setup_get_wire_by_dst_machine(setup,sink);
  fail_unless(wire==wire1, NULL);
  g_object_try_unref(wire);

  /* enlarge the sequence */
  g_object_set(sequence,"length",1L,"tracks",1L,NULL);
  bt_sequence_add_track(sequence,gen1,-1);

  /* try to start playing the song */
  if(bt_song_play(song)) {
    mark_point();
    g_usleep(G_USEC_PER_SEC/10);
    /* stop the song */
    bt_song_stop(song);
  } else {
    fail("playing of network song failed");
  }

  g_object_unref(gen1);
  g_object_unref(sink);
  g_object_unref(wire1);
  g_object_unref(setup);
  g_object_unref(sequence);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/*
 * check if we can connect two sine machines to one sink. Also try to play after
 * connecting the machines.
 */
BT_START_TEST(test_btsong_static2) {
  BtApplication *app=NULL;
  GError *err=NULL;
  // the song
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  BtSequence *sequence=NULL;
  // machines
  BtMachine *gen1=NULL,*gen2=NULL;
  BtMachine *sink=NULL;
  // wires
  BtWire *wire1=NULL, *wire2=NULL;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);

  /* get the song setup and sequence */
  g_object_get(song,"setup",&setup,"sequence",&sequence,NULL);
  fail_unless(setup!=NULL, NULL);
  fail_unless(sequence!=NULL, NULL);

  /* try to create the sink */
  sink=BT_MACHINE(bt_sink_machine_new(song,"master",&err));
  fail_unless(sink!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create generator1 */
  gen1=BT_MACHINE(bt_source_machine_new(song,"generator1","audiotestsrc",0,&err));
  fail_unless(gen1!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create generator2 */
  gen2=BT_MACHINE(bt_source_machine_new(song,"generator2","audiotestsrc",0,&err));
  fail_unless(gen2!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create a wire from gen1 to sink */
  wire1=bt_wire_new(song, gen1, sink,&err);
  fail_unless(wire1!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create a wire from gen2 to sink */
  wire2=bt_wire_new(song, gen2, sink,&err);
  fail_unless(wire2!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* enlarge the sequence */
  g_object_set(sequence,"length",1L,"tracks",2L,NULL);
  bt_sequence_add_track(sequence,gen1,-1);
  bt_sequence_add_track(sequence,gen2,-1);
  mark_point();

  /* try to start playing the song */
  if(bt_song_play(song)) {
    mark_point();
    g_usleep(G_USEC_PER_SEC/10);
    /* stop the song */
    bt_song_stop(song);
  } else {
    fail("playing song failed");
  }

  g_object_unref(gen1);
  g_object_unref(gen2);
  g_object_unref(sink);
  g_object_unref(wire1);
  g_object_unref(wire2);
  g_object_unref(setup);
  g_object_unref(sequence);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/*
 * check if we can connect two sine machines to one effect and this to the
 * sink. Also try to start play after connecting the machines.
 */
BT_START_TEST(test_btsong_static3) {
  BtApplication *app=NULL;
  GError *err=NULL;
  // the song
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  BtSequence *sequence=NULL;
  BtSongInfo *song_info=NULL;
  // machines
  BtMachine *gen1=NULL,*gen2=NULL;
  BtMachine *proc=NULL;
  BtMachine *sink=NULL;
  // wires
  BtWire *wire1=NULL, *wire2=NULL, *wire3=NULL;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);

  /* get the song setup and sequence */
  g_object_get(song,"setup",&setup,"sequence",&sequence,"song-info",&song_info,NULL);
  fail_unless(setup!=NULL, NULL);
  fail_unless(sequence!=NULL, NULL);
  fail_unless(song_info!=NULL, NULL);
  g_object_set(song_info,"name","btsong_example3",NULL);

  /* try to create the sink */
  sink=BT_MACHINE(bt_sink_machine_new(song,"master",&err));
  fail_unless(sink!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create generator1 */
  gen1=BT_MACHINE(bt_source_machine_new(song,"generator1","audiotestsrc",0,&err));
  fail_unless(gen1!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create generator2 */
  gen2=BT_MACHINE(bt_source_machine_new(song,"generator2","audiotestsrc",0,&err));
  fail_unless(gen2!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create a processor with volume */
  proc=BT_MACHINE(bt_processor_machine_new(song,"processor","volume",0,&err));
  fail_unless(proc!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create a wire from gen1 to proc */
  wire1=bt_wire_new(song,gen1,proc,&err);
  fail_unless(wire1!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create a wire from gen2 to proc */
  wire2=bt_wire_new(song,gen2,proc,&err);
  fail_unless(wire2!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create a wire from proc to sink */
  wire3=bt_wire_new(song,proc,sink,&err);
  fail_unless(wire3!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* enlarge the sequence */
  g_object_set(sequence,"length",1L,"tracks",2L,NULL);
  bt_sequence_add_track(sequence,gen1,-1);
  bt_sequence_add_track(sequence,gen2,-1);
  mark_point();

  /* try to start playing the song */
  if(bt_song_play(song)) {
    mark_point();
    g_usleep(G_USEC_PER_SEC/10);
    /* stop the song */
    bt_song_stop(song);
  } else {
    fail("playing song failed");
  }

  g_object_unref(gen1);
  g_object_unref(gen2);
  g_object_unref(proc);
  g_object_unref(sink);
  g_object_unref(wire1);
  g_object_unref(wire2);
  g_object_unref(wire3);
  g_object_unref(setup);
  g_object_unref(sequence);
  g_object_unref(song_info);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/*
 * check if we can connect two sine machines to one sink, then play() and
 * stop(). After stopping remove one machine and play again.
 */
BT_START_TEST(test_btsong_static4) {
  BtApplication *app=NULL;
  GError *err=NULL;
  // the song
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  BtSequence *sequence=NULL;
  BtSongInfo *song_info=NULL;
  // machines
  BtMachine *gen1=NULL,*gen2=NULL;
  BtMachine *sink=NULL;
  // wires
  BtWire *wire1=NULL, *wire2=NULL;
  gboolean res;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);

  /* get the song setup and sequence */
  g_object_get(song,"setup",&setup,"sequence",&sequence,"song-info",&song_info,NULL);
  fail_unless(setup!=NULL, NULL);
  fail_unless(sequence!=NULL, NULL);
  g_object_set(song_info,"name","btsong_example3",NULL);

  /* try to create the sink */
  sink=BT_MACHINE(bt_sink_machine_new(song,"master",&err));
  fail_unless(sink!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create generator1 */
  gen1=BT_MACHINE(bt_source_machine_new(song,"generator1","audiotestsrc",0,&err));
  fail_unless(gen1!=NULL, NULL);
  fail_unless(err==NULL, NULL);
  GST_DEBUG_OBJECT(gen1,"added machine: %p,ref_count=%d",gen1,G_OBJECT_REF_COUNT(gen1));

  /* try to create generator2 */
  gen2=BT_MACHINE(bt_source_machine_new(song,"generator2","audiotestsrc",0,&err));
  fail_unless(gen2!=NULL, NULL);
  fail_unless(err==NULL, NULL);
  GST_DEBUG_OBJECT(gen2,"added machine: %p,ref_count=%d",gen2,G_OBJECT_REF_COUNT(gen2));

  /* try to create a wire from gen1 to proc */
  wire1=bt_wire_new(song,gen1,sink,&err);
  fail_unless(wire1!=NULL, NULL);
  fail_unless(err==NULL, NULL);
  GST_DEBUG_OBJECT(gen1,"linked machine: %p,ref_count=%d",gen1,G_OBJECT_REF_COUNT(gen1));

  /* try to create a wire from gen2 to proc */
  wire2=bt_wire_new(song,gen2,sink,&err);
  fail_unless(wire2!=NULL, NULL);
  fail_unless(err==NULL, NULL);
  GST_DEBUG_OBJECT(gen2,"linked machine: %p,ref_count=%d",gen2,G_OBJECT_REF_COUNT(gen2));

  /* enlarge the sequence */
  g_object_set(sequence,"length",1L,"tracks",2L,NULL);
  bt_sequence_add_track(sequence,gen1,-1);
  GST_DEBUG_OBJECT(gen1,"added machine to sequence: %p,ref_count=%d",gen1,G_OBJECT_REF_COUNT(gen1));
  bt_sequence_add_track(sequence,gen2,-1);
  GST_DEBUG_OBJECT(gen2,"added machine to sequence: %p,ref_count=%d",gen2,G_OBJECT_REF_COUNT(gen2));
  mark_point();

  /* try to start playing the song */
  if(bt_song_play(song)) {
    mark_point();
    g_usleep(G_USEC_PER_SEC/10);
    /* stop the song */
    bt_song_stop(song);
  } else {
    fail("playing of network song failed");
  }

  /* remove one machine */
  bt_setup_remove_wire(setup,wire2);
  g_object_unref(wire2);
  GST_DEBUG_OBJECT(gen2,"unlinked machine: %p,ref_count=%d",gen2,G_OBJECT_REF_COUNT(gen2));
  res=bt_sequence_remove_track_by_machine(sequence,gen2);
  fail_unless(res, NULL);
  bt_setup_remove_machine(setup,gen2);
  GST_DEBUG_OBJECT(gen2,"removed machine from sequence: %p,ref_count=%d",gen2,G_OBJECT_REF_COUNT(gen2));
  g_object_unref(gen2);
  mark_point();

  /* try to start playing the song again */
  if(bt_song_play(song)) {
    mark_point();
    g_usleep(G_USEC_PER_SEC/10);
    /* stop the song */
    bt_song_stop(song);
  } else {
    fail("playing song failed again");
  }

  g_object_unref(gen1);
  g_object_unref(sink);
  g_object_unref(wire1);
  g_object_unref(setup);
  g_object_unref(sequence);
  g_object_unref(song_info);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST


/*
* check if we can connect a src machine to a sink while playing
*/
BT_START_TEST(test_btsong_dynamic_add_src) {
  BtApplication *app=NULL;
  GError *err=NULL;
  // the song
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  BtSequence *sequence=NULL;
  // machines
  BtMachine *gen1=NULL,*gen2=NULL;
  BtMachine *sink=NULL;
  // wires
  BtWire *wire1=NULL, *wire2=NULL;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);

  /* get the song setup and sequence */
  g_object_get(song,"setup",&setup,"sequence",&sequence,NULL);
  fail_unless(setup!=NULL, NULL);

  /* try to create the sink */
  sink=BT_MACHINE(bt_sink_machine_new(song,"master",&err));
  fail_unless(sink!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create generator1 */
  gen1=BT_MACHINE(bt_source_machine_new(song,"generator1","audiotestsrc",0,&err));
  fail_unless(gen1!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to link machines */
  wire1=bt_wire_new(song, gen1, sink,&err);
  fail_unless(wire1!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* enlarge the sequence */
  g_object_set(sequence,"length",10L,"tracks",1L,NULL);
  bt_sequence_add_track(sequence,gen1,-1);

  /* try to start playing the song */
  if(bt_song_play(song)) {
    mark_point();
    g_usleep(G_USEC_PER_SEC/10);
    GST_DEBUG("song plays");

    /* try to create generator1 */
    gen2=BT_MACHINE(bt_source_machine_new(song,"generator2","audiotestsrc",0,&err));
    fail_unless(gen2!=NULL, NULL);
    fail_unless(err==NULL, NULL);
    GST_DEBUG("generator added");

    /* try to link machines */
    wire2=bt_wire_new(song, gen2, sink,&err);
    fail_unless(wire2!=NULL, NULL);
    fail_unless(err==NULL, NULL);
    GST_DEBUG("wire added");

    g_usleep(G_USEC_PER_SEC/10);

    /* stop the song */
    bt_song_stop(song);
  } else {
    fail("playing of network song failed");
  }

  g_object_unref(gen1);
  g_object_unref(gen2);
  g_object_unref(sink);
  g_object_unref(wire1);
  g_object_unref(wire2);
  g_object_unref(setup);
  g_object_unref(sequence);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST


/*
* check if we can disconnect a src machine from a sink while playing.
*/
BT_START_TEST(test_btsong_dynamic_rem_src) {
  BtApplication *app=NULL;
  GError *err=NULL;
  // the song
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  BtSequence *sequence=NULL;
  // machines
  BtMachine *gen1=NULL,*gen2=NULL;
  BtMachine *sink=NULL;
  // wires
  BtWire *wire1=NULL, *wire2=NULL;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);

  /* get the song setup and sequence */
  g_object_get(song,"setup",&setup,"sequence",&sequence,NULL);
  fail_unless(setup!=NULL, NULL);

  /* try to create the sink */
  sink=BT_MACHINE(bt_sink_machine_new(song,"master",&err));
  fail_unless(sink!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create generator1 */
  gen1=BT_MACHINE(bt_source_machine_new(song,"generator1","audiotestsrc",0,&err));
  fail_unless(gen1!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to link machines */
  wire1=bt_wire_new(song, gen1, sink,&err);
  fail_unless(wire1!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create generator1 */
  gen2=BT_MACHINE(bt_source_machine_new(song,"generator2","audiotestsrc",0,&err));
  fail_unless(gen2!=NULL, NULL);
  fail_unless(err==NULL, NULL);
  GST_DEBUG("generator added");

  /* try to link machines */
  wire2=bt_wire_new(song, gen2, sink,&err);
  fail_unless(wire2!=NULL, NULL);
  fail_unless(err==NULL, NULL);
  GST_DEBUG("wire added");

  /* enlarge the sequence */
  g_object_set(sequence,"length",10L,"tracks",1L,NULL);
  bt_sequence_add_track(sequence,gen1,-1);

  /* try to start playing the song */
  if(bt_song_play(song)) {
    mark_point();
    g_usleep(G_USEC_PER_SEC/10);
    GST_DEBUG("song plays");

    /* try to unlink machines */
    bt_setup_remove_wire(setup,wire2);
    GST_DEBUG("wire removed");

    g_usleep(G_USEC_PER_SEC/10);

    /* stop the song */
    bt_song_stop(song);
  } else {
    fail("playing of network song failed");
  }

  g_object_unref(gen1);
  g_object_unref(gen2);
  g_object_unref(sink);
  g_object_unref(wire1);
  g_object_unref(wire2);
  g_object_unref(setup);
  g_object_unref(sequence);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/*
 * check if we can connect a processor machine to a src and sink while playing
 */
BT_START_TEST(test_btsong_dynamic_add_proc) {
  BtApplication *app=NULL;
  GError *err=NULL;
  // the song
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  BtSequence *sequence=NULL;
  // machines
  BtMachine *gen1=NULL;
  BtMachine *proc=NULL;
  BtMachine *sink=NULL;
  // wires
  BtWire *wire1=NULL, *wire2=NULL, *wire3=NULL;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);

  /* get the song setup and sequence */
  g_object_get(song,"setup",&setup,"sequence",&sequence,NULL);
  fail_unless(setup!=NULL, NULL);
  fail_unless(sequence!=NULL, NULL);

  /* try to create the sink */
  sink=BT_MACHINE(bt_sink_machine_new(song,"master",&err));
  fail_unless(sink!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create generator1 */
  gen1=BT_MACHINE(bt_source_machine_new(song,"generator1","audiotestsrc",0,&err));
  fail_unless(gen1!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create a wire from gen1 to sink */
  wire1=bt_wire_new(song,gen1,sink,&err);
  fail_unless(wire1!=NULL, NULL);
  fail_unless(err==NULL, NULL);
  GST_DEBUG("wire added");

  /* enlarge the sequence */
  g_object_set(sequence,"length",10L,"tracks",1L,NULL);
  bt_sequence_add_track(sequence,gen1,-1);
  mark_point();

  /* try to start playing the song */
  if(bt_song_play(song)) {
    mark_point();
    g_usleep(G_USEC_PER_SEC/10);
    GST_DEBUG("song plays");

    /* try to create a processor with volume */
    proc=BT_MACHINE(bt_processor_machine_new(song,"processor","volume",0,&err));
    fail_unless(proc!=NULL, NULL);
    fail_unless(err==NULL, NULL);

    /* try to create a wire from gen1 to proc */
    wire2=bt_wire_new(song,gen1,proc,&err);
    fail_unless(wire2!=NULL, NULL);
    fail_unless(err==NULL, NULL);
  
    /* try to create a wire from proc to sink */
    wire3=bt_wire_new(song,proc,sink,&err);
    fail_unless(wire3!=NULL, NULL);
    fail_unless(err==NULL, NULL);

    g_usleep(G_USEC_PER_SEC/10);

    /* stop the song */
    bt_song_stop(song);
  } else {
    fail("playing song failed");
  }

  g_object_unref(gen1);
  g_object_unref(proc);
  g_object_unref(sink);
  g_object_unref(wire1);
  g_object_unref(wire2);
  g_object_unref(wire3);
  g_object_unref(setup);
  g_object_unref(sequence);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/*
 * check if we can disconnect a processor machine to a src and sink while playing
 */
BT_START_TEST(test_btsong_dynamic_rem_proc) {
  BtApplication *app=NULL;
  GError *err=NULL;
  // the song
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  BtSequence *sequence=NULL;
  // machines
  BtMachine *gen1=NULL;
  BtMachine *proc=NULL;
  BtMachine *sink=NULL;
  // wires
  BtWire *wire1=NULL, *wire2=NULL, *wire3=NULL;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);

  /* get the song setup and sequence */
  g_object_get(song,"setup",&setup,"sequence",&sequence,NULL);
  fail_unless(setup!=NULL, NULL);
  fail_unless(sequence!=NULL, NULL);

  /* try to create the sink */
  sink=BT_MACHINE(bt_sink_machine_new(song,"master",&err));
  fail_unless(sink!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create generator1 */
  gen1=BT_MACHINE(bt_source_machine_new(song,"generator1","audiotestsrc",0,&err));
  fail_unless(gen1!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create a wire from gen1 to sink */
  wire1=bt_wire_new(song,gen1,sink,&err);
  fail_unless(wire1!=NULL, NULL);
  fail_unless(err==NULL, NULL);
  GST_DEBUG("wire added");

  /* try to create a processor with volume */
  proc=BT_MACHINE(bt_processor_machine_new(song,"processor","volume",0,&err));
  fail_unless(proc!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create a wire from gen1 to proc */
  wire2=bt_wire_new(song,gen1,proc,&err);
  fail_unless(wire2!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* try to create a wire from proc to sink */
  wire3=bt_wire_new(song,proc,sink,&err);
  fail_unless(wire3!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* enlarge the sequence */
  g_object_set(sequence,"length",10L,"tracks",1L,NULL);
  bt_sequence_add_track(sequence,gen1,-1);
  mark_point();

  /* try to start playing the song */
  if(bt_song_play(song)) {
    mark_point();
    g_usleep(G_USEC_PER_SEC/10);
    GST_DEBUG("song plays");

    /* try to unlink machines */
    bt_setup_remove_wire(setup,wire2);
    GST_DEBUG("wire 2 removed");

    bt_setup_remove_wire(setup,wire3);
    GST_DEBUG("wire 2 removed");

    g_usleep(G_USEC_PER_SEC/10);

    /* stop the song */
    bt_song_stop(song);
  } else {
    fail("playing song failed");
  }

  g_object_unref(gen1);
  g_object_unref(proc);
  g_object_unref(sink);
  g_object_unref(wire1);
  g_object_unref(wire2);
  g_object_unref(wire3);
  g_object_unref(setup);
  g_object_unref(sequence);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/* should we have variants, where we remove the machines instead of the wires? */


TCase *bt_song_example_case(void) {
  TCase *tc = tcase_create("BtSongExamples");

  tcase_add_test(tc,test_btsong_obj1);
  tcase_add_test(tc,test_btsong_load1);
  tcase_add_test(tc,test_btsong_load2);
  tcase_add_test(tc,test_btsong_load3);
  tcase_add_test(tc,test_btsong_play1);
  tcase_add_test(tc,test_btsong_play2);
  tcase_add_test(tc,test_btsong_play3);
  tcase_add_test(tc,test_btsong_play4);
  tcase_add_test(tc,test_btsong_new1);
  tcase_add_test(tc,test_btsong_idle1);
  tcase_add_test(tc,test_btsong_idle2);
  tcase_add_test(tc,test_btsong_static1);
  tcase_add_test(tc,test_btsong_static2);
  tcase_add_test(tc,test_btsong_static3);
  tcase_add_test(tc,test_btsong_static4);
  tcase_add_test(tc,test_btsong_dynamic_add_src);
  tcase_add_test(tc,test_btsong_dynamic_rem_src);
  tcase_add_test(tc,test_btsong_dynamic_add_proc);
  tcase_add_test(tc,test_btsong_dynamic_rem_proc);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
