/** $Id: song.c,v 1.10 2004-05-05 13:53:15 ensonic Exp $
 * song 
 *   holds all song related globals
 *
 */

#define BT_CORE
#define BT_SONG_C

#include <libbtcore/core.h>

enum {
  SONG_NAME=1
};

struct _BtSongPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  /* the name for the song */
  gchar *name;
	
	BtSongInfo* song_info; 
	/*
	BtSetup*    setup;
	BtSequence* sequence;
	*/
};

//-- methods

static gboolean bt_song_real_load(const BtSong *self, const gchar *filename) {
	gboolean result=FALSE;
	xmlParserCtxtPtr ctxt=NULL;
	xmlDocPtr song_doc=NULL;
	xmlNsPtr ns=NULL;

	g_assert(filename!=NULL);
	g_assert(*filename);

	GST_INFO("will now load song from \"%s\"",filename);
	
	if((ctxt=xmlCreateFileParserCtxt(filename))) {
		ctxt->validate=FALSE;
		xmlParseDocument(ctxt);
		if(!ctxt->valid) {
			GST_WARNING("the supplied document is not a XML/Buzztard document");
		}
		else if(!ctxt->wellFormed) {
			GST_WARNING("the supplied document is not a wellformed XML document");
		}
		else {
			xmlNodePtr cur;
			song_doc=ctxt->myDoc;
			if((cur=xmlDocGetRootElement(song_doc))==NULL) {
				GST_WARNING("xmlDoc is empty");
			}
			else if((ns=xmlSearchNsByHref(song_doc,cur,(const xmlChar *)BUZZTARD_NS_URL))==NULL) {
				GST_WARNING("no or incorrect namespace found in xmlDoc");
			}
			else if(xmlStrcmp(cur->name,(const xmlChar *)"buzztard")) {
				GST_WARNING("wrong document type root node in xmlDoc src");
			}
			else {
				xmlNodePtr *xml_node;
				GST_INFO("file looks good!");
				// get meta-node
				// xml_node=("//buzztard/meta");
				bt_song_info_load(self->private->song_info,xml_node);
				// get setup-node
				// bt_setup_load(self->setup,xml_node);
				// get sequence-node
				// bt_sequence_load(self->sequence,xml_node);
				// ... patterns
				result=TRUE;
			}
		}		
	}
	else GST_ERROR("failed to create file-parser context for \"%s\"",filename);
	if(ctxt) xmlFreeParserCtxt(ctxt);
	if(song_doc) xmlFreeDoc(song_doc);
	return(result);
}

static void bt_song_real_start_play(const BtSong *self) {
  /* emitting signal if we start play */
  g_signal_emit(self, 
                BT_SONG_GET_CLASS(self)->play_signal_id,
                0,
                NULL);
}

//-- wrapper

gboolean bt_song_load(const BtSong *self, const gchar *filename) {
	return(BT_SONG_GET_CLASS(self)->load(self,filename));
}

/* wrapper method from song
 * @todo inline the wrapper?
 */
void bt_song_start_play(const BtSong *self) {
  BT_SONG_GET_CLASS(self)->start_play(self);
}

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_song_get_property (GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtSong *self = (BtSong *)object;
  return_if_disposed();
  switch (property_id) {
    case SONG_NAME: {
      g_value_set_string(value, self->private->name);
    } break;
    default: {
      g_assert(FALSE);
      break;
    }
  }
}

/* sets the given properties for this object */
static void bt_song_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtSong *self = (BtSong *)object;
  return_if_disposed();
  switch (property_id) {
    case SONG_NAME: {
      g_free(self->private->name);
      self->private->name = g_value_dup_string(value);
      g_print("set the name for song: %s\n",self->private->name);
    } break;
    default: {
      g_assert(FALSE);
      break;
    }
  }
}

static void bt_song_dispose(GObject *object) {
  BtSong *self = (BtSong *)object;
	return_if_disposed();
  self->private->dispose_has_run = TRUE;
}

static void bt_song_finalize(GObject *object) {
  BtSong *self = (BtSong *)object;
	g_object_unref(G_OBJECT(self->private->song_info));
	g_free(self->private->name);
  g_free(self->private);
}

static void bt_song_init(GTypeInstance *instance, gpointer g_class) {
  BtSong *self = (BtSong*)instance;
	
	//g_print("song_init self=%p\n",self);
  self->private = g_new0(BtSongPrivate,1);
  self->private->dispose_has_run = FALSE;
	self->private->song_info = (BtSongInfo *)g_object_new(BT_SONG_INFO_TYPE,"song",self,NULL);
}

static void bt_song_class_init(BtSongClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GParamSpec *g_param_spec;
  
  gobject_class->set_property = bt_song_set_property;
  gobject_class->get_property = bt_song_get_property;
  gobject_class->dispose      = bt_song_dispose;
  gobject_class->finalize     = bt_song_finalize;
  
  klass->load       = bt_song_real_load;
  klass->start_play = bt_song_real_start_play;
  
  /* adding simple signal */
  klass->play_signal_id = g_signal_newv("play",
                                        G_TYPE_FROM_CLASS (gobject_class),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                        NULL, // class closure
                                        NULL, // accumulator
                                        NULL, // acc data
                                        g_cclosure_marshal_VOID__VOID,
                                        G_TYPE_NONE, // return type
                                        0, // n_params
                                        NULL /* param data */ );
  
  g_param_spec = g_param_spec_string("name",
                                     "name contruct prop",
                                     "Set songs name",
                                     "unnamed song", /* default value */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE);
                                           
  g_object_class_install_property(gobject_class,
                                 SONG_NAME,
                                 g_param_spec);
}

GType bt_song_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtSongClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_song_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtSong),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_song_init, // instance_init
    };
		type = g_type_register_static(G_TYPE_OBJECT,"BtSongType",&info,0);
  }
  return type;
}

