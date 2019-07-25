/* Buzztrax
 * Copyright (C) 2010 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
/**
 * SECTION:btchangelog
 * @short_description: class for the editor action journaling
 * @see_also: #BtCrashRecoverDialog
 *
 * Tracks edits actions since last save. Provides undo/redo. Supports grouping
 * of edits into single undo/redo items (see bt_change_log_start_group()).
 *
 * Edit actions are logged to disk under the users-cache directory for crash
 * recovery. Groups are logged atomically, when they are closed (to have a
 * recoverable log).
 *
 * Logs are reset when saving a song. The log is removed when a song is closed.
 *
 * #BtEditApplication checks for left-over logs at startup and uses
 * #BtCrashRecoverDialog to offer a list of recoverable songs to the user.
 *
 * When running the application with BT_DEBUG_CHANGE_LOG=1 and if debugging is
 * enabled, one will get extra comments in the edit journal.
 */
/* TODO(ensonic): we should also check for pending logs when opening a file!
 * - this happens if the user previously skipped recovery
 * - need to keep the change-log entries as a hash-table and offer api to check
 *   for it and if wanted replay
 * - bt_change_log_recover calls bt_edit_application_load_song
 *   - maybe we need to refactor this a bit
 */
/* TODO(ensonic): using groups
 * - start and stop the group in the UI-callback that causes e.g. deleting an
 *   item
 * - make sure all on_xxx_remove handler are called before closing the group
 * - in the on_xxx_handlers can we unconditionally add to the log?
 *   - we want to avoid that stuff gets logged:
 *     - when loading a song
 *     - when disposing a song
 *     - when undo/redo-ing actions
 *   - when loading we set the song to the application, when it is done
 *   - when disposing we unset the song from the application and then dispose
 *   - we catch the case of app->song==NULL already here
 *   - ideally we want a cheap way of checking wheter the change-log is active
 *     to avoid builing the undo/redo strings if they are not needed
 *     (in undo/redo already or construction/destructiuon)
 *     -> add gboolean bt_change_log_is_active(BtChangeLog *self)
 * - log example (remove effect machine with two wires conencted)
 *   bt_main_page_machines_delete_machine: undo/redo: remove machine beg
 *   on_machine_removed: undo/redo: beg
 *   on_machine_removed: undo/redo: end
 *   on_wire_removed: undo/redo: beg
 *   on_wire_removed: undo/redo: end
 *   on_wire_removed: undo/redo: beg
 *   on_wire_removed: undo/redo: end
 *   bt_main_page_machines_delete_machine: undo/redo: remove machine end
 */

/* scheduling design
 * - add/remove signals happen at core level
 *   - add after adding
 *   - remove before removing
 *   - there can be chains:
 *     setup::machine-removed(setup:wire-removed,machine:pattern-removed)
 * - undo/redo happens at UI level
 *   - what we'd like to do is when we trigger something from the ui:
 *     - start the group
 *     - add/remove the top-level element
 *     - handle undo/redo for all subsequenty triggered elements
 *     - close the group
 *   - when adding a top-level caused adding other stuff we don't want to handle
 *     the implicitely added objects (as redo the add would already recreate
 *     them too)
 *   - likewise when deleting a top-level that also deletes other items, we don't
 *     want to handle them, although here we have to backup the old data:
 *     removing a machine, removes the track and patterns, redo will add the
 *     machine, patterns and track, but we have to fill the data then
 *     - the pattern removal here is implicit and thus we don't want to handle
 *       undo/redo for it in the sequence where otherwise it would clear the
 *       cells that where using the pattern
 * - we probably need to extend the machine:pattern-{added,removed} and
 *   setup:{machine,wire}-{added,removed} signals with a gboolean explicit
 *   parameter, but that won't work as we call this from core too
 */

/* design: the undo/redo stack
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
 *
 * new:  []         u[]     r[]     next_undo,next_redo,item_ct
 *                   ^        ^     -1,0,0
 * add:  [a]        u[ ]    r[a]
 *                    ^        ^     0,1,1
 * add:  [ab]       u[  ]   r[ab]
 *                     ^ 1      ^    1,2,2
 * add:  [abc]      u[   ]  r[abc]
 *                      ^        ^   2,3,3
 * undo: [ab]       u[   ]  r[abc]    \
 *                     ^        ^    1,2,3
 * redo: [abc]      u[   ]  r[abc]    /
 *                      ^        ^   2,3,3
 * undo: [ab]       u[   ]  r[abc]    \
 *                     ^        ^    1,2,3
 * add:  [abd]      u[   ]  r[abd]    /    { this looses the 'c' }
 *                      ^        ^   2,3,3
 * undo: [ab]       u[   ]  r[abd]    \
 *                     ^        ^    1,2,3
 * undo: [a]        u[   ]  r[abd]    \
 *                    ^        ^     0,1,3
 * add:  [ae]       u[   ]  r[ae]     /    { this looses the 'bd' }
 *                     ^        ^    1,2,2
 *
 * -- can we get the 'c' back - no
 * when adding a new action, we will always truncate the undo/redo stack.
 * Otherwise it becomes a graph and then redo would need to know the direction.
 *
 */
