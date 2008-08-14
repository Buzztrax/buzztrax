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

//-- helper

static void assert_machine_refcount(BtSetup *setup, const gchar *id, guint refs) {
  BtMachine *machine=bt_setup_get_machine_by_id(setup,id);
  
  fail_unless(machine!=NULL,NULL);
  GST_INFO("setup.machine[%s].ref-count=%d",id,G_OBJECT(machine)->ref_count);
  fail_unless(G_OBJECT(machine)->ref_count==(1+refs),NULL);
  g_object_unref(machine);
}

static void assert_song_part_refcounts(BtSong *song) {
  BtSetup *setup;
  BtSequence *sequence;
  BtSongInfo *songinfo;
  BtWavetable *wavetable;

  g_object_get(song,"setup",&setup,"sequence",&sequence,"song-info",&songinfo,"wavetable",&wavetable,NULL);

  fail_unless(setup!=NULL,NULL);
  GST_INFO("setup.ref-count=%d",G_OBJECT(setup)->ref_count);
  fail_unless(G_OBJECT(setup)->ref_count==2,NULL);
  
  fail_unless(sequence!=NULL,NULL);
  GST_INFO("sequence.ref-count=%d",G_OBJECT(sequence)->ref_count);
  fail_unless(G_OBJECT(sequence)->ref_count==2,NULL);
  
  fail_unless(songinfo!=NULL,NULL);
  GST_INFO("songinfo.ref-count=%d",G_OBJECT(songinfo)->ref_count);
  fail_unless(G_OBJECT(songinfo)->ref_count==2,NULL);
  
  fail_unless(wavetable!=NULL,NULL);
  GST_INFO("wavetable.ref-count=%d",G_OBJECT(wavetable)->ref_count);
  fail_unless(G_OBJECT(wavetable)->ref_count==2,NULL);

  g_object_unref(setup);
  g_object_unref(sequence);
  g_object_unref(songinfo);
  g_object_unref(wavetable);  
}

//-- tests

BT_START_TEST(test_btsong_io_native_refcounts) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSongIO *song_io;
  gboolean res;
  BtSetup *setup;
  BtSequence *sequence;
  BtSongInfo *songinfo;
  BtWavetable *wavetable;
 
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  
  /* load the song */
  song_io=bt_song_io_new(check_get_test_song_path("example.xml"));
  fail_unless(song_io != NULL, NULL);
  
  res=bt_song_io_load(song_io,song);
  fail_unless(res == TRUE, NULL);
  GST_INFO("song.ref-count=%d",G_OBJECT(song)->ref_count);
  fail_unless(G_OBJECT(song)->ref_count==1,NULL);
  
  /* assert main song part refcounts */
  g_object_get(song,"setup",&setup,"sequence",&sequence,"song-info",&songinfo,"wavetable",&wavetable,NULL);

  fail_unless(setup!=NULL,NULL);
  GST_INFO("setup.ref-count=%d",G_OBJECT(setup)->ref_count);
  fail_unless(G_OBJECT(setup)->ref_count==2,NULL);
  
  fail_unless(sequence!=NULL,NULL);
  GST_INFO("sequence.ref-count=%d",G_OBJECT(sequence)->ref_count);
  fail_unless(G_OBJECT(sequence)->ref_count==2,NULL);
  
  fail_unless(songinfo!=NULL,NULL);
  GST_INFO("songinfo.ref-count=%d",G_OBJECT(songinfo)->ref_count);
  fail_unless(G_OBJECT(songinfo)->ref_count==2,NULL);
  
  fail_unless(wavetable!=NULL,NULL);
  GST_INFO("wavetable.ref-count=%d",G_OBJECT(wavetable)->ref_count);
  fail_unless(G_OBJECT(wavetable)->ref_count==2,NULL);
  
  /* assert machine refcounts */
  // 1 x setup, 1 x wire
  assert_machine_refcount(setup,"audio_sink",2);
  // 1 x setup, 2 x wire
  assert_machine_refcount(setup,"amp1",3);
  // 1 x setup, 1 x wire, 1 x sequence
  assert_machine_refcount(setup,"sine1",3);

  /* @todo: check more refcounts (wires)
   * grep ".ref-count=" /tmp/bt_core.log
   */

  g_object_unref(setup);
  g_object_unref(sequence);
  g_object_unref(songinfo);
  g_object_unref(wavetable);

  g_object_checked_unref(song_io);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/* for testing we use autoaudiosink and that slows down loging files here :/ */
BT_START_TEST(test_btsong_io_native_song_refcounts) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSongIO *song_io;
  gboolean res;
  GstElement *bin;
  gchar **song_name,*song_names[]={
    "example.xml",
    "test-simple1.xml",
    "test-simple2.xml",
    "test-simple3.xml",
    "test-simple4.xml",
    "test-simple5.xml",
    NULL
  };
 
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  g_object_get(app,"bin",&bin,NULL);
  GST_INFO("song.elements=%d",GST_BIN_NUMCHILDREN(bin));
  fail_unless(GST_BIN_NUMCHILDREN(bin) == 0, NULL);

  song_name=song_names;
  while(*song_name) {
    /* load the song */
    song_io=bt_song_io_new(check_get_test_song_path(*song_name));
    fail_unless(song_io != NULL, NULL);
    song=bt_song_new(app);
    res=bt_song_io_load(song_io,song);
    fail_unless(res == TRUE, NULL);
    GST_INFO("song.ref-count=%d",G_OBJECT(song)->ref_count);
    fail_unless(G_OBJECT(song)->ref_count==1,NULL);
    GST_INFO("song[%s].elements=%d",*song_name,GST_BIN_NUMCHILDREN(bin));
  
    /* assert main song part refcounts */
    assert_song_part_refcounts(song);
  
    g_object_checked_unref(song_io);
    g_object_checked_unref(song);
    GST_INFO("song.elements=%d",GST_BIN_NUMCHILDREN(bin));
    fail_unless(GST_BIN_NUMCHILDREN(bin) == 0, NULL);

    song_name++;
  };

  gst_object_unref(bin);
  g_object_checked_unref(app);
}
BT_END_TEST


TCase *bt_song_io_native_example_case(void) {
  TCase *tc = tcase_create("BtSongIONativeExamples");

  tcase_add_test(tc,test_btsong_io_native_refcounts);
  tcase_add_test(tc,test_btsong_io_native_song_refcounts);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
