/* $Id: song-io-native.c,v 1.122 2007-03-20 23:22:58 ensonic Exp $
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
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
 * SECTION:btsongionative
 * @short_description: class for song input and output in builtin native format
 *
 * Buzztard stores its songs in a own file-format. This internal io-module 
 * implements loading and saving of this format.
 * The format is an archive, that contains an XML file and optionally binary
 * data, such as audio samples.
 */ 
 
#define BT_CORE
#define BT_SONG_IO_NATIVE_C

#include <libbtcore/core.h>

struct _BtSongIONativePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
};

static GQuark error_domain=0;

static BtSongIOClass *parent_class=NULL;

//-- plugin detect
/**
 * bt_song_io_native_detect:
 * @file_name: the file to check against
 *
 * Checks if this plugin should manage this kind of file.
 *
 * Returns: the GType of this plugin of %NULL
 */
GType bt_song_io_native_detect(const gchar * const file_name) {
  GType type=0;
  GnomeVFSResult result;

  // test filename first  
  GST_INFO("file_name='%s'",file_name);
  if(!file_name) return(type);

  // creating a absolute uri string from the given input string.
  // works also if the given string was a absolute uri.
  char * const absolute_uri_string = gnome_vfs_make_uri_from_input_with_dirs (file_name, GNOME_VFS_MAKE_URI_DIR_CURRENT);
  GST_INFO("creating absolute file uri string: %s",absolute_uri_string);
  // creating the gnome-vfs uri from the absolute path string
  GnomeVFSURI * const input_uri = gnome_vfs_uri_new(absolute_uri_string);
  // check if the input uri is ok
  if (input_uri == NULL) {
    GST_WARNING("cannot create input uri for gnome vfs");
    goto Error;
  }
  // check if the given uri exists
  if (!gnome_vfs_uri_exists(input_uri)) {
    GST_INFO("given uri does not exists (saving?)");

    // check extension
    gchar * const lc_file_name=g_ascii_strdown(file_name,-1);
    if(g_str_has_suffix(lc_file_name,".xml")) {
      GST_INFO(".xml extension -> accept");
      type=BT_TYPE_SONG_IO_NATIVE;
    }
    g_free(lc_file_name);
  }
  else {
    // create new file info pointer.
    GnomeVFSFileInfo * const file_info = gnome_vfs_file_info_new ();
    
    GST_INFO("given uri exists");
    
    // now we check the mime type
    if((result=gnome_vfs_get_file_info_uri(input_uri,file_info,GNOME_VFS_FILE_INFO_GET_MIME_TYPE))!=GNOME_VFS_OK) {
      GST_WARNING("Cannot determine mime type. Error: %s\n", gnome_vfs_result_to_string (result));
      if(file_info) gnome_vfs_file_info_unref(file_info);
      goto Error;
    }
    // @todo: check mime-type ?    
    if(file_info) gnome_vfs_file_info_unref(file_info);

    // check extension
    gchar * const lc_file_name=g_ascii_strdown(file_name,-1);
    if(g_str_has_suffix(lc_file_name,".xml")) {
      type=BT_TYPE_SONG_IO_NATIVE;
    }
    g_free(lc_file_name);
  }
Error:
  gnome_vfs_uri_unref(input_uri);
  if(absolute_uri_string) g_free(absolute_uri_string);
  return(type);
}

//-- methods

static gboolean bt_song_io_native_load(gconstpointer const _self, const BtSong * const song) {
  const BtSongIONative * const self=BT_SONG_IO_NATIVE(_self);
  gboolean result=FALSE;
  gchar * const file_name;
  
  g_object_get(G_OBJECT(self),"file-name",&file_name,NULL);
  GST_INFO("native io will now load song from \"%s\"",file_name);

  const gchar * const msg=_("Loading file '%s'");
  gchar * const status=g_alloca(strlen(msg)+strlen(file_name));
  g_sprintf(status,msg,file_name);
  g_object_set(G_OBJECT(self),"status",status,NULL);
    
  // @todo add gnome-vfs detection method. This method should detect the
  // filetype of the given file and returns a gnome-vfs uri to open this
  // file with gnome-vfs. For example if the given file is song.xml the method
  // should return file:/home/waffel/buzztard/songs/song.xml
  
  /* @todo add zip file processing
   * zip_file=bt_zip_file_new(file_name,BT_ZIP_FILE_MODE_READ);
   * xml_doc_buffer=bt_zip_file_read_file(zip_file,"song.xml",&xml_doc_size);
   */
  
  // @todo read from zip_file
  const xmlParserCtxtPtr const ctxt=xmlNewParserCtxt();
  if(ctxt) {
    //xmlDocPtr song_doc=xmlCtxtReadMemory(ctxt,xml_doc_buffer,xml_doc_size,file_name,NULL,0L)
    xmlDocPtr const song_doc=xmlCtxtReadFile(ctxt,file_name,NULL,0L);
    if(song_doc) {
      if(!ctxt->valid) {
        GST_WARNING("the supplied document is not a XML/Buzztard document");
      }
      else if(!ctxt->wellFormed) {
        GST_WARNING("the supplied document is not a wellformed XML document");
      }
      else {
        const xmlNodePtr const root_node=xmlDocGetRootElement(song_doc);

        if(root_node==NULL) {
          GST_WARNING("xmlDoc is empty");
        }
        else if(xmlStrcmp(root_node->name,(const xmlChar *)"buzztard")) {
          GST_WARNING("wrong document type root node in xmlDoc src");
        }
        else {
          result=bt_persistence_load(BT_PERSISTENCE(song),root_node,NULL);
        }
      }
      if(song_doc) xmlFreeDoc(song_doc);
    }
    else GST_ERROR("failed to read song file '%s'",file_name);
  }
  else GST_ERROR("failed to create file-parser context");
  if(ctxt) xmlFreeParserCtxt(ctxt);
  g_free(file_name);
  g_object_set(G_OBJECT(self),"status",NULL,NULL);
  return(result);
}

