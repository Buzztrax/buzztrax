/** $Id: song.c,v 1.1 2004-04-19 18:30:40 ensonic Exp $
 * song 
 *   holds all song related globals
 *
 */

#define BT_CORE
#define BT_SONG_C

#include <libbtcore/core.h>

/** @brief creates new song instance */
BtSongPtr bt_song_new(void) {
	BtSongPtr song=g_new0(BtSong,1);

  /* create a new thread to hold the elements */
  song->thread = gst_thread_new("thread");
  g_assert(song->thread != NULL);

	return(song);
}

