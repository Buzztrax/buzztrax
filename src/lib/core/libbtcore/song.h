/** $Id: song.h,v 1.4 2004-04-30 15:19:00 ensonic Exp $
 * song
 *
 */
 
#ifndef BT_SONG_H
#define BT_SONG_H

typedef struct _BtSong BtSong;
typedef BtSong *BtSongPtr;
struct _BtSong {
	// the main bin that holds all children elements
	GstElement *thread;
	// the element that has the clock
	GstElement *master;
	
	// setup data
	// all used machines
	GList *machines;
	// add used connections
	GList *connections;
	
	// sequence data
	// @todo the song-sequence needs to be here
	// @todo the size data (sequence-length, number of tracks( needs to be here
};

#endif /* BT_SONG_H */

