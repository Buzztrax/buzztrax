/* $Id: cmd-application.c,v 1.3 2004-05-12 21:05:47 ensonic Exp $
 * class for a commandline buzztard based application
 */
 
//#define BT_CORE -> BT_CMD ?
#define BT_CMD_APPLICATION_C

#include "bt-cmd.h"

GST_DEBUG_CATEGORY(GST_CAT_DEFAULT);

struct _BtCmdApplicationPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
};

//-- helper methods

/**
 * print_usage:
 *
 * give short commandline help when no arg has been supplied
 */
static void print_usage(void) {
	puts("Usage: bt-cmd <song-filename>");
}

/**
 * play_event:
 *
 * signal callback funktion
 */
static void play_event(void) {
  GST_INFO("start play invoked per signal\n");
}

//-- methods

gboolean bt_cmd_application_run(const BtCmdApplication *app, int argc, char **argv) {
	BtSong *song;
	BtSongIO *loader;
	GValue sval={0,},oval={0,};
	gchar *filename;

	if(argc==1) {
		print_usage();
		return(FALSE);
	}
	filename=argv[1];

	g_value_init(&oval,G_TYPE_OBJECT);
	g_object_get_property(G_OBJECT(app),"bin", &oval);
		
	// @todo we need to pass the "bin" object from the app to the song
	song = (BtSong *)g_object_new(BT_SONG_TYPE,"name","first buzztard song", NULL);
	loader = (BtSongIO *)g_object_new(bt_song_io_detect(filename),NULL);
	
	GST_INFO("objects initialized");
	
	//if(bt_song_load(song,filename)) {
	if(bt_song_io_load(loader,song,filename)) {
		/* print some info about the song */
		g_value_init(&sval,G_TYPE_STRING);
		g_object_get_property(G_OBJECT(song),"name", &sval);
		g_print("song.name: \"%s\"\n", g_value_get_string(&sval));
		g_object_get_property(G_OBJECT(bt_song_get_song_info(song)),"info", &sval);
		g_print("song.song_info.info: \"%s\"\n", g_value_get_string(&sval));
		
		/* connection play signal and invoking the play_event function */
		g_signal_connect(G_OBJECT(song), "play", (GCallback)play_event, NULL);
		bt_song_start_play(song);
		return(TRUE);
	}
	else {
		GST_ERROR("could not load song \"%s\"",filename);
		return(FALSE);
	}
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_cmd_application_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtCmdApplication *self = BT_CMD_APPLICATION(object);
  return_if_disposed();
  switch (property_id) {
    default: {
 			BtCmdApplicationClass *klass=BT_CMD_APPLICATION_GET_CLASS(object);
			BtApplicationClass *base_klass=BT_APPLICATION_CLASS(klass);
			GObjectClass *base_gobject_class = G_OBJECT_CLASS(base_klass);
			
			base_gobject_class->get_property(object,property_id,value,pspec);
      break;
    }
  }
}

/* sets the given properties for this object */
static void bt_cmd_application_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtCmdApplication *self = BT_CMD_APPLICATION(object);
  return_if_disposed();
  switch (property_id) {
    default: {
			BtCmdApplicationClass *klass=BT_CMD_APPLICATION_GET_CLASS(object);
			BtApplicationClass *base_klass=BT_APPLICATION_CLASS(klass);
			GObjectClass *base_gobject_class = G_OBJECT_CLASS(base_klass);
			
			base_gobject_class->set_property(object,property_id,value,pspec);
      break;
    }
  }
}

static void bt_cmd_application_dispose(GObject *object) {
  BtCmdApplication *self = BT_CMD_APPLICATION(object);
	return_if_disposed();
  self->private->dispose_has_run = TRUE;
}

static void bt_cmd_application_finalize(GObject *object) {
  BtCmdApplication *self = BT_CMD_APPLICATION(object);
  g_free(self->private);
}

static void bt_cmd_application_init(GTypeInstance *instance, gpointer g_class) {
  BtCmdApplication *self = BT_CMD_APPLICATION(instance);
  self->private = g_new0(BtCmdApplicationPrivate,1);
  self->private->dispose_has_run = FALSE;
}

static void bt_cmd_application_class_init(BtCmdApplicationClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;
  
  gobject_class->set_property = bt_cmd_application_set_property;
  gobject_class->get_property = bt_cmd_application_get_property;
  gobject_class->dispose      = bt_cmd_application_dispose;
  gobject_class->finalize     = bt_cmd_application_finalize;
}

GType bt_cmd_application_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtCmdApplicationClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_cmd_application_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtCmdApplication),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_cmd_application_init, // instance_init
    };
		type = g_type_register_static(BT_APPLICATION_TYPE,"BtCmdApplication",&info,0);
  }
  return type;
}

