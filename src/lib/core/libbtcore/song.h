/** $Id: song.h,v 1.1 2004-04-19 18:30:42 ensonic Exp $
 * song
 *
 */
 
#ifndef BT_SONG_H
#define BT_SONG_H

typedef struct _BtSong BtSong;
typedef BtSong *BtSongPtr;
struct _BtSong {
	// the main bin
	GstElement *thread;
	// all used machines
	GList *machines;
	// add used connections
	GList *connections;
};

#ifndef BT_SONG_C
	extern BtSongPtr bt_song_new(void);
#endif

#endif /* BT_SONG_H */

