/** $Id: t-network.c,v 1.1 2004-08-17 17:02:17 waffel Exp $
 */

#include "t-core.h"

START_TEST(test_btcore_net0) {
	bt_init(&test_argc,&test_argvptr);
}
END_TEST
/**
* try to check if we can simple connect a sine machine to an sink. The sink can be
* a esdsink or an alsasink. We also try after connecting the machines to start
* play.
*/
START_TEST(test_btcore_net1) {
  // the song
	BtSongPtr song;
	// machines
	BtMachinePtr gen1,sink;
  
  bt_init(&test_argc,&test_argvptr);
  /* create a new song */
  song=bt_song_new();
  /* try to create the esd sink */
	sink=bt_machine_new(song,"esdsink","master");
  if (sink == NULL) {
    /* try to create an alsa sink */
    sink=bt_machine_new(song,"alsasink","master");
  }
  /* if no sink can found */ 
  if (sink == NULL) {
    /* dispose the song object */
    bt_song_destroy(song);
    fail("no applicable sink found");
  }
  
  /* try to craete generator1 with sinesrc */
  gen1 = bt_machine_new(song,"sinesrc","generator1");
  if (gen1 == NULL) {
    bt_song_destroy(song);
    fail("no sinesrc generator found");
  }
  
  bt_song_set_master(song,sink);
  
  /* try to link machines */
	if(!(bt_connection_new(song,gen1,sink))) {
		fail("can't connect machines\n");
	}
	
  /* try to start playing the song */
  if(bt_song_play(song)) {
		/* stop the song */
		bt_song_stop(song);
	} else {
    fail("playing of network song failed");
  }
}
END_TEST

/**
* try to check if we can connect two sine machines to one sink. The sink can be
* a esdsink or an alsasink. We also try after connecting the machines to start
* play
*/
START_TEST(test_btcore_net2) {
  // the song
	BtSongPtr song;
	// machines
	BtMachinePtr gen1,gen2,sink;
  
  bt_init(&test_argc,&test_argvptr);
  /* create a new song */
  song=bt_song_new();
  /* try to create the esd sink */
	sink=bt_machine_new(song,"esdsink","master");
  if (sink == NULL) {
    /* try to create an alsa sink */
    sink=bt_machine_new(song,"alsasink","master");
  }
  /* if no sink can found */ 
  if (sink == NULL) {
    /* dispose the song object */
    bt_song_destroy(song);
    fail("no applicable sink found");
  }
  
  /* try to craete generator1 with sinesrc */
  gen1 = bt_machine_new(song,"sinesrc","generator1");
  if (gen1 == NULL) {
    bt_song_destroy(song);
    fail("no sinesrc generator found");
  }
  
  /* try to craete generator2 with sinesrc */
  gen2 = bt_machine_new(song,"sinesrc","generator2");
  if (gen2 == NULL) {
    bt_song_destroy(song);
    fail("no sinesrc generator found");
  }
  
  bt_song_set_master(song,sink);
  
  /* try to link machines */
	if(!(bt_connection_new(song,gen1,sink))) {
		fail("can't connect machines\n");
	}
	
  /* try to link machines */
	if(!(bt_connection_new(song,gen2,sink))) {
		fail("can't connect machines\n");
	}
  
  /* try to start playing the song */
  if(bt_song_play(song)) {
		/* stop the song */
		bt_song_stop(song);
	} else {
    fail("playing of network song failed");
  }
}
END_TEST


TCase *libbtcore_network_tcase(void) {
  TCase *tc = tcase_create("Network");

  tcase_add_test(tc,test_btcore_net0);
  tcase_add_test(tc,test_btcore_net1);
  tcase_add_test(tc,test_btcore_net2);
	//tcase_add_unchecked_fixture (tc, test_core_setup, test_core_teardown);
  return(tc);
}
