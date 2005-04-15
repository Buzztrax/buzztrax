/* $Id: s-song-io.c,v 1.1 2005-04-15 17:05:14 ensonic Exp $
 */

#include "m-bt-core.h"

extern TCase *bt_song_io_test_case(void);
//extern TCase *bt_song_io_example_case(void);

Suite *bt_song_io_suite(void) { 
  Suite *s=suite_create("BtSongIO"); 

  suite_add_tcase(s,bt_song_io_test_case());
  //suite_add_tcase(s,bt_song_io_example_case());
  return(s);
}
