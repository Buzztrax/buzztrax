/** $Id: song.c,v 1.2 2004-04-21 14:38:59 ensonic Exp $
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

void bt_song_destroy(BtSongPtr song) {
	/* we don't need a reference to these objects anymore */
	gst_object_unref(GST_OBJECT(song->thread));
	/** @todo iterate over song->connections, song->machines and free the memory */
}

GstBin *bt_song_get_bin(BtSongPtr song) {
	return(GST_BIN(song->thread));
}

void bt_song_set_master(BtSongPtr song, BtMachinePtr master) {
	song->master=master->machine;
}

gboolean bt_song_play(BtSongPtr song) {
	return(gst_element_set_state(song->thread,GST_STATE_PLAYING)!=GST_STATE_FAILURE);
}

gboolean bt_song_stop(BtSongPtr song) {
	return(gst_element_set_state(song->thread,GST_STATE_NULL)!=GST_STATE_FAILURE);
}

gboolean bt_song_pause(BtSongPtr song) {
	return(gst_element_set_state(song->thread,GST_STATE_PAUSED)!=GST_STATE_FAILURE);
}

// debugging
void bt_song_store_as_xml(BtSongPtr song, gchar *file_name) {
	FILE *tmp=fopen(file_name,"wb");
	if(tmp) {
		gst_xml_write_file(song->thread,tmp);
		fclose(tmp);
	}
}

