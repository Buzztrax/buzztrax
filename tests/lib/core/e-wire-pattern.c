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

//-- fixtures

static void test_setup(void) {
  bt_core_setup();
  GST_INFO("================================================================================");
}

static void test_teardown(void) {
  bt_core_teardown();
}

//-- tests

BT_START_TEST(test_btwirepattern_obj1) {
  BtApplication *app=NULL;
  GError *err=NULL;
  BtSong *song=NULL;
  BtMachine *src_machine=NULL,*sink_machine=NULL;
  BtWire *wire=NULL;
  BtPattern *pattern=NULL;
  BtWirePattern *wire_pattern=NULL;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  fail_unless(song!=NULL, NULL);

  /* create a source machine */
  src_machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0L,&err));
  fail_unless(src_machine!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* create sink machine (default audio sink) */
  sink_machine=BT_MACHINE(bt_sink_machine_new(song,"sink",&err));
  fail_unless(sink_machine!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* create the wire */
  wire = bt_wire_new(song,src_machine,sink_machine,&err);
  fail_unless(wire!=NULL, NULL);
  fail_unless(err==NULL, NULL);
  
  /* create a pattern */
  pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,sink_machine);
  fail_unless(pattern!=NULL, NULL);
  
  /* create a wire-pattern */
  wire_pattern=bt_wire_pattern_new(song,wire,pattern);
  fail_unless(wire_pattern!=NULL, NULL);

  g_object_unref(wire_pattern);
  g_object_unref(pattern);
  g_object_unref(wire);
  g_object_try_unref(src_machine);
  g_object_try_unref(sink_machine);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

BT_START_TEST(test_btwirepattern_obj2) {
  BtApplication *app=NULL;
  GError *err=NULL;
  BtSong *song=NULL;
  BtSequence *sequence;
  BtMachine *src_machine=NULL,*sink_machine=NULL;
  BtWire *wire=NULL;
  BtPattern *pattern=NULL;
  BtWirePattern *wire_pattern=NULL;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  fail_unless(song!=NULL, NULL);
  g_object_get(song,"sequence",&sequence,NULL);

  /* create a source machine */
  src_machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0L,&err));
  fail_unless(src_machine!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* create sink machine (default audio sink) */
  sink_machine=BT_MACHINE(bt_sink_machine_new(song,"sink",&err));
  fail_unless(sink_machine!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* create the wire */
  wire = bt_wire_new(song,src_machine,sink_machine,&err);
  fail_unless(wire!=NULL, NULL);
  fail_unless(err==NULL, NULL);
  
  /* create a pattern */
  pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,sink_machine);
  fail_unless(pattern!=NULL, NULL);
  
  /* create a wire-pattern */
  wire_pattern=bt_wire_pattern_new(song,wire,pattern);
  fail_unless(wire_pattern!=NULL, NULL);
  
   /* enlarge length */
  g_object_set(sequence,"length",4L,NULL);

  /* set machine */
  bt_sequence_add_track(sequence,sink_machine,-1);

  /* set pattern */
  bt_sequence_set_pattern(sequence,0,0,(BtCmdPattern *)pattern);
  
  /* set some test data */
  bt_pattern_set_global_event(pattern,0,0,"50");
  bt_wire_pattern_set_event(wire_pattern,0,0,"100");

  g_object_unref(wire_pattern);
  g_object_unref(pattern);
  g_object_unref(wire);
  g_object_unref(src_machine);
  g_object_unref(sink_machine);
  g_object_unref(sequence);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

BT_START_TEST(test_btwirepattern_copy) {
  BtApplication *app=NULL;
  GError *err=NULL;
  BtSong *song=NULL;
  BtMachine *src_machine=NULL,*sink_machine=NULL;
  BtWire *wire=NULL;
  BtPattern *pattern1=NULL,*pattern2=NULL;
  BtWirePattern *wire_pattern1=NULL,*wire_pattern2=NULL;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  fail_unless(song!=NULL, NULL);

  /* create a source machine */
  src_machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0L,&err));
  fail_unless(src_machine!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* create sink machine (default audio sink) */
  sink_machine=BT_MACHINE(bt_sink_machine_new(song,"sink",&err));
  fail_unless(sink_machine!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* create the wire */
  wire = bt_wire_new(song,src_machine,sink_machine,&err);
  fail_unless(wire!=NULL, NULL);
  fail_unless(err==NULL, NULL);
  
  /* create a pattern */
  pattern1=bt_pattern_new(song,"pattern-id","pattern-name",8L,sink_machine);
  fail_unless(pattern1!=NULL, NULL);
  
  /* create a wire-pattern */
  wire_pattern1=bt_wire_pattern_new(song,wire,pattern1);
  fail_unless(wire_pattern1!=NULL, NULL);
  
  /* set some test data */
  bt_pattern_set_global_event(pattern1,0,0,"50");
  bt_wire_pattern_set_event(wire_pattern1,0,0,"100");
  
  /* create a copy */
  pattern2=bt_pattern_copy(pattern1);
  fail_unless(pattern2!=NULL, NULL);
  fail_unless(pattern2!=pattern1, NULL);

  /* get the copied wire_pattern */
  wire_pattern2=bt_wire_get_pattern(wire,pattern2);
  fail_unless(wire_pattern2!=NULL, NULL);
  fail_unless(wire_pattern2!=wire_pattern1, NULL);

  g_object_unref(wire_pattern2);
  g_object_unref(pattern2);
  g_object_unref(wire_pattern1);
  g_object_unref(pattern1);
  g_object_unref(wire);
  g_object_try_unref(src_machine);
  g_object_try_unref(sink_machine);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

BT_START_TEST(test_btwirepattern_enlarge_length) {
  BtApplication *app=NULL;
  GError *err=NULL;
  BtSong *song=NULL;
  BtMachine *src_machine=NULL,*sink_machine=NULL;
  BtWire *wire=NULL;
  BtPattern *pattern=NULL;
  BtWirePattern *wire_pattern=NULL;
  gulong length;
  gchar *data;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  fail_unless(song!=NULL, NULL);

  /* create a source machine */
  src_machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0L,&err));
  fail_unless(src_machine!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* create sink machine (default audio sink) */
  sink_machine=BT_MACHINE(bt_sink_machine_new(song,"sink",&err));
  fail_unless(sink_machine!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* create the wire */
  wire = bt_wire_new(song,src_machine,sink_machine,&err);
  fail_unless(wire!=NULL, NULL);
  fail_unless(err==NULL, NULL);
  
  /* create a pattern */
  pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,sink_machine);
  fail_unless(pattern!=NULL, NULL);
  
  /* create a wire-pattern */
  wire_pattern=bt_wire_pattern_new(song,wire,pattern);
  fail_unless(wire_pattern!=NULL, NULL);
    
  /* set some test data */
  bt_pattern_set_global_event(pattern,0,0,"50");
  bt_wire_pattern_set_event(wire_pattern,0,0,"100");

  /* change and verify length */
  g_object_set(pattern,"length",16L,NULL);
  g_object_get(pattern,"length",&length,NULL);
  fail_unless(length==16, NULL);

  /* verify data */
  data=bt_wire_pattern_get_event(wire_pattern,0,0);
  fail_unless(data!=NULL, NULL);
  fail_if(strncmp(data,"100",1),"data is '%s' instead of '100'",data);
  g_free(data);

  data=bt_wire_pattern_get_event(wire_pattern,4,0);
  fail_unless(data==NULL, "data is '%s' instead of ''",data);

  g_object_unref(wire_pattern);
  g_object_unref(pattern);
  g_object_unref(wire);
  g_object_unref(src_machine);
  g_object_unref(sink_machine);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

BT_START_TEST(test_btwirepattern_shrink_length) {
  BtApplication *app=NULL;
  GError *err=NULL;
  BtSong *song=NULL;
  BtMachine *src_machine=NULL,*sink_machine=NULL;
  BtWire *wire=NULL;
  BtPattern *pattern=NULL;
  BtWirePattern *wire_pattern=NULL;
  gulong length;
  gchar *data;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  fail_unless(song!=NULL, NULL);

  /* create a source machine */
  src_machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0L,&err));
  fail_unless(src_machine!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* create sink machine (default audio sink) */
  sink_machine=BT_MACHINE(bt_sink_machine_new(song,"sink",&err));
  fail_unless(sink_machine!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* create the wire */
  wire = bt_wire_new(song,src_machine,sink_machine,&err);
  fail_unless(wire!=NULL, NULL);
  fail_unless(err==NULL, NULL);
  
  /* create a pattern */
  pattern=bt_pattern_new(song,"pattern-id","pattern-name",16L,sink_machine);
  fail_unless(pattern!=NULL, NULL);
  
  /* create a wire-pattern */
  wire_pattern=bt_wire_pattern_new(song,wire,pattern);
  fail_unless(wire_pattern!=NULL, NULL);
    
  /* set some test data */
  bt_pattern_set_global_event(pattern,0,0,"50");
  bt_wire_pattern_set_event(wire_pattern,0,0,"100");
  bt_wire_pattern_set_event(wire_pattern,12,0,"15");

  /* change and verify length */
  g_object_set(pattern,"length",8L,NULL);
  g_object_get(pattern,"length",&length,NULL);
  fail_unless(length==8, NULL);

  /* verify data */
  data=bt_wire_pattern_get_event(wire_pattern,0,0);
  fail_unless(data!=NULL, NULL);
  fail_if(strncmp(data,"100",1),"data is '%s' instead of '100'",data);
  g_free(data);

  data=bt_wire_pattern_get_event(wire_pattern,4,0);
  fail_unless(data==NULL, "data is '%s' instead of ''",data);

  g_object_unref(wire_pattern);
  g_object_unref(pattern);
  g_object_unref(wire);
  g_object_unref(src_machine);
  g_object_unref(sink_machine);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

BT_START_TEST(test_btwirepattern_insert_row) {
  BtApplication *app=NULL;
  GError *err=NULL;
  BtSong *song=NULL;
  BtMachine *src_machine=NULL,*sink_machine=NULL;
  BtWire *wire=NULL;
  BtPattern *pattern=NULL;
  BtWirePattern *wire_pattern=NULL;
  gchar *data;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  fail_unless(song!=NULL, NULL);

  /* create a source machine */
  src_machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0L,&err));
  fail_unless(src_machine!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* create sink machine (default audio sink) */
  sink_machine=BT_MACHINE(bt_sink_machine_new(song,"sink",&err));
  fail_unless(sink_machine!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* create the wire */
  wire = bt_wire_new(song,src_machine,sink_machine,&err);
  fail_unless(wire!=NULL, NULL);
  fail_unless(err==NULL, NULL);
  
  /* create a pattern */
  pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,sink_machine);
  fail_unless(pattern!=NULL, NULL);
  
  /* create a wire-pattern */
  wire_pattern=bt_wire_pattern_new(song,wire,pattern);
  fail_unless(wire_pattern!=NULL, NULL);
    
  /* set some test data */
  bt_pattern_set_global_event(pattern,0,0,"50");
  bt_wire_pattern_set_event(wire_pattern,0,0,"100");

  /* insert row */
  bt_wire_pattern_insert_row(wire_pattern,0,0);
  
  /* verify data */
  data=bt_wire_pattern_get_event(wire_pattern,0,0);
  fail_unless(data==NULL, "data is '%s' instead of ''",data);
  data=bt_wire_pattern_get_event(wire_pattern,1,0);
  fail_unless(data!=NULL, NULL);
  fail_if(strncmp(data,"100",1),"data is '%s' instead of '100'",data);
  g_free(data);

  /* cleanup */
  g_object_unref(wire_pattern);
  g_object_unref(pattern);
  g_object_unref(wire);
  g_object_unref(src_machine);
  g_object_unref(sink_machine);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

BT_START_TEST(test_btwirepattern_delete_row) {
  BtApplication *app=NULL;
  GError *err=NULL;
  BtSong *song=NULL;
  BtMachine *src_machine=NULL,*sink_machine=NULL;
  BtWire *wire=NULL;
  BtPattern *pattern=NULL;
  BtWirePattern *wire_pattern=NULL;
  gchar *data;

  /* create app and song */
  app=bt_test_application_new();
  song=bt_song_new(app);
  fail_unless(song!=NULL, NULL);

  /* create a source machine */
  src_machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0L,&err));
  fail_unless(src_machine!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* create sink machine (default audio sink) */
  sink_machine=BT_MACHINE(bt_sink_machine_new(song,"sink",&err));
  fail_unless(sink_machine!=NULL, NULL);
  fail_unless(err==NULL, NULL);

  /* create the wire */
  wire = bt_wire_new(song,src_machine,sink_machine,&err);
  fail_unless(wire!=NULL, NULL);
  fail_unless(err==NULL, NULL);
  
  /* create a pattern */
  pattern=bt_pattern_new(song,"pattern-id","pattern-name",8L,sink_machine);
  fail_unless(pattern!=NULL, NULL);
  
  /* create a wire-pattern */
  wire_pattern=bt_wire_pattern_new(song,wire,pattern);
  fail_unless(wire_pattern!=NULL, NULL);
    
  /* set some test data */
  bt_pattern_set_global_event(pattern,0,0,"50");
  bt_wire_pattern_set_event(wire_pattern,0,0,"100");
  bt_wire_pattern_set_event(wire_pattern,4,0,"15");

  /* insert row */
  bt_wire_pattern_delete_row(wire_pattern,0,0);
  
  /* verify data */
  data=bt_wire_pattern_get_event(wire_pattern,0,0);
  fail_unless(data==NULL, "data is '%s' instead of ''",data);
  data=bt_wire_pattern_get_event(wire_pattern,3,0);
  fail_unless(data!=NULL, NULL);
  fail_if(strncmp(data,"15",1),"data is '%s' instead of '15'",data);
  g_free(data);

  /* cleanup */
  g_object_unref(wire_pattern);
  g_object_unref(pattern);
  g_object_unref(wire);
  g_object_unref(src_machine);
  g_object_unref(sink_machine);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST


TCase *bt_wire_pattern_example_case(void) {
  TCase *tc = tcase_create("BtWirePatternExamples");

  tcase_add_test(tc,test_btwirepattern_obj1);
  tcase_add_test(tc,test_btwirepattern_obj2);
  tcase_add_test(tc,test_btwirepattern_copy);
  tcase_add_test(tc,test_btwirepattern_enlarge_length);
  tcase_add_test(tc,test_btwirepattern_shrink_length);
  tcase_add_test(tc,test_btwirepattern_insert_row);
  tcase_add_test(tc,test_btwirepattern_delete_row);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
