/** $Id: core.h,v 1.5 2004-04-21 14:38:59 ensonic Exp $*/

#ifndef BT_CORE_H
#define BT_CORE_H

#undef GST_DISABLE_GST_DEBUG

#include <glib.h>
#include <gst/gst.h>
#include <gst/control/control.h>

#include "song.h"
#include "network.h"
#include "pattern.h"
#include "version.h"

#ifdef BT_CORE
	#define GST_CAT_DEFAULT bt_core_debug
	#ifndef BT_CORE_C
		GST_DEBUG_CATEGORY_EXTERN(GST_CAT_DEFAULT);
	#endif
#endif

#ifndef BT_CORE_C
	extern void bt_init(int *argc, char ***argv);
#endif

#ifndef BT_SONG_C
	extern BtSongPtr bt_song_new(void);
	extern void bt_song_destroy(BtSongPtr song);

	extern GstBin *bt_song_get_bin(BtSongPtr song);
	extern void bt_song_set_master(BtSongPtr song, BtMachinePtr master);
	
	extern gboolean bt_song_play(BtSongPtr song);
	extern gboolean bt_song_stop(BtSongPtr song);
	extern gboolean bt_song_pause(BtSongPtr song);
	
	extern void bt_song_store_as_xml(BtSongPtr song, gchar *file_name);
#endif

#ifndef BT_NETWORK_C
	extern BtMachinePtr bt_machine_new(BtSongPtr song, gchar *factory, gchar *id);
	extern BtConnectionPtr bt_connection_new(BtSongPtr song, BtMachinePtr src, BtMachinePtr dst);
#endif

#endif /* BT_CORE_H */
