/* $Id: t-setup.c,v 1.28 2005-11-27 22:44:47 ensonic Exp $
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

//-- tests

BT_START_TEST(test_btsetup_properties) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  gboolean check_prop_ret=FALSE;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);
  
  check_prop_ret=check_gobject_properties(G_OBJECT(setup));
  fail_unless(check_prop_ret==TRUE,NULL);

  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/**
* Try to add the same machine twice to the setup
*/
BT_START_TEST(test_btsetup_obj1){
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  BtMachine *machine=NULL;
  gboolean res;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);
  
  /* create a machine (already adds the machine) */
  machine=BT_MACHINE(bt_source_machine_new(song,"gen","buzztard-test-mono-source",0));
  fail_unless(machine != NULL, NULL);
  
  /* try to add machine again */
  res=bt_setup_add_machine(setup,machine);
  fail_unless(res==FALSE, NULL);
  
  /* try to free the ressources */
  g_object_unref(machine);
  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/**
* try to add the same wire twice
*/
BT_START_TEST(test_btsetup_obj2){
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  BtMachine *source=NULL;
  BtMachine *sink=NULL;
  BtWire *wire1,*wire2;
  gboolean res;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);
  
  /* create the source machine */
  source=BT_MACHINE(bt_source_machine_new(song,"gen","audiotestsrc",0));
  
  /* create the sink machine */
  sink=BT_MACHINE(bt_sink_machine_new(song,"sink"));
  
  /* try to create the wire */
  wire1=bt_wire_new(song,source,sink);
  fail_unless(wire1 != NULL, NULL);
  
  /* try to add again the same wire */
  wire2=bt_wire_new(song,source,sink);
  fail_unless(wire2 == NULL, NULL);
  
  /* try to add again the same wire */
  res=bt_setup_add_wire(setup,wire1);
  fail_unless(res==FALSE, NULL);

  /* try to free the ressources */
  g_object_unref(wire1);
  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/**
* try to create a new setup with a NULL song object
*/
BT_START_TEST(test_btsetup_obj3) {
  BtSetup *setup=NULL;
  
  check_init_error_trapp("bt_setup_new","BT_IS_SONG(song)");
  setup=bt_setup_new(NULL);
  fail_unless(check_has_error_trapped(), NULL);
}
BT_END_TEST

