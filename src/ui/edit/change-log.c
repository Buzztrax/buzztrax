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
/* @todo: we should also check for pending logs when opening a file!
 * - need to keep the change-log entries as a hash-table and offer api to check
 *   for it and if wanted replay
 * - bt_change_log_recover calls bt_edit_application_load_song
 *   - maybe we need to refactor this a bit
 * - we could make bt_change_log_crash_check() static and run this in _init()
 *   - the list of crash-log entries would be available as a property
 *
 * @todo: need change grouping
 * - when clearing a selection, we can represent this as a group of edits
 * - bt_change_log_{start,end}_group
 * - groups should be hierarchical
 *
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

new:  []         u[]     r[]     next_undo,next_redo,item_ct
                  ^        ^     -1,0,0
add:  [a]        u[ ]    r[a]
                   ^        ^     0,1,1
add:  [ab]       u[  ]   r[ab]
                    ^ 1      ^    1,2,2
add:  [abc]      u[   ]  r[abc]
                     ^        ^   2,3,3
undo: [ab]       u[   ]  r[abc]    \
                    ^        ^    1,2,3
redo: [abc]      u[   ]  r[abc]    /
                     ^        ^   2,3,3
undo: [ab]       u[   ]  r[abc]    \
                    ^        ^    1,2,3
add:  [abd]      u[   ]  r[abd]    /    { this looses the 'c' }
                     ^        ^   2,3,3
undo: [ab]       u[   ]  r[abd]    \
                    ^        ^    1,2,3
undo: [a]        u[   ]  r[abd]    \
                   ^        ^     0,1,3
add:  [ae]       u[   ]  r[ae]     /    { this looses the 'bd' }
                    ^        ^    1,2,2

-- can we get the 'c' back - no
when adding a new action, we will always truncate the undo/redo stack.
Otherwise it becomes a graph and then redo would need to know the direction.

*/

#define BT_EDIT
#define BT_CHANGE_LOG_C

#include "bt-edit.h"

#include <dirent.h>

//-- property ids

enum {
  CHANGE_LOG_CAN_UNDO=1,
  CHANGE_LOG_CAN_REDO,
  CHANGE_LOG_CRASH_LOGS
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
  gchar *cache_dir;
  
  /* known ChangeLoggers */
  GHashTable *loggers;
  
  /* each entry pointing to a BtChangeLogEntry */
  GPtrArray *changes;
  guint last_saved;
  gint next_redo;  // -1 for none, or just have pointers?
  gint next_undo;  // -1 for none, or just have pointers?
  gint item_ct;

  /* crash log entries */
  GList *crash_logs;
};

static BtChangeLog *singleton=NULL;

#define BT_CHANGE_LOG_MAX_HEADER_LINE_LEN 200
// date time stamp format YYYY-MM-DDThh:mm:ssZ
#define DTS_LEN 20

//-- the class

G_DEFINE_TYPE (BtChangeLog, bt_change_log, G_TYPE_OBJECT);

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

static gchar *make_log_file_name(BtChangeLog *self, BtSongInfo *song_info) {
  gchar *file_name,*name,*log_file_name;
  time_t now=time(NULL);
  gchar dts[DTS_LEN+1];

  /* we need unique filenames (see bt_change_log_recover() */
  g_object_get(song_info,"name",&name,/*"change-dts",&dts,*/NULL);
  strftime(dts,DTS_LEN+1,"%FT%TZ",gmtime(&now));
  file_name=g_strdup_printf("%s_%s.log",dts,name);
  g_free(name);
  //g_free(dts);
  
  log_file_name=g_build_filename(self->priv->cache_dir,file_name,NULL);
  g_free(file_name);
  return(log_file_name);
}

static void open_and_init_log(BtChangeLog *self, BtSongInfo *song_info) {
  if(!(self->priv->log_file=fopen(self->priv->log_file_name,"wt"))) {
    GST_WARNING("can't open log file '%s' : %d : %s",self->priv->log_file_name,errno,g_strerror(errno));
    g_free(self->priv->log_file_name);
    self->priv->log_file_name=NULL;
  }
  else {
    gchar *file_name;

    g_object_get(song_info,"file-name",&file_name,NULL);
    fputs(PACKAGE" edit journal : "PACKAGE_VERSION"\n",self->priv->log_file);
    if(file_name) {
      fputs(file_name,self->priv->log_file);
      g_free(file_name);
    }
    fputs("\n",self->priv->log_file);
    fflush(self->priv->log_file);
  }
}

static gint sort_by_mtime(gconstpointer a,gconstpointer b) {
  return(((BtChangeLogFile *)b)->mtime - ((BtChangeLogFile *)a)->mtime);
}

