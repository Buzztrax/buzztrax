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
/* @todo: reset change-log on new/open-song (app.notify::song)
 * - flush old entries
 *
 * @todo: rename change-log on save-as (notify on song:name)
 *
 * @todo: check for logs on startup -> bt_change_log_crash_check()
 *
 * @todo: do we want an iface?
 * - then objects can implement undo/redo vmethods
 *
 * check http://github.com/herzi/gundo more
 */

#define BT_EDIT
#define BT_CHANGE_LOG_C

#include "bt-edit.h"

//-- property ids

#if 0

typedef struct {
  gboolean (*undo)(BtChangeLogger *owner,gchar *data);
  gboolean (*redo)(BtChangeLogger *owner,gchar *data);
} BtChangeLoggerClass;


typedef struct {
  BtChangeLogger *owner;
  gchar *data;
} BtChangeLogEntry;

#endif

struct _BtChangeLogPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  G_POINTER_ALIAS(BtEditApplication *,app);

  FILE *log_file;
  gchar *log_file_name;
  const gchar *cache_dir;
  
#if 0
  /* each entry pointing to a BtChangeLogEntry */
  GPtrArray *change_stack;
  guint last_saved;
  guint current;
#endif
};

static GObjectClass *parent_class=NULL;

static BtChangeLog *singleton=NULL;

//-- helper

static void free_log_file(BtChangeLog *self) {
  if(self->priv->log_file) {
    fclose(self->priv->log_file);
    self->priv->log_file=NULL;
  }
  if(self->priv->log_file_name) {
    g_unlink(self->priv->log_file_name);
    g_free(self->priv->log_file_name);
    self->priv->log_file_name=NULL;
  }
}

//-- event handler

static void on_song_changed(const BtEditApplication *app,GParamSpec *arg,gpointer user_data) {
  BtChangeLog *self = BT_CHANGE_LOG(user_data);
  BtSong *song;
  BtSongInfo *song_info;
  gchar *file_name,*name,*dts;

  GST_INFO("song has changed : app=%p, self=%p",app,self);
  // get song from app
  g_object_get(self->priv->app,"song",&song,NULL);
  if(!song) {
    GST_INFO("song (null) has changed done");
    return;
  }

  // remove old log file
  free_log_file(self);
  
  // ensure cachedir
  file_name=g_build_filename(self->priv->cache_dir,PACKAGE,NULL);
  g_mkdir_with_parents(file_name,S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
  g_free(file_name);
  
  // create new log file
  g_object_get(song,"song-info",&song_info,NULL);
  g_object_get(song_info,"name",&name,"create-dts",&dts,NULL);
  file_name=g_strdup_printf("%s_%s.log",dts,name);
  g_free(name);
  g_free(dts);
  self->priv->log_file_name=g_build_filename(self->priv->cache_dir,PACKAGE,file_name,NULL);
  g_free(file_name);
  if(!(self->priv->log_file=fopen(self->priv->log_file_name,"wt"))) {
    GST_WARNING("can't open log file '%s' : %d : %s",self->priv->log_file_name,errno,g_strerror(errno));
    g_free(self->priv->log_file_name);
    self->priv->log_file_name=NULL;
  }
  else {
    g_object_get(song_info,"file-name",&file_name,NULL);
    fputs(PACKAGE" edit journal : "PACKAGE_VERSION"\n",self->priv->log_file);
    if(file_name) {
      fputs(file_name,self->priv->log_file);
      g_free(file_name);
    }
    fputs("\n",self->priv->log_file);
    fflush(self->priv->log_file);
  }
  
  // @todo: notify on name changes to move the log?
  // g_signal_connect(song, "notify::name", G_CALLBACK(on_song_file_name_changed), (gpointer)self);

  // release the reference
  g_object_unref(song_info);
  g_object_unref(song);
  GST_INFO("song has changed done");
}


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

#if 0

GList *bt_change_log_crash_check(BtChangeLog *self) {
 /*
 - can be done at any time, even if we create a new _unnamed_, it will have a
   different dts
 - should list found logs, except the current one
 - the log would need to point to the actual filename
   - only offer recover if the original file is available
 */
 return(NULL);
}

gboolean bt_change_log_recover(BtChangeLog *self,XXX *entry) {
  /*
  - we should not have any unsaved work at this momement
  - load the song pointed to be entry
  - replay the log
  - message box, asking the user to check it and save if happy
  - saving, will remove the log
  */
}

void bt_change_log_reset(BtChangeLog *self) {
  /*
  - call after a song has been saved
  - reset the on-disk log
  - update pointer last_saved pointer in change stack
  */
}

#endif


//-- class internals

static void bt_change_log_dispose(GObject *object) {
  BtChangeLog *self = BT_CHANGE_LOG(object);
  
  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  g_signal_handlers_disconnect_matched(self->priv->app,G_SIGNAL_MATCH_FUNC,0,0,NULL,on_song_changed,NULL);
  
  free_log_file(self);
  g_object_try_weak_unref(self->priv->app);

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
  }
  else {
    object=g_object_ref(singleton);
  }
  return object;
}

static void bt_change_log_init(GTypeInstance *instance, gpointer g_class) {
  BtChangeLog *self = BT_CHANGE_LOG(instance);
  
  self->priv=G_TYPE_INSTANCE_GET_PRIVATE(self,BT_TYPE_CHANGE_LOG,BtChangeLogPrivate);
  /* this is created from the app, we need to avoid a ref-cycle */
  self->priv->app = bt_edit_application_new();
  g_object_try_weak_ref(self->priv->app);
  g_object_unref(self->priv->app);

  self->priv->cache_dir=g_get_user_cache_dir();
  g_signal_connect(self->priv->app, "notify::song", G_CALLBACK(on_song_changed), (gpointer)self);
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
