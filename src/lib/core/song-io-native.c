/* $Id: song-io-native.c,v 1.9 2004-07-02 13:44:50 ensonic Exp $
 * class for native song input and output
 */
 
#define BT_CORE
#define BT_SONG_IO_NATIVE_C

#include <libbtcore/core.h>

struct _BtSongIONativePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
};

//-- xml helper methods

/* @brief test if the given XPathObject is of the expected type, otherwise discard the object
 * @param xpath_optr the xpath object to test
 * @param type the required type
 * @return the supplied xpath object or NULL is types do not match
 */
xmlXPathObjectPtr xpath_type_filter(xmlXPathObjectPtr xpath_optr,const xmlXPathObjectType type) {
	if(xpath_optr && (xpath_optr->type!=type)) {
		GST_ERROR("xpath expr does not returned the expected type: ist=%ld <-> soll=%ld",(unsigned long)xpath_optr->type,(unsigned long)type);
		xmlXPathFreeObject(xpath_optr);
		xpath_optr=NULL;
	}
	return(xpath_optr);
}


/* @brief return the result as xmlXPathObjectPtr of the evaluation of the supplied compiled xpath expression agains the given document
 * @param dialog gitk dialog
 * @param access_type how to access the dialog
 * @param xpath_comp_expression compiled xpath expression to use
 * @param root_node from where to search, uses doc root when NULL
 * @return xpathobject; do not forget to free the result after use
 * @ingroup gitkcore
 */
xmlXPathObjectPtr cxpath_get_object(const xmlDocPtr doc,xmlXPathCompExprPtr const xpath_comp_expression, xmlNodePtr const root_node) {
  xmlXPathObjectPtr result=NULL;
  xmlXPathContextPtr ctxt;

  g_assert(doc!=NULL);

  //gitk_log_intro();
	
	if((ctxt=xmlXPathNewContext(doc))) {
		if(root_node) ctxt->node=root_node;
		else ctxt->node=xmlDocGetRootElement(doc);
		if((!xmlXPathRegisterNs(ctxt,BT_NS_PREFIX,BT_NS_URL))
		&& (!xmlXPathRegisterNs(ctxt,"dc","http://purl.org/dc/elements/1.1/"))) {
			result=xmlXPathCompiledEval(xpath_comp_expression,ctxt);
			xmlXPathRegisteredNsCleanup(ctxt);
		}
		else GST_ERROR("failed to register \"buzztard\" or \"dc\" namespace");
		xmlXPathFreeContext(ctxt);
	}
	else GST_ERROR("failed to get xpath context");
  return(result);
}

//-- helper methods

static gboolean bt_song_io_native_load_song_info(const BtSongIONative *self, const BtSong *song, const xmlDocPtr song_doc) {
	const BtSongInfo *song_info=bt_song_get_song_info(song);
	xmlXPathObjectPtr items_xpoptr;
	xmlNodePtr xml_node,xml_child_node;
	xmlChar *elem;
  GValue val={0,};
	
	GST_INFO("loading the meta-data from the song");

	g_value_init(&val,G_TYPE_STRING);

	// get top xml-node
	if((items_xpoptr=xpath_type_filter(
				cxpath_get_object(song_doc,BT_SONG_IO_NATIVE_GET_CLASS(self)->xpath_get_meta,NULL),
				XPATH_NODESET)))
	{
		gint i;
		xmlNodeSetPtr items=(xmlNodeSetPtr)items_xpoptr->nodesetval;
		gint items_len=xmlXPathNodeSetGetLength(items);

		GST_INFO(" got meta root node with %d items",items_len);
		for(i=0;i<items_len;i++) {
			xml_node=xmlXPathNodeSetItem(items,i);
			if(!xmlNodeIsText(xml_node)) {
				//GST_INFO("  %2d : \"%s\"",i,xml_node->name);
				xml_child_node=xml_node->children;
				if(xml_child_node && xmlNodeIsText(xml_child_node)) {
					if(!xmlIsBlankNode(xml_child_node)) {
						if((elem=xmlNodeGetContent(xml_child_node))) {
							GST_INFO("  %2d : \"%s\"=\"%s\"",i,xml_node->name,elem);
							// maybe we need a hashmap based mapping from xml-tag names to class properties
							g_value_set_string(&val,elem);
							g_object_set_property(G_OBJECT(song_info),xml_node->name, &val);
							xmlFree(elem);
						}
					}
				}
			}
		}
		xmlXPathFreeObject(items_xpoptr);
	}
	return(TRUE);
}

