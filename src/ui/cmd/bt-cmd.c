/* $Id: bt-cmd.c,v 1.2 2004-05-04 15:24:59 ensonic Exp $
 * You can try to run the uninstalled program via
 *   libtool --mode=execute bt-cmd <filename>
 * to enable debugging add e.g. --gst-debug="*:2,bt-*:3"
*/

#include <stdio.h>

#include <libbtcore/core.h>
#include <glib.h>
#include <glib-object.h>

#define GST_CAT_DEFAULT bt_cmd_debug
GST_DEBUG_CATEGORY_STATIC(GST_CAT_DEFAULT);

static void print_usage(void) {
	puts("Usage: bt-cmd <song-filename>");
	exit(1);
}

/**
* signal callback funktion
*/
static void play_event(void) {
  g_print("start play invoked per signal\n");
}

int main(int argc, char **argv) {
	BtSong *song;
	// init buzztard core
	bt_init(&argc,&argv);
	GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "bt-cmd", 0, "music production environment / commands");

	if(argc==1) print_usage();
	
	song = (BtSong *)g_object_new(BT_SONG_TYPE,"name","first buzztard song", NULL);

	bt_song_load(song,argv[1]);

	/* connection play signal and invoking the play_event function */
  g_signal_connect(G_OBJECT(song), "play", (GCallback)play_event, NULL);
	bt_song_start_play(song);

	/* free song */
	g_object_unref(G_OBJECT(song));
	
	return(0);
}
