/* $Id: song-io-native.c,v 1.44 2004-11-26 18:53:26 waffel Exp $
 * class for native song input and output
 */
 
#define BT_CORE
#define BT_SONG_IO_NATIVE_C

#include <libbtcore/core.h>

struct _BtSongIONativePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
};

static BtSongIOClass *parent_class=NULL;

//-- plugin detect
/**
 * bt_song_io_native_detect:
 * @file_name: the file to check against
 *
 * Checks if this plugin should manage this kind of file.
 *
 * Retunrs: the GType of this plugin of NULL
 */
GType bt_song_io_native_detect(const gchar *file_name) {
  GType type=0;
  gchar *lc_file_name;
  
  GST_INFO("file_name=\"%s\"",file_name);
  if(!file_name) return(type);
  
  lc_file_name=g_ascii_strdown(file_name,-1);
  if(g_str_has_suffix(lc_file_name,".xml")) {
    type=BT_TYPE_SONG_IO_NATIVE;
  }
  g_free(lc_file_name);
  return(type);
}

//-- xml helper methods

/**
 * xpath_type_filter:
 * @xpath_optr: the xpath object to test
 * @type: the required type
 *
 * test if the given XPathObject is of the expected type, otherwise discard the object
 * Returns: the supplied xpath object or NULL is types do not match
 */
xmlXPathObjectPtr xpath_type_filter(xmlXPathObjectPtr xpath_optr,const xmlXPathObjectType type) {
	if(xpath_optr && (xpath_optr->type!=type)) {
		GST_ERROR("xpath expr does not returned the expected type: ist=%ld <-> soll=%ld",(unsigned long)xpath_optr->type,(unsigned long)type);
		xmlXPathFreeObject(xpath_optr);
		xpath_optr=NULL;
	}
	return(xpath_optr);
}