static gboolean bt_song_io_native_load_setup_machines(const BtSongIONative *self, const BtSong *song, xmlNodePtr xml_node) {
	const BtSetup *setup=bt_song_get_setup(song);
	BtMachine *machine;
	xmlChar *id,*plugin_name;
	
	GST_INFO(" got setup.machines root node");
  while(xml_node) {
		if(!xmlNodeIsText(xml_node)) {
			machine=NULL;
			id=xmlGetProp(xml_node,"id");
			plugin_name=xmlGetProp(xml_node,"pluginname");
			if(!strncmp(xml_node->name,"sink\0",5)) {
				GST_INFO("  new sink_machine(\"%s\",\"%s\")",id,plugin_name);
				// parse additional params
				// create new sink machine
				machine=g_object_new(BT_TYPE_SINK_MACHINE,"song",song,"id",id,"plugin_name",plugin_name,NULL);
				// ...
			}
			else if(!strncmp(xml_node->name,"source\0",7)) {
				GST_INFO("  new source_machine(\"%s\",\"%s\")",id,plugin_name);
				// parse additional params
				// create new source machine
				machine=g_object_new(BT_TYPE_SOURCE_MACHINE,"song",song,"id",id,"plugin_name",plugin_name,NULL);
			}
			else if(!strncmp(xml_node->name,"processor\0",10)) {
				GST_INFO("  new processor_machine(\"%s\",\"%s\")",id,plugin_name);
				// parse additional params
				// create new processor machine
				machine=g_object_new(BT_TYPE_PROCESSOR_MACHINE,"song",song,"id",id,"plugin_name",plugin_name,NULL);
			}
			if(machine) { // add machine to setup
				/*{ // DEBUG
					GValue _val={0,};
					g_value_init(&_val,G_TYPE_STRING);
					g_object_get_property(G_OBJECT(machine),"id", &_val);
					g_print("machine.id: \"%s\"\n", g_value_get_string(&_val));
				} // DEBUG*/
				bt_setup_add_machine(setup,machine);
			}
			xmlFree(id);xmlFree(plugin_name);
		}
		xml_node=xml_node->next;
	}
	return(TRUE);
}

static gboolean bt_song_io_native_load_setup_wires(const BtSongIONative *self, const BtSong *song, xmlNodePtr xml_node) {
	const BtSetup *setup=bt_song_get_setup(song);
	BtWire *wire;
	xmlChar *src,*dst;
	BtMachine *src_machine,*dst_machine;

	GST_INFO(" got setup.wires root node");
  while(xml_node) {
		if(!xmlNodeIsText(xml_node)) {
			src=xmlGetProp(xml_node,"src");
			dst=xmlGetProp(xml_node,"dst");
			GST_INFO("  new wire(\"%s\",\"%s\") --------------------",src,dst);
			// parse params
			// create new wire
			src_machine=bt_setup_get_machine_by_id(setup,src);
			dst_machine=bt_setup_get_machine_by_id(setup,dst);
			wire=g_object_new(BT_TYPE_WIRE,"song",song,"src",src_machine,"dst",dst_machine,NULL);
		}
		xml_node=xml_node->next;
	}
	return(TRUE);
}

static gboolean bt_song_io_native_load_setup(const BtSongIONative *self, const BtSong *song, const xmlDocPtr song_doc) {
	//const BtSetup *setup=bt_song_get_setup(song);
	xmlXPathObjectPtr items_xpoptr;
	xmlNodePtr xml_node;
	xmlChar *elem;
	
	GST_INFO("loading the setup-data from the song");

	// get top xml-node
	if((items_xpoptr=xpath_type_filter(
				cxpath_get_object(song_doc,BT_SONG_IO_NATIVE_GET_CLASS(self)->xpath_get_setup,NULL),
				XPATH_NODESET)))
	{
		gint i;
		xmlNodeSetPtr items=(xmlNodeSetPtr)items_xpoptr->nodesetval;
		gint items_len=xmlXPathNodeSetGetLength(items);

		GST_INFO(" got setup root node with %d items",items_len);
		for(i=0;i<items_len;i++) {
			xml_node=xmlXPathNodeSetItem(items,i);
			if(!xmlNodeIsText(xml_node)) {
				if(!strncmp(xml_node->name,"machines\0",8)) {
					bt_song_io_native_load_setup_machines(self,song,xml_node->children);
				}
				else if(!strncmp(xml_node->name,"wires\0",6)) {
					bt_song_io_native_load_setup_wires(self,song,xml_node->children);
				}
			}
		}
		xmlXPathFreeObject(items_xpoptr);
	}
	return(TRUE);
}

static gboolean bt_song_io_native_load_patterns(const BtSongIONative *self, const BtSong *song, const xmlDocPtr song_doc) {
	const BtSetup *setup=bt_song_get_setup(song);
	xmlXPathObjectPtr items_xpoptr;
	xmlNodePtr xml_node,xml_child_node;
	xmlChar *elem;

	GST_INFO("loading the pattern-data from the song");
	// get top xml-node
	if((items_xpoptr=xpath_type_filter(
				cxpath_get_object(song_doc,BT_SONG_IO_NATIVE_GET_CLASS(self)->xpath_get_patterns,NULL),
				XPATH_NODESET)))
	{
		gint i;
		xmlNodeSetPtr items=(xmlNodeSetPtr)items_xpoptr->nodesetval;
		gint items_len=xmlXPathNodeSetGetLength(items);

		GST_INFO(" got pattern root node with %d items",items_len);
		for(i=0;i<items_len;i++) {
			xml_node=xmlXPathNodeSetItem(items,i);
			if(!xmlNodeIsText(xml_node)) {
				// todo
			}
		}
		xmlXPathFreeObject(items_xpoptr);
	}
	return(TRUE);
}

