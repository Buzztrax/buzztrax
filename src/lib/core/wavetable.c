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
 * SECTION:btwavetable
 * @short_description: the list of #BtWave items in a #BtSong
 *
 * Each wave table entry can constist of multiple #BtWaves, were each of the
 * waves has a #BtWavelevel with the data for a note range.
 */
/* @todo: defer freeing waves if playing
 * - buzzmachines don't ref waves
 * - we can replace waves in the wavetable, but we should put the old ones in a to-be-freed list
 * - on song::stop we clear them
 */

#define BT_CORE
#define BT_WAVETABLE_C

#include "core_private.h"

//-- signal ids

enum {
  WAVE_ADDED_EVENT,
  WAVE_REMOVED_EVENT,
  LAST_SIGNAL
};

//-- property ids

enum {
  WAVETABLE_SONG=1,
  WAVETABLE_WAVES,
  WAVETABLE_MISSING_WAVES
};

struct _BtWavetablePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the song the wavetable belongs to */
  G_POINTER_ALIAS(BtSong *,song);

  /* @idea: change to GPtrArray, the baseptr does not change when modifying and
   * we can save the 'index' field in BtWave
   */
  GList *waves;         // each entry points to a BtWave
  GList *missing_waves; // each entry points to a gchar*
};

static GObjectClass *parent_class=NULL;

static guint signals[LAST_SIGNAL]={0,};

//-- constructor methods

/**
 * bt_wavetable_new:
 * @song: the song the new instance belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtWavetable *bt_wavetable_new(const BtSong * const song) {
  return(BT_WAVETABLE(g_object_new(BT_TYPE_WAVETABLE,"song",song,NULL)));
}

//-- private methods

//-- public methods

/**
 * bt_wavetable_add_wave:
 * @self: the wavetable to add the wave to
 * @wave: the new wave instance
 *
 * Add the supplied wave to the wavetable. This is automatically done by
 * #bt_wave_new().
 *
 * Returns: %TRUE for success, %FALSE otheriwse
 */
gboolean bt_wavetable_add_wave(const BtWavetable * const self, const BtWave * const wave) {
  gboolean ret=FALSE;

  g_assert(BT_IS_WAVETABLE(self));
  g_assert(BT_IS_WAVE(wave));

  if(!g_list_find(self->priv->waves,wave)) {
    gulong index;
    BtWave *other_wave;
    
    // check if there is already a wave with this id ad remove if so
    g_object_get((gpointer)wave,"index",&index,NULL);
    if((other_wave=bt_wavetable_get_wave_by_index(self,index))) {
      GST_DEBUG("replacing old wave with same id");
      self->priv->waves=g_list_remove(self->priv->waves,other_wave);
      //g_signal_emit((gpointer)self,signals[WAVE_REMOVED_EVENT], 0, wave);
      /* @todo: if song::is-playing==TRUE add this to a  self->priv->old_waves
       * and wait for song::is-playing==FALSE and free then */
      g_object_unref(other_wave);
    }
    
    self->priv->waves=g_list_append(self->priv->waves,g_object_ref((gpointer)wave));
    g_signal_emit((gpointer)self,signals[WAVE_ADDED_EVENT], 0, wave);
    bt_song_set_unsaved(self->priv->song,TRUE);
    ret=TRUE;
  }
  else {
    GST_WARNING("trying to add wave again");
  }
  return ret;
}

/**
 * bt_wavetable_remove_wave:
 * @self: the wavetable to remove the wave from
 * @wave: the wave instance
 *
 * Remove the supplied wave from the wavetable.
 *
 * Returns: %TRUE for success, %FALSE otheriwse
 */
gboolean bt_wavetable_remove_wave(const BtWavetable * const self, const BtWave * const wave) {
  gboolean ret=FALSE;

  g_assert(BT_IS_WAVETABLE(self));
  g_assert(BT_IS_WAVE(wave));

  if(g_list_find(self->priv->waves,wave)) {
    self->priv->waves=g_list_remove(self->priv->waves,wave);
    g_signal_emit((gpointer)self,signals[WAVE_REMOVED_EVENT], 0, wave);
    g_object_unref((gpointer)wave);
    bt_song_set_unsaved(self->priv->song,TRUE);
    ret=TRUE;
  }
  else {
    GST_WARNING("trying to remove wave that is not in the list");
  }
  return ret;
}

