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
 * SECTION:btsonginfo
 * @short_description: class that keeps the meta-data for a #BtSong instance
 *
 * Exposes the meta-data of a song as #GObject properties. These are for one
 * pure data fields such as author and song name. These fields get used when
 * recording a song to a file (rendering) in the form of meta-tags.
 *
 * Further there are fields that determine rythm and song-speed. The speed is
 * determined by BtSongInfo::bpm. The rythm is determined by BtSongInfo::bars
 * and BtSongInfo::tpb. If 'bars' is 16, than on can have 1/16 notes.
 * And if 'ticks per beat' is 4 one will have 4 beats - a classic 4/4 meassure.
 * For a 3/4 meassure, 'bars' would be 12. Thus bars = beats * tpb.
 */
/* @todo: add more metadata
 *  - copyright: GST_TAG_COPYRIGHT
 *    string: "(C) YYYY <SONG_INFO_AUTHOR>"
 *  - license: GST_TAG_LICENSE (http://creativecommons.org/licenses/)
 *    - would be nice to use liblicense (not widely packaged :/)
 *
 * @todo: add sample-rate and channels properties
 *  - they will be set on sink-bin
 */

#define BT_CORE
#define BT_SONG_INFO_C

#include "core_private.h"

enum {
  SONG_INFO_SONG=1,
  SONG_INFO_TAGLIST,
  SONG_INFO_FILE_NAME,
  SONG_INFO_INFO,
  SONG_INFO_NAME,
  SONG_INFO_GENRE,
  SONG_INFO_AUTHOR,
  SONG_INFO_BPM,
  SONG_INFO_TPB,
  SONG_INFO_BARS,
  SONG_INFO_CREATE_DTS,
  SONG_INFO_CHANGE_DTS
};

struct _BtSongInfoPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the song the song-info belongs to */
  G_POINTER_ALIAS(BtSong *,song);

  /* the song-info as tag-data */
  GstTagList *taglist;
  GDate *tag_date;

  /* the file name of the song */
  gchar *file_name;

  /* freeform info about the song */
  gchar *info;
  /* the name of the tune */
  gchar *name;
  /* the genre of the tune */
  gchar *genre;
  /* the author of the tune */
  gchar *author;
  /* how many beats should be played in a minute */
  gulong beats_per_minute;
  /* how many event fire in one fraction of a beat */
  gulong ticks_per_beat;
  /* how many bars per beat */
  gulong bars;
  /* date stamps */
  gchar *create_dts,*change_dts;
};

// date time stamp format YYYY-MM-DDThh:mm:ssZ
#define DTS_LEN 20

/* default name for new songs */
#define DEFAULT_SONG_NAME _("untitled song")

//-- the class

static void bt_song_info_persistence_interface_init(gpointer const g_iface, gpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtSongInfo, bt_song_info, G_TYPE_OBJECT,
  G_IMPLEMENT_INTERFACE (BT_TYPE_PERSISTENCE,
    bt_song_info_persistence_interface_init));

//-- helper methods

/*
static gint safe_strcmp(const gchar *s1, const gchar *s2) {
  if(!s1 && !s2) return 0;
  else if(!s1) return -1;
  else if(!s2) return 1;
  else return strcmp(s1,s2);
}
*/

static void bt_song_info_tempo_changed(const BtSongInfo * const self) {
  BtSequence *sequence;
  
  g_object_get(self->priv->song,"sequence",&sequence,NULL);
  bt_sequence_update_tempo(sequence);
  g_object_unref(sequence);
}

//-- constructor methods

