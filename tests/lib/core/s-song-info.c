/* $Id: s-song-info.c,v 1.2 2006-08-02 19:34:20 ensonic Exp $
 */

#include "m-bt-core.h"

extern TCase *bt_song_info_test_case(void);
extern TCase *bt_song_info_example_case(void);

Suite *bt_song_info_suite(void) { 
  Suite *s=suite_create("BtSongInfo"); 

  suite_add_tcase(s,bt_song_info_test_case());
  suite_add_tcase(s,bt_song_info_example_case());
  return(s);
}
