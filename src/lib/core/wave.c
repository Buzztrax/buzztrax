/* $Id: wave.c,v 1.29 2007-07-19 13:23:06 ensonic Exp $
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
 * SECTION:btwave
 * @short_description: one #BtWavetable entry that keeps a list of #BtWavelevels
 *
 * Represents one instrument. Contains one or more #BtWavelevels.
 */
/* @todo: save sample file length and/or md5sum in file:
 * - if we miss files, we can do a file-system search and use the details to verify
 * - when loading, we might also use the details as a sanity check
 */
#define BT_CORE
#define BT_WAVE_C

#include <libbtcore/core.h>

//-- signal ids

enum {
  WAVELEVEL_ADDED_EVENT,
  LAST_SIGNAL
};

//-- property ids

enum {
  WAVE_SONG=1,
  WAVE_WAVELEVELS,
  WAVE_INDEX,
  WAVE_NAME,
  WAVE_URI
};

struct _BtWavePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the song the wave belongs to */
  G_POINTER_ALIAS(BtSong *,song);

  /* each wave has an index number, the list of waves can have empty slots */
  gulong index;
  /* the name of the wave and the the sample file */
  gchar *name;
  gchar *uri;

  GList *wavelevels;    // each entry points to a BtWavelevel
};

static GObjectClass *parent_class=NULL;

//static guint signals[LAST_SIGNAL]={0,};

//-- helper

//-- constructor methods

/**
 * bt_wave_new:
 * @song: the song the new instance belongs to
 * @name: the display name for the new wave
 * @uri: the location of the sample data
 * @index: the list slot for the new wave
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtWave *bt_wave_new(const BtSong * const song, const gchar * const name, const gchar * const uri, const gulong index) {
  BtWavetable *wavetable;

  g_return_val_if_fail(BT_IS_SONG(song),NULL);

  BtWave * const self=BT_WAVE(g_object_new(BT_TYPE_WAVE,"song",song,"name",name,"uri",uri,"index",index,NULL));
  if(!self) {
    goto Error;
  }
  // add the wave to the wavetable of the song
  g_object_get(G_OBJECT(song),"wavetable",&wavetable,NULL);
  g_assert(wavetable!=NULL);
  bt_wavetable_add_wave(wavetable,self);
  g_object_unref(wavetable);

  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- private methods

//-- public methods

/**
 * bt_wave_add_wavelevel:
 * @self: the wavetable to add the new wavelevel to
 * @wavelevel: the new wavelevel instance
 *
 * Add the supplied wavelevel to the wave. This is automatically done by
 * #bt_wavelevel_new().
 *
 * Returns: %TRUE for success, %FALSE otheriwse
 */
gboolean bt_wave_add_wavelevel(const BtWave * const self, const BtWavelevel * const wavelevel) {
  gboolean ret=FALSE;

  g_assert(BT_IS_WAVE(self));
  g_assert(BT_IS_WAVELEVEL(wavelevel));

  if(!g_list_find(self->priv->wavelevels,wavelevel)) {
    ret=TRUE;
    self->priv->wavelevels=g_list_append(self->priv->wavelevels,g_object_ref(G_OBJECT(wavelevel)));
    //g_signal_emit(G_OBJECT(self),signals[WAVELEVEL_ADDED_EVENT], 0, wavelevel);
    bt_song_set_unsaved(self->priv->song,TRUE);
  }
  else {
    GST_WARNING("trying to add wavelevel again");
  }
  return ret;
}

/**
 * bt_wave_load_from_uri:
 * @self: the wave to load
 *
 * Will check the URI and if valid load the wavedata.
 *
 * Returns: %TRUE if the wavedata could be loaded
 */
