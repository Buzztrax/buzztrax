/* $Id: s-song.c,v 1.1 2005-04-15 17:05:14 ensonic Exp $
 */

#include "m-bt-core.h"

extern TCase *bt_song_test_case(void);
extern TCase *bt_song_example_case(void);

Suite *bt_song_suite(void) { 
  Suite *s=suite_create("BtSong"); 

  suite_add_tcase(s,bt_song_test_case());
  suite_add_tcase(s,bt_song_example_case());
  return(s);
}