/* design: change grouping
 * - some action are a sequence of undo/redo actions
 *   - when clearing a selection, we can represent this as a group of edits
 *   - undo it the remove of a machine (and its patterns)
 * - self->priv->changes is a array of BtChangeLogEntry
 * - we need to have a special entry, that again contains a array of
 *   BtChangeLogEntry
 *   - we could make BtChangeLogEntry a union and add a 'type' field first
 *   - one would call bt_change_log_{start,end}_group to brace grouped changes
 *   - bt_change_log_add would add to the active group (default = top_level)
 * - groups could be hierarchical, but are applied only as a whole
 * - bt_change_log_undo/redo would need to check for groups and in that case loop
 *   over the group
 * - the log-file serialisation can ignore the groups
 */

#define BT_EDIT
#define BT_CHANGE_LOG_C

#include "bt-edit.h"
#include <glib/gstdio.h>
#include <dirent.h>

//-- property ids

enum
{
  CHANGE_LOG_CAN_UNDO = 1,
  CHANGE_LOG_CAN_REDO,
  CHANGE_LOG_CRASH_LOGS
};

//-- structs

typedef enum
{
  CHANGE_LOG_ENTRY_SINGLE = 0,
  CHANGE_LOG_ENTRY_GROUP
} BtChangeLogEntryType;

typedef struct
{
  BtChangeLogEntryType type;
} BtChangeLogEntry;

typedef struct
{
  BtChangeLogEntryType type;
  BtChangeLogger *owner;
  gchar *undo_data;
  gchar *redo_data;
} BtChangeLogEntrySingle;

typedef struct _BtChangeLogEntryGroup BtChangeLogEntryGroup;
struct _BtChangeLogEntryGroup
{
  BtChangeLogEntryType type;
  GPtrArray *changes;
  BtChangeLogEntryGroup *old_group;
};

struct _BtChangeLogPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the application */
  BtEditApplication *app;

  /* logging is inactive during song-construction/destruction and when
   * replaying and undo/redo action */
  gboolean is_active;

  /* log file */
  FILE *log_file;
  gchar *log_file_name;
  gchar *cache_dir;

  /* known ChangeLoggers */
  GHashTable *loggers;

  /* each entry pointing to a BtChangeLogEntry{Single,Group} */
  GPtrArray *changes;
  guint last_saved;
  gint next_redo;               // -1 for none, or just have pointers?
  gint next_undo;               // -1 for none, or just have pointers?
  gint item_ct;                 // same as changed->len, but also accesible when changes=NULL
  BtChangeLogEntryGroup *cur_group;

  /* crash log entries */
  GList *crash_logs;

#ifdef USE_DEBUG
  /* write extra comments into the log, env: BT_DEBUG_CHANGE_LOG=1 */
  gboolean debug_mode;
#endif
};

static BtChangeLog *singleton = NULL;

#define BT_CHANGE_LOG_MAX_HEADER_LINE_LEN 200
// date time stamp format YYYY-MM-DDThh:mm:ssZ
#define DTS_LEN 20

//-- the class

G_DEFINE_TYPE (BtChangeLog, bt_change_log, G_TYPE_OBJECT);

//-- helper

static void
free_change_log_entry (BtChangeLogEntry * cle)
{
  switch (cle->type) {
    case CHANGE_LOG_ENTRY_SINGLE:{
      BtChangeLogEntrySingle *cles = (BtChangeLogEntrySingle *) cle;
      g_free (cles->undo_data);
      g_free (cles->redo_data);
      g_slice_free (BtChangeLogEntrySingle, cles);
      break;
    }
    case CHANGE_LOG_ENTRY_GROUP:{
      BtChangeLogEntryGroup *cleg = (BtChangeLogEntryGroup *) cle;
      guint i;

      // recurse
      for (i = 0; i < cleg->changes->len; i++) {
        free_change_log_entry (g_ptr_array_index (cleg->changes, i));
      }
      g_ptr_array_free (cleg->changes, TRUE);
      g_slice_free (BtChangeLogEntryGroup, cleg);
      break;
    }
  }
}

static void
log_change_log_entry (BtChangeLog * self, BtChangeLogEntry * cle)
{
  if (!self->priv->log_file)
    return;

  switch (cle->type) {
    case CHANGE_LOG_ENTRY_SINGLE:{
      BtChangeLogEntrySingle *cles = (BtChangeLogEntrySingle *) cle;

      GST_DEBUG ("logging change %p", cle);
      // owners are the editor objects where the change was made
      fprintf (self->priv->log_file, "%s::%s\n",
          G_OBJECT_TYPE_NAME (cles->owner), cles->redo_data);
#ifdef USE_DEBUG
      if (self->priv->debug_mode) {
        fprintf (self->priv->log_file, "# undo: %s\n", cles->undo_data);
      }
#endif
      // IDEA(ensonic): should we fdatasync(fileno(self->priv->log_file)); from
      // time to time
      break;
    }
    case CHANGE_LOG_ENTRY_GROUP:{
      BtChangeLogEntryGroup *cleg = (BtChangeLogEntryGroup *) cle;
      gint i;

      GST_DEBUG ("logging group %p", cle);
#ifdef USE_DEBUG
      if (self->priv->debug_mode) {
        fprintf (self->priv->log_file, "# {\n");
      }
#endif
      // recurse, apply from end to start of group
      for (i = cleg->changes->len - 1; i >= 0; i--) {
        log_change_log_entry (self, g_ptr_array_index (cleg->changes, i));
      }
#ifdef USE_DEBUG
      if (self->priv->debug_mode) {
        fprintf (self->priv->log_file, "# }\n");
      }
#endif
      break;
    }
    default:
      g_assert_not_reached ();
  }
}

