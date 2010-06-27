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
 * SECTION:btsongionativebzt
 * @short_description: class for song input and output in builtin native format
 *
 * This internal #BtSongIONative module implements loading and saving of an own
 * xml format with externals.
 * The format is an archive, that contains an XML file and optionally binary
 * data, such as audio samples.
 */ 
 
#define BT_CORE
#define BT_SONG_IO_NATIVE_BZT_C

#include "core_private.h"
#ifdef USE_GSF
//#include <gsf/gsf.h>
#include <gsf/gsf-utils.h>
#include <gsf/gsf-input-stdio.h>
#include <gsf/gsf-input-memory.h>
#include <gsf/gsf-infile.h>
#include <gsf/gsf-infile-zip.h>
#include <gsf/gsf-output-stdio.h>
#include <gsf/gsf-outfile.h>
#include <gsf/gsf-outfile-zip.h>
#endif

struct _BtSongIONativeBZTPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
  
#ifdef USE_GSF
  GsfInput *input;
  GsfInfile *infile;
  GsfOutput *output;
  GsfOutfile *outfile;
#endif
};

static GQuark error_domain=0;

static BtSongIONativeClass *parent_class=NULL;

//-- public methods

/**
 * bt_song_io_native_bzt_copy_to_fd:
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
gboolean bt_song_io_native_bzt_copy_to_fd(const BtSongIONativeBZT * const self, const gchar *file_name, gint fd) {
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
      num=0;
    }
    if(infile!=self->priv->infile)
      g_object_unref(infile);
    infile=tmp_infile;
  }
  g_strfreev(parts);

  // get file from zip
  if((data=GSF_INPUT(tmp_infile))) {
#else
  // this won't work : http://bugzilla.gnome.org/show_bug.cgi?id=540521
  // there will be new api in libgsf-1.14.9
  // opensuse-11.0 has 1.14.8
  // gsf_infile_child_by_aname / gsf_infile_child_by_vaname
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
    g_object_unref(data);
  }
  else {
    GST_WARNING("error opening \"%s\"",file_name);
  }
#endif
  return(res);
}

/**
 * bt_song_io_native_bzt_copy_from_uri:
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

gboolean bt_song_io_native_bzt_copy_from_uri(const BtSongIONativeBZT * const self, const gchar *file_name, const gchar *uri) {
  gboolean res=FALSE;
#ifdef USE_GSF
  GsfOutput *data;

  if((data=gsf_outfile_new_child(self->priv->outfile,file_name,FALSE))) {
    GError *err=NULL;
    gchar *src_file_name;
    gchar *bytes;
    gsize size;
    gboolean have_data=FALSE;
    
    GST_INFO("src uri : %s",uri);
    
    // @idea: what about using gio here
    src_file_name=g_filename_from_uri(uri,NULL,NULL);
    if(src_file_name) {
      if(g_file_get_contents(src_file_name,&bytes,&size,&err)) {
        have_data=TRUE;
      }
      else {
        GST_ERROR("error reading data \"%s\" : %s",file_name,err->message);
        g_error_free(err);
      }
    }
    else {
      if(g_str_has_prefix(uri,"fd://")) {
        struct stat buf={0,};
        gint fd;

        sscanf (uri, "fd://%d", &fd);
        GST_INFO("read data from file-deskriptor: fd=%d",fd);
        
        if(!(fstat(fd, &buf))) {
          if((bytes=g_try_malloc(buf.st_size))) {
            if(lseek(fd,0,SEEK_SET) == 0) {
              size=read(fd,bytes,buf.st_size);
              if(size!=buf.st_size) {
                GST_WARNING("did not write all data");
              }
              have_data=TRUE;
            }
            else {
              GST_WARNING("can't seek in filedeskriptor");
            }
          }
        }
      }
    }
    
    if(have_data) {
      GST_INFO("write %d bytes to sample file", size);
      if(gsf_output_write(data, (size_t)size, (guint8 const *)bytes)) {
        res=TRUE;
      }
      else {
        GST_ERROR("error writing data \"%s\"",file_name);
      }
      g_free(bytes);
    }
    g_free(src_file_name);
    gsf_output_close(data);
    g_object_unref(data);
  }
#endif
  return(res);
}

//-- methods

static gboolean bt_song_io_native_bzt_load(gconstpointer const _self, const BtSong * const song) {
  gboolean result=FALSE;
#ifdef USE_GSF
  const BtSongIONativeBZT * const self=BT_SONG_IO_NATIVE_BZT(_self);
  gchar * const file_name;
  guint len;
  gpointer data;
  gchar *status;
  
  g_object_get((gpointer)self,"file-name",&file_name,"data",&data,"data-len",&len,NULL);
  GST_INFO("native io bzt will now load song from \"%s\"",file_name?file_name:"data");

  const gchar * const msg=_("Loading file '%s'");
  if(file_name) {
    status=g_alloca(1+strlen(msg)+strlen(file_name));
    g_sprintf(status,msg,file_name);
  }
  else {
    status=g_alloca(1+strlen(msg)+4);
    g_sprintf(status,msg,"data");
  }
  g_object_set((gpointer)self,"status",status,NULL);
      
  xmlParserCtxtPtr const ctxt=xmlNewParserCtxt();
  if(ctxt) {
    xmlDocPtr song_doc=NULL;
    
    GError *err=NULL;
    
    if(data && len) {
      // parse the file from the memory block
      self->priv->input=gsf_input_memory_new(data,len,FALSE);
    }
    else {
      // open the file from the file_name argument
      self->priv->input=gsf_input_stdio_new(file_name, &err);
    }
    if(self->priv->input) {
      // create an gsf input file
      if((self->priv->infile=gsf_infile_zip_new(self->priv->input, &err))) {
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
          g_object_unref(data);
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
    
    if(self->priv->infile) {
      g_object_unref(self->priv->infile);
      self->priv->infile=NULL;
    }
    if(self->priv->input) {
      g_object_unref(self->priv->input);
      self->priv->input=NULL;
    }

  }
  else GST_ERROR("failed to create file-parser context");
  if(ctxt) xmlFreeParserCtxt(ctxt);
  g_free(file_name);
  g_object_set((gpointer)self,"status",NULL,NULL);
#endif
  return(result);
}

static gboolean bt_song_io_native_bzt_save(gconstpointer const _self, const BtSong * const song) {
  gboolean result=FALSE;
#ifdef USE_GSF
  const BtSongIONativeBZT * const self=BT_SONG_IO_NATIVE_BZT(_self);
  gchar * const file_name;
  
  g_object_get((gpointer)self,"file-name",&file_name,NULL);
  GST_INFO("native io bzt will now save song to \"%s\"",file_name);

  const gchar * const msg=_("Saving file '%s'");
  gchar * const status=g_alloca(1+strlen(msg)+strlen(file_name));
  g_sprintf(status,msg,file_name);
  //gchar * const status=g_alloca(1+strlen(_("Saving file '%s'"))+strlen(file_name));
  //g_sprintf(status,_("Saving file '%s'"),file_name);
  g_object_set((gpointer)self,"status",status,NULL);

  xmlDocPtr const song_doc=xmlNewDoc(XML_CHAR_PTR("1.0"));
  if(song_doc) {
    gboolean cont=TRUE;
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

    if(cont) {
      xmlNodePtr const root_node=bt_persistence_save(BT_PERSISTENCE(song),NULL);
      if(root_node) {
        xmlDocSetRootElement(song_doc,root_node);
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
            g_object_unref(data);
          }
        }
      }
    }
  }
  
  if(self->priv->output) {
    gsf_output_close(GSF_OUTPUT(self->priv->outfile));
    g_object_unref(self->priv->outfile);
    self->priv->outfile=NULL;
  }
  if(self->priv->output) {
    gsf_output_close(self->priv->output);
    g_object_unref(self->priv->output);
    self->priv->output=NULL;
  }
 
  g_free(file_name);

  g_object_set((gpointer)self,"status",NULL,NULL);
#endif
  return(result);
}

//-- wrapper

//-- class internals

static void bt_song_io_native_bzt_dispose(GObject * const object) {
  const BtSongIONativeBZT * const self = BT_SONG_IO_NATIVE_BZT(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_song_io_native_bzt_init(GTypeInstance * const instance, gconstpointer g_class) {
  BtSongIONativeBZT * const self = BT_SONG_IO_NATIVE_BZT(instance);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_SONG_IO_NATIVE_BZT, BtSongIONativeBZTPrivate);
}

static void bt_song_io_native_bzt_class_init(BtSongIONativeBZTClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);
  BtSongIOClass * const btsongio_class = BT_SONG_IO_CLASS(klass);

  error_domain=g_type_qname(BT_TYPE_SONG_IO_NATIVE_BZT);
  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtSongIONativeBZTPrivate));

  gobject_class->dispose      = bt_song_io_native_bzt_dispose;
  
  btsongio_class->load        = bt_song_io_native_bzt_load;
  btsongio_class->save        = bt_song_io_native_bzt_save;
}

GType bt_song_io_native_bzt_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof(BtSongIONativeBZTClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_song_io_native_bzt_class_init, // class_init
      NULL, // class_finalize
      //(GClassFinalizeFunc)bt_song_io_native_bzt_class_finalize,
      NULL, // class_data
      sizeof(BtSongIONativeBZT),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_song_io_native_bzt_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(BT_TYPE_SONG_IO_NATIVE,"BtSongIONativeBZT",&info,0);
  }
  return type;
}