static gboolean bt_song_io_native_load_sequence(const BtSongIONative *self, const BtSong *song, const xmlDocPtr song_doc) {
	const BtSequence *sequence=bt_song_get_sequence(song);
	xmlXPathObjectPtr items_xpoptr;
	xmlNodePtr xml_node,xml_child_node;
	xmlChar *elem;
	
	GST_INFO("loading the sequence-data from the song");

	// get top xml-node
	if((items_xpoptr=xpath_type_filter(
				cxpath_get_object(song_doc,BT_SONG_IO_NATIVE_GET_CLASS(self)->xpath_get_sequence,NULL),
				XPATH_NODESET)))
	{
		gint i;
		xmlNodeSetPtr items=(xmlNodeSetPtr)items_xpoptr->nodesetval;
		gint items_len=xmlXPathNodeSetGetLength(items);

		GST_INFO(" got sequence root node with %d items",items_len);
		for(i=0;i<items_len;i++) {
			xml_node=xmlXPathNodeSetItem(items,i);
			if(!xmlNodeIsText(xml_node)) {
				// todo
			}
		}
		xmlXPathFreeObject(items_xpoptr);
	}
	return(TRUE);
}

//-- methods

gboolean bt_song_io_native_real_load(const gpointer _self, const BtSong *song, const gchar *filename) {
	const BtSongIONative *self=BT_SONG_IO_NATIVE(_self);
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
			else if((ns=xmlSearchNsByHref(song_doc,xml_node,(const xmlChar *)BT_NS_URL))==NULL) {
				GST_WARNING("no or incorrect namespace found in xmlDoc");
			}
			else if(xmlStrcmp(xml_node->name,(const xmlChar *)"buzztard")) {
				GST_WARNING("wrong document type root node in xmlDoc src");
			}
			else {
				GST_INFO("file looks good!");
				bt_song_io_native_load_song_info(self,song,song_doc);
				bt_song_io_native_load_setup(    self,song,song_doc);
				bt_song_io_native_load_patterns( self,song,song_doc);
				bt_song_io_native_load_sequence( self,song,song_doc);
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

static void bt_song_io_native_class_init(BtSongIONativeClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	BtSongIOClass *base_class = BT_SONG_IO_CLASS(klass);
	
  gobject_class->set_property = bt_song_io_native_set_property;
  gobject_class->get_property = bt_song_io_native_get_property;
  gobject_class->dispose      = bt_song_io_native_dispose;
  gobject_class->finalize     = bt_song_io_native_finalize;
	
  /* implement virtual class function. */
	base_class->load       = bt_song_io_native_real_load;
	
	/* compile xpath-expressions */
	klass->xpath_get_meta = xmlXPathCompile("/"BT_NS_PREFIX":buzztard/"BT_NS_PREFIX":meta/"BT_NS_PREFIX":*");
	g_assert(klass->xpath_get_meta);
	klass->xpath_get_setup = xmlXPathCompile("/"BT_NS_PREFIX":buzztard/"BT_NS_PREFIX":setup/"BT_NS_PREFIX":*");
	g_assert(klass->xpath_get_setup);
	klass->xpath_get_patterns = xmlXPathCompile("/"BT_NS_PREFIX":buzztard/"BT_NS_PREFIX":patterns/"BT_NS_PREFIX":*");
	g_assert(klass->xpath_get_patterns);
	klass->xpath_get_sequence = xmlXPathCompile("/"BT_NS_PREFIX":buzztard/"BT_NS_PREFIX":sequence/"BT_NS_PREFIX":*");
	g_assert(klass->xpath_get_sequence);
}

/* as ob gobject documentation static types are keept alive untile the program ends.
   therefore we do not free shared class-data
static void bt_song_io_native_class_finalize(BtSongIOClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	xmlXPathFreeCompExpr(gobject_class->xpath_get_meta);
	gobject_class->xpath_get_meta=NULL;
}
*/

GType bt_song_io_native_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtSongIONativeClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_song_io_native_class_init, // class_init
			NULL, // class_finalize
      //(GClassFinalizeFunc)bt_song_io_native_class_finalize,
      NULL, // class_data
      sizeof (BtSongIONative),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_song_io_native_init, // instance_init
    };
		type = g_type_register_static(BT_TYPE_SONG_IO,"BtSongIONative",&info,0);
  }
  return type;
}