static void
add_change_log_entry (BtChangeLog * self, BtChangeLogEntry * cle)
{
  // only on top-level group
  if (self->priv->cur_group == NULL) {
    // if we have redo data, we have to cut the trail here otherwise the history
    // becomes a graph
    if (self->priv->item_ct && (self->priv->next_redo < self->priv->item_ct)) {
      gint i;

      GST_WARNING ("trunc %d<%d", self->priv->next_redo, self->priv->item_ct);
      for (i = self->priv->item_ct - 1; i >= self->priv->next_redo; i--) {
        free_change_log_entry (g_ptr_array_remove_index (self->priv->changes,
                i));
        self->priv->item_ct--;
      }
    }
    /*else {
       GST_INFO("don't trunc %d>=%d",self->priv->next_redo,self->priv->item_ct);
       } */
    // append and update undo undo/redo pointers
    g_ptr_array_add (self->priv->changes, cle);

    self->priv->item_ct++;
    // update undo undo/redo pointers
    self->priv->next_undo++;
    self->priv->next_redo++;
    GST_INFO ("add %d, %d", self->priv->next_undo, self->priv->next_redo);
    //GST_INFO("add %d[%s], %d[%s]",self->priv->next_undo,undo_data,self->priv->next_redo,redo_data);
    if (self->priv->next_undo == 0) {
      g_object_notify ((GObject *) self, "can-undo");
    }
  } else {
    // append and update undo undo/redo pointers
    g_ptr_array_add (self->priv->cur_group->changes, cle);
  }
}

static void
undo_change_log_entry (BtChangeLog * self, BtChangeLogEntry * cle)
{
  switch (cle->type) {
    case CHANGE_LOG_ENTRY_SINGLE:{
      BtChangeLogEntrySingle *cles = (BtChangeLogEntrySingle *) cle;

      bt_change_logger_change (cles->owner, cles->undo_data);
      if (self->priv->log_file) {
        fprintf (self->priv->log_file, "%s::%s\n",
            G_OBJECT_TYPE_NAME (cles->owner), cles->undo_data);
#ifdef USE_DEBUG
        if (self->priv->debug_mode) {
          fprintf (self->priv->log_file, "# undo: %s\n", cles->redo_data);
        }
#endif
      }
      break;
    }
    case CHANGE_LOG_ENTRY_GROUP:{
      BtChangeLogEntryGroup *cleg = (BtChangeLogEntryGroup *) cle;
      gint i;

      GST_DEBUG ("undo group %p", cle);
#ifdef USE_DEBUG
      if (self->priv->debug_mode) {
        fprintf (self->priv->log_file, "# {\n");
      }
#endif
      // recurse, apply from end to start of group
      for (i = cleg->changes->len - 1; i >= 0; i--) {
        undo_change_log_entry (self, g_ptr_array_index (cleg->changes, i));
      }
#ifdef USE_DEBUG
      if (self->priv->debug_mode) {
        fprintf (self->priv->log_file, "# }\n");
      }
#endif
      break;
    }
    default:
      g_assert_not_reached ();
  }
}

static void
redo_change_log_entry (BtChangeLog * self, BtChangeLogEntry * cle)
{
  switch (cle->type) {
    case CHANGE_LOG_ENTRY_SINGLE:{
      BtChangeLogEntrySingle *cles = (BtChangeLogEntrySingle *) cle;
      bt_change_logger_change (cles->owner, cles->redo_data);
      if (self->priv->log_file) {
        fprintf (self->priv->log_file, "%s::%s\n",
            G_OBJECT_TYPE_NAME (cles->owner), cles->redo_data);
#ifdef USE_DEBUG
        if (self->priv->debug_mode) {
          fprintf (self->priv->log_file, "# undo: %s\n", cles->undo_data);
        }
#endif
      }
      break;
    }
    case CHANGE_LOG_ENTRY_GROUP:{
      BtChangeLogEntryGroup *cleg = (BtChangeLogEntryGroup *) cle;
      gint i;

      GST_DEBUG ("redo group %p", cle);
#ifdef USE_DEBUG
      if (self->priv->debug_mode) {
        fprintf (self->priv->log_file, "# {\n");
      }
#endif
      // recurse, apply from start to end of group
      for (i = 0; i < cleg->changes->len; i++) {
        redo_change_log_entry (self, g_ptr_array_index (cleg->changes, i));
      }
#ifdef USE_DEBUG
      if (self->priv->debug_mode) {
        fprintf (self->priv->log_file, "# }\n");
      }
#endif
      break;
    }
    default:
      g_assert_not_reached ();
  }
}

