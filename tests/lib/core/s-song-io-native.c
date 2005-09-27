/* $Id: s-song-io-native.c,v 1.1 2005-09-27 17:59:15 ensonic Exp $
 */

#include "m-bt-core.h"

extern TCase *bt_song_io_native_test_case(void);
//extern TCase *bt_song_io_native_example_case(void);

Suite *bt_song_io_native_suite(void) { 
  Suite *s=suite_create("BtSongIONative"); 

  suite_add_tcase(s,bt_song_io_native_test_case());
  //suite_add_tcase(s,bt_song_io_native_example_case());
  return(s);
}
