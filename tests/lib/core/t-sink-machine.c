/* $Id: t-sink-machine.c,v 1.10 2006-08-24 20:00:55 ensonic Exp $
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

//-- tests

BT_START_TEST(test_btsinkmachine_settings1) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSinkMachine *machine=NULL;
  BtSettings *settings=NULL;
  gchar *saved_audiosink_name;
  
  /* create a dummy app, song and get settings */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  song=bt_song_new(app);
  settings=bt_settings_new();
  mark_point();
  
  g_object_get(settings,"audiosink",&saved_audiosink_name,NULL);
  g_object_set(settings,"audiosink","osssink sync=false",NULL);
  
  machine=bt_sink_machine_new(song,"master");
  fail_unless(machine!=NULL, NULL);
  
  g_object_set(settings,"audiosink",saved_audiosink_name,NULL);
  
  g_object_unref(settings);
  g_free(saved_audiosink_name);
  g_object_unref(machine);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST;

/**
* Try to create a sink machine, if we set the sink property with the gconf 
* properties to the string "audioconvert ! osssink sync=false". This string 
* should be replaced by the sink machine to "ossink" and the machine should be 
* instantiable.
*/
BT_START_TEST(test_btsinkmachine_settings2) {
  BtApplication *app=NULL;
  BtSong *song=NULL;
  BtSinkMachine *machine=NULL;
  BtSettings *settings=NULL;
  gchar *saved_audiosink_name;
  
  /* create a dummy app, song and get settings */
  app=g_object_new(BT_TYPE_APPLICATION,NULL);
  bt_application_new(app);
  /* create a new song */
  song=bt_song_new(app);
  settings=bt_settings_new();
  mark_point();
  
  g_object_get(settings,"audiosink",&saved_audiosink_name,NULL);
  // @todo it crashes after instantiating the element,
  //       no matter wheter its alsasink or osssink 
  g_object_set(settings,"audiosink","audioconvert ! osssink sync=false",NULL);
  //g_object_set(settings,"audiosink","audioconvert ! alsasink sync=false",NULL);
  mark_point();
  
  machine=bt_sink_machine_new(song,"master");
  fail_unless(machine!=NULL, NULL);
  
  g_object_set(settings,"audiosink",saved_audiosink_name,NULL);
  
  g_object_unref(settings);
  g_free(saved_audiosink_name);
  g_object_unref(machine);
  g_object_checked_unref(song);
  g_object_checked_unref(app);
}
BT_END_TEST;

TCase *bt_sink_machine_test_case(void) {
  TCase *tc = tcase_create("BtSinkMachineTests");

  tcase_add_test(tc,test_btsinkmachine_settings1);
  tcase_add_test(tc,test_btsinkmachine_settings2);
  tcase_add_unchecked_fixture(tc, test_setup, test_teardown);
  return(tc);
}