/**
 * cxpath_get_object:
 * @doc: gitk dialog
 * @xpath_comp_expression: compiled xpath expression to use
 * @root_node: from where to search, uses doc root when NULL
 *
 * return the result as xmlXPathObjectPtr of the evaluation of the supplied
 * compiled xpath expression agains the given document
 *
 * Returns: the xpathobject; do not forget to free the result after use
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

//-- loader helper methods

static gboolean bt_song_io_native_load_properties(const BtSongIONative *self, const BtSong *song, xmlNodePtr xml_node, GObject *object) {
  xmlNodePtr xml_subnode;
  GHashTable *properties;
  xmlChar *key,*value;
  
  // get property hashtable from object, return if NULL
  g_object_get(object,"properties",&properties,NULL);
  if(!properties) return(TRUE);
  
  // look for <properties> node
  while(xml_node) {
    if((!xmlNodeIsText(xml_node)) && (!strncmp(xml_node->name,"properties\0",11))) {
      GST_DEBUG("  reading properties ...");
      // iterate over children
      xml_subnode=xml_node->children;
      while(xml_subnode) {
        if(!xmlNodeIsText(xml_subnode)) {
          key=xmlGetProp(xml_subnode,"key");
          value=xmlGetProp(xml_subnode,"value");
          GST_DEBUG("    [%s] => [%s]",key,value);
          g_hash_table_insert(properties,key,value);
          // do not free, as the hashtable now owns the memory
          //xmlFree(key);xmlFree(value);
        }
        xml_subnode=xml_subnode->next;
      }
    }
    xml_node=xml_node->next;
  }
  return(TRUE);
}

static gboolean bt_song_io_native_load_song_info(const BtSongIONative *self, const BtSong *song, const xmlDocPtr song_doc) {
	BtSongInfo *song_info;
	xmlXPathObjectPtr items_xpoptr;
	xmlNodePtr xml_node,xml_child_node;
	xmlChar *elem;
	
	GST_INFO("loading the meta-data from the song");
  g_object_get(G_OBJECT(song),"song-info",&song_info,NULL);
	// get top xml-node
	if((items_xpoptr=xpath_type_filter(
				cxpath_get_object(song_doc,BT_SONG_IO_NATIVE_GET_CLASS(self)->xpath_get_meta,NULL),
				XPATH_NODESET)))
	{
		gint i;
		xmlNodeSetPtr items=(xmlNodeSetPtr)items_xpoptr->nodesetval;
		gint items_len=xmlXPathNodeSetGetLength(items);
    const gchar *property_name;

		GST_INFO(" got meta root node with %d items",items_len);
		for(i=0;i<items_len;i++) {
			xml_node=xmlXPathNodeSetItem(items,i);
			if(!xmlNodeIsText(xml_node)) {
				//GST_DEBUG("  %2d : \"%s\"",i,xml_node->name);
				xml_child_node=xml_node->children;
				if(xml_child_node && xmlNodeIsText(xml_child_node)) {
					if(!xmlIsBlankNode(xml_child_node)) {
						if((elem=xmlNodeGetContent(xml_child_node))) {
              property_name=xml_node->name;
							GST_DEBUG("  %2d : \"%s\"=\"%s\"",i,property_name,elem);
              // depending on th name of the property, treat it's type
              if(!strncmp(property_name,"info",4) ||
                !strncmp(property_name,"name",4) ||
                !strncmp(property_name,"genre",5)) {
                g_object_set(G_OBJECT(song_info),property_name,elem,NULL);
              }
              else if(!strncmp(property_name,"bpm",3) ||
                !strncmp(property_name,"tpb",3) ||
                !strncmp(property_name,"bars",4)) {
                g_object_set(G_OBJECT(song_info),property_name,atol(elem),NULL);
              }
							xmlFree(elem);
						}
					}
				}
			}
		}
		xmlXPathFreeObject(items_xpoptr);
	}
  // release the references
  g_object_try_unref(song_info);  
	return(TRUE);
}

static gboolean bt_song_io_native_load_setup_machines(const BtSongIONative *self, const BtSong *song, xmlNodePtr xml_node) {
	BtSetup *setup;
	BtMachine *machine;
	xmlChar *id,*plugin_name,*voices_str;
  glong voices;
	
	GST_INFO(" got setup.machines root node");
  g_object_get(G_OBJECT(song),"setup",&setup,NULL);
  while(xml_node) {
		if(!xmlNodeIsText(xml_node)) {
			machine=NULL;
			id=xmlGetProp(xml_node,"id");
			plugin_name=xmlGetProp(xml_node,"pluginname");
			// parse additional params
      if(voices_str=xmlGetProp(xml_node,"voices")) {
        voices=atol(voices_str);
      }
      else voices=1;
			if(!strncmp(xml_node->name,"sink\0",5)) {
				GST_INFO("  new sink_machine(\"%s\") -----------------",id);
				// create new sink machine
				machine=BT_MACHINE(bt_sink_machine_new(song,id));
			}
			else if(!strncmp(xml_node->name,"source\0",7)) {
				GST_INFO("  new source_machine(\"%s\",\"%s\",%d) -----------------",id,plugin_name,voices);
				// create new source machine
				machine=BT_MACHINE(bt_source_machine_new(song,id,plugin_name,voices));
			}
			else if(!strncmp(xml_node->name,"processor\0",10)) {
				GST_INFO("  new processor_machine(\"%s\",\"%s\",%d) -----------------",id,plugin_name,voices);
				// create new processor machine
				machine=BT_MACHINE(bt_processor_machine_new(song,id,plugin_name,voices));
			}
			if(machine) { // add machine to setup
        bt_song_io_native_load_properties(self,song,xml_node->children,G_OBJECT(machine));
				bt_setup_add_machine(setup,machine);
        g_object_unref(machine);
			}
			xmlFree(id);xmlFree(plugin_name);xmlFree(voices_str);
		}
		xml_node=xml_node->next;
	}
  //-- release the reference
  g_object_try_unref(setup);
	return(TRUE);
}

static gboolean bt_song_io_native_load_setup_wires(const BtSongIONative *self, const BtSong *song, xmlNodePtr xml_node) {
	BtSetup *setup;
	BtWire *wire;
	xmlChar *src,*dst;

	GST_INFO(" got setup.wires root node");
  g_object_get(G_OBJECT(song),"setup",&setup,NULL);
  while(xml_node) {
		if(!xmlNodeIsText(xml_node)) {
			src=xmlGetProp(xml_node,"src");
			dst=xmlGetProp(xml_node,"dst");
			GST_INFO("  new wire(\"%s\",\"%s\") --------------------",src,dst);
			// create new wire
			wire=bt_wire_new(song,bt_setup_get_machine_by_id(setup,src),bt_setup_get_machine_by_id(setup,dst));
      if(wire) {
        bt_setup_add_wire(setup,wire);
        g_object_unref(wire);
      }
      xmlFree(src);xmlFree(dst);
		}
		xml_node=xml_node->next;
	}
  //-- release the reference
  g_object_try_unref(setup);
	return(TRUE);
}

static gboolean bt_song_io_native_load_setup(const BtSongIONative *self, const BtSong *song, const xmlDocPtr song_doc) {
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

static gboolean bt_song_io_native_load_pattern_data(const BtSongIONative *self, const BtPattern *pattern, const xmlDocPtr song_doc, xmlNodePtr xml_node) {
  xmlNodePtr xml_subnode;
  xmlChar *tick_str,*name,*value,*voice_str;
  glong tick,voice,param;
  GValue *event;

  while(xml_node) {
		if((!xmlNodeIsText(xml_node)) && (!strncmp(xml_node->name,"tick\0",5))) {
      tick_str=xmlGetProp(xml_node,"time");
      tick=atoi(tick_str);
      // iterate over children
      xml_subnode=xml_node->children;
      while(xml_subnode) {
        if(!xmlNodeIsText(xml_subnode)) {
          name=xmlGetProp(xml_subnode,"name");
          value=xmlGetProp(xml_subnode,"value");
          if(!strncmp(xml_subnode->name,"globaldata\0",11)) {
            param=bt_pattern_get_global_dparam_index(pattern,name);
            if((event=bt_pattern_get_global_event_data(pattern,tick,param))) {
              bt_pattern_init_global_event(pattern,event,param);
              bt_pattern_set_event(pattern,event,value);
            }
          }
          if(!strncmp(xml_subnode->name,"voicedata\0",10)) {
            voice_str=xmlGetProp(xml_subnode,"voice");
            voice=atol(voice_str);
            param=bt_pattern_get_voice_dparam_index(pattern,name);
            if((event=bt_pattern_get_voice_event_data(pattern,tick,voice,param))) {
              bt_pattern_init_voice_event(pattern,event,param);
              bt_pattern_set_event(pattern,event,value);
            }
            xmlFree(voice_str);
          }
          xmlFree(name);xmlFree(value);
        }
        xml_subnode=xml_subnode->next;
      }
      xmlFree(tick_str);
		}
		xml_node=xml_node->next;
	}
	return(TRUE);
}

static gboolean bt_song_io_native_load_pattern(const BtSongIONative *self, const BtSong *song, const xmlDocPtr song_doc, xmlNodePtr xml_node ) {
	BtSetup *setup;
  BtMachine *machine;
  BtPattern *pattern;
	xmlChar *id,*machine_id,*pattern_name,*length_str;
  glong length,voices;

  g_object_get(G_OBJECT(song),"setup",&setup,NULL);
  id=xmlGetProp(xml_node,"id");
  machine_id=xmlGetProp(xml_node,"machine");
  pattern_name=xmlGetProp(xml_node,"name");
  length_str=xmlGetProp(xml_node,"length");
  length=atol(length_str);
  // get the related machine
  if((machine=bt_setup_get_machine_by_id(setup,machine_id))) {
    g_object_get(G_OBJECT(machine),"voices",&voices,NULL);
    // create pattern, add to machine's pattern-list and load data
    GST_INFO("  new pattern(\"%s\",%d,%d) --------------------",id,length,voices);
    pattern=bt_pattern_new(song,id,pattern_name,length,voices,machine);
    //bt_song_io_native_load_properties(self,song,xml_node->children,pattern);
    bt_song_io_native_load_pattern_data(self,pattern,song_doc,xml_node->children);
    g_object_unref(pattern);
  }
  else {
    GST_ERROR("invalid machine-id=\"%s\"",machine_id);
  }
  xmlFree(id);xmlFree(machine_id);xmlFree(pattern_name);xmlFree(length_str);
  //-- release the reference
  g_object_try_unref(setup);
  return(TRUE);
}

static gboolean bt_song_io_native_load_patterns(const BtSongIONative *self, const BtSong *song, const xmlDocPtr song_doc) {
	xmlXPathObjectPtr items_xpoptr;
	xmlNodePtr xml_node,xml_child_node;

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
        bt_song_io_native_load_pattern(self,song,song_doc,xml_node);
			}
		}
		xmlXPathFreeObject(items_xpoptr);
	}
	return(TRUE);
}

static gboolean bt_song_io_native_get_sequence_length(const BtSongIONative *self, const BtSong *song, const xmlDocPtr song_doc, xmlNodePtr root_node) {
	BtSequence *sequence;
	xmlXPathObjectPtr items_xpoptr;
	xmlNodePtr xml_node,xml_child_node;

  g_object_get(G_OBJECT(song),"sequence",&sequence,NULL);
  if((items_xpoptr=xpath_type_filter(
    cxpath_get_object(song_doc,BT_SONG_IO_NATIVE_GET_CLASS(self)->xpath_get_sequence_length,root_node),
    XPATH_NODESET)))
  {
		gint i;
		xmlNodeSetPtr items=(xmlNodeSetPtr)items_xpoptr->nodesetval;
		gint items_len=xmlXPathNodeSetGetLength(items);
    glong maxtime=0,curtime;

    for(i=0;i<items_len;i++) {
      curtime=atol(xmlNodeGetContent(xmlXPathNodeSetItem(items,i)));
      if(curtime>maxtime) maxtime=curtime;
		}
    maxtime++;  // time values start with 0
    GST_INFO(" got %d sequence.length with a max time of %d",items_len,maxtime);
    g_object_set(G_OBJECT(sequence),"length",maxtime,NULL);
    xmlXPathFreeObject(items_xpoptr);
	}
  // release the references
  g_object_try_unref(sequence);
  return(TRUE);
}

static gboolean bt_song_io_native_load_sequence_labels(const BtSongIONative *self, const BtSong *song, const xmlDocPtr song_doc, xmlNodePtr root_node) {
	BtSequence *sequence;
	xmlXPathObjectPtr items_xpoptr;
	xmlNodePtr xml_node,xml_child_node;

  g_object_get(G_OBJECT(song),"sequence",&sequence,NULL);
  if((items_xpoptr=xpath_type_filter(
    cxpath_get_object(song_doc,BT_SONG_IO_NATIVE_GET_CLASS(self)->xpath_get_sequence_labels,root_node),
    XPATH_NODESET)))
  {
    BtTimeLine *timeline;
    xmlChar *time_str,*name;
		gint i;
		xmlNodeSetPtr items=(xmlNodeSetPtr)items_xpoptr->nodesetval;
		gint items_len=xmlXPathNodeSetGetLength(items);

    GST_INFO(" got sequence.labels root node with %d items",items_len);
    for(i=0;i<items_len;i++) {
      xml_node=xmlXPathNodeSetItem(items,i);
      if((!xmlNodeIsText(xml_node)) && (!strncmp(xml_node->name,"label\0",6))) {
        time_str=xmlGetProp(xml_node,"time");
        name=xmlGetProp(xml_node,"name");
        if((timeline=bt_sequence_get_timeline_by_time(sequence,atol(time_str)))) {
          GST_INFO("  new timeline.label(%s,\"%s\")",time_str,name);
          g_object_set(G_OBJECT(timeline),"label",name,NULL);
        }
        xmlFree(time_str);xmlFree(name);
      }
		}
    xmlXPathFreeObject(items_xpoptr);
	}
  // release the references
  g_object_try_unref(sequence);
  return(TRUE);
}

static gboolean bt_song_io_native_load_sequence_track_data(const BtSongIONative *self, const BtSong *song, const BtMachine *machine, glong index, xmlNodePtr xml_node) {
  BtSequence *sequence;
  BtPattern *pattern;
  BtTimeLine *timeline;
  BtTimeLineTrack *timelinetrack;
  xmlChar *time_str,*pattern_id;

  g_object_get(G_OBJECT(song),"sequence",&sequence,NULL);

  bt_sequence_set_machine_by_track(sequence,index,machine);
  while(xml_node) {
    if(!xmlNodeIsText(xml_node)) {
      time_str=xmlGetProp(xml_node,"time");
      pattern_id=xmlGetProp(xml_node,"pattern");
      // @todo comand=xmlGetProp(xml_node,"comand");
      GST_DEBUG("  at %s, pattern \"%s\"",time_str,pattern_id);
      // get pattern by name from machine
      if((pattern=bt_machine_get_pattern_by_id(machine,pattern_id))) {
        // get timeline from sequence
        if((timeline=bt_sequence_get_timeline_by_time(sequence,atol(time_str)))) {
          // get timelinetrack from timeline
          if((timelinetrack=bt_timeline_get_timelinetrack_by_index(timeline,index))) {
            g_object_set(timelinetrack,"pattern",pattern,"type",BT_TIMELINETRACK_TYPE_PATTERN,NULL);
          }
          else GST_ERROR("  unknown timelinetrack index \"%d\"",index);
        }
        else GST_ERROR("  unknown timeline index \"%s\"",time_str);
      }
      else GST_ERROR("  unknown pattern \"%s\"",pattern_id);
      xmlFree(pattern_id);xmlFree(time_str);
		}
		xml_node=xml_node->next;
	}
  // release the references
  g_object_try_unref(sequence);
	return(TRUE);
}

static gboolean bt_song_io_native_load_sequence_tracks(const BtSongIONative *self, const BtSong *song, const xmlDocPtr song_doc, xmlNodePtr root_node) {
  BtSequence *sequence;
  BtSetup *setup;
	xmlXPathObjectPtr items_xpoptr;
	xmlNodePtr xml_node,xml_child_node;
  xmlChar *index_str,*machine_name;
  BtMachine *machine;

  g_object_get((gpointer)song,"sequence",&sequence,"setup",&setup,NULL);
  if((items_xpoptr=xpath_type_filter(
    cxpath_get_object(song_doc,BT_SONG_IO_NATIVE_GET_CLASS(self)->xpath_get_sequence_tracks,root_node),
    XPATH_NODESET)))
  {
		gint i;
		xmlNodeSetPtr items=(xmlNodeSetPtr)items_xpoptr->nodesetval;
		gint items_len=xmlXPathNodeSetGetLength(items);
    
    g_object_set(sequence,"tracks",(glong)items_len,NULL);

    GST_INFO(" got sequence.tracks root node with %d items",items_len);
    for(i=0;i<items_len;i++) {
      xml_node=xmlXPathNodeSetItem(items,i);
      if((!xmlNodeIsText(xml_node)) && (!strncmp(xml_node->name,"track\0",6))) {
        machine_name=xmlGetProp(xml_node,"machine");
        index_str=xmlGetProp(xml_node,"index");
        if((machine=bt_setup_get_machine_by_id(setup, machine_name))) {
          GST_DEBUG("loading track with index=%s for machine=\"%s\"",index_str,machine_name);
          bt_song_io_native_load_sequence_track_data(self,song,machine,atol(index_str),xml_node->children);
        }
        else {
          GST_ERROR("invalid machine referenced");
        }
        xmlFree(index_str);xmlFree(machine_name);
      }
		}
    xmlXPathFreeObject(items_xpoptr);
	}
  // release the reference
  g_object_try_unref(sequence);
  g_object_try_unref(setup);
  return(TRUE);
}

static gboolean bt_song_io_native_load_sequence(const BtSongIONative *self, const BtSong *song, const xmlDocPtr song_doc) {
	BtSequence *sequence;
	xmlXPathObjectPtr items_xpoptr;
	xmlNodePtr xml_node,xml_child_node;
	xmlChar *id;

	GST_INFO("loading the sequence-data from the song");

  g_object_get((gpointer)song,"sequence",&sequence,NULL);
	// get top xml-node
	if((items_xpoptr=xpath_type_filter(
		cxpath_get_object(song_doc,BT_SONG_IO_NATIVE_GET_CLASS(self)->xpath_get_sequence,NULL),
		XPATH_NODESET)))
	{
		gint i;
		xmlNodeSetPtr items=(xmlNodeSetPtr)items_xpoptr->nodesetval;
		gint items_len=xmlXPathNodeSetGetLength(items);

		GST_INFO(" got sequence root node with %d items",items_len);
    if(items_len==1) {
      xml_node=xmlXPathNodeSetItem(items,0);
      bt_song_io_native_get_sequence_length(self,song,song_doc,xml_node);
            
      bt_song_io_native_load_sequence_labels(self,song,song_doc,xml_node);
      bt_song_io_native_load_sequence_tracks(self,song,song_doc,xml_node);
    }
		xmlXPathFreeObject(items_xpoptr);
	}
  // release the reference
  g_object_try_unref(sequence);
	return(TRUE);
}

//-- loader method

gboolean bt_song_io_native_real_load(const gpointer _self, const BtSong *song) {
	const BtSongIONative *self=BT_SONG_IO_NATIVE(_self);
	gboolean result=FALSE;
	xmlParserCtxtPtr ctxt=NULL;
	xmlDocPtr song_doc=NULL;
	xmlNsPtr ns=NULL;
  gchar *file_name,*status;
  
	g_object_get(G_OBJECT(self),"file-name",&file_name,NULL);
	GST_INFO("native io will now load song from \"%s\"",file_name);

  status=g_strdup_printf(_("Loading file \"%s\""),file_name);
  g_object_set(G_OBJECT(self),"status",status,NULL);
  g_free(status);
  
  //DEBUG
  //sleep(1);
  //DEBUG
  
	// @todo add gnome-vfs detection method. This method should detect the
	// filetype of the given file and returns a gnome-vfs uri to open this
	// file with gnome-vfs. For example if the given file is song.xml the method
	// should return file:/home/waffel/buzztard/songs/song.xml
	
  // @todo add zip file processing
  /*
   * zip_file=bt_zip_file_new(file_name,BT_ZIP_FILE_MODE_READ);
   * xml_doc=bt_zip_file_read_file(zip_file,"song.xml",&xml_doc_size);
   */
  
  // @todo read from zip_file
  /* ctxt=xmlCreateMemoryParserCtxt(xml_doc,xml_doc_size) */
	if((ctxt=xmlCreateFileParserCtxt(file_name))) {
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
        //DEBUG
        bt_song_write_to_xml_file(song);
        //DEBUG
				result=TRUE;
			}
		}		
	}
	else GST_ERROR("failed to create file-parser context for \"%s\"",file_name);
	if(ctxt) xmlFreeParserCtxt(ctxt);
	if(song_doc) xmlFreeDoc(song_doc);
  g_free(file_name);
  //DEBUG
  //sleep(1);
  //DEBUG
  g_object_set(G_OBJECT(self),"status",NULL,NULL);
	return(result);
}