static void
close_and_free_log (BtChangeLog * self)
{
  self->priv->is_active = FALSE;

  if (self->priv->log_file) {
    fclose (self->priv->log_file);
    self->priv->log_file = NULL;
  }
  if (self->priv->log_file_name) {
    g_unlink (self->priv->log_file_name);
    g_free (self->priv->log_file_name);
    self->priv->log_file_name = NULL;
  }
  if (self->priv->changes) {
    guint i, l = self->priv->changes->len;

    for (i = 0; i < l; i++)
      free_change_log_entry (g_ptr_array_index (self->priv->changes, i));
    g_ptr_array_free (self->priv->changes, TRUE);
    self->priv->changes = NULL;
    self->priv->next_undo = -1;
    self->priv->next_redo = 0;
    self->priv->item_ct = 0;
  }
}

static gchar *
make_log_file_name (BtChangeLog * self, BtSongInfo * song_info)
{
  gchar *file_name, *name, *log_file_name;
  time_t now = time (NULL);
  gchar dts[DTS_LEN + 1];

  /* we need unique filenames (see bt_change_log_recover() */
  g_object_get (song_info, "name", &name, /*"change-dts",&dts, */ NULL);
  strftime (dts, DTS_LEN + 1, "%FT%TZ", gmtime (&now));
  file_name = g_strdup_printf ("%s_%s.log", dts, name);
  g_free (name);
  //g_free(dts);

  log_file_name = g_build_filename (self->priv->cache_dir, file_name, NULL);
  g_free (file_name);
  return log_file_name;
}

static void
open_and_init_log (BtChangeLog * self, BtSongInfo * song_info)
{
  if (!(self->priv->log_file = fopen (self->priv->log_file_name, "wt"))) {
    GST_WARNING ("can't open log file '%s' : %d : %s",
        self->priv->log_file_name, errno, g_strerror (errno));
    g_free (self->priv->log_file_name);
    self->priv->log_file_name = NULL;
  } else {
    gchar *file_name;

    self->priv->changes = g_ptr_array_new ();
    self->priv->next_undo = -1;
    self->priv->next_redo = 0;
    self->priv->cur_group = NULL;
    self->priv->item_ct = 0;

    setvbuf (self->priv->log_file, (char *) NULL, _IOLBF, 0);

    g_object_get (song_info, "file-name", &file_name, NULL);
    fputs (PACKAGE " edit journal : " PACKAGE_VERSION "\n",
        self->priv->log_file);
    if (file_name) {
      fputs (file_name, self->priv->log_file);
      g_free (file_name);
    }
    fputs ("\n", self->priv->log_file);

    self->priv->is_active = TRUE;
  }
}

static gint
sort_by_mtime (gconstpointer a, gconstpointer b)
{
  return (((BtChangeLogFile *) b)->mtime - ((BtChangeLogFile *) a)->mtime);
}

