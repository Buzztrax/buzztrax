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
 * @todo: need change grouping
 * - when clearing a selection, we can represent this as a group of edits
 * - bt_change_log_{start,end}_group
 * - groups should be hierarchical
 *
 * @todo: need a way to log object identifiers and a mechanism to llok then up
 * when replaying a log
 * 1.)
 * - each class that implement change_logger registers a get_child_by_name to the change log
 * - all vmethods are store in a hastable with the type name as the key
 * - all new object are added in a class local hashtable and removed on dispose
 * - then log format is: type::name
 * - to lock up one object we get the type, pick the get_child_by_name and call
 *   instance = get_child_by_name(name);
 *
 * check http://github.com/herzi/gundo more
 *
 * empty stack: undo: -1               redo: 0
 *               -                      -
 *
 * add:         undo: 0                redo: 1
 *              >pattern.set(0,0,0.0)   pattern.set(0,0,1.0)
 *
 * add:         undo: 1                redo: 2
 *               pattern.set(0,0,0.0)   pattern.set(0,0,1.0)
 *              >pattern.set(0,0,1.0)   pattern.set(0,0,2.0)
 *
 * undo:        undo: 0                redo: 1
 *              >pattern.set(0,0,0.0)   pattern.set(0,0,1.0)
 *               pattern.set(0,0,1.0)  >pattern.set(0,0,2.0)
 *
 * redo:        undo: 1                redo: 2
 *               pattern.set(0,0,0.0)   pattern.set(0,0,1.0)
 *              >pattern.set(0,0,1.0)   pattern.set(0,0,2.0)

new:  []         u[]     r[]
                  ^        ^     -1,0
add:  [a]        u[ ]    r[a]
                   ^        ^     0,1
add:  [ab]       u[  ]   r[ab]
                    ^ 1      ^    1,2
add:  [abc]      u[   ]  r[abc]
                     ^        ^   2,3
undo: [ab]       u[   ]  r[abc]    \
                    ^        ^    1,2
redo: [abc]      u[   ]  r[abc]    /
                     ^        ^   2,3
undo: [ab]       u[   ]  r[abc]    \
                    ^        ^    1,2
add:  [abd]      u[   ]  r[abd]    /    { this looses the 'c' }
                     ^        ^   2,3
undo: [ab]       u[   ]  r[abd]    \
                    ^        ^    1,2

-- can we get the 'c' back - no
when adding a new action, we will always truncate the undo/redo stack.
Otherwise it becomes a graph and then redo would need to know the direction.

*/

#define BT_EDIT
#define BT_CHANGE_LOG_C

#include "bt-edit.h"

//-- signal ids

//-- property ids

enum {
  CHANGE_LOG_CAN_UNDO=1,
  CHANGE_LOG_CAN_REDO
};

//-- structs

typedef struct {
  BtChangeLogger *owner;
  gchar *undo_data;
  gchar *redo_data;
} BtChangeLogEntry;

struct _BtChangeLogPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  G_POINTER_ALIAS(BtEditApplication *,app);

  FILE *log_file;
  gchar *log_file_name;
  const gchar *cache_dir;
  
  /* each entry pointing to a BtChangeLogEntry */
  GPtrArray *changes;
  guint last_saved;
  gint next_redo;  // -1 for none, or just have pointers?
  gint next_undo;  // -1 for none, or just have pointers?
  gint item_ct;
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

GList *bt_change_log_crash_check(BtChangeLog *self) {
 /*
 - can be done at any time, even if we create a new _unnamed_, it will have a
   different dts
 - should list found logs, except the current (loaded song) one
 - run a pre-check over the logs
   - auto-clean logs that only consis of the header
   - the log would need to point to the actual filename
     - only offer recover if the original file is available or
       if file-anem is empty
 - should we have a way to delete logs?
   - song not there anymore
   - log was replayed, song saved, but then it crashed (before resetting the log)
 */
#if 0
  gchar *path=g_build_filename(self->priv->cache_dir,PACKAGE,NULL);
  DIR * const dirp=opendir(path);
 
  if(dirp) {
    const struct dirent *dire;
    gchar link_target[FILENAME_MAX],log_name[FILENAME_MAX];
 
    while((dire=readdir(dirp))!=NULL) {
      // skip names starting with a dot
      if((!dire->d_name) || (*dire->d_name=='.')) continue;
      g_sprintf(log_name,"%s"G_DIR_SEPARATOR_S"%s",path,dire->d_name);
      // skip symlinks
      if(readlink((const char *)log_name,link_target,FILENAME_MAX-1)!=-1) continue;
      // skip files other then shared librares
      if(!g_str_has_suffix(log_name,".log")) continue;
      GST_INFO("    found file '%s'",log_name);
      
      // ...
    }
    closedir(dirp);
  }
  g_free(path);
#endif
 return(NULL);
}

gboolean bt_change_log_recover(BtChangeLog *self,const gchar *entry) {
  /*
  - we should not have any unsaved work at this momement
  - load the song pointed to by entry or replay the new song
    (no filename = never saved)
  - replay the log
    - read the header
    - foreach line
      - determine owner
      - parse redo_data (there is only redo data)
      - bt_change_logger_change(owner,redo_data);
  - message box, asking the user to check it and save if happy
  - saving and proper closing the song will remove the log
  */
#if 0
#endif
  return(FALSE);
}

void bt_change_log_reset(BtChangeLog *self) {
  /*
  - call after a song has been saved
  - reset the on-disk log
    - seek to start and rewrite the header
  - update pointer last_saved pointer in change stack
  */
}

