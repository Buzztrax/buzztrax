
#include "song.h"

int main(void) {
  BtSong *song1, *song2;
  
  GValue val={0,};
  
  g_type_init();
  g_value_init(&val,G_TYPE_STRING);
  
  song1=(BtSong*)g_object_new(BT_SONG_TYPE,NULL);
  g_object_get_property(G_OBJECT(song1),"name", &val);
  g_print("song1 has name: %s\n", g_value_get_string(&val));
  
  song2=(BtSong*)g_object_new(BT_SONG_TYPE,"name","my song",NULL);
  g_object_get_property(G_OBJECT(song2),"name", &val);
  g_print("song2 has name: %s\n", g_value_get_string(&val));
  
  return (0);
}