static gboolean bt_song_io_native_save(gconstpointer const _self, const BtSong * const song) {
  const BtSongIONative * const self=BT_SONG_IO_NATIVE(_self);
  gboolean result=FALSE;
  gchar * const file_name;
  
  g_object_get(G_OBJECT(self),"file-name",&file_name,NULL);
  GST_INFO("native io will now save song to \"%s\"",file_name);

  const gchar * const msg=_("Saving file '%s'");
  gchar * const status=g_alloca(strlen(msg)+strlen(file_name));
  g_sprintf(status,msg,file_name);
  g_object_set(G_OBJECT(self),"status",status,NULL);

  const xmlDocPtr const song_doc=xmlNewDoc(XML_CHAR_PTR("1.0"));
  if(song_doc) {
    const xmlNodePtr const root_node=bt_persistence_save(BT_PERSISTENCE(song),NULL,NULL);
    if(root_node) {
      xmlDocSetRootElement(song_doc,root_node);
      if(xmlSaveFile(file_name,song_doc)!=-1) {
        result=TRUE;
      }
      else GST_ERROR("failed to write song file \"%s\"",file_name);
    }
  }
  
  g_free(file_name);

  g_object_set(G_OBJECT(self),"status",NULL,NULL);
  return(result);
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_song_io_native_get_property(GObject      * const object,
                               const guint         property_id,
                               GValue       * const value,
                               GParamSpec   * const pspec)
{
  const BtSongIONative * const self = BT_SONG_IO_NATIVE(object);
  return_if_disposed();
  switch (property_id) {
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_song_io_native_set_property(GObject      * const object,
                              const guint         property_id,
                              const GValue * const value,
                              GParamSpec   * const pspec)
{
  const BtSongIONative * const self = BT_SONG_IO_NATIVE(object);
  return_if_disposed();
  switch (property_id) {
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_song_io_native_dispose(GObject * const object) {
  const BtSongIONative * const self = BT_SONG_IO_NATIVE(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_song_io_native_finalize(GObject * const object) {
  const BtSongIONative * const self = BT_SONG_IO_NATIVE(object);

  GST_DEBUG("!!!! self=%p",self);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_song_io_native_init(GTypeInstance * const instance, gconstpointer g_class) {
  BtSongIONative * const self = BT_SONG_IO_NATIVE(instance);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_SONG_IO_NATIVE, BtSongIONativePrivate);
}

static void bt_song_io_native_class_init(BtSongIONativeClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);
  BtSongIOClass * const btsongio_class = BT_SONG_IO_CLASS(klass);

  error_domain=g_quark_from_static_string("BtSongIONative");
  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtSongIONativePrivate));

  gobject_class->set_property = bt_song_io_native_set_property;
  gobject_class->get_property = bt_song_io_native_get_property;
  gobject_class->dispose      = bt_song_io_native_dispose;
  gobject_class->finalize     = bt_song_io_native_finalize;
  
  btsongio_class->load        = bt_song_io_native_load;
  btsongio_class->save        = bt_song_io_native_save;
}

GType bt_song_io_native_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      G_STRUCT_SIZE(BtSongIONativeClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_song_io_native_class_init, // class_init
      NULL, // class_finalize
      //(GClassFinalizeFunc)bt_song_io_native_class_finalize,
      NULL, // class_data
      G_STRUCT_SIZE(BtSongIONative),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_song_io_native_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(BT_TYPE_SONG_IO,"BtSongIONative",&info,0);
  }
  return type;
}
