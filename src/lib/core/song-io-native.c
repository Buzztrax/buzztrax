/** $Id: song-io-native.c,v 1.1 2004-05-06 18:26:58 ensonic Exp $
 * class for native song input and output
 */
 
#define BT_CORE
#define BT_SONG_IO_NATIVE_C

#include <libbtcore/core.h>
#include <libbtcore/song-io-native.h>

struct _BtSongIONativePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
};

//-- helper methods

static gboolean bt_song_io_native_load_song_info(const BtSongInfo *song_info, const xmlDocPtr song_doc) {
	xmlNodePtr xml_node;
  GValue val={0,};
	
	GST_INFO("loading the meta-data from the song");

	// get meta-node
	// xml_node=("//buzztard/meta");
	// parse xml
	g_value_init(&val,G_TYPE_STRING);
	g_value_set_string(&val,"blabla");
	g_object_set_property(G_OBJECT(song_info),"info", &val);
	
	return(TRUE);
}

//-- methods

gboolean bt_song_io_native_real_load(const BtSongIONative *self, const BtSong *song, const gchar *filename) {
	gboolean result=FALSE;
	xmlParserCtxtPtr ctxt=NULL;
	xmlDocPtr song_doc=NULL;
	xmlNsPtr ns=NULL;

	g_assert(filename!=NULL);
	g_assert(*filename);

	GST_INFO("native loader will now load song from \"%s\"",filename);
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
			xmlNodePtr xml_node;
			song_doc=ctxt->myDoc;
			if((xml_node=xmlDocGetRootElement(song_doc))==NULL) {
				GST_WARNING("xmlDoc is empty");
			}
			else if((ns=xmlSearchNsByHref(song_doc,xml_node,(const xmlChar *)BUZZTARD_NS_URL))==NULL) {
				GST_WARNING("no or incorrect namespace found in xmlDoc");
			}
			else if(xmlStrcmp(xml_node->name,(const xmlChar *)"buzztard")) {
				GST_WARNING("wrong document type root node in xmlDoc src");
			}
			else {
				GST_INFO("file looks good!");
				bt_song_io_native_load_song_info(bt_song_get_song_info(song),song_doc);
				// bt_song_info_load(self->private->song_info,song_doc);
				// bt_setup_load(self->private->setup,song_doc);
				// bt_sequence_load(self->private->sequence,song_doc);
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

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_song_io_native_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtSongIONative *self = BT_SONG_IO_NATIVE(object);
  return_if_disposed();
  switch (property_id) {
    default: {
      g_assert(FALSE);
      break;
    }
  }
}

/* sets the given properties for this object */
static void bt_song_io_native_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtSongIONative *self = BT_SONG_IO_NATIVE(object);
  return_if_disposed();
  switch (property_id) {
    default: {
      g_assert(FALSE);
      break;
    }
  }
}

static void bt_song_io_native_dispose(GObject *object) {
  BtSongIONative *self = BT_SONG_IO_NATIVE(object);
	return_if_disposed();
  self->private->dispose_has_run = TRUE;
}

static void bt_song_io_native_finalize(GObject *object) {
  BtSongIONative *self = BT_SONG_IO_NATIVE(object);
  g_free(self->private);
}

static void bt_song_io_native_init(GTypeInstance *instance, gpointer g_class) {
  BtSongIONative *self = BT_SONG_IO_NATIVE(instance);
  self->private = g_new0(BtSongIONativePrivate,1);
  self->private->dispose_has_run = FALSE;
}

static void bt_song_io_native_class_init(BtSongIOClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	BtSongIOClass *base_class = BT_SONG_IO_CLASS(klass);
  
  gobject_class->set_property = bt_song_io_native_set_property;
  gobject_class->get_property = bt_song_io_native_get_property;
  gobject_class->dispose      = bt_song_io_native_dispose;
  gobject_class->finalize     = bt_song_io_native_finalize;

  /* implement virtual class function. */
	base_class->load       = bt_song_io_native_real_load;
	
	/* compile xpath-expressions */
}

GType bt_song_io_native_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtSongIONativeClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_song_io_native_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtSongIONative),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_song_io_native_init, // instance_init
    };
		type = g_type_register_static(BT_SONG_IO_TYPE,"BtSongIONativeType",&info,0);
  }
  return type;
}

