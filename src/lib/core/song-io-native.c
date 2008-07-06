/* $Id$
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

#include "core_private.h"
#ifdef USE_GSF
//#include <gsf/gsf.h>
#include <gsf/gsf-utils.h>
#include <gsf/gsf-input-stdio.h>
#include <gsf/gsf-infile.h>
#include <gsf/gsf-infile-zip.h>
#include <gsf/gsf-output-stdio.h>
#include <gsf/gsf-outfile.h>
#include <gsf/gsf-outfile-zip.h>
#endif

//-- property ids

enum {
  SONG_IO_NATIVE_MODE=1,
};

struct _BtSongIONativePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
  BtSongIONativeMode mode;
#ifdef USE_GSF
  GsfInput *input;
  GsfInfile *infile;
  GsfOutput *output;
  GsfOutfile *outfile;
#endif
};

static GQuark error_domain=0;

static BtSongIOClass *parent_class=NULL;

static BtSongIONativeMode last_detect_mode=BT_SONG_IO_NATIVE_MODE_UNDEF;

//-- enums

GType bt_song_io_native_mode_get_type(void) {
  static GType type = 0;
  if(G_UNLIKELY(type==0)) {
    static const GEnumValue values[] = {
      { BT_SONG_IO_NATIVE_MODE_UNDEF, "BT_SONG_IO_NATIVE_MODE_UNDEF", "format not yet known" },
      { BT_SONG_IO_NATIVE_MODE_XML,   "BT_SONG_IO_NATIVE_MODE_XML",   "plain xml without external files" },
      { BT_SONG_IO_NATIVE_MODE_BZT,   "BT_SONG_IO_NATIVE_MODE_BZT",   "zip'ed xml with externals included" },
      { 0, NULL, NULL},
    };
    type = g_enum_register_static("BtSongIONativeMode", values);
  }
  return type;
}

//-- plugin detect
/*
 * bt_song_io_native_detect:
 * @file_name: the file to check against
 *
 * Checks if this plugin should manage this kind of file.
 *
 * Returns: the GType of this plugin of %NULL
 */
static GType bt_song_io_native_detect(const gchar * const file_name) {
  GType type=0;

  // test filename first  
  if(!file_name) return(type);

  /* @todo add proper mime-type detection (gio) */
  // check extension
  gchar * const lc_file_name=g_ascii_strdown(file_name,-1);
  if(g_str_has_suffix(lc_file_name,".xml")) {
    type=BT_TYPE_SONG_IO_NATIVE;
    last_detect_mode=BT_SONG_IO_NATIVE_MODE_XML;
  }
  else if(g_str_has_suffix(lc_file_name,".bzt")) {
    type=BT_TYPE_SONG_IO_NATIVE;
    last_detect_mode=BT_SONG_IO_NATIVE_MODE_BZT;
  }
  g_free(lc_file_name);

  GST_INFO("file_name='%s' => %d",file_name,last_detect_mode);

#if 0
  // lets replace this with GIO later
  GnomeVFSResult result;

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
      type=BT_TYPE_SONG_IO_NATIVE;
    }
    g_free(lc_file_name);
  }
  else {
    // create new file info pointer.
    GnomeVFSFileInfo *file_info = gnome_vfs_file_info_new();
    
    GST_INFO("given uri exists");
    
    // now we check the mime type
    if((result=gnome_vfs_get_file_info_uri(input_uri,file_info,GNOME_VFS_FILE_INFO_GET_MIME_TYPE))!=GNOME_VFS_OK) {
      GST_WARNING("Cannot determine mime type. Error: %s\n", gnome_vfs_result_to_string (result));
      gnome_vfs_file_info_unref(file_info);
      goto Error;
    }
    /* @todo: check mime-type and set type accordingly */
    GST_INFO("Mime type: %s\n",gnome_vfs_file_info_get_mime_type (file_info));
    gnome_vfs_file_info_unref(file_info);

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
  
#endif
  return(type);
}