/**
 * bt_wavetable_get_wave_by_index:
 * @self: the wavetable to search for the wave
 * @index: the index of the wave
 *
 * Search the wavetable for a wave by the supplied index.
 * The wave must have been added previously to this wavetable with bt_wavetable_add_wave().
 * Unref the wave, when done with it.
 *
 * Returns: #BtWave instance or %NULL if not found
 */
BtWave *bt_wavetable_get_wave_by_index(const BtWavetable * const self, const gulong index) {
  const GList *node;
  gulong wave_index;

  g_assert(BT_IS_WAVETABLE(self));

  for(node=self->priv->waves;node;node=g_list_next(node)) {
    BtWave * const wave=BT_WAVE(node->data);
    g_object_get(wave,"index",&wave_index,NULL);
    if(index==wave_index) return(g_object_ref(wave));
  }
  return(NULL);
}

/**
 * bt_wavetable_remember_missing_wave:
 * @self: the wavetable
 * @str: human readable description of the missing wave
 *
 * Loaders can use this function to collect information about wavetable entries
 * that failed to load.
 * The front-end can access this later by reading BtWavetable::missing-waves
 * property.
 */
void bt_wavetable_remember_missing_wave(const BtWavetable * const self, const gchar * const str) {
  GST_INFO("missing wave %s",str);
  self->priv->missing_waves=g_list_prepend(self->priv->missing_waves,(gpointer)str);
}

//-- io interface

static xmlNodePtr bt_wavetable_persistence_save(const BtPersistence * const persistence, xmlNodePtr const parent_node) {
  const BtWavetable * const self = BT_WAVETABLE(persistence);
  xmlNodePtr node=NULL;

  GST_DEBUG("PERSISTENCE::wavetable");

  if((node=xmlNewChild(parent_node,NULL,XML_CHAR_PTR("wavetable"),NULL))) {
    bt_persistence_save_list(self->priv->waves,node);
  }
  return(node);
}

static BtPersistence *bt_wavetable_persistence_load(const GType type, const BtPersistence * const persistence, xmlNodePtr node, GError **err, va_list var_args) {
  const BtWavetable * const self = BT_WAVETABLE(persistence);
  xmlNodePtr child_node;

  GST_DEBUG("PERSISTENCE::wavetable");
  g_assert(node);

  for(child_node=node->children;child_node;child_node=child_node->next) {
    if((!xmlNodeIsText(child_node)) && (!strncmp((char *)child_node->name,"wave\0",5))) {
      GError *err=NULL;
      
      BtWave * const wave=BT_WAVE(bt_persistence_load(BT_TYPE_WAVE,NULL,child_node,&err,"song",self->priv->song,NULL));
      if(err!=NULL) {
        // collect failed waves
        gchar * const name, * const uri;

        g_object_get(wave,"name",&name,"uri",&uri,NULL);
        gchar * const str=g_strdup_printf("%s: %s",name, uri);
        bt_wavetable_remember_missing_wave(self,str);
        g_free(name);
	    g_free(uri);
        GST_WARNING("Can't create wavetable: %s",err->message);
        g_error_free(err);
      }
      g_object_unref(wave);
    }
  }
  return(BT_PERSISTENCE(persistence));
}

static void bt_wavetable_persistence_interface_init(gpointer const g_iface, gpointer const iface_data) {
  BtPersistenceInterface * const iface = g_iface;

  iface->load = bt_wavetable_persistence_load;
  iface->save = bt_wavetable_persistence_save;
}

//-- wrapper

//-- g_object overrides

#if 0
static void bt_setup_constructed(GObject *object) {
  BtSetup *self=BT_SETUP(object);
  
  if(G_OBJECT_CLASS(parent_class)->constructed)
    G_OBJECT_CLASS(parent_class)->constructed(object);

  g_return_val_if_fail(BT_IS_SONG(self->priv->song),NULL);
}
#endif