/**
 * bt_song_info_new:
 * @song: the song the new instance belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtSongInfo *bt_song_info_new(const BtSong * const song) {
  return(BT_SONG_INFO(g_object_new(BT_TYPE_SONG_INFO,"song",song,NULL)));
}

//-- methods


//-- io interface

static xmlNodePtr bt_song_info_persistence_save(const BtPersistence * const persistence, xmlNodePtr const parent_node) {
  const BtSongInfo * const self = BT_SONG_INFO(persistence);
  xmlNodePtr node=NULL;

  GST_DEBUG("PERSISTENCE::song-info");

  if((node=xmlNewChild(parent_node,NULL,XML_CHAR_PTR("meta"),NULL))) {
    if(!strcmp(self->priv->name,DEFAULT_SONG_NAME)) {
      BtSongIONative *song_io;
      gchar *file_path,*file_name,*ext;
      
      g_object_get(self->priv->song,"song-io",&song_io,NULL);
      g_object_get(song_io,"file-name",&file_path,NULL);
      file_name=g_path_get_basename(file_path);
      if((ext=strrchr(file_name,'.'))) {
        *ext='\0';
      }
      GST_INFO("using '%s' instead of default title",file_name);  
      g_object_set((gpointer)self,"name",file_name,NULL);
      g_free(file_name);
      g_free(file_path);
      g_object_unref(song_io);
    }
    
    if(self->priv->info) {
      xmlNewChild(node,NULL,XML_CHAR_PTR("info"),XML_CHAR_PTR(self->priv->info));
    }
    if(self->priv->name) {
      xmlNewChild(node,NULL,XML_CHAR_PTR("name"),XML_CHAR_PTR(self->priv->name));
    }
    if(self->priv->genre) {
      xmlNewChild(node,NULL,XML_CHAR_PTR("genre"),XML_CHAR_PTR(self->priv->genre));
    }
    if(self->priv->author) {
      xmlNewChild(node,NULL,XML_CHAR_PTR("author"),XML_CHAR_PTR(self->priv->author));
    }
    if(self->priv->create_dts) {
      xmlNewChild(node,NULL,XML_CHAR_PTR("create-dts"),XML_CHAR_PTR(self->priv->create_dts));
    }
    if(self->priv->change_dts) {
      xmlNewChild(node,NULL,XML_CHAR_PTR("change-dts"),XML_CHAR_PTR(self->priv->change_dts));
    }
    xmlNewChild(node,NULL,XML_CHAR_PTR("bpm"),XML_CHAR_PTR(bt_persistence_strfmt_ulong(self->priv->beats_per_minute)));
    xmlNewChild(node,NULL,XML_CHAR_PTR("tpb"),XML_CHAR_PTR(bt_persistence_strfmt_ulong(self->priv->ticks_per_beat)));
    xmlNewChild(node,NULL,XML_CHAR_PTR("bars"),XML_CHAR_PTR(bt_persistence_strfmt_ulong(self->priv->bars)));
  }
  return(node);
}

static BtPersistence *bt_song_info_persistence_load(const GType type, const BtPersistence * const persistence, xmlNodePtr node, GError **err, va_list var_args) {
  const BtSongInfo * const self = BT_SONG_INFO(persistence);

  GST_DEBUG("PERSISTENCE::song-info");
  g_assert(node);

  for(node=node->children;node;node=node->next) {
    if(!xmlNodeIsText(node)) {
      xmlNodePtr const child_node=node->children;
      if(child_node && xmlNodeIsText(child_node) && !xmlIsBlankNode(child_node)) {
	    xmlChar * const elem=xmlNodeGetContent(child_node);
        if(elem) {
          const gchar * const property_name=(gchar *)node->name;
          GST_DEBUG("  \"%s\"=\"%s\"",property_name,elem);
          // depending on the name of the property, treat it's type
          if(!strncmp(property_name,"info",4) ||
            !strncmp(property_name,"name",4) ||
            !strncmp(property_name,"genre",5) ||
            !strncmp(property_name,"author",6) ||
            !strncmp(property_name,"create-dts",10) ||
            !strncmp(property_name,"change-dts",10)
          ) {
            g_object_set((gpointer)self,property_name,elem,NULL);
          }
          else if(!strncmp(property_name,"bpm",3) ||
            !strncmp(property_name,"tpb",3) ||
            !strncmp(property_name,"bars",4)) {
            g_object_set((gpointer)self,property_name,atol((char *)elem),NULL);
          }
          xmlFree(elem);
        }
      }
    }
  }
  return(BT_PERSISTENCE(persistence));
}

static void bt_song_info_persistence_interface_init(gpointer const g_iface, gpointer const iface_data) {
  BtPersistenceInterface * const iface = g_iface;

  iface->load = bt_song_info_persistence_load;
  iface->save = bt_song_info_persistence_save;
}

//-- wrapper

//-- g_object overrides

/* returns a property for the given property_id for this object */
static void bt_song_info_get_property(GObject * const object, const guint property_id, GValue * const value, GParamSpec * const pspec) {
  const BtSongInfo * const self = BT_SONG_INFO(object);
  return_if_disposed();
  switch (property_id) {
    case SONG_INFO_SONG: {
      g_value_set_object(value, self->priv->song);
    } break;
    case SONG_INFO_TAGLIST: {
      g_value_set_pointer(value, gst_tag_list_copy(self->priv->taglist));
    } break;
    case SONG_INFO_FILE_NAME: {
      g_value_set_string(value, self->priv->file_name);
    } break;
    case SONG_INFO_INFO: {
      g_value_set_string(value, self->priv->info);
    } break;
    case SONG_INFO_NAME: {
      g_value_set_string(value, self->priv->name);
    } break;
    case SONG_INFO_GENRE: {
      g_value_set_string(value, self->priv->genre);
    } break;
    case SONG_INFO_AUTHOR: {
      g_value_set_string(value, self->priv->author);
    } break;
    case SONG_INFO_BPM: {
      g_value_set_ulong(value, self->priv->beats_per_minute);
    } break;
    case SONG_INFO_TPB: {
      g_value_set_ulong(value, self->priv->ticks_per_beat);
    } break;
    case SONG_INFO_BARS: {
      g_value_set_ulong(value, self->priv->bars);
    } break;
    case SONG_INFO_CREATE_DTS: {
      g_value_set_string(value, self->priv->create_dts);
    } break;
    case SONG_INFO_CHANGE_DTS: {
      g_value_set_string(value, self->priv->change_dts);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_song_info_set_property(GObject * const object, const guint property_id, const GValue * const value, GParamSpec * const pspec) {
  const BtSongInfo * const self = BT_SONG_INFO(object);
  return_if_disposed();
  switch (property_id) {
    case SONG_INFO_SONG: {
      self->priv->song = BT_SONG(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->song);
      //GST_DEBUG("set the song for song-info: %p",self->priv->song);
    } break;
    case SONG_INFO_FILE_NAME: {
      g_free(self->priv->file_name);
      self->priv->file_name = g_value_dup_string(value);
      GST_DEBUG("set the file-name for song_info: %s",self->priv->file_name);
    } break;
    case SONG_INFO_INFO: {
      const gchar *str=g_value_get_string(value);
      if((self->priv->info!=str) && (!self->priv->info || !str || strcmp(self->priv->info,str))) {
        g_free(self->priv->info);
        if(str) {
          self->priv->info = g_value_dup_string(value);
          gst_tag_list_add(self->priv->taglist,GST_TAG_MERGE_REPLACE,GST_TAG_DESCRIPTION,self->priv->info,NULL);
        }
        else {
          self->priv->info=NULL;
          gst_tag_list_remove_tag(self->priv->taglist,GST_TAG_DESCRIPTION);
        }
        bt_song_set_unsaved(self->priv->song,TRUE);
        GST_DEBUG("set the info for song_info: %s",self->priv->info);
      }
    } break;
    case SONG_INFO_NAME: {
      const gchar *str=g_value_get_string(value);
      if((self->priv->name!=str) && (!self->priv->name || !str || strcmp(self->priv->name,str))) {
        g_free(self->priv->name);
        if(str) {
          self->priv->name = g_value_dup_string(value);
          gst_tag_list_add(self->priv->taglist,GST_TAG_MERGE_REPLACE,GST_TAG_TITLE,self->priv->name,NULL);
        }
        else {
          self->priv->name=NULL;
          gst_tag_list_remove_tag(self->priv->taglist,GST_TAG_TITLE);
        }
        bt_song_set_unsaved(self->priv->song,TRUE);
        GST_DEBUG("set the name for song_info: %s",self->priv->name);
      }
    } break;
    case SONG_INFO_GENRE: {
      const gchar *str=g_value_get_string(value);
      if((self->priv->genre!=str) && (!self->priv->genre || !str || strcmp(self->priv->genre,str))) {
        g_free(self->priv->genre);
        if(str) {
          self->priv->genre = g_value_dup_string(value);
          gst_tag_list_add(self->priv->taglist,GST_TAG_MERGE_REPLACE,GST_TAG_GENRE,self->priv->genre,NULL);
        }
        else {
          self->priv->genre=NULL;
          gst_tag_list_remove_tag(self->priv->taglist,GST_TAG_GENRE);
        }
        bt_song_set_unsaved(self->priv->song,TRUE);
        GST_DEBUG("set the genre for song_info: %s",self->priv->genre);
      }
    } break;
    case SONG_INFO_AUTHOR: {
      const gchar *str=g_value_get_string(value);
      if((self->priv->author!=str) && (!self->priv->author || !str || strcmp(self->priv->author,str))) {
        g_free(self->priv->author);
        if(str) {
          self->priv->author=g_value_dup_string(value);
          gst_tag_list_add(self->priv->taglist,GST_TAG_MERGE_REPLACE,GST_TAG_ARTIST,self->priv->author,NULL);
        }
        else {
          self->priv->author=NULL;
          gst_tag_list_remove_tag(self->priv->taglist,GST_TAG_ARTIST);
        }
        bt_song_set_unsaved(self->priv->song,TRUE);
        GST_DEBUG("set the author for song_info: %s",self->priv->author);
      }
    } break;
    case SONG_INFO_BPM: {
      gulong val=g_value_get_ulong(value);
      if(self->priv->beats_per_minute!=val) {
        self->priv->beats_per_minute = g_value_get_ulong(value);
#if GST_CHECK_VERSION(0,10,12)
        gst_tag_list_add(self->priv->taglist, GST_TAG_MERGE_REPLACE,GST_TAG_BEATS_PER_MINUTE, (gdouble)self->priv->beats_per_minute,NULL);
#endif
        bt_song_info_tempo_changed(self);
        bt_song_set_unsaved(self->priv->song,TRUE);
        GST_DEBUG("set the bpm for song_info: %lu",self->priv->beats_per_minute);
      }
    } break;
    case SONG_INFO_TPB: {
      gulong val=g_value_get_ulong(value);
      if(self->priv->ticks_per_beat!=val) {
        self->priv->ticks_per_beat = g_value_get_ulong(value);
        bt_song_info_tempo_changed(self);
        bt_song_set_unsaved(self->priv->song,TRUE);
        GST_DEBUG("set the tpb for song_info: %lu",self->priv->ticks_per_beat);
      }
    } break;
    case SONG_INFO_BARS: {
      gulong val=g_value_get_ulong(value);
      if(self->priv->bars!=val) {
        self->priv->bars = g_value_get_ulong(value);
        bt_song_info_tempo_changed(self);
        bt_song_set_unsaved(self->priv->song,TRUE);
        GST_DEBUG("set the bars for song_info: %lu",self->priv->bars);
      }
    } break;
    case SONG_INFO_CREATE_DTS: {
      const gchar * const dts = g_value_get_string(value);

      if(dts) {
        if(strlen(dts)==DTS_LEN) {
          strcpy(self->priv->create_dts,dts);
        }
      }
      else {
        time_t now=time(NULL);
        /* this is ISO 8601 Date and Time Format
         * %F     Equivalent to %Y-%m-%d (the ISO 8601 date format). (C99)
         * %T     The time in 24-hour notation (%H:%M:%S). (SU)
         */
        strftime(self->priv->create_dts,DTS_LEN+1,"%FT%TZ",gmtime(&now));
      }
    } break;
    case SONG_INFO_CHANGE_DTS: {
      const gchar * const dts = g_value_get_string(value);

      if(dts) {
        if(strlen(dts)==DTS_LEN) {
          struct tm tm={0,};
          strcpy(self->priv->change_dts,dts);
          // parse date and update tag
          strptime(dts, "%FT%TZ", &tm);
#if GLIB_CHECK_VERSION(2,10,0)
          g_date_set_time_t(self->priv->tag_date,mktime(&tm));
#else
          g_date_set_time(self->priv->tag_date,mktime(&tm));
#endif
          gst_tag_list_add(self->priv->taglist, GST_TAG_MERGE_REPLACE,GST_TAG_DATE, self->priv->tag_date,NULL);
        }
      }
      else {
        time_t now=time(NULL);
        strftime(self->priv->change_dts,DTS_LEN+1,"%FT%TZ",gmtime(&now));
#if GLIB_CHECK_VERSION(2,10,0)
        g_date_set_time_t(self->priv->tag_date,now);
#else
        g_date_set_time(self->priv->tag_date,now);
#endif
        gst_tag_list_add(self->priv->taglist, GST_TAG_MERGE_REPLACE,GST_TAG_DATE, self->priv->tag_date,NULL);
      }
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_song_info_dispose(GObject * const object) {
  const BtSongInfo * const self = BT_SONG_INFO(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  g_object_try_weak_unref(self->priv->song);

  G_OBJECT_CLASS(bt_song_info_parent_class)->dispose(object);
}

static void bt_song_info_finalize(GObject * const object) {
  const BtSongInfo * const self = BT_SONG_INFO(object);

  GST_DEBUG("!!!! self=%p",self);

  g_date_free(self->priv->tag_date);
  gst_tag_list_free(self->priv->taglist);

  g_free(self->priv->file_name);
  g_free(self->priv->info);
  g_free(self->priv->name);
  g_free(self->priv->genre);
  g_free(self->priv->author);
  g_free(self->priv->create_dts);
  g_free(self->priv->change_dts);

  G_OBJECT_CLASS(bt_song_info_parent_class)->finalize(object);
}

//-- class internals

static void bt_song_info_init(BtSongInfo * self) {
  time_t now=time(NULL);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_SONG_INFO, BtSongInfoPrivate);
  self->priv->taglist=gst_tag_list_new();

  self->priv->name=g_strdup(DEFAULT_SONG_NAME);
  self->priv->author=g_strdup(g_get_real_name());
  // @idea alternate bpm's a little at new_song (user defined range?)
  self->priv->beats_per_minute=125;  // 1..1000
  self->priv->ticks_per_beat=4;      // 1..128
  self->priv->bars=16;               // 1..16
  // init dates 'YYYY-MM-DDThh:mm:ssZ'
  self->priv->create_dts=g_new(gchar,DTS_LEN+1);
  self->priv->change_dts=g_new(gchar,DTS_LEN+1);
  strftime(self->priv->create_dts,DTS_LEN+1,"%FT%TZ",gmtime(&now));
  strncpy(self->priv->change_dts,self->priv->create_dts,DTS_LEN+1);
  GST_DEBUG("date initialized as %s",self->priv->change_dts);

  // init taglist
  self->priv->tag_date=g_date_new();
#if GLIB_CHECK_VERSION(2,10,0)
  g_date_set_time_t(self->priv->tag_date,now);
#else
  g_date_set_time(self->priv->tag_date,now);
#endif
  gst_tag_list_add(self->priv->taglist, GST_TAG_MERGE_REPLACE,
    GST_TAG_TITLE, self->priv->name,
    GST_TAG_ARTIST, self->priv->author,
    GST_TAG_DATE, self->priv->tag_date,
#if GST_CHECK_VERSION(0,10,12)
    GST_TAG_BEATS_PER_MINUTE, (gdouble)self->priv->beats_per_minute,
#endif
    NULL);
}

static void bt_song_info_class_init(BtSongInfoClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  g_type_class_add_private(klass,sizeof(BtSongInfoPrivate));

  gobject_class->set_property = bt_song_info_set_property;
  gobject_class->get_property = bt_song_info_get_property;
  gobject_class->dispose      = bt_song_info_dispose;
  gobject_class->finalize     = bt_song_info_finalize;

  g_object_class_install_property(gobject_class,SONG_INFO_SONG,
                                  g_param_spec_object("song",
                                     "song contruct prop",
                                     "song object, the song-info belongs to",
                                     BT_TYPE_SONG, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SONG_INFO_TAGLIST,
                                  g_param_spec_pointer("taglist",
                                     "songs taglist",
                                     "songs meta data as a taglist",
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SONG_INFO_FILE_NAME,
                                  g_param_spec_string("file-name",
                                     "file name prop",
                                     "songs file name",
                                     NULL, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SONG_INFO_INFO,
                                  g_param_spec_string("info",
                                     "info prop",
                                     "songs freeform info",
                                     "comment me!", /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SONG_INFO_NAME,
                                  g_param_spec_string("name",
                                     "name prop",
                                     "songs name",
                                     DEFAULT_SONG_NAME, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SONG_INFO_GENRE,
                                  g_param_spec_string("genre",
                                     "genre prop",
                                     "songs genre",
                                     NULL, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SONG_INFO_AUTHOR,
                                  g_param_spec_string("author",
                                     "author prop",
                                     "songs author",
                                     NULL, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SONG_INFO_BPM,
                                  g_param_spec_ulong("bpm",
                                     "bpm prop",
                                     "how many beats should be played in a minute",
                                     1,
                                     1000,
                                     125,
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SONG_INFO_TPB,
                                  g_param_spec_ulong("tpb",
                                     "tpb prop",
                                     "how many event fire in one fraction of a beat",
                                     1,
                                     128,
                                     4,
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SONG_INFO_BARS,
                                  g_param_spec_ulong("bars",
                                     "bars prop",
                                     "how many bars per meassure",
                                     1,
                                     64,
                                     16,
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SONG_INFO_CREATE_DTS,
                                  g_param_spec_string("create-dts",
                                     "creation dts prop",
                                     "song creation date time stamp (iso 8601 format)",
                                     NULL, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,SONG_INFO_CHANGE_DTS,
                                  g_param_spec_string("change-dts",
                                     "changed dts prop",
                                     "song changed date time stamp (iso 8601 format)",
                                     NULL, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));
}