/**
 * bt_change_log_add:
 * @self: the change log
 * @owner: the owner of the change
 * @undo_data: how to undo the change
 * @redo_data: how to redo the change
 *
 * Add a new change to the change log. Changes are passed as serialized strings.
 * The change-log takes ownership of  @undo_data and @redo_data.
 */
void bt_change_log_add(BtChangeLog *self,BtChangeLogger *owner,gchar *undo_data,gchar *redo_data) {
  BtChangeLogEntry *cle;
  
  // if we have redo data, we have to cut the trail here otherwise the history
  // becomes a graph
  if(self->priv->next_redo<(self->priv->item_ct-1)) {
    gint i;
    
    GST_WARNING("trunc %d<%d",self->priv->next_redo,(self->priv->item_ct-1));
    for(i=self->priv->item_ct-1;i>self->priv->next_redo;i--) {
      cle=g_ptr_array_remove_index(self->priv->changes,i);
      g_free(cle->undo_data);
      g_free(cle->redo_data);
      g_slice_free(BtChangeLogEntry,cle);
      self->priv->item_ct--;
    }
  }
  // make new BtChangeLogEntry from the parameters
  cle=g_slice_new(BtChangeLogEntry);
  cle->owner=owner;
  cle->undo_data=undo_data;
  cle->redo_data=redo_data;
  // append and update undo undo/redo pointers
  g_ptr_array_add(self->priv->changes,cle);
  self->priv->item_ct++;
  // owners are the editor objects where the change was made
  if(self->priv->log_file) {
    fprintf(self->priv->log_file,"%s::%s\n",G_OBJECT_TYPE_NAME(owner),redo_data);
    fflush(self->priv->log_file);
  }
  // update undo undo/redo pointers
  self->priv->next_undo++;
  self->priv->next_redo++;
  GST_INFO("add %d[%s], %d[%s]",self->priv->next_undo,undo_data,self->priv->next_redo,redo_data);
  if(self->priv->next_undo==0) {
    g_object_notify((GObject *)self,"can-undo");
  }
}

void bt_change_log_undo(BtChangeLog *self)
{
  if(self->priv->next_undo!=-1) {
    BtChangeLogEntry *cle=g_ptr_array_index(self->priv->changes,self->priv->next_undo);
    bt_change_logger_change(cle->owner,cle->undo_data);
    // update undo undo/redo pointers
    self->priv->next_redo=self->priv->next_undo;
    self->priv->next_undo--;
    GST_INFO("undo %d, %d",self->priv->next_undo,self->priv->next_redo);
    if(self->priv->next_undo==-1) {
      g_object_notify((GObject *)self,"can-undo");
    }
    if(self->priv->next_redo==(self->priv->item_ct-1)) {
      g_object_notify((GObject *)self,"can-redo");
    }
  }
}

void bt_change_log_redo(BtChangeLog *self)
{
  if(self->priv->next_redo!=-1) {
    BtChangeLogEntry *cle=g_ptr_array_index(self->priv->changes,self->priv->next_redo);
    bt_change_logger_change(cle->owner,cle->redo_data);
    // update undo undo/redo pointers
    self->priv->next_undo=self->priv->next_redo;
    self->priv->next_redo++;
    GST_INFO("redo %d, %d",self->priv->next_undo,self->priv->next_redo);
    if(self->priv->next_redo==self->priv->item_ct) {
      g_object_notify((GObject *)self,"can-redo");
    }
    if(self->priv->next_undo==0) {
      g_object_notify((GObject *)self,"can-undo");
    }
  }
}

//-- class internals

static void bt_change_log_dispose(GObject *object) {
  BtChangeLog *self = BT_CHANGE_LOG(object);
  BtChangeLogEntry *cle;
  guint i;
  
  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  
  for(i=0;i<self->priv->item_ct;i++) {
    cle=g_ptr_array_index(self->priv->changes,i);
    g_free(cle->undo_data);
    g_free(cle->redo_data);
    g_slice_free(BtChangeLogEntry,cle);
  }
  g_ptr_array_free(self->priv->changes,TRUE);
  
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

static void bt_change_log_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec) {
  BtChangeLog *self = BT_CHANGE_LOG(object);
  return_if_disposed();
  switch (property_id) {
    case CHANGE_LOG_CAN_UNDO: {
      g_value_set_boolean(value,(self->priv->next_undo!=-1));
    } break;
    case CHANGE_LOG_CAN_REDO: {
      g_value_set_boolean(value,(self->priv->next_redo!=self->priv->item_ct));
    } break;
    default: {
       G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
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
  
  self->priv->changes=g_ptr_array_new();
  self->priv->next_undo=-1;
  self->priv->next_redo=0;
}

static void bt_change_log_class_init(BtChangeLogClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtChangeLogPrivate));

  gobject_class->get_property = bt_change_log_get_property;
  gobject_class->constructor  = bt_change_log_constructor;
  gobject_class->dispose      = bt_change_log_dispose;
  gobject_class->finalize     = bt_change_log_finalize;

  g_object_class_install_property(gobject_class,CHANGE_LOG_CAN_UNDO,
                                  g_param_spec_boolean("can-undo",
                                     "can-undo prop",
                                     "Where there are action to undo",
                                     FALSE,
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));
  
  g_object_class_install_property(gobject_class,CHANGE_LOG_CAN_REDO,
                                  g_param_spec_boolean("can-redo",
                                     "can-redo prop",
                                     "Where there are action to redo",
                                     FALSE,
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));
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