/* returns a property for the given property_id for this object */
static void bt_wavetable_get_property(GObject * const object, const guint property_id, GValue * const value, GParamSpec * const pspec) {
  const BtWavetable * const self = BT_WAVETABLE(object);
  return_if_disposed();
  switch (property_id) {
    case WAVETABLE_SONG: {
      g_value_set_object(value, self->priv->song);
    } break;
    case WAVETABLE_WAVES: {
      g_value_set_pointer(value,g_list_copy(self->priv->waves));
    } break;
    case WAVETABLE_MISSING_WAVES: {
      g_value_set_pointer(value,self->priv->missing_waves);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_wavetable_set_property(GObject * const object, const guint property_id, const GValue * const value, GParamSpec * const pspec) {
  const BtWavetable * const self = BT_WAVETABLE(object);
  return_if_disposed();
  switch (property_id) {
    case WAVETABLE_SONG: {
      self->priv->song = BT_SONG(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->song);
      //GST_DEBUG("set the song for wavetable: %p",self->priv->song);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_wavetable_dispose(GObject * const object) {
  const BtWavetable * const self = BT_WAVETABLE(object);
  GList* node;

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);

  g_object_try_weak_unref(self->priv->song);
  // unref list of waves
  if(self->priv->waves) {
    node=self->priv->waves;
    while(node) {
      {
        GObject * const obj=node->data;
        GST_DEBUG("  free wave : %p (%d)",obj,obj->ref_count);
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

static void bt_wavetable_finalize(GObject * const object) {
  const BtWavetable * const self = BT_WAVETABLE(object);

  GST_DEBUG("!!!! self=%p",self);

  // free list of waves
  if(self->priv->waves) {
    g_list_free(self->priv->waves);
    self->priv->waves=NULL;
  }
  // free list of missing_waves
  if(self->priv->missing_waves) {
    const GList* node;
    for(node=self->priv->missing_waves;node;node=g_list_next(node)) {
      g_free(node->data);
    }
    g_list_free(self->priv->missing_waves);
    self->priv->missing_waves=NULL;
  }

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

//-- class internals

static void bt_wavetable_init(GTypeInstance * const instance, gconstpointer const g_class) {
  BtWavetable * const self = BT_WAVETABLE(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_WAVETABLE, BtWavetablePrivate);
}

static void bt_wavetable_class_init(BtWavetableClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtWavetablePrivate));

  gobject_class->set_property = bt_wavetable_set_property;
  gobject_class->get_property = bt_wavetable_get_property;
  gobject_class->dispose      = bt_wavetable_dispose;
  gobject_class->finalize     = bt_wavetable_finalize;

  /**
   * BtWavetable::wave-added:
   * @self: the wavetable object that emitted the signal
   * @wave: the new wave
   *
   * A new wave item has been added to the wavetable
   */
  signals[WAVE_ADDED_EVENT] = g_signal_new("wave-added",
                                        G_TYPE_FROM_CLASS(klass),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                        0,
                                        NULL, // accumulator
                                        NULL, // acc data
                                        g_cclosure_marshal_VOID__OBJECT,
                                        G_TYPE_NONE, // return type
                                        1, // n_params
                                        BT_TYPE_WAVE // param data
                                        );

  /**
   * BtWavetable::wave-removed:
   * @self: the setup object that emitted the signal
   * @wave: the old wave
   *
   * A wave item has been removed from the wavetable
   */
  signals[WAVE_REMOVED_EVENT] = g_signal_new("wave-removed",
                                        G_TYPE_FROM_CLASS(klass),
                                        G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                                        0,
                                        NULL, // accumulator
                                        NULL, // acc data
                                        g_cclosure_marshal_VOID__OBJECT,
                                        G_TYPE_NONE, // return type
                                        1, // n_params
                                        BT_TYPE_WAVE // param data
                                        );

  g_object_class_install_property(gobject_class,WAVETABLE_SONG,
                                  g_param_spec_object("song",
                                     "song contruct prop",
                                     "Set song object, the wavetable belongs to",
                                     BT_TYPE_SONG, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WAVETABLE_WAVES,
                                  g_param_spec_pointer("waves",
                                     "waves list prop",
                                     "A copy of the list of waves",
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class,WAVETABLE_MISSING_WAVES,
                                  g_param_spec_pointer("missing-waves",
                                     "missing-waves list prop",
                                     "The list of missing waves, don't change",
                                     G_PARAM_READABLE|G_PARAM_STATIC_STRINGS));
}

GType bt_wavetable_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof(BtWavetableClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_wavetable_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtWavetable),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_wavetable_init, // instance_init
      NULL // value_table
    };
    const GInterfaceInfo persistence_interface_info = {
      (GInterfaceInitFunc) bt_wavetable_persistence_interface_init,  // interface_init
      NULL, // interface_finalize
      NULL  // interface_data
    };
    type = g_type_register_static(G_TYPE_OBJECT,"BtWavetable",&info,0);
    g_type_add_interface_static(type, BT_TYPE_PERSISTENCE, &persistence_interface_info);
  }
  return type;
}
