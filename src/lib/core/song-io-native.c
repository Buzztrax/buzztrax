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

static gboolean bt_song_io_init(void);

/**
 * bt_song_io_native_module_info:
 *
 * Buzztard native format song loader/saver metadata.
 */
BtSongIOModuleInfo bt_song_io_native_module_info = {
  bt_song_io_init,
  {
#ifdef USE_GSF
    { 0, "buzztard song with externals", "audio/x-bzt", "bzt" },
#endif
    { 0, "buzztard song without externals", "audio/x-bzt-xml", "xml" },
    { 0, NULL, NULL, NULL }
  }
};

static gboolean bt_song_io_init(void) {
  static gboolean first_run=TRUE;

  if(first_run) {
#ifdef ENABLE_NLS
    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

    GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "bt-bsl", 0, "music production environment / buzz song io plugin");
    
#ifdef USE_GSF
    bt_song_io_native_module_info.formats[0].type=BT_TYPE_SONG_IO_NATIVE_BZT;
    bt_song_io_native_module_info.formats[1].type=BT_TYPE_SONG_IO_NATIVE_XML;
#else
    bt_song_io_native_module_info.formats[0].type=BT_TYPE_SONG_IO_NATIVE_XML;
#endif
    first_run=FALSE;
  }
  return(TRUE);
}

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
