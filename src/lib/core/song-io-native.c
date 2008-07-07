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

struct _BtSongIONativePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
};

static BtSongIOClass *parent_class=NULL;

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
    type=BT_TYPE_SONG_IO_NATIVE_XML;
  }
#ifdef USE_GSF
  else if(g_str_has_suffix(lc_file_name,".bzt")) {
    type=BT_TYPE_SONG_IO_NATIVE_BZT;
  }
#endif
  g_free(lc_file_name);

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
#ifdef USE_GSF
    { "buzztard song with externals", "audio/x-bzt", "bzt" },
#endif
    { "buzztard song without externals", "audio/x-bzt-xml", "xml" },
    { NULL, }
  }
};

//-- methods

//-- wrapper

//-- class internals

static void bt_song_io_native_dispose(GObject * const object) {
  const BtSongIONative * const self = BT_SONG_IO_NATIVE(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_song_io_native_init(GTypeInstance * const instance, gconstpointer g_class) {
  BtSongIONative * const self = BT_SONG_IO_NATIVE(instance);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_SONG_IO_NATIVE, BtSongIONativePrivate);
}

static void bt_song_io_native_class_init(BtSongIONativeClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtSongIONativePrivate));

  gobject_class->dispose      = bt_song_io_native_dispose; 
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
    type = g_type_register_static(BT_TYPE_SONG_IO,"BtSongIONative",&info,G_TYPE_FLAG_ABSTRACT);
  }
  return type;
}
