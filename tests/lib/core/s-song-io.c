/* $Id: s-song-io.c,v 1.2 2005-09-27 17:59:15 ensonic Exp $
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