/**
 * bt_song_io_native_module_info:
 *
 * Buzztard native format song loader/saver metadata.
 */
BtSongIOModuleInfo bt_song_io_native_module_info = {
  bt_song_io_native_detect,
  {
    { "buzztard song with externals", "audio/x-bzt", "bzt" },
    { "buzztard song without externals", "audio/x-bzt-xml", "xml" },
    { NULL, }
  }
};


//-- public methods

/**
 * bt_song_io_native_copy_to_fd:
 * @self: the song-plugin
 * @file_name: the path to the file inside the song
 * @fd: a file-descriptor of an opened file to copy @file_name to
 *
 * Copies the file specified by @file_name from the song file to the @fd.
 *
 * This is a helper for #BtSong persistence.
 *
 * Returns: %TRUE on success 
 */
gboolean bt_song_io_native_copy_to_fd(const BtSongIONative * const self, const gchar *file_name, gint fd) {
  gboolean res=FALSE;
#ifdef USE_GSF  
  GsfInput *data;
#if 1
  GsfInfile *infile=self->priv->infile,*tmp_infile=NULL;
  gchar **parts;
  gint i=0,num;
  
  // bahh, we need to get the dir for the file_name
  // or use gsf_infile_child_by_vname(infile,parts[0],parts[1],...NULL);
  parts=g_strsplit(file_name,G_DIR_SEPARATOR_S,0);
  num=gsf_infile_num_children (infile);
  while(parts[i] && (num>0)) {
    GST_INFO("%2d : %s",i,parts[i]);
    if((tmp_infile=GSF_INFILE(gsf_infile_child_by_name(infile,parts[i])))) {
      num=gsf_infile_num_children (tmp_infile);
      i++;
    }
    else {
      GST_WARNING("error opening \"%s\"",parts[i]);
    }
    if(infile!=self->priv->infile)
      g_object_unref(G_OBJECT(self->priv->infile));
    infile=tmp_infile;
  }
  g_strfreev(parts);

  // get file from zip
  if((data=GSF_INPUT(tmp_infile))) {
#else
  if((data=gsf_infile_child_by_name(self->priv->infile,file_name))) {
#endif
    const guint8 *bytes;
    size_t len=(size_t)gsf_input_size(data);

    GST_INFO ("'%s' size: %" G_GSIZE_FORMAT, gsf_input_name(data), len);
    
    if((bytes=gsf_input_read(data,len,NULL))) {
      // write to fd
      if((write(fd,bytes,len))) {
        if(lseek(fd,0,SEEK_SET)==0) {
          res=TRUE;
        }
        else {
          GST_ERROR("error seeking back to 0 \"%s\"",file_name);
        }
      }
      else {
        GST_ERROR("error writing data \"%s\"",file_name);
      }
    }
    else {
      GST_WARNING("error reading data \"%s\"",file_name);
    }
    g_object_unref(G_OBJECT(data));
  }
  else {
    GST_WARNING("error opening \"%s\"",file_name);
  }
#endif
  return(res);
}

/**
 * bt_song_io_native_copy_from_uri:
 * @self: the song-plugin
 * @file_name: the path to the file inside the song
 * @uri: location of the source file
 *
 * Copies the file specified by @uri to @file_name into the song file.
 *
 * This is a helper for #BtSong persistence.
 *
 * Returns: %TRUE on success 
 */

gboolean bt_song_io_native_copy_from_uri(const BtSongIONative * const self, const gchar *file_name, const gchar *uri) {
  gboolean res=FALSE;
#ifdef USE_GSF  
  GsfOutput *data;

  if((data=gsf_outfile_new_child(self->priv->outfile,file_name,FALSE))) {
    GError *err=NULL;
    gchar *src_file_name;
    gchar *bytes;
    gsize size;
    
    // @todo: what about using gio here 
    src_file_name=g_filename_from_uri(uri,NULL,NULL);
    
    if(g_file_get_contents(src_file_name,&bytes,&size,&err)) {
      if(gsf_output_write(data, (size_t)size, (guint8 const *)bytes)) {
        res=TRUE;
      }
      else {
        GST_ERROR("error writing data \"%s\"",file_name);
      }
      g_free(bytes);
    }
    else {
      GST_ERROR("error reading data \"%s\" : %s",file_name,err->message);
      g_error_free(err);
    }
    g_free(src_file_name);
    gsf_output_close(data);
    g_object_unref(G_OBJECT(data));
  }
#endif
  return(res);
}

//-- methods

static gboolean bt_song_io_native_load(gconstpointer const _self, const BtSong * const song) {
  const BtSongIONative * const self=BT_SONG_IO_NATIVE(_self);
  gboolean result=FALSE;
  gchar * const file_name;
  
  g_object_get(G_OBJECT(self),"file-name",&file_name,NULL);
  GST_INFO("native io (%d) will now load song from \"%s\"",last_detect_mode,file_name);

  const gchar * const msg=_("Loading file '%s'");
  gchar * const status=g_alloca(1+strlen(msg)+strlen(file_name));
  g_sprintf(status,msg,file_name);
  //gchar * const status=g_alloca(1+strlen(_("Loading file '%s'"))+strlen(file_name));
  //g_sprintf(status,_("Loading file '%s'"),file_name);
  g_object_set(G_OBJECT(self),"status",status,NULL);
      
  xmlParserCtxtPtr const ctxt=xmlNewParserCtxt();
  if(ctxt) {
    xmlDocPtr song_doc=NULL;
    
    if(self->priv->mode==BT_SONG_IO_NATIVE_MODE_XML) {
      song_doc=xmlCtxtReadFile(ctxt,file_name,NULL,0L);
    }
#ifdef USE_GSF
    else if(self->priv->mode==BT_SONG_IO_NATIVE_MODE_BZT) {
      GError *err=NULL;
      
      // open the file from the file_name argument
      if((self->priv->input=gsf_input_stdio_new (file_name, &err))) {
        // create an gsf input file
        if((self->priv->infile=gsf_infile_zip_new (self->priv->input, &err))) {
          GsfInput *data;
          
          GST_INFO("'%s' size: %" GSF_OFF_T_FORMAT ", files: %d", 
            gsf_input_name (self->priv->input),
            gsf_input_size (self->priv->input),
            gsf_infile_num_children (self->priv->infile));

          // get file from zip
          if((data=gsf_infile_child_by_name(self->priv->infile,"song.xml"))) {
            const guint8 *bytes;
            size_t len=(size_t)gsf_input_size(data);
        
            GST_INFO ("'%s' size: %" G_GSIZE_FORMAT, gsf_input_name(data), len);
            
            if((bytes=gsf_input_read(data,len,NULL))) {
              song_doc=xmlCtxtReadMemory(ctxt,(const char *)bytes,len,"http://www.buzztard.org",NULL,0L);
            }
            else {
              GST_WARNING("'%s': error reading data",file_name);
            }
            g_object_unref(G_OBJECT(data));
          }
        }
        else {
          GST_ERROR("'%s' is not a zip file: %s",file_name,err->message);
          g_error_free(err);
        }
      }
      else {
        GST_ERROR("'%s' error: %s",file_name,err->message);
        g_error_free(err);
      }
    }
#endif
    
    if(song_doc) {
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
          result=bt_persistence_load(BT_PERSISTENCE(song),root_node,NULL);
        }
      }
      if(song_doc) xmlFreeDoc(song_doc);
    }
    else GST_ERROR("failed to read song file '%s'",file_name);
    
#ifdef USE_GSF
    if(self->priv->infile) {
      g_object_unref(G_OBJECT(self->priv->infile));
      self->priv->infile=NULL;
    }
    if(self->priv->input) {
      g_object_unref(G_OBJECT(self->priv->input));
      self->priv->input=NULL;
    }
#endif

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
  GST_INFO("native io (%d) will now save song to \"%s\"",last_detect_mode,file_name);

  const gchar * const msg=_("Saving file '%s'");
  gchar * const status=g_alloca(1+strlen(msg)+strlen(file_name));
  g_sprintf(status,msg,file_name);
  //gchar * const status=g_alloca(1+strlen(_("Saving file '%s'"))+strlen(file_name));
  //g_sprintf(status,_("Saving file '%s'"),file_name);
  g_object_set(G_OBJECT(self),"status",status,NULL);

  xmlDocPtr const song_doc=xmlNewDoc(XML_CHAR_PTR("1.0"));
  if(song_doc) {
    gboolean cont=TRUE;

#ifdef USE_GSF
    if(self->priv->mode==BT_SONG_IO_NATIVE_MODE_BZT) {
      GError *err=NULL;

      // open the file from the file_name argument
      if((self->priv->output=gsf_output_stdio_new (file_name, &err))) {
        // create an gsf output file
        if(!(self->priv->outfile=gsf_outfile_zip_new (self->priv->output, &err))) {
          GST_ERROR("failed to create zip song file \"%s\" : %s",file_name,err->message);
          g_error_free(err);
          cont=FALSE;
        }
      }
      else {
        GST_ERROR("failed to write song file \"%s\" : %s",file_name,err->message);
        g_error_free(err);
        cont=FALSE;
      }
    }
#endif

    if(cont) {
      xmlNodePtr const root_node=bt_persistence_save(BT_PERSISTENCE(song),NULL,NULL);
      if(root_node) {
        xmlDocSetRootElement(song_doc,root_node);
        if(self->priv->mode==BT_SONG_IO_NATIVE_MODE_XML) {
          if(xmlSaveFile(file_name,song_doc)!=-1) {
            result=TRUE;
            GST_INFO("xml saved okay");
          }
          else GST_ERROR("failed to write song file \"%s\"",file_name);
        }
#ifdef USE_GSF
        else if(self->priv->mode==BT_SONG_IO_NATIVE_MODE_BZT) {
          if(self->priv->output && self->priv->outfile) {
            GsfOutput *data;
            
            // create file in zip
            if((data=gsf_outfile_new_child(self->priv->outfile,"song.xml",FALSE))) {
              xmlChar *bytes;
              gint size;
              
              xmlDocDumpMemory(song_doc,&bytes,&size);
              if(gsf_output_write(data, (size_t)size, (guint8 const *)bytes)) {
                result=TRUE;
                GST_INFO("bzt saved okay");
              }
              else GST_ERROR("failed to write song file \"%s\"",file_name);
              xmlFree(bytes);
              gsf_output_close(data);
              g_object_unref(G_OBJECT(data));
            }
          }
        }
#endif
      }
    }
  }
  
#ifdef USE_GSF
  if(self->priv->output) {
    gsf_output_close(GSF_OUTPUT(self->priv->outfile));
    g_object_unref(G_OBJECT(self->priv->outfile));
    self->priv->outfile=NULL;
  }
  if(self->priv->output) {
    gsf_output_close(self->priv->output);
    g_object_unref(G_OBJECT(self->priv->output));
    self->priv->output=NULL;
  }

#endif
  
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
    case SONG_IO_NATIVE_MODE: {
      g_value_set_enum(value, self->priv->mode);
    } break;
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
  self->priv->mode=last_detect_mode;
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
  
  g_object_class_install_property(gobject_class,SONG_IO_NATIVE_MODE,
                                  g_param_spec_enum("mode",
                                     "mode prop",
                                     "file format mode",
                                     BT_TYPE_SONG_IO_NATIVE_MODE,  /* enum type */
                                     BT_SONG_IO_NATIVE_MODE_UNDEF, /* default value */
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

}

GType bt_song_io_native_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof(BtSongIONativeClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_song_io_native_class_init, // class_init
      NULL, // class_finalize
      //(GClassFinalizeFunc)bt_song_io_native_class_finalize,
      NULL, // class_data
      sizeof(BtSongIONative),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_song_io_native_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(BT_TYPE_SONG_IO,"BtSongIONative",&info,0);
  }
  return type;
}
