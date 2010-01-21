/* $Id$
 *
 * Buzztard
 * Copyright (C) 2008 Buzztard team <buzztard-devel@lists.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/**
 * SECTION:btsongionativexml
 * @short_description: class for song input and output in builtin native format
 *
 * This internal #BtSongIONative module implements loading and saving of an own
 * xml format without externals.
 */ 
 
#define BT_CORE
#define BT_SONG_IO_NATIVE_XML_C

#include "core_private.h"

struct _BtSongIONativeXMLPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
};

static GQuark error_domain=0;

static BtSongIONativeClass *parent_class=NULL;

//-- methods

static gboolean bt_song_io_native_xml_load(gconstpointer const _self, const BtSong * const song) {
  const BtSongIONativeXML * const self=BT_SONG_IO_NATIVE_XML(_self);
  gboolean result=FALSE;
  gchar * const file_name;
  
  g_object_get((gpointer)self,"file-name",&file_name,NULL);
  GST_INFO("native io xml will now load song from \"%s\"",file_name);

  const gchar * const msg=_("Loading file '%s'");
  gchar * const status=g_alloca(1+strlen(msg)+strlen(file_name));
  g_sprintf(status,msg,file_name);
  //gchar * const status=g_alloca(1+strlen(_("Loading file '%s'"))+strlen(file_name));
  //g_sprintf(status,_("Loading file '%s'"),file_name);
  g_object_set((gpointer)self,"status",status,NULL);
      
  xmlParserCtxtPtr const ctxt=xmlNewParserCtxt();
  if(ctxt) {
    xmlDocPtr song_doc;
    
    if((song_doc=xmlCtxtReadFile(ctxt,file_name,NULL,0L))) {
      if(!ctxt->valid) {
        GST_WARNING("the supplied document is not a XML/Buzztard document");
      }
      else if(!ctxt->wellFormed) {
        GST_WARNING("the supplied document is not a wellformed XML document");
      }
      else {
        xmlNodePtr const root_node=xmlDocGetRootElement(song_doc);

        if(root_node==NULL) {
          GST_WARNING("xmlDoc is empty");
        }
        else if(xmlStrcmp(root_node->name,(const xmlChar *)"buzztard")) {
          GST_WARNING("wrong document type root node in xmlDoc src");
        }
        else {
          GError *err=NULL;

          bt_persistence_load(BT_TYPE_SONG,BT_PERSISTENCE(song),root_node,&err,NULL);
          if(err!=NULL) {
            g_error_free(err);
          }
          else result=TRUE;
        }
      }
      if(song_doc) xmlFreeDoc(song_doc);
    }
    else GST_ERROR("failed to read song file '%s'",file_name);
  }
  else GST_ERROR("failed to create file-parser context");
  if(ctxt) xmlFreeParserCtxt(ctxt);
  g_free(file_name);
  g_object_set((gpointer)self,"status",NULL,NULL);
  return(result);
}

static gboolean bt_song_io_native_xml_save(gconstpointer const _self, const BtSong * const song) {
  const BtSongIONativeXML * const self=BT_SONG_IO_NATIVE_XML(_self);
  gboolean result=FALSE;
  gchar * const file_name;
  
  g_object_get((gpointer)self,"file-name",&file_name,NULL);
  GST_INFO("native io xml will now save song to \"%s\"",file_name);

  const gchar * const msg=_("Saving file '%s'");
  gchar * const status=g_alloca(1+strlen(msg)+strlen(file_name));
  g_sprintf(status,msg,file_name);
  //gchar * const status=g_alloca(1+strlen(_("Saving file '%s'"))+strlen(file_name));
  //g_sprintf(status,_("Saving file '%s'"),file_name);
  g_object_set((gpointer)self,"status",status,NULL);

  xmlDocPtr const song_doc=xmlNewDoc(XML_CHAR_PTR("1.0"));
  if(song_doc) {
    xmlNodePtr const root_node=bt_persistence_save(BT_PERSISTENCE(song),NULL);
    if(root_node) {
      xmlDocSetRootElement(song_doc,root_node);
      if(xmlSaveFile(file_name,song_doc)!=-1) {
        result=TRUE;
        GST_INFO("xml saved okay");
      }
      else GST_ERROR("failed to write song file \"%s\"",file_name);
    }
  }
    
  g_free(file_name);

  g_object_set((gpointer)self,"status",NULL,NULL);
  return(result);
}

//-- wrapper

//-- class internals

static void bt_song_io_native_xml_dispose(GObject * const object) {
  const BtSongIONativeXML * const self = BT_SONG_IO_NATIVE_XML(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_song_io_native_xml_init(GTypeInstance * const instance, gconstpointer g_class) {
  BtSongIONativeXML * const self = BT_SONG_IO_NATIVE_XML(instance);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_SONG_IO_NATIVE_XML, BtSongIONativeXMLPrivate);
}

static void bt_song_io_native_xml_class_init(BtSongIONativeClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);
  BtSongIOClass * const btsongio_class = BT_SONG_IO_CLASS(klass);

  error_domain=g_quark_from_static_string("BtSongIONativeXML");
  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtSongIONativeXMLPrivate));

  gobject_class->dispose      = bt_song_io_native_xml_dispose;
  
  btsongio_class->load        = bt_song_io_native_xml_load;
  btsongio_class->save        = bt_song_io_native_xml_save;
}

GType bt_song_io_native_xml_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof(BtSongIONativeXMLClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_song_io_native_xml_class_init, // class_init
      NULL, // class_finalize
      //(GClassFinalizeFunc)bt_song_io_native_class_finalize,
      NULL, // class_data
      sizeof(BtSongIONativeXML),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_song_io_native_xml_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(BT_TYPE_SONG_IO_NATIVE,"BtSongIONativeXML",&info,0);
  }
  return type;
}