/**
* try to call bt_setup_add_machine with NULL object for self
*/
BT_START_TEST(test_btsetup_obj4) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL,NULL);

  check_init_error_trapp("bt_setup_add_machine","BT_IS_SETUP(self)");
  bt_setup_add_machine(NULL,NULL);
  fail_unless(check_has_error_trapped(), NULL);

  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/**
* try to call bt_setup_add_machine with NULL object for machine
*/
BT_START_TEST(test_btsetup_obj5) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL,NULL);

  check_init_error_trapp("bt_setup_add_machine","BT_IS_MACHINE(machine)");
  bt_setup_add_machine(setup,NULL);
  fail_unless(check_has_error_trapped(), NULL);

  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/**
* try to call bt_setup_add_wire with NULL object for self
*/
BT_START_TEST(test_btsetup_obj6) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL,NULL);

  check_init_error_trapp("bt_setup_add_wire","BT_IS_SETUP(self)");
  bt_setup_add_wire(NULL,NULL);
  fail_unless(check_has_error_trapped(), NULL);

  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/**
* try to call bt_setup_add_wire with NULL object for wire
*/
BT_START_TEST(test_btsetup_obj7) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL,NULL);

  check_init_error_trapp("bt_setup_add_wire","BT_IS_WIRE(wire)");
  bt_setup_add_wire(setup,NULL);
  fail_unless(check_has_error_trapped(), NULL);

  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/**
* try to call bt_setup_get_machine_by_id with NULL object for self
*/
BT_START_TEST(test_btsetup_obj8) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL,NULL);

  check_init_error_trapp("bt_setup_get_machine_by_id","BT_IS_SETUP(self)");
  bt_setup_get_machine_by_id(NULL,NULL);
  fail_unless(check_has_error_trapped(), NULL);

  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/**
* try to call bt_setup_get_machine_by_id with NULL object for id
*/
BT_START_TEST(test_btsetup_obj9) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL,NULL);

  check_init_error_trapp("bt_setup_get_machine_by_id",NULL);
  bt_setup_get_machine_by_id(setup,NULL);
  fail_unless(check_has_error_trapped(), NULL);

  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/**
* try to call get_machine_by_index with NULL for setup parameter
*/
BT_START_TEST(test_btsetup_obj10) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL,NULL);
  
  check_init_error_trapp("bt_setup_get_machine_by_index","BT_IS_SETUP(self)");
  bt_setup_get_machine_by_index(NULL,0);
  fail_unless(check_has_error_trapped(), NULL);

  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/**
* try to call bt_setup_get_wire_by_src_machine with NULL for setup parameter 
*/
BT_START_TEST(test_btsetup_get_wire_by_src_machine1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL,NULL);
  
  check_init_error_trapp("bt_setup_get_wire_by_src_machine","BT_IS_SETUP(self)");
  bt_setup_get_wire_by_src_machine(NULL,NULL);
  fail_unless(check_has_error_trapped(), NULL);

  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/**
* try to call bt_setup_get_wire_by_src_machine with NULL for machine parameter 
*/
BT_START_TEST(test_btsetup_get_wire_by_src_machine2) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL,NULL);
  
  check_init_error_trapp("bt_setup_get_wire_by_src_machine","BT_IS_MACHINE(src)");
  bt_setup_get_wire_by_src_machine(setup,NULL);
  fail_unless(check_has_error_trapped(), NULL);

  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/**
* try to get wires by source machine with NULL for setup
*/
BT_START_TEST(test_btsetup_get_wires_by_src_machine1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  BtSourceMachine *src_machine=NULL;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL,NULL);
  
  /* create source machine */
  src_machine=bt_source_machine_new(song,"src","buzztard-test-mono-source",0);
  
  /* try to get the wires */
  check_init_error_trapp("bt_setup_get_wires_by_src_machine","BT_IS_SETUP(self)");
  bt_setup_get_wires_by_src_machine(NULL,BT_MACHINE(src_machine));
  fail_unless(check_has_error_trapped(), NULL);

  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST


/**
* try to get wires by source machine with NULL for machine
*/
BT_START_TEST(test_btsetup_get_wires_by_src_machine2) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL,NULL);
  
  /* try to get the wires */
  check_init_error_trapp("bt_setup_get_wires_by_src_machine","BT_IS_MACHINE(src)");
  bt_setup_get_wires_by_src_machine(setup,NULL);
  fail_unless(check_has_error_trapped(), NULL);

  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST


/**
* try to get wires by source machine with never added machine
*/
BT_START_TEST(test_btsetup_get_wires_by_src_machine3) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  GList *wire_list=NULL;
  BtSourceMachine *src_machine=NULL;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL,NULL);
  
  /* create source machine */
  src_machine=bt_source_machine_new(song,"src","buzztard-test-mono-source",0);
  
  /* remove machine from setup */
  bt_setup_remove_machine(setup, BT_MACHINE(src_machine));
  
  /* try to get the wires */
  wire_list=bt_setup_get_wires_by_src_machine(setup,BT_MACHINE(src_machine));
  fail_unless(wire_list==NULL,NULL);

  g_object_unref(src_machine);
  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/**
* try to call bt_setup_get_wire_by_dst_machine with NULL for setup parameter 
*/
BT_START_TEST(test_btsetup_get_wire_by_dst_machine1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL,NULL);
  
  check_init_error_trapp("bt_setup_get_wire_by_dst_machine","BT_IS_SETUP(self)");
  bt_setup_get_wire_by_dst_machine(NULL,NULL);
  fail_unless(check_has_error_trapped(), NULL);

  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/**
* try to call bt_setup_get_wire_by_dst_machine with NULL for machine parameter 
*/
BT_START_TEST(test_btsetup_get_wire_by_dst_machine2) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL,NULL);
  
  check_init_error_trapp("bt_setup_get_wire_by_dst_machine","BT_IS_MACHINE(dst)");
  bt_setup_get_wire_by_dst_machine(setup,NULL);
  fail_unless(check_has_error_trapped(), NULL);

  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/**
* try to get wires by destination machine with NULL for setup
*/
BT_START_TEST(test_btsetup_get_wires_by_dst_machine1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  BtSinkMachine *dst_machine=NULL;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);
  
  /* create sink machine */
  dst_machine=bt_sink_machine_new(song,"dst");
  
  /* try to get the wires */
  check_init_error_trapp("bt_setup_get_wires_by_dst_machine","BT_IS_SETUP(self)");
  bt_setup_get_wires_by_dst_machine(NULL,BT_MACHINE(dst_machine));
  fail_unless(check_has_error_trapped(), NULL);

  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST


