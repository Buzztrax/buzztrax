/** $Id: song.h,v 1.2 2004-04-21 14:39:02 ensonic Exp $
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
	// all used machines
	GList *machines;
	// add used connections
	GList *connections;
};

#endif /* BT_SONG_H */

