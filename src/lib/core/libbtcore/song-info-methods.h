/* $Id: song-info-methods.h,v 1.2 2004-05-05 13:53:25 ensonic Exp $
* defines all public methods of the song-info class
*/

#ifndef BT_SONG_INFO_METHODS_H
#define BT_SONG_INFO_METHODS_H

#include "song-info.h"

/** load the song-info from a xml-node
 */
gboolean bt_song_info_load(const BtSongInfo *self, const xmlNodePtr xml_node);

#endif // BT_SONG_INFO_METHDOS_H