/*
 * bt_change_log_crash_check:
 * @self: the changelog
 *
 * Looks for changelog-journals in the cache dir. Finding those would indicate
 * that a song has not been saved (e.g the app crashed). This check can be done
 * at any time, but application startup would make most sense.
 * Currently open songs (which have a open journal) are filtered.
 *
 * Returns: a list with journals, free when done.
 */
static void bt_change_log_crash_check(BtChangeLog *self) {
  DIR *dirp;
  GList *crash_logs=NULL;
  
  if(!self->priv->cache_dir)
    return;
  
  if((dirp=opendir(self->priv->cache_dir))) {
    const struct dirent *dire;
    gchar link_target[FILENAME_MAX],log_name[FILENAME_MAX];
    FILE *log_file;
    gboolean valid_log,auto_clean;
  
    GST_INFO("looking for crash left-overs in %s",self->priv->cache_dir);
    while((dire=readdir(dirp))!=NULL) {
      // skip names starting with a dot
      if((!dire->d_name) || (*dire->d_name=='.')) continue;
      g_sprintf(log_name,"%s"G_DIR_SEPARATOR_S"%s",self->priv->cache_dir,dire->d_name);
      // skip symlinks
      if(readlink((const char *)log_name,link_target,FILENAME_MAX-1)!=-1) continue;
      // skip files other than logs and our current log
      if(!g_str_has_suffix(log_name,".log")) continue;
      GST_INFO("    found file '%s'",log_name);

      /* run a pre-check over the logs
       * - auto-clean logs that only consist of the header (2 lines)
       * - the log would need to point to the actual filename
       *   - only offer recover if the original file is available or
       *     if file-name is empty
       *   - otherwise auto-clean
       */
      valid_log=auto_clean=FALSE;
      if((log_file=fopen(log_name,"rt"))) {
        gchar linebuf[BT_CHANGE_LOG_MAX_HEADER_LINE_LEN];
        gchar song_file_name[BT_CHANGE_LOG_MAX_HEADER_LINE_LEN];
        BtChangeLogFile *crash_log;
        struct stat fileinfo;
        
        if(!(fgets(linebuf, BT_CHANGE_LOG_MAX_HEADER_LINE_LEN, log_file))) {
          GST_INFO("    '%s' is not a change log, eof too early",log_name);
          goto done;
        }
        if(!g_str_has_prefix(linebuf, PACKAGE" edit journal : ")) {
          GST_INFO("    '%s' is not a change log, wrong header",log_name);
          goto done;
        }
        // from now one, we know its a log, if its useless we can kill it
        if(!(fgets(song_file_name, BT_CHANGE_LOG_MAX_HEADER_LINE_LEN, log_file))) {
          GST_INFO("    '%s' is not a change log, eof too early",log_name);
          auto_clean=TRUE;
          goto done;
        }
        g_strchomp(song_file_name);
        if(*song_file_name && !g_file_test (song_file_name, G_FILE_TEST_IS_REGULAR|G_FILE_TEST_EXISTS)) {
          GST_INFO("    '%s' a change log for '%s' but that file does not exists",log_name,song_file_name);
          auto_clean=TRUE;
          goto done;
        }
        if(!(fgets(linebuf, BT_CHANGE_LOG_MAX_HEADER_LINE_LEN, log_file))) {
          GST_INFO("    '%s' is an empty change log",log_name);
          auto_clean=TRUE;
          goto done;
        }
        valid_log=TRUE;
        // add to crash_entries list
        crash_log=g_slice_new(BtChangeLogFile);
        crash_log->log_name=g_strdup(log_name);
        crash_log->song_file_name=*song_file_name?g_strdup(song_file_name):g_strdup(_("unsaved song"));
        stat(log_name,&fileinfo);
        strftime(linebuf,BT_CHANGE_LOG_MAX_HEADER_LINE_LEN-1,"%c",localtime(&fileinfo.st_mtime));
        crash_log->change_ts=g_strdup(linebuf);
        crash_log->mtime=fileinfo.st_mtime;
        crash_logs=g_list_prepend(crash_logs,crash_log);
      done:
        fclose(log_file);
      }
      if(!valid_log) {
        /* be careful with removing invalid log-files
         * - if g_get_user_cache_dir() returns non-sense due to some bug,
         *   we eventualy walk some random dir and remove files
         * - we can remove files that *are* log files, but invalid
         */
        if(auto_clean) {
          GST_WARNING("auto removing '%s'",log_name);
          g_remove(log_name);
        }
      }
    }
    closedir(dirp);
  }
  // sort list by time
  self->priv->crash_logs=g_list_sort(crash_logs,sort_by_mtime);
}


//-- event handler