gboolean bt_wave_load_from_uri(const BtWave * const self) {
  gboolean res=TRUE;

  GnomeVFSURI * const uri=gnome_vfs_uri_new(self->priv->uri);
  // check if the url is valid
  if(!gnome_vfs_uri_exists(uri)) goto invalid_uri;

  // @todo: load wave-data (into wavelevels)

done:
  gnome_vfs_uri_unref(uri);
  return(res);

  /* Errors */
invalid_uri:
  res=FALSE;
  goto done;
}

//-- io interface

static xmlNodePtr bt_wave_persistence_save(const BtPersistence * const persistence, const xmlNodePtr const parent_node, const BtPersistenceSelection * const selection) {
  const BtWave * const self = BT_WAVE(persistence);
  xmlNodePtr node=NULL;
  xmlNodePtr child_node;

  GST_DEBUG("PERSISTENCE::wave");

  if((node=xmlNewChild(parent_node,NULL,XML_CHAR_PTR("wave"),NULL))) {
    xmlNewProp(node,XML_CHAR_PTR("index"),XML_CHAR_PTR(bt_persistence_strfmt_ulong(self->priv->index)));
    xmlNewProp(node,XML_CHAR_PTR("name"),XML_CHAR_PTR(self->priv->name));
    xmlNewProp(node,XML_CHAR_PTR("uri"),XML_CHAR_PTR(self->priv->uri));

    // save wavelevels
    if((child_node=xmlNewChild(node,NULL,XML_CHAR_PTR("wavelevels"),NULL))) {
      bt_persistence_save_list(self->priv->wavelevels,node);
    }
  }
  return(node);
}

static gboolean bt_wave_persistence_load(const BtPersistence * const persistence, xmlNodePtr node, const BtPersistenceLocation * const location) {
  const BtWave * const self = BT_WAVE(persistence);
  xmlNodePtr child_node;
  gboolean res=FALSE;

  GST_DEBUG("PERSISTENCE::wave");
  g_assert(node);

  xmlChar * const index_str=xmlGetProp(node,XML_CHAR_PTR("index"));
  const gulong index=index_str?atol((char *)index_str):0;
  xmlChar * const name=xmlGetProp(node,XML_CHAR_PTR("name"));
  xmlChar * const uri=xmlGetProp(node,XML_CHAR_PTR("uri"));
  g_object_set(G_OBJECT(self),"index",index,"name",name,"uri",uri,NULL);
  xmlFree(index_str);
  xmlFree(name);
  xmlFree(uri);

  for(child_node=node->children;child_node;child_node=child_node->next) {
    if((!xmlNodeIsText(child_node)) && (!strncmp((char *)child_node->name,"wavelevel\0",10))) {
      BtWavelevel * const wave_level=BT_WAVELEVEL(g_object_new(BT_TYPE_WAVELEVEL,NULL));
      if(bt_persistence_load(BT_PERSISTENCE(wave_level),child_node,NULL)) {
        bt_wave_add_wavelevel(self,wave_level);
      }
      g_object_unref(wave_level);
    }
  }
  // try to load wavedata
  res=bt_wave_load_from_uri(self);

  return(res);
}

