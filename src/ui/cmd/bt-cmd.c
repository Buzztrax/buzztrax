/* $Id: bt-cmd.c,v 1.5 2004-05-11 20:01:24 ensonic Exp $
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
 * play_event:
 * signal callback funktion
 */
static void play_event(void) {
  GST_INFO("start play invoked per signal\n");
}

int main(int argc, char **argv) {
	BtSong *song;
	BtSongIO *loader;
	GValue val={0,};
	gchar *filename;

	// init buzztard core
	bt_init(&argc,&argv);
	GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "bt-cmd", 0, "music production environment / commands");

	if(argc==1) print_usage();
	filename=argv[1];
	
	song = (BtSong *)g_object_new(BT_SONG_TYPE,"name","first buzztard song", NULL);
	loader = (BtSongIO *)g_object_new(bt_song_io_detect(filename),NULL);
	
	GST_INFO("objects initialized");
	
	//if(bt_song_load(song,filename)) {
	if(bt_song_io_load(loader,song,filename)) {
		/* print some info about the song */
		g_value_init(&val,G_TYPE_STRING);
		g_object_get_property(G_OBJECT(song),"name", &val);
		g_print("song.name: \"%s\"\n", g_value_get_string(&val));
		g_object_get_property(G_OBJECT(bt_song_get_song_info(song)),"info", &val);
		g_print("song.song_info.info: \"%s\"\n", g_value_get_string(&val));
		
		/* connection play signal and invoking the play_event function */
		g_signal_connect(G_OBJECT(song), "play", (GCallback)play_event, NULL);
		bt_song_start_play(song);
	}
	else {
		GST_ERROR("could not load song \"%s\"",filename);
	}
	
	/* free song */
	g_object_unref(G_OBJECT(song));
	
	return(0);
}
