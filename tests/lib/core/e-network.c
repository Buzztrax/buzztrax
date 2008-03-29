/* $Id: e-network.c,v 1.25 2007-04-30 14:15:26 ensonic Exp $
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

#include "m-bt-core.h"

//-- globals

//-- fixtures

static void test_setup(void) {
  bt_core_setup();
  GST_INFO("================================================================================");
}

static void test_teardown(void) {
  bt_core_teardown();
  //puts(__FILE__":teardown");
}

/**
* try to check if we can simple connect a sine machine to an sink. We also try
* to start play after connecting the machines.
*/
BT_START_TEST(test_btcore_net_example1) {
  BtApplication *app=NULL;
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

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);

  /* create a new song */
  song=bt_song_new(app);

  /* get the song setup and sequence */
  g_object_get(song,"setup",&setup,"sequence",&sequence,NULL);
  fail_unless(setup!=NULL, NULL);

  /* try to create the sink */
  sink=BT_MACHINE(bt_sink_machine_new(song,"master"));
  fail_unless(sink!=NULL, NULL);

  /* try to create generator1 */
  gen1=BT_MACHINE(bt_source_machine_new(song,"generator1","audiotestsrc",0));
  fail_unless(gen1!=NULL, NULL);

  /* check if we can retrieve the machine via the id */
  machine=bt_setup_get_machine_by_id(setup,"master");
  fail_unless(machine==sink, NULL);
  g_object_unref(machine);

  machine=bt_setup_get_machine_by_id(setup,"generator1");
  fail_unless(machine==gen1, NULL);
  g_object_unref(machine);

  /* try to link machines */
  wire1=bt_wire_new(song, gen1, sink);
  fail_unless(wire1!=NULL, NULL);

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
  bt_sequence_add_track(sequence,gen1);

  /* try to start playing the song */
  if(bt_song_play(song)) {
    mark_point();
    sleep(1);
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

/**
 * try to check if we can connect two sine machines to one sink. We also try
 * to start play after connecting the machines.
 */
BT_START_TEST(test_btcore_net_example2) {
  BtApplication *app=NULL;
  // the song
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  BtSequence *sequence=NULL;
  // machines
  BtMachine *gen1=NULL,*gen2=NULL;
  BtMachine *sink=NULL;
  // wires
  BtWire *wire1=NULL, *wire2=NULL;

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);

  /* create a new song */
  song=bt_song_new(app);

  /* get the song setup and sequence */
  g_object_get(song,"setup",&setup,"sequence",&sequence,NULL);
  fail_unless(setup!=NULL, NULL);

  /* try to create the sink */
  sink=BT_MACHINE(bt_sink_machine_new(song,"master"));
  fail_unless(sink!=NULL, NULL);

  /* try to create generator1 */
  gen1=BT_MACHINE(bt_source_machine_new(song,"generator1","audiotestsrc",0));
  fail_unless(gen1!=NULL, NULL);

  /* try to create generator2 */
  gen2=BT_MACHINE(bt_source_machine_new(song,"generator2","audiotestsrc",0));
  fail_unless(gen2!=NULL, NULL);

  /* try to create a wire from gen1 to sink */
  wire1=bt_wire_new(song, gen1, sink);
  fail_unless(wire1!=NULL, NULL);

  /* try to create a wire from gen2 to sink */
  wire2=bt_wire_new(song, gen2, sink);
  fail_unless(wire2!=NULL, NULL);

  /* enlarge the sequence */
  g_object_set(sequence,"length",1L,"tracks",2L,NULL);
  bt_sequence_add_track(sequence,gen1);
  bt_sequence_add_track(sequence,gen2);
  mark_point();

  /* try to start playing the song */
  if(bt_song_play(song)) {
    mark_point();
    sleep(1);
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

/**
 * try to check if we can connect two sine machines to one effect and this to the
 * sink. We also try to start play after connecting the machines.
 */
BT_START_TEST(test_btcore_net_example3) {
  BtApplication *app=NULL;
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

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);

  /* create a new song */
  song=bt_song_new(app);

  /* get the song setup and sequence */
  g_object_get(song,"setup",&setup,"sequence",&sequence,"song-info",&song_info,NULL);
  fail_unless(setup!=NULL, NULL);
  fail_unless(sequence!=NULL, NULL);
  g_object_set(song_info,"name","btcore_net_example3",NULL);

  /* try to create the sink */
  sink=BT_MACHINE(bt_sink_machine_new(song,"master"));
  fail_unless(sink!=NULL, NULL);

  /* try to create generator1 */
  gen1=BT_MACHINE(bt_source_machine_new(song,"generator1","audiotestsrc",0));
  fail_unless(gen1!=NULL, NULL);

  /* try to create generator2 */
  gen2=BT_MACHINE(bt_source_machine_new(song,"generator2","audiotestsrc",0));
  fail_unless(gen2!=NULL, NULL);

  /* try to create a processor with volume */
  proc=BT_MACHINE(bt_processor_machine_new(song,"processor","volume",0));
  fail_unless(proc!=NULL, NULL);

  /* try to create a wire from gen1 to proc */
  wire1=bt_wire_new(song,gen1,proc);
  fail_unless(wire1!=NULL, NULL);

  /* try to create a wire from gen2 to proc */
  wire2=bt_wire_new(song,gen2,proc);
  fail_unless(wire2!=NULL, NULL);

  /* try to create a wire from proc to sink */
  wire3=bt_wire_new(song,proc,sink);
  fail_unless(wire3!=NULL, NULL);

  /* enlarge the sequence */
  g_object_set(sequence,"length",1L,"tracks",2L,NULL);
  bt_sequence_add_track(sequence,gen1);
  bt_sequence_add_track(sequence,gen2);
  mark_point();

  /* try to start playing the song */
  if(bt_song_play(song)) {
    mark_point();
    sleep(1);
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

/**
 * try to check if we can connect two sine machines to one sink, then play() and
 * stop(). After stopping remove one machine and play again.
 *
 * Not yet ready.
 */
#ifdef __CHECK_DISABLED__
BT_START_TEST(test_btcore_net_example4) {
  BtApplication *app=NULL;
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

  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);

  /* create a new song */
  song=bt_song_new(app);

  /* get the song setup and sequence */
  g_object_get(song,"setup",&setup,"sequence",&sequence,"song-info",&song_info,NULL);
  fail_unless(setup!=NULL, NULL);
  fail_unless(sequence!=NULL, NULL);
  g_object_set(song_info,"name","btcore_net_example3",NULL);

  /* try to create the sink */
  sink=BT_MACHINE(bt_sink_machine_new(song,"master"));
  fail_unless(sink!=NULL, NULL);

  /* try to create generator1 */
  gen1=BT_MACHINE(bt_source_machine_new(song,"generator1","audiotestsrc",0));
  fail_unless(gen1!=NULL, NULL);

  /* try to create generator2 */
  gen2=BT_MACHINE(bt_source_machine_new(song,"generator2","audiotestsrc",0));
  fail_unless(gen2!=NULL, NULL);

  /* try to create a wire from gen1 to proc */
  wire1=bt_wire_new(song,gen1,sink);
  fail_unless(wire1!=NULL, NULL);

  /* try to create a wire from gen2 to proc */
  wire2=bt_wire_new(song,gen2,sink);
  fail_unless(wire2!=NULL, NULL);

  /* enlarge the sequence */
  g_object_set(sequence,"length",1L,"tracks",2L,NULL);
  bt_sequence_add_track(sequence,gen1);
  bt_sequence_add_track(sequence,gen2);
  mark_point();

  /* try to start playing the song */
  if(bt_song_play(song)) {
    mark_point();
    sleep(1);
    /* stop the song */
    bt_song_stop(song);
  } else {
    fail("playing of network song failed");
  }

  /* remove one machine */
  bt_setup_remove_wire(setup,wire2);
  g_object_unref(wire2);
  res=bt_sequence_remove_track_by_machine(sequence,gen2);
  fail_unless(res, NULL);
  bt_setup_remove_machine(setup,gen2);
  g_object_unref(gen2);
  mark_point();

  /* try to start playing the song again */
  if(bt_song_play(song)) {
    mark_point();
    sleep(1);
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
#endif

TCase *bt_network_example_case(void) {
  TCase *tc = tcase_create("BtNetworkExamples");

  tcase_add_test(tc,test_btcore_net_example1);
  tcase_add_test(tc,test_btcore_net_example2);
  tcase_add_test(tc,test_btcore_net_example3);
  /* these tests aren't ready yet
  tcase_add_test(tc,test_btcore_net_example4);
  */
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
