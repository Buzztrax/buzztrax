
#include "song.h"

int main(void) {
  BtSong *bt_song;
  
  g_type_init();
  bt_song=(BtSong*)g_object_new(BT_SONG_TYPE,NULL);
  //g_print("%d \n",song_get_type());
  return (0);
}