/**
* try to get wires by sink machine with NULL for machine
*/
BT_START_TEST(test_btsetup_get_wires_by_dst_machine2) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);
  
  /* try to get the wires */
  check_init_error_trapp("bt_setup_get_wires_by_dst_machine","BT_IS_MACHINE(dst)");
  bt_setup_get_wires_by_dst_machine(setup,NULL);
  fail_unless(check_has_error_trapped(), NULL);

  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST


/**
* try to get wires by sink machine with never added machine
*/
BT_START_TEST(test_btsetup_get_wires_by_dst_machine3) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  GList *wire_list=NULL;
  BtSinkMachine *dst_machine=NULL;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  g_object_get(song,"setup",&setup,NULL);
  
  /* create sink machine */
  dst_machine=bt_sink_machine_new(song,"dst");
  
  /* remove machine from setup */
  bt_setup_remove_machine(setup, BT_MACHINE(dst_machine));
  
  /* try to get the wires */
  wire_list=bt_setup_get_wires_by_dst_machine(setup,BT_MACHINE(dst_machine));
  fail_unless(wire_list==NULL,NULL);

  g_object_unref(dst_machine);
  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST


/**
* try to remove a machine from setup with NULL pointer for setup
*/
BT_START_TEST(test_btsetup_obj13) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  fail_unless(song!=NULL,NULL);
  
  check_init_error_trapp("bt_setup_remove_machine",NULL);
  bt_setup_remove_machine(NULL,NULL);
  fail_unless(check_has_error_trapped(), NULL);

  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/**
* try to remove a wire from setup with NULL pointer for setup
*/
BT_START_TEST(test_btsetup_obj14) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  fail_unless(song!=NULL,NULL);
  
  check_init_error_trapp("bt_setup_remove_wire",NULL);
  bt_setup_remove_wire(setup,NULL);
  fail_unless(check_has_error_trapped(), NULL);

  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST


