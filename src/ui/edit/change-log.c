/* $Id$
 *
 * Buzztard
 * Copyright (C) 2010 Buzztard team <buzztard-devel@lists.sf.net>
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
 * SECTION:btchangelog
 * @short_description: class for the editor action journaling
 *
 * Tracks edits actions since last save. Logs those to disk for crash recovery.
 * Provides undo/redo.
 */
/* @todo: reset change-log on new/open-song (song property)
 * - flush old entries
 * - remove old log file
 * - start new log file
 * @todo: rename change-log on save-as (notify on song:name)
 * - rename log file
 *
 * @todo: do we want an iface, so that objects can implement undo/redo vmethods
 *
 * @todo: what info do we need in a changelog entry?
 * - owner (object that implements undo/redo iface)
 * - serialized data
 */

#define BT_EDIT
#define BT_CHANGE_LOG_C

#include "bt-edit.h"

struct _BtChangeLogPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /*
  BtSong *song;
  gchar *song_file_name;
  
  gchar *cache_dir; // xdg-cache dir, init in constructor
  */
};

static GObjectClass *parent_class=NULL;

static BtChangeLog *singleton=NULL;

//-- helper

//-- constructor methods

/**
 * bt_change_log_new:
 *
 * Create a new instance on first call and return a reference later on.
 *
 * Returns: the new signleton instance
 */
BtChangeLog *bt_change_log_new(void) {
  return (g_object_new(BT_TYPE_CHANGE_LOG,NULL));
}


//-- methods

/*
*/


//-- class internals

static void bt_change_log_dispose(GObject *object) {
  BtChangeLog *self = BT_CHANGE_LOG(object);
  
  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  

  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_change_log_finalize(GObject *object) {
  BtChangeLog *self = BT_CHANGE_LOG(object);
  
  GST_DEBUG("!!!! self=%p",self);

  G_OBJECT_CLASS(parent_class)->finalize(object);
  singleton=NULL;
}

static GObject *bt_change_log_constructor(GType type,guint n_construct_params,GObjectConstructParam *construct_params) {
  GObject *object;

  if(G_UNLIKELY(!singleton)) {
    object=G_OBJECT_CLASS(parent_class)->constructor(type,n_construct_params,construct_params);
    singleton=BT_CHANGE_LOG(object);

    // initialise
    // singleton->priv->xxx=...;
  }
  else {
    object=g_object_ref(singleton);
  }
  return object;
}

static void bt_change_log_init(GTypeInstance *instance, gpointer g_class) {
  BtChangeLog *self = BT_CHANGE_LOG(instance);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_CHANGE_LOG, BtChangeLogPrivate);
}

static void bt_change_log_class_init(BtChangeLogClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtChangeLogPrivate));

  gobject_class->constructor  = bt_change_log_constructor;
  gobject_class->dispose      = bt_change_log_dispose;
  gobject_class->finalize     = bt_change_log_finalize;
}

GType bt_change_log_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof(BtChangeLogClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_change_log_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtChangeLog),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_change_log_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(G_TYPE_OBJECT,"BtChangeLog",&info,0);
  }
  return type;
}
