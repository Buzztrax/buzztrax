/* $Id: song-info.c,v 1.64 2007-02-12 21:47:12 ensonic Exp $
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
 
#define BT_CORE
#define BT_SONG_INFO_C

#include <libbtcore/core.h>

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
  union {
    BtSong *song;
    gconstpointer song_ptr;
  };
  
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

static GObjectClass *parent_class=NULL;

// date time stamp format YYYY-MM-DDThh:mm:ssZ
#define DTS_LEN 20

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
  g_return_val_if_fail(BT_IS_SONG(song),NULL);
  
  BtSongInfo * const self=BT_SONG_INFO(g_object_new(BT_TYPE_SONG_INFO,"song",song,NULL));

  return(self);
}

//-- methods


//-- io interface

static xmlNodePtr bt_song_info_persistence_save(const BtPersistence * const persistence, xmlNodePtr const parent_node, const BtPersistenceSelection * const selection) {
  const BtSongInfo * const self = BT_SONG_INFO(persistence);
  xmlNodePtr node=NULL;

  GST_DEBUG("PERSISTENCE::song-info");
  
  if((node=xmlNewChild(parent_node,NULL,XML_CHAR_PTR("meta"),NULL))) {
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

static gboolean bt_song_info_persistence_load(const BtPersistence * const persistence, xmlNodePtr node, const BtPersistenceLocation * const location) {
  const BtSongInfo * const self = BT_SONG_INFO(persistence);
  gboolean res=FALSE;
  
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
            g_object_set(G_OBJECT(self),property_name,elem,NULL);
          }
          else if(!strncmp(property_name,"bpm",3) ||
            !strncmp(property_name,"tpb",3) ||
            !strncmp(property_name,"bars",4)) {
            g_object_set(G_OBJECT(self),property_name,atol((char *)elem),NULL);
          }
          xmlFree(elem);
        }
      }
    }
  }
  res=TRUE;
  return(res);
}

static void bt_song_info_persistence_interface_init(gpointer const g_iface, gpointer const iface_data) {
  BtPersistenceInterface * const iface = g_iface;
  
  iface->load = bt_song_info_persistence_load;
  iface->save = bt_song_info_persistence_save;
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_song_info_get_property(GObject      * const object,
                               const guint         property_id,
                               GValue       * const value,
                               GParamSpec   * const pspec)
{
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
static void bt_song_info_set_property(GObject      * const object,
                              const guint         property_id,
                              const GValue * const value,
                              GParamSpec   * const pspec)
{
  const BtSongInfo * const self = BT_SONG_INFO(object);
  return_if_disposed();
  switch (property_id) {
    case SONG_INFO_SONG: {  
      g_object_try_weak_unref(self->priv->song);
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
      if(!self->priv->info || strcmp(self->priv->info,g_value_get_string(value))) {
        g_free(self->priv->info);
        self->priv->info = g_value_dup_string(value);
        gst_tag_list_add(self->priv->taglist, GST_TAG_MERGE_REPLACE,GST_TAG_DESCRIPTION, self->priv->info,NULL);
        bt_song_set_unsaved(self->priv->song,TRUE);
        GST_DEBUG("set the info for song_info: %s",self->priv->info);
      }
    } break;
    case SONG_INFO_NAME: {
      if(!self->priv->name || strcmp(self->priv->name,g_value_get_string(value))) {
        g_free(self->priv->name);
        self->priv->name = g_value_dup_string(value);
        gst_tag_list_add(self->priv->taglist, GST_TAG_MERGE_REPLACE,GST_TAG_TITLE, self->priv->name,NULL);
        bt_song_set_unsaved(self->priv->song,TRUE);
        GST_DEBUG("set the name for song_info: %s",self->priv->name);
      }
    } break;
    case SONG_INFO_GENRE: {
      if(!self->priv->genre || strcmp(self->priv->genre,g_value_get_string(value))) {
        g_free(self->priv->genre);
        self->priv->genre = g_value_dup_string(value);
        gst_tag_list_add(self->priv->taglist, GST_TAG_MERGE_REPLACE,GST_TAG_GENRE, self->priv->genre,NULL);
        bt_song_set_unsaved(self->priv->song,TRUE);
        GST_DEBUG("set the genre for song_info: %s",self->priv->genre);
      }
    } break;
    case SONG_INFO_AUTHOR: {
      if(!self->priv->author || strcmp(self->priv->author,g_value_get_string(value))) {
        g_free(self->priv->author);
        self->priv->author = g_value_dup_string(value);
        gst_tag_list_add(self->priv->taglist, GST_TAG_MERGE_REPLACE,GST_TAG_ARTIST, self->priv->author,NULL);
        bt_song_set_unsaved(self->priv->song,TRUE);
        GST_DEBUG("set the author for song_info: %s",self->priv->author);
      }
    } break;
    case SONG_INFO_BPM: {
      gulong val=g_value_get_ulong(value);
      if(self->priv->beats_per_minute!=val) {
        self->priv->beats_per_minute = g_value_get_ulong(value);
#if (GST_VERSION_MAJOR>=0 && GST_VERSION_MINOR>=10 && GST_VERSION_MICRO>=12) || (GST_VERSION_MAJOR>=0 && GST_VERSION_MINOR>=10 && GST_VERSION_MICRO>=11 && GST_VERSION_NANO>0 )
        gst_tag_list_add(self->priv->taglist, GST_TAG_MERGE_REPLACE,GST_TAG_BEATS_PER_MINUTE, (gdouble)self->priv->beats_per_minute,NULL);
#endif
        bt_song_set_unsaved(self->priv->song,TRUE);
        GST_DEBUG("set the bpm for song_info: %d",self->priv->beats_per_minute);
      }
    } break;
    case SONG_INFO_TPB: {
      gulong val=g_value_get_ulong(value);
      if(self->priv->ticks_per_beat!=val) {
        self->priv->ticks_per_beat = g_value_get_ulong(value);
        bt_song_set_unsaved(self->priv->song,TRUE);
        GST_DEBUG("set the tpb for song_info: %d",self->priv->ticks_per_beat);
      }
    } break;
    case SONG_INFO_BARS: {
      gulong val=g_value_get_ulong(value);
      if(self->priv->bars!=val) {
        self->priv->bars = g_value_get_ulong(value);
        bt_song_set_unsaved(self->priv->song,TRUE);
        GST_DEBUG("set the bars for song_info: %d",self->priv->bars);
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
        strftime(self->priv->create_dts,DTS_LEN+1,"%FT%TZ",gmtime(&now));
      }
    } break;
    case SONG_INFO_CHANGE_DTS: {
      const gchar * const dts = g_value_get_string(value);

      if(dts) {
        if(strlen(dts)==DTS_LEN) {
					struct tm tm;
          strcpy(self->priv->change_dts,dts);
					// parse date and update tag
					strptime(dts, "%FT%TZ", &tm);
#ifdef HAVE_GLIB_2_10
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
#ifdef HAVE_GLIB_2_10
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

  G_OBJECT_CLASS(parent_class)->dispose(object);
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

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_song_info_init(GTypeInstance * const instance, gconstpointer g_class) {
  BtSongInfo * const self = BT_SONG_INFO(instance);
  time_t now=time(NULL);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_SONG_INFO, BtSongInfoPrivate);
  self->priv->taglist=gst_tag_list_new();

  self->priv->name=g_strdup(_("untitled song"));
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
#ifdef HAVE_GLIB_2_10
  g_date_set_time_t(self->priv->tag_date,now);
#else
  g_date_set_time(self->priv->tag_date,now);
#endif
  gst_tag_list_add(self->priv->taglist, GST_TAG_MERGE_REPLACE,
    GST_TAG_TITLE, self->priv->name,
    GST_TAG_DATE, self->priv->tag_date,
    NULL);
}

static void bt_song_info_class_init(BtSongInfoClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
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
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,SONG_INFO_TAGLIST,
                                  g_param_spec_pointer("taglist",
                                     "songs taglist",
                                     "songs meta data as a taglist",
                                     G_PARAM_READABLE));

  g_object_class_install_property(gobject_class,SONG_INFO_FILE_NAME,
                                  g_param_spec_string("file-name",
                                     "file name prop",
                                     "songs file name",
                                     NULL, /* default value */
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,SONG_INFO_INFO,
                                  g_param_spec_string("info",
                                     "info prop",
                                     "songs freeform info",
                                     "comment me!", /* default value */
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,SONG_INFO_NAME,
                                  g_param_spec_string("name",
                                     "name prop",
                                     "songs name",
                                     "unnamed", /* default value */
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,SONG_INFO_GENRE,
                                  g_param_spec_string("genre",
                                     "genre prop",
                                     "songs genre",
                                     NULL, /* default value */
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,SONG_INFO_AUTHOR,
                                  g_param_spec_string("author",
                                     "author prop",
                                     "songs author",
                                     NULL, /* default value */
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,SONG_INFO_BPM,
                                  g_param_spec_ulong("bpm",
                                     "bpm prop",
                                     "how many beats should be played in a minute",
                                     1,
                                     1000,
                                     125,
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,SONG_INFO_TPB,
                                  g_param_spec_ulong("tpb",
                                     "tpb prop",
                                     "how many event fire in one fraction of a beat",
                                     1,
                                     128,
                                     4,
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,SONG_INFO_BARS,
                                  g_param_spec_ulong("bars",
                                     "bars prop",
                                     "how many bars per meassure",
                                     1,
                                     64,
                                     16,
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,SONG_INFO_CREATE_DTS,
                                  g_param_spec_string("create-dts",
                                     "creation dts prop",
                                     "song creation date time stamp",
                                     NULL, /* default value */
                                     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,SONG_INFO_CHANGE_DTS,
                                  g_param_spec_string("change-dts",
                                     "changed dts prop",
                                     "song changed date time stamp",
                                     NULL, /* default value */
                                     G_PARAM_READWRITE));
}

GType bt_song_info_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      G_STRUCT_SIZE(BtSongInfoClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_song_info_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtSongInfo),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_song_info_init, // instance_init
      NULL // value_table
    };
    const GInterfaceInfo persistence_interface_info = {
      (GInterfaceInitFunc) bt_song_info_persistence_interface_init,  // interface_init
      NULL, // interface_finalize
      NULL  // interface_data
    };
    type = g_type_register_static(G_TYPE_OBJECT,"BtSongInfo",&info,0);
    g_type_add_interface_static(type, BT_TYPE_PERSISTENCE, &persistence_interface_info);
  }
  return type;
}