static void on_song_file_name_changed(const BtSong *song,GParamSpec *arg,gpointer user_data) {
  BtChangeLog *self = BT_CHANGE_LOG(user_data);
  BtSongInfo *song_info;
  gchar *log_file_name;

  // move the log
  g_object_get((GObject*)song,"song-info",&song_info,NULL);
  log_file_name=make_log_file_name(self,song_info);
  g_rename(self->priv->log_file_name,log_file_name);
  g_free(self->priv->log_file_name);
  self->priv->log_file_name=log_file_name;
  g_object_unref(song_info);
}

static void on_song_file_unsaved_changed(const BtSong *song,GParamSpec *arg,gpointer user_data) {
  BtChangeLog *self = BT_CHANGE_LOG(user_data);
  BtSongInfo *song_info;
  gboolean unsaved;

  g_object_get((GObject*)song,"unsaved",&unsaved,"song-info",&song_info,NULL);
  if(!unsaved) {
    GST_WARNING("reset log file");
    // truncate the log
    fclose(self->priv->log_file);
    open_and_init_log(self,song_info);
  }
  g_object_unref(song_info);
}

static void on_song_changed(const BtEditApplication *app,GParamSpec *arg,gpointer user_data) {
  BtChangeLog *self = BT_CHANGE_LOG(user_data);
  BtSong *song;
  BtSongInfo *song_info;
  
  if(!self->priv->cache_dir)
    return;

  GST_INFO("song has changed : app=%p, self=%p",app,self);
  // get song from app
  g_object_get(self->priv->app,"song",&song,NULL);
  if(!song) {
    GST_INFO("song (null) has changed: done");
    return;
  }

  // remove old log file
  free_log_file(self);
 
  // create new log file
  g_object_get(song,"song-info",&song_info,NULL);
  self->priv->log_file_name=make_log_file_name(self,song_info);
  open_and_init_log(self,song_info);
  
  // notify on name changes to move the log
  g_signal_connect(song, "notify::name", G_CALLBACK(on_song_file_name_changed), (gpointer)self);
  // notify on unsaved changed to truncate the log when unsaved==FALSE
  g_signal_connect(song, "notify::unsaved", G_CALLBACK(on_song_file_unsaved_changed), (gpointer)self);

  // release the reference
  g_object_unref(song_info);
  g_object_unref(song);
  GST_INFO("song has changed: done");
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

/**
 * bt_change_log_recover:
 * @self: the changelog
 * @log_name: the log file to replay
 *
 * Recover the given song.
 *
 * Return: %TRUE for successful recovery.
 */
gboolean bt_change_log_recover(BtChangeLog *self,const gchar *log_name) {
  FILE *log_file;
  gboolean res=FALSE;
  
  if((log_file=fopen(log_name,"rt"))) {
    gchar linebuf[BT_CHANGE_LOG_MAX_HEADER_LINE_LEN];
    gchar *redo_data;
    BtChangeLogger *logger;
    guint lines=0,lines_ok=0;
    
    if(!(fgets(linebuf, BT_CHANGE_LOG_MAX_HEADER_LINE_LEN, log_file))) {
      GST_INFO("    '%s' is not a change log, eof too early",log_name);
      goto done;
    }
    if(!(fgets(linebuf, BT_CHANGE_LOG_MAX_HEADER_LINE_LEN, log_file))) {
      GST_INFO("    '%s' is not a change log, eof too early",log_name);
      goto done;
    }
    g_strchomp(linebuf);
    /*
    - load the song pointed to by entry or replay the new song
      - no filename = never saved -> new file
    */
    if(*linebuf) {
      if(!bt_edit_application_load_song(self->priv->app,linebuf)) {
        GST_INFO("    song '%s' failed to load",linebuf);
        goto done;
      }
    }
    // replay the log
    while(!feof(log_file)) {
      if(fgets(linebuf, BT_CHANGE_LOG_MAX_HEADER_LINE_LEN, log_file)) {
        g_strchomp(linebuf);lines++;
        GST_DEBUG("changelog-event: '%s'", linebuf);
        // log event: BtMainPagePatterns::set_global_event "simsyn","simsyn 00",8,0,c-4
        if((redo_data=strstr(linebuf,"::"))) {
          redo_data[0]='\0';
          redo_data=&redo_data[2];
          // determine owner (BtMainPagePatterns)
          if((logger=g_hash_table_lookup(self->priv->loggers,linebuf))) {
            if(bt_change_logger_change(logger,redo_data)) {
              lines_ok++;
            }
            /* FIXME: shall we add those to the new log?
             * then it would be fine to overwrite the log
             * as we have no undo-data we cannot restore the change-stack though
             */
          }
          else {
            GST_WARNING("no changelogger for '%s'",linebuf);
          }
        }
        else {
          GST_WARNING("missing :: separator in '%s'",linebuf);
        }
      }
    }
    GST_INFO("%u of %u lines replayed okay", lines_ok, lines);
    /*
      - message box, asking the user to check it and save if happy
      - defer removing the old log to saving the song
        -> on_song_file_unsaved_changed()
        - remove the change log from self->priv-crash_logs then also
    */
    res=(lines_ok==lines);
  done:
    fclose(log_file);
  }
  return(res);
}

/**
 * bt_change_log_register:
 * @self: the change log
 * @logger: a change logger
 *
 * Register a change-logger for use in change log replay.
 */
void bt_change_log_register(BtChangeLog *self,BtChangeLogger *logger) {
  GST_DEBUG("registering: '%s'",G_OBJECT_TYPE_NAME(logger));
  
  g_hash_table_insert(self->priv->loggers,(gpointer)G_OBJECT_TYPE_NAME(logger),(gpointer)logger);
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
  if(self->priv->next_redo<self->priv->item_ct) {
    gint i;
    
    GST_WARNING("trunc %d<%d",self->priv->next_redo,self->priv->item_ct);
    for(i=self->priv->item_ct-1;i>=self->priv->next_redo;i--) {
      cle=g_ptr_array_remove_index(self->priv->changes,i);
      g_free(cle->undo_data);
      g_free(cle->redo_data);
      g_slice_free(BtChangeLogEntry,cle);
      self->priv->item_ct--;
    }
  }
  /*else {
    GST_INFO("don't trunc %d>=%d",self->priv->next_redo,self->priv->item_ct);
  }*/
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

/**
 * bt_change_log_undo:
 * @self: the change log
 *
 * Undo the last action.
 */
void bt_change_log_undo(BtChangeLog *self) {
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

/**
 * bt_change_log_redo:
 * @self: the change log
 *
 * Redo the last action.
 */

void bt_change_log_redo(BtChangeLog *self) {
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

  G_OBJECT_CLASS(bt_change_log_parent_class)->dispose(object);
}

static void bt_change_log_finalize(GObject *object) {
  BtChangeLog *self = BT_CHANGE_LOG(object);
  BtChangeLogFile *crash_log;
  GList *node;
  
  GST_DEBUG("!!!! self=%p",self);
  g_free(self->priv->cache_dir);

  g_hash_table_destroy(self->priv->loggers);
  
  // free cgrash-logs list and entries
  for(node=self->priv->crash_logs;node;node=g_list_next(node)) {
    crash_log=(BtChangeLogFile *)node->data;
    g_free(crash_log->log_name);
    g_free(crash_log->song_file_name);
    g_free(crash_log->change_ts);
    g_slice_free(BtChangeLogFile,crash_log);
  }
  g_list_free(self->priv->crash_logs);

  G_OBJECT_CLASS(bt_change_log_parent_class)->finalize(object);
}

static GObject *bt_change_log_constructor(GType type,guint n_construct_params,GObjectConstructParam *construct_params) {
  GObject *object;

  if(G_UNLIKELY(!singleton)) {
    object=G_OBJECT_CLASS(bt_change_log_parent_class)->constructor(type,n_construct_params,construct_params);
    singleton=BT_CHANGE_LOG(object);
    g_object_add_weak_pointer(object,(gpointer*)(gpointer)&singleton);
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
    case CHANGE_LOG_CRASH_LOGS: {
      g_value_set_pointer(value,self->priv->crash_logs);
    } break;
    default: {
       G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_change_log_init(BtChangeLog *self) {  
  self->priv=G_TYPE_INSTANCE_GET_PRIVATE(self,BT_TYPE_CHANGE_LOG,BtChangeLogPrivate);
  /* this is created from the app, we need to avoid a ref-cycle */
  self->priv->app = bt_edit_application_new();
  g_object_try_weak_ref(self->priv->app);
  g_object_unref(self->priv->app);

  self->priv->cache_dir=g_build_filename(g_get_user_cache_dir(),PACKAGE,NULL);
  /* ensure cachedir, set to NULL if we can't make it to deactivate the change logger */
  if(g_mkdir_with_parents(self->priv->cache_dir,S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH)==-1) {
    GST_WARNING("Can't create change-log dir: '%s': %s",self->priv->cache_dir, g_strerror(errno));
    g_free(self->priv->cache_dir);
    self->priv->cache_dir=NULL;
  }

  self->priv->loggers=g_hash_table_new(g_str_hash,g_str_equal);
  
  self->priv->changes=g_ptr_array_new();
  self->priv->next_undo=-1;
  self->priv->next_redo=0;
  
  bt_change_log_crash_check(self);

  g_signal_connect(self->priv->app, "notify::song", G_CALLBACK(on_song_changed), (gpointer)self);
}

static void bt_change_log_class_init(BtChangeLogClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

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

  g_object_class_install_property(gobject_class,CHANGE_LOG_CRASH_LOGS,
                                  g_param_spec_pointer("crash-logs",
                                     "crash logs prop",
                                     "A list of found crash logs",
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));
}

