/* $Id: song-info-methods.h,v 1.3 2004-05-05 19:49:57 ensonic Exp $
* defines all public methods of the song-info class
*/

#ifndef BT_SONG_INFO_METHODS_H
#define BT_SONG_INFO_METHODS_H

#include "song-info.h"

/** load the song-info from a xml-node
 */
gboolean bt_song_info_load(const BtSongInfo *self, const xmlDocPtr song_doc);

#endif // BT_SONG_INFO_METHDOS_H
