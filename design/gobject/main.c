
#include "song.h"

/**
* signal callback funktion
*/
void static
play_event (void)
{
  g_print ("start play invoked per signal\n");
}

int
main (void)
{
  BtSong *song1, *song2;

  GValue val = { 0, };

  g_type_init ();
  g_value_init (&val, G_TYPE_STRING);

  song1 = (BtSong *) g_object_new (BT_TYPE_SONG, NULL);
  g_object_get_property (G_OBJECT (song1), "name", &val);
  g_print ("song1 has name: %s\n", g_value_get_string (&val));

  song2 = (BtSong *) g_object_new (BT_TYPE_SONG, "name", "my song", NULL);
  g_object_get_property (G_OBJECT (song2), "name", &val);
  g_print ("song2 has name: %s\n", g_value_get_string (&val));

  /* connection play signal and invoking the play_event function */
  g_signal_connect (G_OBJECT (song2), "play", G_CALLBACK (play_event), NULL);

  bt_song_start_play (song2);

  // this must give a compiler error
  //g_print("name %s\n",song2->private->name);

  /* destroy classes */
  g_object_unref (G_OBJECT (song1));
  g_object_unref (G_OBJECT (song2));
  return (0);
}