static void
free_crash_log_file (BtChangeLogFile * crash_entry)
{
  g_free (crash_entry->log_name);
  g_free (crash_entry->song_file_name);
  g_free (crash_entry->change_ts);
  g_slice_free (BtChangeLogFile, crash_entry);
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
static void
bt_change_log_crash_check (BtChangeLog * self)
{
  GDir *dir;
  GList *crash_logs = NULL;

  if (!self->priv->cache_dir)
    return;

  if ((dir = g_dir_open (self->priv->cache_dir, 0, NULL))) {
    const gchar *log_name;
    gchar log_path[FILENAME_MAX];
    FILE *log_file;
    gboolean valid_log, auto_clean;

    GST_INFO ("looking for crash left-overs in %s", self->priv->cache_dir);
    while ((log_name = g_dir_read_name (dir))) {
      // skip names starting with a dot
      if (*log_name == '.')
        continue;
      g_snprintf (log_path, sizeof(log_path), "%s" G_DIR_SEPARATOR_S "%s", self->priv->cache_dir,
          log_name);
      // skip symlinks
      if (g_file_test (log_path, G_FILE_TEST_IS_SYMLINK))
        continue;
      // skip files other than logs and our current log
      if (!g_str_has_suffix (log_name, ".log"))
        continue;
      GST_INFO ("    found file '%s'", log_path);

      /* run a pre-check over the logs
       * - auto-clean logs that only consist of the header (2 lines)
       * - the log would need to point to the actual filename
       *   - only offer recover if the original file is available or
       *     if file-name is empty
       *   - otherwise auto-clean
       */
      valid_log = auto_clean = FALSE;
      if ((log_file = fopen (log_path, "rt"))) {
        gchar linebuf[BT_CHANGE_LOG_MAX_HEADER_LINE_LEN];
        gchar song_file_name[BT_CHANGE_LOG_MAX_HEADER_LINE_LEN];
        BtChangeLogFile *crash_log;
        struct stat fileinfo;

        if (!(fgets (linebuf, BT_CHANGE_LOG_MAX_HEADER_LINE_LEN, log_file))) {
          GST_INFO ("    '%s' is not a change log, eof too early", log_name);
          goto done;
        }
        if (!g_str_has_prefix (linebuf, PACKAGE " edit journal : ")) {
          GST_INFO ("    '%s' is not a change log, wrong header", log_name);
          goto done;
        }
        // from now one, we know its a log, if its useless we can kill it
        if (!(fgets (song_file_name, BT_CHANGE_LOG_MAX_HEADER_LINE_LEN,
                    log_file))) {
          GST_INFO ("    '%s' is not a change log, eof too early", log_name);
          auto_clean = TRUE;
          goto done;
        }
        g_strchomp (song_file_name);
        if (*song_file_name
            && !g_file_test (song_file_name,
                G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS)) {
          GST_INFO
              ("    '%s' a change log for '%s' but that file does not exists",
              log_name, song_file_name);
          auto_clean = TRUE;
          goto done;
        }
        if (!(fgets (linebuf, BT_CHANGE_LOG_MAX_HEADER_LINE_LEN, log_file))) {
          GST_INFO ("    '%s' is an empty change log", log_name);
          auto_clean = TRUE;
          goto done;
        }
        valid_log = TRUE;
        // add to crash_entries list
        crash_log = g_slice_new (BtChangeLogFile);
        crash_log->log_name = g_strdup (log_path);
        crash_log->song_file_name =
            *song_file_name ? g_strdup (song_file_name) :
            g_strdup (_("unsaved song"));
        if (!fstat (fileno (log_file), &fileinfo)) {
          strftime (linebuf, BT_CHANGE_LOG_MAX_HEADER_LINE_LEN - 1, "%c",
              localtime (&fileinfo.st_mtime));
        } else {
          strcpy (linebuf, "?");
          GST_WARNING ("Failed to stat '%s': %s", log_path, g_strerror (errno));
        }
        crash_log->change_ts = g_strdup (linebuf);
        crash_log->mtime = fileinfo.st_mtime;
        crash_logs = g_list_prepend (crash_logs, crash_log);
      done:
        fclose (log_file);
      }
      if (!valid_log) {
        /* be careful with removing invalid log-files
         * - if g_get_user_cache_dir() returns non-sense due to some bug,
         *   we eventualy walk some random dir and remove files
         * - we can remove files that *are* log files, but invalid
         */
        if (auto_clean) {
          GST_WARNING ("auto removing '%s'", log_name);
          if (g_remove (log_path)) {
            GST_WARNING ("failed removing '%s': %s", log_name,
                g_strerror (errno));
          }
        }
      }
    }
    g_dir_close (dir);
  }
  // sort list by time
  self->priv->crash_logs = g_list_sort (crash_logs, sort_by_mtime);
}

//-- event handler

static void
on_song_file_name_changed (const BtSong * song, GParamSpec * arg,
    gpointer user_data)
{
  BtChangeLog *self = BT_CHANGE_LOG (user_data);
  BtSongInfo *song_info;
  gchar *log_file_name;

  // move the log
  g_object_get ((GObject *) song, "song-info", &song_info, NULL);
  log_file_name = make_log_file_name (self, song_info);
  if (g_rename (self->priv->log_file_name, log_file_name)) {
    GST_WARNING ("failed renaming '%s' to '%s': %s", self->priv->log_file_name,
        log_file_name, g_strerror (errno));
  }
  g_free (self->priv->log_file_name);
  self->priv->log_file_name = log_file_name;
  g_object_unref (song_info);
}

static void
on_song_file_unsaved_changed (const BtEditApplication * app, GParamSpec * arg,
    gpointer user_data)
{
  BtChangeLog *self = BT_CHANGE_LOG (user_data);
  BtSong *song;
  BtSongInfo *song_info;
  gboolean unsaved;

  g_object_get ((GObject *) app, "unsaved", &unsaved, "song", &song, NULL);
  if (!song)
    return;

  g_object_get (song, "song-info", &song_info, NULL);
  if (!unsaved) {
    GST_DEBUG ("reset log file");
    // reset the log
    close_and_free_log (self);
    self->priv->log_file_name = make_log_file_name (self, song_info);
    open_and_init_log (self, song_info);
  } else {
    // this updates the time-stamp (we need that to show the since when we have
    // unsaved changes, if some one closes the song)
    g_object_set (song_info, "change-dts", NULL, NULL);
  }
  g_object_unref (song_info);
  g_object_unref (song);
}

static void
on_song_changed (const BtEditApplication * app, GParamSpec * arg,
    gpointer user_data)
{
  BtChangeLog *self = BT_CHANGE_LOG (user_data);
  BtSong *song;
  BtSongInfo *song_info;

  if (!self->priv->cache_dir)
    return;

  GST_DEBUG ("song has changed : app=%p, self=%p", app, self);

  // remove old log file
  close_and_free_log (self);

  // get song from app
  g_object_get (self->priv->app, "song", &song, NULL);
  if (!song) {
    GST_INFO ("song (null) has changed: done");
    return;
  }
  // create new log file
  g_object_get (song, "song-info", &song_info, NULL);
  self->priv->log_file_name = make_log_file_name (self, song_info);
  open_and_init_log (self, song_info);

  // notify on name changes to move the log
  g_signal_connect (song, "notify::name",
      G_CALLBACK (on_song_file_name_changed), (gpointer) self);
  // notify on unsaved changed to truncate the log when unsaved==FALSE
  g_signal_connect (self->priv->app, "notify::unsaved",
      G_CALLBACK (on_song_file_unsaved_changed), (gpointer) self);

  // release the reference
  g_object_unref (song_info);
  g_object_unref (song);
  GST_INFO ("song has changed: done");
}


//-- constructor methods

/**
 * bt_change_log_new:
 *
 * Create a new instance on first call and return a reference later on.
 *
 * Returns: the new signleton instance
 */
BtChangeLog *
bt_change_log_new (void)
{
  return g_object_new (BT_TYPE_CHANGE_LOG, NULL);
}


//-- methods

/**
 * bt_change_log_is_active:
 * @self: the changelog
 *
 * Checks if the changelog journalling is active. Should be checked before
 * adding entries to avoid logging when e.g. loading a song.
 *
 * Returns: %TRUE if the log is active
 */
gboolean
bt_change_log_is_active (BtChangeLog * self)
{
  return self->priv->is_active;
}

/**
 * bt_change_log_recover:
 * @self: the changelog
 * @log_name: the log file to replay
 *
 * Recover the given song.
 *
 * Return: %TRUE for successful recovery.
 */
gboolean
bt_change_log_recover (BtChangeLog * self, const gchar * log_name)
{
  FILE *log_file;
  gboolean res = FALSE;
  gboolean copy = FALSE;

  if ((log_file = fopen (log_name, "rt"))) {
    gchar linebuf[10000];       /* TODO(ensonic): a line can be a full column */
    gchar *redo_data;
    BtChangeLogger *logger;
    guint lines = 0, lines_ok = 0;

    if (!(fgets (linebuf, BT_CHANGE_LOG_MAX_HEADER_LINE_LEN, log_file))) {
      GST_INFO ("    '%s' is not a change log, eof too early", log_name);
      goto done;
    }
    if (!(fgets (linebuf, BT_CHANGE_LOG_MAX_HEADER_LINE_LEN, log_file))) {
      GST_INFO ("    '%s' is not a change log, eof too early", log_name);
      goto done;
    }
    g_strchomp (linebuf);
    /* load the song pointed to by entry or replay the new song
     * no filename = never saved -> new file
     */
    if (*linebuf) {
      GError *err = NULL;
      /* this creates a new song object and thus triggers
       * on_song_changed() where we setup a new logfile */
      if (!bt_edit_application_load_song (self->priv->app, linebuf, &err)) {
        GST_WARNING ("    song '%s' failed to load: %s", linebuf, err->message);
        // TODO(ensonic): propagate GError, bt_change_log_recover() neeeds GError arg
        g_error_free (err);
        goto done;
      }
    } else {
      /* TODO(ensonic): we need to either copy the log data to the new log or use the
       * previous log as the current one, otherwise we loose the changes */
      copy = TRUE;
    }
    // replay the log
    while (!feof (log_file)) {
      if (fgets (linebuf, BT_CHANGE_LOG_MAX_HEADER_LINE_LEN, log_file)) {
        if (copy) {
          fputs (linebuf, self->priv->log_file);
        }
        g_strchomp (linebuf);
        if (linebuf[0] == '#') {
          GST_LOG ("changelog-comment: '%s'", &linebuf[1]);
          continue;
        }
        lines++;
        GST_DEBUG ("changelog-event: '%s'", linebuf);
        // log event: BtMainPagePatterns::set_global_event "simsyn","simsyn 00",8,0,c-4
        if ((redo_data = strstr (linebuf, "::"))) {
          redo_data[0] = '\0';
          redo_data = &redo_data[2];
          // determine owner (BtMainPagePatterns)
          if ((logger = g_hash_table_lookup (self->priv->loggers, linebuf))) {
            if (bt_change_logger_change (logger, redo_data)) {
              lines_ok++;
              /* we don't add those to the new log, as we have no undo-data. Thus
               * we cannot restore the change-stack fully.
               */
            } else {
              GST_WARNING ("failed to replay line: '%s::%s'", linebuf,
                  redo_data);
            }
          } else {
            GST_WARNING ("no changelogger for '%s'", linebuf);
          }
        } else {
          GST_WARNING ("missing :: separator in '%s'", linebuf);
        }
      }
    }
    GST_INFO ("%u of %u lines replayed okay", lines_ok, lines);
    res = (lines_ok == lines);
  done:
    fclose (log_file);
    if (res) {
      GList *node;
      /* TODO(ensonic): defer removing the old log to saving the song
       *   -> on_song_file_unsaved_changed()
       *   - ev. need to store recovered_log_name in self, so that we can check
       *     it om _unsaved_changed()
       */
      g_unlink (log_name);
      for (node = self->priv->crash_logs; node; node = g_list_next (node)) {
        BtChangeLogFile *crash_entry = (BtChangeLogFile *) node->data;
        if (!strcmp (log_name, crash_entry->log_name)) {
          self->priv->crash_logs =
              g_list_delete_link (self->priv->crash_logs, node);
          free_crash_log_file (crash_entry);
          break;
        }
      }
    }
  }
  return res;
}

/**
 * bt_change_log_register:
 * @self: the change log
 * @logger: a change logger
 *
 * Register a change-logger for use in change log replay.
 */
void
bt_change_log_register (BtChangeLog * self, BtChangeLogger * logger)
{
  GST_DEBUG ("registering: '%s'", G_OBJECT_TYPE_NAME (logger));

  g_hash_table_insert (self->priv->loggers,
      (gpointer) G_OBJECT_TYPE_NAME (logger), (gpointer) logger);
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
void
bt_change_log_add (BtChangeLog * self, BtChangeLogger * owner,
    gchar * undo_data, gchar * redo_data)
{
  if (!self->priv->changes) {
    GST_WARNING ("change log not initialized?");
    return;
  }
  if (self->priv->is_active) {
    BtChangeLogEntrySingle *cle;

    // make new BtChangeLogEntry from the parameters
    cle = g_slice_new (BtChangeLogEntrySingle);
    cle->type = CHANGE_LOG_ENTRY_SINGLE;
    cle->owner = owner;
    cle->undo_data = undo_data;
    cle->redo_data = redo_data;
    GST_INFO ("add %d[%s], %d[%s]", self->priv->next_undo, undo_data,
        self->priv->next_redo, redo_data);
    add_change_log_entry (self, (BtChangeLogEntry *) cle);
    if (self->priv->cur_group == NULL) {
      // log ungrouped changes immediately
      log_change_log_entry (self, (BtChangeLogEntry *) cle);
    }
  } else {
    GST_INFO ("change log not active");
  }
}

/**
 * bt_change_log_start_group:
 * @self: the change log
 *
 * Open a new group. All further bt_change_log_add() calls will add to the
 * active group. The group needs to be closed using bt_change_log_end_group().
 * Groups can be nested.
 *
 * A top-level group is undone or redone with a single bt_change_log_undo() or
 * bt_change_log_redo() call.
 *
 * One would start and finish such a group in the first signal handler that
 * triggered the change.
 */
void
bt_change_log_start_group (BtChangeLog * self)
{
  if (!self->priv->changes) {
    GST_WARNING ("change log not initialized?");
    return;
  }
  if (self->priv->is_active) {
    BtChangeLogEntryGroup *cle;

    cle = g_slice_new (BtChangeLogEntryGroup);
    cle->type = CHANGE_LOG_ENTRY_GROUP;
    cle->changes = g_ptr_array_new ();
    cle->old_group = self->priv->cur_group;
    add_change_log_entry (self, (BtChangeLogEntry *) cle);

    // make this the current group;
    self->priv->cur_group = cle;
  } else {
    GST_INFO ("change log not active");
  }
}

/**
 * bt_change_log_end_group:
 * @self: the change log
 *
 * Closed the last group opened with bt_change_log_start_group(). Usually
 * a group would be closed in the same local scope where it was opened.
 */
void
bt_change_log_end_group (BtChangeLog * self)
{
  if (!self->priv->changes) {
    GST_WARNING ("change log not initialized?");
    return;
  }
  if (self->priv->is_active) {
    BtChangeLogEntryGroup *cle = self->priv->cur_group;

    if (cle) {
      if (!cle->changes->len) {
        GST_INFO ("closing empty changelog group");
      }
      // when we finished a top-level group, log the content to the journal
      if (cle->old_group == NULL) {
        GST_DEBUG ("closing a top-level group %p, logging changes", cle);
        log_change_log_entry (self, (BtChangeLogEntry *) cle);
      }
      self->priv->cur_group = cle->old_group;
    }
  } else {
    GST_INFO ("change log not active");
  }
}

/**
 * bt_change_log_undo:
 * @self: the change log
 *
 * Undo the last action.
 */
void
bt_change_log_undo (BtChangeLog * self)
{
  if (self->priv->next_undo != -1) {
    gboolean is_active = self->priv->is_active;
    self->priv->is_active = FALSE;

    GST_INFO ("before undo %d, %d", self->priv->next_undo,
        self->priv->next_redo);
    undo_change_log_entry (self, g_ptr_array_index (self->priv->changes,
            self->priv->next_undo));
    // update undo undo/redo pointers
    self->priv->next_redo = self->priv->next_undo;
    self->priv->next_undo--;
    GST_INFO ("after undo %d, %d", self->priv->next_undo,
        self->priv->next_redo);
    if (self->priv->next_undo == -1) {
      g_object_notify ((GObject *) self, "can-undo");
    }
    if (self->priv->next_redo == (self->priv->item_ct - 1)) {
      g_object_notify ((GObject *) self, "can-redo");
    }

    self->priv->is_active = is_active;
  } else {
    GST_INFO ("no undo at: %d, %d", self->priv->next_undo,
        self->priv->next_redo);
  }
}

/**
 * bt_change_log_redo:
 * @self: the change log
 *
 * Redo the last action.
 */

void
bt_change_log_redo (BtChangeLog * self)
{
  if (self->priv->next_redo != self->priv->item_ct) {
    gboolean is_active = self->priv->is_active;
    self->priv->is_active = FALSE;

    GST_INFO ("before redo %d, %d", self->priv->next_undo,
        self->priv->next_redo);
    redo_change_log_entry (self, g_ptr_array_index (self->priv->changes,
            self->priv->next_redo));
    // update undo undo/redo pointers
    self->priv->next_undo = self->priv->next_redo;
    self->priv->next_redo++;
    GST_INFO ("after redo %d, %d", self->priv->next_undo,
        self->priv->next_redo);
    if (self->priv->next_redo == self->priv->item_ct) {
      g_object_notify ((GObject *) self, "can-redo");
    }
    if (self->priv->next_undo == 0) {
      g_object_notify ((GObject *) self, "can-undo");
    }

    self->priv->is_active = is_active;
  } else {
    GST_INFO ("no redo at: %d, %d", self->priv->next_undo,
        self->priv->next_redo);
  }
}

//-- class internals

static void
bt_change_log_dispose (GObject * object)
{
  BtChangeLog *self = BT_CHANGE_LOG (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  close_and_free_log (self);
  g_object_try_weak_unref (self->priv->app);

  G_OBJECT_CLASS (bt_change_log_parent_class)->dispose (object);
}

static void
bt_change_log_finalize (GObject * object)
{
  BtChangeLog *self = BT_CHANGE_LOG (object);
  GList *node;

  GST_DEBUG ("!!!! self=%p", self);
  g_free (self->priv->cache_dir);

  g_hash_table_destroy (self->priv->loggers);

  // free cgrash-logs list and entries
  for (node = self->priv->crash_logs; node; node = g_list_next (node)) {
    free_crash_log_file ((BtChangeLogFile *) node->data);
  }
  g_list_free (self->priv->crash_logs);

  G_OBJECT_CLASS (bt_change_log_parent_class)->finalize (object);
}

static GObject *
bt_change_log_constructor (GType type, guint n_construct_params,
    GObjectConstructParam * construct_params)
{
  GObject *object;

  if (G_UNLIKELY (!singleton)) {
    object =
        G_OBJECT_CLASS (bt_change_log_parent_class)->constructor (type,
        n_construct_params, construct_params);
    singleton = BT_CHANGE_LOG (object);
    g_object_add_weak_pointer (object, (gpointer *) (gpointer) & singleton);
  } else {
    object = g_object_ref ((gpointer) singleton);
  }
  return object;
}

static void
bt_change_log_get_property (GObject * object, guint property_id, GValue * value,
    GParamSpec * pspec)
{
  BtChangeLog *self = BT_CHANGE_LOG (object);
  return_if_disposed ();
  switch (property_id) {
    case CHANGE_LOG_CAN_UNDO:
      g_value_set_boolean (value, (self->priv->next_undo != -1));
      break;
    case CHANGE_LOG_CAN_REDO:
      g_value_set_boolean (value,
          (self->priv->next_redo != self->priv->item_ct));
      break;
    case CHANGE_LOG_CRASH_LOGS:
      g_value_set_pointer (value, self->priv->crash_logs);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_change_log_init (BtChangeLog * self)
{
  self->priv =
      G_TYPE_INSTANCE_GET_PRIVATE (self, BT_TYPE_CHANGE_LOG,
      BtChangeLogPrivate);
  /* this is created from the app, we need to avoid a ref-cycle */
  self->priv->app = bt_edit_application_new ();
  g_object_try_weak_ref (self->priv->app);
  g_object_unref (self->priv->app);

#ifdef USE_DEBUG
  if (g_getenv ("BT_DEBUG_CHANGE_LOG")) {
    self->priv->debug_mode = TRUE;
  }
#endif

  self->priv->cache_dir =
      g_build_filename (g_get_user_cache_dir (), PACKAGE, NULL);
  /* ensure cachedir, set to NULL if we can't make it to deactivate the change logger */
  if (g_mkdir_with_parents (self->priv->cache_dir,
          S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
      == -1) {
    GST_WARNING ("Can't create change-log dir: '%s': %s", self->priv->cache_dir,
        g_strerror (errno));
    g_free (self->priv->cache_dir);
    self->priv->cache_dir = NULL;
  } else {
    bt_change_log_crash_check (self);
  }

  self->priv->loggers = g_hash_table_new (g_str_hash, g_str_equal);
  on_song_changed (self->priv->app, NULL, self);

  g_signal_connect_object (self->priv->app, "notify::song",
      G_CALLBACK (on_song_changed), (gpointer) self, 0);
}

static void
bt_change_log_class_init (BtChangeLogClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BtChangeLogPrivate));

  gobject_class->get_property = bt_change_log_get_property;
  gobject_class->constructor = bt_change_log_constructor;
  gobject_class->dispose = bt_change_log_dispose;
  gobject_class->finalize = bt_change_log_finalize;

  g_object_class_install_property (gobject_class, CHANGE_LOG_CAN_UNDO,
      g_param_spec_boolean ("can-undo",
          "can-undo prop",
          "Where there are action to undo",
          FALSE, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, CHANGE_LOG_CAN_REDO,
      g_param_spec_boolean ("can-redo",
          "can-redo prop",
          "Where there are action to redo",
          FALSE, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, CHANGE_LOG_CRASH_LOGS,
      g_param_spec_pointer ("crash-logs",
          "crash logs prop",
          "A list of found crash logs",
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}