//-- saver helper methods


static gboolean bt_song_io_native_save_song_info(const BtSongIONative *self, const BtSong *song, const xmlDocPtr song_doc,xmlNodePtr *root_node) {
	BtSongInfo *song_info;
	xmlNodePtr xml_node,xml_child_node;
	gchar *name,*genre,*info;
	
	GST_INFO("saving the meta-data to the song");
  g_object_get(G_OBJECT(song),"song-info",&song_info,NULL);
	
	xml_node=xmlNewChild(root_node,NULL,"meta",NULL);
	g_object_get(G_OBJECT(song_info),"name",&name,"genre",&genre,"info",&info,NULL);
  if(info) {
		xmlNewChild(xml_node,NULL,"info",info);
		g_free(info);
	}
  if(name) {
		xmlNewChild(xml_node,NULL,"name",name);
		g_free(name);
	}
  if(genre) {
		xmlNewChild(xml_node,NULL,"genre",genre);
  	g_free(genre);
	}
	g_object_try_unref(song_info);
}

//-- saver method

gboolean bt_song_io_native_real_save(const gpointer _self, const BtSong *song) {
	const BtSongIONative *self=BT_SONG_IO_NATIVE(_self);
	gboolean result=FALSE;
	xmlDocPtr song_doc=NULL;
	xmlNodePtr root_node=NULL;

  gchar *file_name,*status;
  
	g_object_get(G_OBJECT(self),"file-name",&file_name,NULL);
	GST_INFO("native io will now save song to \"%s\"",file_name);

  status=g_strdup_printf(_("Saving file \"%s\""),file_name);
  g_object_set(G_OBJECT(self),"status",status,NULL);
  g_free(status);

	if((song_doc=xmlNewDoc("1.0"))) {
		// create the root-node
    root_node=xmlNewNode(NULL,"buzztard");
		xmlNewProp(root_node,"xmlns",(const xmlChar *)BT_NS_URL);
		xmlNewProp(root_node,"xmlns:xsd","http://www.w3.org/2001/XMLSchema-instance");
		xmlNewProp(root_node,"xsd:noNamespaceSchemaLocation","buzztard.xsd");
    xmlDocSetRootElement(song_doc,root_node);
		// @todo build the xml document tree
		bt_song_io_native_save_song_info(self,song,song_doc,root_node);
		
		if(xmlSaveFile(file_name,song_doc)!=-1) {
			result=TRUE;
		}
	}
	
	g_free(file_name);

  g_object_set(G_OBJECT(self),"status",NULL,NULL);
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
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
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
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_song_io_native_dispose(GObject *object) {
  BtSongIONative *self = BT_SONG_IO_NATIVE(object);

	return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_song_io_native_finalize(GObject *object) {
  BtSongIONative *self = BT_SONG_IO_NATIVE(object);

  GST_DEBUG("!!!! self=%p",self);

  g_free(self->priv);
  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_song_io_native_init(GTypeInstance *instance, gpointer g_class) {
  BtSongIONative *self = BT_SONG_IO_NATIVE(instance);
  self->priv = g_new0(BtSongIONativePrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_song_io_native_class_init(BtSongIONativeClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	BtSongIOClass *base_class = BT_SONG_IO_CLASS(klass);

  parent_class=g_type_class_ref(BT_TYPE_SONG_IO);
	
  gobject_class->set_property = bt_song_io_native_set_property;
  gobject_class->get_property = bt_song_io_native_get_property;
  gobject_class->dispose      = bt_song_io_native_dispose;
  gobject_class->finalize     = bt_song_io_native_finalize;
	
  /* implement virtual class function. */
	base_class->load       = bt_song_io_native_real_load;
	base_class->save       = bt_song_io_native_real_save;
	
	/* compile xpath-expressions */
	klass->xpath_get_meta = xmlXPathCompile("/"BT_NS_PREFIX":buzztard/"BT_NS_PREFIX":meta/"BT_NS_PREFIX":*");
	g_assert(klass->xpath_get_meta);
	klass->xpath_get_setup = xmlXPathCompile("/"BT_NS_PREFIX":buzztard/"BT_NS_PREFIX":setup/"BT_NS_PREFIX":*");
	g_assert(klass->xpath_get_setup);
	klass->xpath_get_patterns = xmlXPathCompile("/"BT_NS_PREFIX":buzztard/"BT_NS_PREFIX":patterns/"BT_NS_PREFIX":*");
	g_assert(klass->xpath_get_patterns);
	klass->xpath_get_sequence = xmlXPathCompile("/"BT_NS_PREFIX":buzztard/"BT_NS_PREFIX":sequence");
	g_assert(klass->xpath_get_sequence);
	klass->xpath_get_sequence_labels = xmlXPathCompile("./"BT_NS_PREFIX":labels/"BT_NS_PREFIX":*");
	g_assert(klass->xpath_get_sequence);
	klass->xpath_get_sequence_tracks = xmlXPathCompile("./"BT_NS_PREFIX":tracks/"BT_NS_PREFIX":*");
	g_assert(klass->xpath_get_sequence);
  klass->xpath_get_sequence_length = xmlXPathCompile("./"BT_NS_PREFIX":labels/"BT_NS_PREFIX":label/@time|./"BT_NS_PREFIX":tracks/"BT_NS_PREFIX":track/"BT_NS_PREFIX":position/@time");
	g_assert(klass->xpath_get_sequence_length);
  //klass->xpath_count_sequence_tracks = xmlXPathCompile("count(./"BT_NS_PREFIX":tracks/"BT_NS_PREFIX":track)");
	//g_assert(klass->xpath_count_sequence_tracks);
}

/* as of gobject documentation, static types are keept alive until the program ends.
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
			NULL // value_table
    };
		type = g_type_register_static(BT_TYPE_SONG_IO,"BtSongIONative",&info,0);
  }
  return type;
}