/**
* try to remove a machine from setup with NULL pointer for machine
*/
BT_START_TEST(test_btsetup_obj15) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL,NULL);
  
  check_init_error_trapp("bt_setup_remove_machine",NULL);
  bt_setup_remove_machine(setup,NULL);
  fail_unless(check_has_error_trapped(), NULL);

  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/**
* try to remove a wire from setup with NULL pointer for wire
*/
BT_START_TEST(test_btsetup_obj16) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL,NULL);
  
  check_init_error_trapp("bt_setup_remove_wire",NULL);
  bt_setup_remove_wire(setup,NULL);
  fail_unless(check_has_error_trapped(), NULL);

  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/**
* try to remove a machine from setup with a machine witch is never added
*/
BT_START_TEST(test_btsetup_obj17) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  BtMachine *gen=NULL;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL,NULL);
  
  /* try to create generator1 */
  gen = BT_MACHINE(bt_source_machine_new(song,"src","buzztard-test-mono-source",0));
  fail_unless(gen!=NULL, NULL);
  /* and remove it from setup */
  bt_setup_remove_machine(setup,gen);
  
  check_init_error_trapp("bt_setup_remove_machine",
     "trying to remove machine that is not in setup");
  bt_setup_remove_machine(setup,gen);
  fail_unless(check_has_error_trapped(), NULL);

  g_object_unref(gen);
  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/**
* try to remove a wire from setup with a wire witch is never added
*/
BT_START_TEST(test_btsetup_obj18) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  // machines
  BtSourceMachine *source=NULL;
  BtSinkMachine *sink=NULL;
  // wire
  BtWire *wire=NULL;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL,NULL);
  
  /* try to create generator1 */
  source = bt_source_machine_new(song,"src","audiotestsrc",0);
  fail_unless(source!=NULL, NULL);
  
  /* try to create sink machine */
  sink = bt_sink_machine_new(song,"sink");
  fail_unless(sink!=NULL, NULL);
  
  /* try to create the wire */
  wire = bt_wire_new(song, BT_MACHINE(source), BT_MACHINE(sink));
  fail_unless(wire!=NULL, NULL);
  /* and remove it from setup */
  bt_setup_remove_wire(setup,wire);
  
  check_init_error_trapp("bt_setup_remove_wire",
     "trying to remove wire that is not in setup");
  bt_setup_remove_wire(setup,wire);
  fail_unless(check_has_error_trapped(), NULL);

  g_object_unref(source);
  g_object_unref(sink);
  g_object_unref(wire);
  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/**
* try to add wire(src,dst) and wire(dst,src) to setup. This should fail (cycle).
*/
BT_START_TEST(test_btsetup_wire1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  // machines
  BtMachine *src=NULL;
  BtMachine *dst=NULL;
  // wire
  BtWire *wire_one=NULL;
  BtWire *wire_two=NULL;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL,NULL);
  
  /* try to craete volume machine */
  src = BT_MACHINE(bt_processor_machine_new(song,"src","volume",0));
  fail_unless(src!=NULL, NULL);
  
  /* try to create volume machine */
  dst = BT_MACHINE(bt_processor_machine_new(song,"dst","volume",0));
  fail_unless(dst!=NULL, NULL);
  
  /* try to create the wire one */
  wire_one = bt_wire_new(song,src, dst);
  fail_unless(wire_one!=NULL, NULL);
  
  /* this should fail */
  wire_two = bt_wire_new(song, dst, src);
  fail_unless(wire_two==NULL,NULL);
  
  g_object_unref(src);
  g_object_unref(dst);
  g_object_unref(wire_one);
  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/**
* try to add wire(dst,src) and wire(src,dst) to setup. This should fail (cycle).
*/
BT_START_TEST(test_btsetup_wire2) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  // machines
  BtMachine *src=NULL;
  BtMachine *dst=NULL;
  // wire
  BtWire *wire_one=NULL;
  BtWire *wire_two=NULL;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL,NULL);
  
  /* try to craete volume machine */
  src = BT_MACHINE(bt_processor_machine_new(song,"src","volume",0));
  fail_unless(src!=NULL, NULL);
  
  /* try to create volume machine */
  dst = BT_MACHINE(bt_processor_machine_new(song,"dst","volume",0));
  fail_unless(dst!=NULL, NULL);
  
  /* try to create the wire one */
  wire_one = bt_wire_new(song, dst, src);
  fail_unless(wire_one!=NULL, NULL);
  
  /* this should fail */  
  wire_two = bt_wire_new(song, src, dst);
  fail_unless(wire_two==NULL,NULL);
  
  g_object_unref(src);
  g_object_unref(dst);
  g_object_unref(wire_one);
  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST

