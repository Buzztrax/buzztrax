/* $Id: cmd-application.c,v 1.34 2004-09-29 14:38:44 ensonic Exp $
 * class for a commandline based buzztard tool application
 */
 
#define BT_CMD
#define BT_CMD_APPLICATION_C

#include "bt-cmd.h"
#include <libbtcore/application-private.h>


// this needs to be here because of gtk-doc and unit-tests
GST_DEBUG_CATEGORY(GST_CAT_DEFAULT);

struct _BtCmdApplicationPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
};

static BtApplicationClass *parent_class=NULL;

//-- helper methods

/**
 * play_event:
 *
 * signal callback funktion
 */
static void on_song_play(const BtSong *song, gpointer user_data) {
  GST_INFO("start playing - invoked per signal : song=%p, user_data=%p",song,user_data);
}

/**
 * stop_event:
 *
 * signal callback funktion
 */
static void on_song_stop(const BtSong *song, gpointer user_data) {
  GST_INFO("stoped playing - invoked per signal : song=%p, user_data=%p",song,user_data);
}

//-- constructor methods

/**
 * bt_cmd_application_new:
 *
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtCmdApplication *bt_cmd_application_new(void) {
  BtCmdApplication *self;
  self=BT_CMD_APPLICATION(g_object_new(BT_TYPE_CMD_APPLICATION,NULL));
  
  // @todo check result
  bt_application_new(BT_APPLICATION(self));
  return(self);
}

//-- methods

/**
 * bt_cmd_application_play:
 * @self: the application instance to run
 * @input_file_name: the file to play
 *
 * load and play the file of the supplied name
 *
 * Returns: true for success
 */
// @todo check if the self pointer is not NULL
gboolean bt_cmd_application_play(const BtCmdApplication *self, const gchar *input_file_name) {
  gboolean res=FALSE;
	BtSong *song=NULL;
	BtSongIO *loader=NULL;
  GstBin *bin=NULL;

	GST_INFO("application.play launched");
  
  if(!is_string(input_file_name)) {
    goto Error;
  }
  // prepare song and song-io
  g_object_get(G_OBJECT(self),"bin",&bin,NULL);
  if(!(song=bt_song_new(bin))) {
    goto Error;
  }
	if(!(loader=bt_song_io_new(input_file_name))) {
    goto Error;
  }
	
	GST_INFO("objects initialized");
	
	if(bt_song_io_load(loader,song)) {
    // connection play and stop signals
		g_signal_connect(G_OBJECT(song), "play", (GCallback)on_song_play, (gpointer)self);
		g_signal_connect(G_OBJECT(song), "stop", (GCallback)on_song_stop, (gpointer)self);
		bt_song_play(song);
    res=TRUE;
	}
	else {
		GST_ERROR("could not load song \"%s\"",input_file_name);
    goto Error;
	}
Error:
  g_object_try_unref(bin);
  g_object_try_unref(song);
  g_object_try_unref(loader);
  return(res);
}

/**
 * bt_cmd_application_info:
 * @self: the application instance to run
 * @input_file_name: the file to print information about
 * @output_file_name: the file to put informations from the input_file_name. 
 * If the given file_name is NULL, stdout is used to print the informations.
 *
 * load the file of the supplied name and print information about it to stdout.
 *
 * Returns: true for success
 */
gboolean bt_cmd_application_info(const BtCmdApplication *self, const gchar *input_file_name, const gchar *output_file_name) {
  gboolean res=FALSE;
	BtSong *song=NULL;
	BtSongIO *loader=NULL;
  GstBin *bin=NULL;
	FILE *output_file=NULL;

	GST_INFO("application.info launched");

  if(!is_string(input_file_name)) {
    goto Error;
  }
  // choose appropriate output
	if (!is_string(output_file_name)) {
		output_file=stdout; 
	} else {
		output_file = fopen(output_file_name,"wb");
	}
  // prepare song and song-io
  g_object_get(G_OBJECT(self),"bin",&bin,NULL);
  if(!(song=bt_song_new(bin))) {
    goto Error;
  }
	if(!(loader=bt_song_io_new(input_file_name))) {
    goto Error;
  }
	
	GST_INFO("objects initialized");
	
	//if(bt_song_load(song,filename)) {
	if(bt_song_io_load(loader,song)) {
    BtSongInfo *song_info;
    BtSequence *sequence;
    BtSetup *setup;
    gchar *name,*info,*id;
    glong length,tracks;
    
    g_object_get(G_OBJECT(song),"song-info",&song_info,"sequence",&sequence,"setup",&setup,NULL);
		/* print some info about the song */
    g_object_get(G_OBJECT(song_info),"name",&name,"info",&info,NULL);
    g_fprintf(output_file,"song.song_info.name: \"%s\"\n",name);g_free(name);
		g_fprintf(output_file,"song.song_info.info: \"%s\"\n",info);g_free(info);
    g_object_get(G_OBJECT(sequence),"length",&length,"tracks",&tracks,NULL);
		g_fprintf(output_file,"song.sequence.length: %d\n",length);
		g_fprintf(output_file,"song.sequence.tracks: %d\n",tracks);
    /* lookup a machine and print some info about it */
    {
      BtMachine *machine=bt_setup_get_machine_by_id(setup,"audio_sink");
      if(machine) {
        g_object_get(G_OBJECT(machine),"id",&id,"plugin_name",&name,NULL);
        g_fprintf(output_file,"machine.id: \"%s\"\n",id);g_free(id);
        g_fprintf(output_file,"machine.plugin_name: \"%s\"\n",name);g_free(name);
      }
    }
    // release the references
    g_object_try_unref(song_info);
    g_object_try_unref(sequence);
    g_object_try_unref(setup);    
    res=TRUE;
	}
	else {
		GST_ERROR("could not load song \"%s\"",input_file_name);
		goto Error;
	}
Error:
  g_object_try_unref(bin);
  g_object_try_unref(song);
  g_object_try_unref(loader);
	if (is_string(output_file_name)) {
		fclose(output_file);
	}
	return(res);
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
 			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
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
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_cmd_application_dispose(GObject *object) {
  BtCmdApplication *self = BT_CMD_APPLICATION(object);

	return_if_disposed();
  self->private->dispose_has_run = TRUE;

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_cmd_application_finalize(GObject *object) {
  BtCmdApplication *self = BT_CMD_APPLICATION(object);

  g_free(self->private);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_cmd_application_init(GTypeInstance *instance, gpointer g_class) {
  BtCmdApplication *self = BT_CMD_APPLICATION(instance);
  self->private = g_new0(BtCmdApplicationPrivate,1);
  self->private->dispose_has_run = FALSE;
}

static void bt_cmd_application_class_init(BtCmdApplicationClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;

  parent_class=g_type_class_ref(BT_TYPE_APPLICATION);
  
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
		type = g_type_register_static(BT_TYPE_APPLICATION,"BtCmdApplication",&info,0);
  }
  return type;
}