static void bt_wave_persistence_interface_init(gpointer const g_iface, gpointer const iface_data) {
  BtPersistenceInterface * const iface = g_iface;

  iface->load = bt_wave_persistence_load;
  iface->save = bt_wave_persistence_save;
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_wave_get_property(GObject      * const object,
                               const guint         property_id,
                               GValue       * const value,
                               GParamSpec   * const pspec)
{
  const BtWave * const self = BT_WAVE(object);
  return_if_disposed();
  switch (property_id) {
    case WAVE_SONG: {
      g_value_set_object(value, self->priv->song);
    } break;
    case WAVE_WAVELEVELS: {
      g_value_set_pointer(value,g_list_copy(self->priv->wavelevels));
    } break;
    case WAVE_INDEX: {
      g_value_set_ulong(value, self->priv->index);
    } break;
    case WAVE_NAME: {
      g_value_set_string(value, self->priv->name);
    } break;
    case WAVE_URI: {
      g_value_set_string(value, self->priv->uri);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_wave_set_property(GObject      * const object,
                              const guint         property_id,
                              const GValue * const value,
                              GParamSpec   * const pspec)
{
  const BtWave * const self = BT_WAVE(object);
  return_if_disposed();
  switch (property_id) {
    case WAVE_SONG: {
      g_object_try_weak_unref(self->priv->song);
      self->priv->song = BT_SONG(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->song);
      //GST_DEBUG("set the song for wave: %p",self->priv->song);
    } break;
    case WAVE_INDEX: {
      self->priv->index = g_value_get_ulong(value);
      GST_DEBUG("set the index for wave: %d",self->priv->index);
    } break;
    case WAVE_NAME: {
      g_free(self->priv->name);
      self->priv->name = g_value_dup_string(value);
      GST_DEBUG("set the name for wave: %s",self->priv->name);
    } break;
    case WAVE_URI: {
      g_free(self->priv->uri);
      self->priv->uri = g_value_dup_string(value);
      GST_DEBUG("set the uri for wave: %s",self->priv->uri);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_wave_dispose(GObject * const object) {
  const BtWave * const self = BT_WAVE(object);
  GList* node;

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);

  g_object_try_weak_unref(self->priv->song);
  // unref list of wavelevels
  if(self->priv->wavelevels) {
    node=self->priv->wavelevels;
    while(node) {
      {
        const GObject * const obj=node->data;
        GST_DEBUG("  free wavelevels : %p (%d)",obj,obj->ref_count);
      }
      g_object_try_unref(node->data);
      node->data=NULL;
      node=g_list_next(node);
    }
  }

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_wave_finalize(GObject * const object) {
  const BtWave * const self = BT_WAVE(object);

  GST_DEBUG("!!!! self=%p",self);

  // free list of wavelevels
  if(self->priv->wavelevels) {
    g_list_free(self->priv->wavelevels);
    self->priv->wavelevels=NULL;
  }
  g_free(self->priv->name);
  g_free(self->priv->uri);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_wave_init(GTypeInstance * const instance, gconstpointer const g_class) {
  BtWave * const self = BT_WAVE(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_WAVE, BtWavePrivate);
}

static void bt_wave_class_init(BtWaveClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtWavePrivate));

  gobject_class->set_property = bt_wave_set_property;
  gobject_class->get_property = bt_wave_get_property;
  gobject_class->dispose      = bt_wave_dispose;
  gobject_class->finalize     = bt_wave_finalize;

  g_object_class_install_property(gobject_class,WAVE_SONG,
                                  g_param_spec_object("song",
                                     "song contruct prop",
                                     "Set song object, the wave belongs to",
                                     BT_TYPE_SONG, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WAVE_WAVELEVELS,
                                  g_param_spec_pointer("wavelevels",
                                     "wavelevels list prop",
                                     "A copy of the list of wavelevels",
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WAVE_INDEX,
                                  g_param_spec_ulong("index",
                                     "index prop",
                                     "The index of the wave in the wavtable",
                                     0,
                                     G_MAXULONG,
                                     0, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WAVE_NAME,
                                  g_param_spec_string("name",
                                     "name prop",
                                     "The name of the wave",
                                     "unamed wave", /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WAVE_URI,
                                  g_param_spec_string("uri",
                                     "uri prop",
                                     "The uri of the wave",
                                     NULL, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));
}

GType bt_wave_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    const GTypeInfo info = {
      sizeof(BtWaveClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_wave_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtWave),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_wave_init, // instance_init
      NULL // value_table
    };
    const GInterfaceInfo persistence_interface_info = {
      (GInterfaceInitFunc) bt_wave_persistence_interface_init,  // interface_init
      NULL, // interface_finalize
      NULL  // interface_data
    };
    type = g_type_register_static(G_TYPE_OBJECT,"BtWave",&info,0);
    g_type_add_interface_static(type, BT_TYPE_PERSISTENCE, &persistence_interface_info);
  }
  return type;
}