/**
* try to add wire(src1,dst), wire(dst,src2) and wire(src2,scr1) to setup. This 
* should fail (cycle).
*/
#ifdef __DISABLED__
BT_START_TEST(test_btsetup_wire3) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSetup *setup=NULL;
  // machines
  BtMachine *elem1=NULL;
  BtMachine *elem2=NULL;
  BtMachine *elem3=NULL;
  // wire
  BtWire *wire_one=NULL;
  BtWire *wire_two=NULL;
  BtWire *wire_three=NULL;
  
  /* create a dummy app */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  fail_unless(app!=NULL,NULL);
  bt_application_new(app);
  
  /* create a new song */
  song=bt_song_new(app);
  fail_unless(song!=NULL,NULL);
  g_object_get(song,"setup",&setup,NULL);
  fail_unless(setup!=NULL,NULL);
  
  /* try to craete volume machine */
  elem1 = BT_MACHINE(bt_processor_machine_new(song,"elem1","volume",0));
  fail_unless(elem1!=NULL, NULL);
  
  /* try to craete volume machine */
  elem2 = BT_MACHINE(bt_processor_machine_new(song,"elem2","volume",0));
  fail_unless(elem2!=NULL, NULL);

  /* try to create volume machine */
  elem3 = BT_MACHINE(bt_processor_machine_new(song,"elem3","volume",0));
  fail_unless(elem3!=NULL, NULL);
  
  /* try to create the wire one and two */
  wire_one = bt_wire_new(song, elem1, elem2);
  fail_unless(wire_one!=NULL, NULL);
  
  wire_two = bt_wire_new(song, elem2, elem3);
  fail_unless(wire_two!=NULL,NULL);
  
  /* this should fail (should it?) */
  wire_three = bt_wire_new(song, elem3, elem1);
  fail_unless(wire_three!=NULL,NULL);
  
  g_object_unref(src1);
  g_object_unref(src2);
  g_object_unref(dst);
  g_object_unref(wire_one);
  g_object_unref(wire_two);
  g_object_unref(setup);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST
#endif

TCase *bt_setup_test_case(void) {
  TCase *tc = tcase_create("BtSetupTests");

  tcase_add_test(tc,test_btsetup_properties);
  tcase_add_test(tc,test_btsetup_obj1);
  tcase_add_test(tc,test_btsetup_obj2);
  tcase_add_test(tc,test_btsetup_obj3);
  tcase_add_test(tc,test_btsetup_obj4);
  tcase_add_test(tc,test_btsetup_obj5);
  tcase_add_test(tc,test_btsetup_obj6);
  tcase_add_test(tc,test_btsetup_obj7);
  tcase_add_test(tc,test_btsetup_obj8);
  tcase_add_test(tc,test_btsetup_obj9);
  tcase_add_test(tc,test_btsetup_obj10);
  tcase_add_test(tc,test_btsetup_get_wire_by_src_machine1);
  tcase_add_test(tc,test_btsetup_get_wire_by_src_machine2);
  tcase_add_test(tc,test_btsetup_get_wires_by_src_machine1);
  tcase_add_test(tc,test_btsetup_get_wires_by_src_machine2);
  tcase_add_test(tc,test_btsetup_get_wires_by_src_machine3);
  tcase_add_test(tc,test_btsetup_get_wire_by_dst_machine1);
  tcase_add_test(tc,test_btsetup_get_wire_by_dst_machine2);
  tcase_add_test(tc,test_btsetup_get_wires_by_dst_machine1);
  tcase_add_test(tc,test_btsetup_get_wires_by_dst_machine2);
  tcase_add_test(tc,test_btsetup_get_wires_by_dst_machine3);
  tcase_add_test(tc,test_btsetup_obj13);
  tcase_add_test(tc,test_btsetup_obj14);
  tcase_add_test(tc,test_btsetup_obj15);
  tcase_add_test(tc,test_btsetup_obj16);
  tcase_add_test(tc,test_btsetup_obj17);
  tcase_add_test(tc,test_btsetup_obj18);
  tcase_add_test(tc,test_btsetup_wire1);
  tcase_add_test(tc,test_btsetup_wire2);
  // @todo: make test work
  //tcase_add_test(tc,test_btsetup_wire3);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  // we need to raise the default timeout of 3 seconds (even 15 seems not to be enough)
  tcase_set_timeout(tc, 10);
  return(tc);
}
