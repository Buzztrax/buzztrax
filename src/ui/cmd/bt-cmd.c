/* $Id: bt-cmd.c,v 1.1 2004-05-03 17:41:56 waffel Exp $
*
*/

#include <stdio.h>

#include <libbtcore/core.h>
#include <glib.h>
#include <glib-object.h>

#define GST_CAT_DEFAULT bt_cmd_debug
GST_DEBUG_CATEGORY_STATIC(GST_CAT_DEFAULT);

/**
* signal callback funktion
*/
void static play_event(void) {
  g_print("start play invoked per signal\n");
}

int main(int argc, char **argv) {
	BtSong *song;
	// init buzztard core
	bt_init(&argc,&argv);
	GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "bt-cmd", 0, "music production environment / commands");
	
	song = (BtSong *)g_object_new(BT_SONG_TYPE,"name","first buzztard song", NULL);
	/* connection play signal and invoking the play_event function */
  g_signal_connect(G_OBJECT(song), "play",
                   (GCallback)play_event,
                   NULL);
	bt_song_start_play(song);
	g_object_unref(G_OBJECT(song));
	
	return(0);
}
