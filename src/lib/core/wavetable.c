/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * SECTION:btwavetable
 * @short_description: the list of #BtWave items in a #BtSong
 *
 * Each wave table entry can consist of multiple #BtWaves, were each of the
 * waves has a #BtWavelevel with the data for a note range.
 *
 * The first entry starts at index pos 1. Index 0 is used in a #BtPattern to
 * indicate that no (new) wave is referenced.
 */
/* TODO(ensonic): defer freeing waves if playing
 * - buzzmachines don't ref waves, machines are supposed to check the wave ptr
 *   in work() each time
 * - we can replace waves in the wavetable, but we should put the old ones in a
 *   to-be-freed list
 * - on song::stop we clear them
 */

#define BT_CORE
#define BT_WAVETABLE_C

#include "core_private.h"
#include <gst/audio/audio.h>

//-- signal ids

enum
{
  WAVE_ADDED_EVENT,
  WAVE_REMOVED_EVENT,
  LAST_SIGNAL
};

//-- property ids

enum
{
  WAVETABLE_SONG = 1,
  WAVETABLE_WAVES,
  WAVETABLE_MISSING_WAVES
};

struct _BtWavetablePrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  /* the song the wavetable belongs to */
  BtSong *song;

  /* IDEA(ensonic): change to GPtrArray, the baseptr does not change when modifying and
   * we can remove the 'index' field in BtWave
   */
  GList *waves;                 // each entry points to a BtWave
  GList *missing_waves;         // each entry points to a gchar*
};

static guint signals[LAST_SIGNAL] = { 0, };

//-- the class

static void bt_wavetable_persistence_interface_init (gpointer const g_iface,
    gpointer const iface_data);

G_DEFINE_TYPE_WITH_CODE (BtWavetable, bt_wavetable, G_TYPE_OBJECT,
    G_ADD_PRIVATE(BtWavetable)
    G_IMPLEMENT_INTERFACE (BT_TYPE_PERSISTENCE,
        bt_wavetable_persistence_interface_init));


//-- constructor methods

/**
 * bt_wavetable_new:
 * @song: the song the new instance belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtWavetable *
bt_wavetable_new (const BtSong * const song)
{
  return BT_WAVETABLE (g_object_new (BT_TYPE_WAVETABLE, "song", song, NULL));
}

//-- private methods

static void
update_wave_index_enum (gulong index, gchar * new_name)
{
  gchar *enum_nick;
  GEnumClass *enum_class;
  GEnumValue *enum_value;

  if (!index) {
    // index 0 is no wave, don't update
    return;
  }

  enum_class = g_type_class_ref (GSTBT_TYPE_WAVE_INDEX);
  enum_value = g_enum_get_value (enum_class, index);
  enum_nick = (gchar *) enum_value->value_nick;
  enum_value->value_nick = new_name;
  g_free (enum_nick);
  g_type_class_unref (enum_class);
}

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
gboolean
bt_wavetable_add_wave (const BtWavetable * const self,
    const BtWave * const wave)
{
  gboolean ret = FALSE;

  g_assert (BT_IS_WAVETABLE (self));
  g_assert (BT_IS_WAVE (wave));

  if (!g_list_find (self->priv->waves, wave)) {
    gulong index;
    gchar *name;
    BtWave *other_wave;

    // check if there is already a wave with this id ad remove if so
    g_object_get ((gpointer) wave, "index", &index, "name", &name, NULL);
    if ((other_wave = bt_wavetable_get_wave_by_index (self, index))) {
      GST_DEBUG ("replacing old wave with same id");
      self->priv->waves = g_list_remove (self->priv->waves, other_wave);
      //g_signal_emit((gpointer)self,signals[WAVE_REMOVED_EVENT], 0, wave);
      /* TODO(ensonic): if song::is-playing==TRUE add this to a  self->priv->old_waves
       * and wait for song::is-playing==FALSE and free then */
      g_object_unref (other_wave);
    }

    self->priv->waves =
        g_list_append (self->priv->waves, g_object_ref ((gpointer) wave));
    g_signal_emit ((gpointer) self, signals[WAVE_ADDED_EVENT], 0, wave);

    update_wave_index_enum (index, name);
    ret = TRUE;
  } else {
    GST_WARNING ("trying to add wave again");
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
gboolean
bt_wavetable_remove_wave (const BtWavetable * const self,
    const BtWave * const wave)
{
  gboolean ret = FALSE;

  g_assert (BT_IS_WAVETABLE (self));
  g_assert (BT_IS_WAVE (wave));

  if (g_list_find (self->priv->waves, wave)) {
    gulong index;

    g_object_get ((gpointer) wave, "index", &index, NULL);
    update_wave_index_enum (index, g_strdup ("---"));

    self->priv->waves = g_list_remove (self->priv->waves, wave);
    g_signal_emit ((gpointer) self, signals[WAVE_REMOVED_EVENT], 0, wave);
    g_object_unref ((gpointer) wave);
    ret = TRUE;
  } else {
    GST_WARNING ("trying to remove wave that is not in the list");
  }
  return ret;
}

/**
 * bt_wavetable_get_wave_by_index:
 * @self: the wavetable to search for the wave
 * @index: the index of the wave
 *
 * Search the wavetable for a wave by the supplied index.
 * The wave must have been added previously to this wavetable with
 * bt_wavetable_add_wave().
 *
 * Returns: (transfer full): #BtWave instance or %NULL if not found. Unref the
 * wave, when done with it.
 */
BtWave *
bt_wavetable_get_wave_by_index (const BtWavetable * const self,
    const gulong index)
{
  const GList *node;
  gulong wave_index;

  g_assert (BT_IS_WAVETABLE (self));

  for (node = self->priv->waves; node; node = g_list_next (node)) {
    BtWave *const wave = BT_WAVE (node->data);
    g_object_get (wave, "index", &wave_index, NULL);
    if (index == wave_index)
      return g_object_ref (wave);
  }
  return NULL;
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
void
bt_wavetable_remember_missing_wave (const BtWavetable * const self,
    const gchar * const str)
{
  GST_INFO ("missing wave %s", str);
  self->priv->missing_waves =
      g_list_prepend (self->priv->missing_waves, (gpointer) str);
}

//-- io interface

static xmlNodePtr
bt_wavetable_persistence_save (const BtPersistence * const persistence,
    xmlNodePtr const parent_node)
{
  const BtWavetable *const self = BT_WAVETABLE (persistence);
  xmlNodePtr node = NULL;

  GST_DEBUG ("PERSISTENCE::wavetable");

  if ((node =
          xmlNewChild (parent_node, NULL, XML_CHAR_PTR ("wavetable"), NULL))) {
    bt_persistence_save_list (self->priv->waves, node);
  }
  return node;
}

static BtPersistence *
bt_wavetable_persistence_load (const GType type,
    const BtPersistence * const persistence, xmlNodePtr node, GError ** err,
    va_list var_args)
{
  const BtWavetable *const self = BT_WAVETABLE (persistence);
  xmlNodePtr child_node;

  GST_DEBUG ("PERSISTENCE::wavetable");
  g_assert (node);

  for (child_node = node->children; child_node; child_node = child_node->next) {
    if ((!xmlNodeIsText (child_node))
        && (!strncmp ((char *) child_node->name, "wave\0", 5))) {
      GError *err = NULL;

      BtWave *const wave =
          BT_WAVE (bt_persistence_load (BT_TYPE_WAVE, NULL, child_node, &err,
              "song", self->priv->song, NULL));
      if (err != NULL) {
        // collect failed waves
        gchar *const name, *const uri;

        g_object_get (wave, "name", &name, "uri", &uri, NULL);
        gchar *const str = g_strdup_printf ("%s: %s", name, uri);
        bt_wavetable_remember_missing_wave (self, str);
        g_free (name);
        g_free (uri);
        GST_WARNING ("Can't create wavetable: %s", err->message);
        g_error_free (err);
      }
      g_object_unref (wave);
    }
  }
  return BT_PERSISTENCE (persistence);
}

static void
bt_wavetable_persistence_interface_init (gpointer const g_iface,
    gpointer const iface_data)
{
  BtPersistenceInterface *const iface = g_iface;

  iface->load = bt_wavetable_persistence_load;
  iface->save = bt_wavetable_persistence_save;
}

//-- wrapper

//-- g_object overrides

static void
bt_wavetable_get_property (GObject * const object, const guint property_id,
    GValue * const value, GParamSpec * const pspec)
{
  const BtWavetable *const self = BT_WAVETABLE (object);
  return_if_disposed ();
  switch (property_id) {
    case WAVETABLE_SONG:
      g_value_set_object (value, self->priv->song);
      break;
    case WAVETABLE_WAVES:
      g_value_set_pointer (value, g_list_copy (self->priv->waves));
      break;
    case WAVETABLE_MISSING_WAVES:
      g_value_set_pointer (value, self->priv->missing_waves);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_wavetable_set_property (GObject * const object, const guint property_id,
    const GValue * const value, GParamSpec * const pspec)
{
  const BtWavetable *const self = BT_WAVETABLE (object);
  return_if_disposed ();
  switch (property_id) {
    case WAVETABLE_SONG:
      self->priv->song = BT_SONG (g_value_get_object (value));
      g_object_try_weak_ref (self->priv->song);
      //GST_DEBUG("set the song for wavetable: %p",self->priv->song);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bt_wavetable_dispose (GObject * const object)
{
  const BtWavetable *const self = BT_WAVETABLE (object);
  GList *node;

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  g_object_try_weak_unref (self->priv->song);
  // unref list of waves
  if (self->priv->waves) {
    for (node = self->priv->waves; node; node = g_list_next (node)) {
      GST_DEBUG ("  free wave : %" G_OBJECT_REF_COUNT_FMT,
          G_OBJECT_LOG_REF_COUNT (node->data));
      g_object_try_unref (node->data);
      node->data = NULL;
    }
  }

  G_OBJECT_CLASS (bt_wavetable_parent_class)->dispose (object);
}

static void
bt_wavetable_finalize (GObject * const object)
{
  const BtWavetable *const self = BT_WAVETABLE (object);

  GST_DEBUG ("!!!! self=%p", self);

  // free list of waves
  if (self->priv->waves) {
    g_list_free (self->priv->waves);
    self->priv->waves = NULL;
  }
  // free list of missing_waves
  if (self->priv->missing_waves) {
    const GList *node;
    for (node = self->priv->missing_waves; node; node = g_list_next (node)) {
      g_free (node->data);
    }
    g_list_free (self->priv->missing_waves);
    self->priv->missing_waves = NULL;
  }

  G_OBJECT_CLASS (bt_wavetable_parent_class)->finalize (object);
}

//-- class internals

static void
bt_wavetable_init (BtWavetable * self)
{
  self->priv = bt_wavetable_get_instance_private(self);
}

static void
bt_wavetable_class_init (BtWavetableClass * const klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = bt_wavetable_set_property;
  gobject_class->get_property = bt_wavetable_get_property;
  gobject_class->dispose = bt_wavetable_dispose;
  gobject_class->finalize = bt_wavetable_finalize;

  /**
   * BtWavetable::wave-added:
   * @self: the wavetable object that emitted the signal
   * @wave: the new wave
   *
   * A new wave item has been added to the wavetable
   */
  signals[WAVE_ADDED_EVENT] =
      g_signal_new ("wave-added", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,
      NULL, g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, BT_TYPE_WAVE);

  /**
   * BtWavetable::wave-removed:
   * @self: the setup object that emitted the signal
   * @wave: the old wave
   *
   * A wave item has been removed from the wavetable
   */
  signals[WAVE_REMOVED_EVENT] =
      g_signal_new ("wave-removed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS, 0, NULL,
      NULL, g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, BT_TYPE_WAVE);

  g_object_class_install_property (gobject_class, WAVETABLE_SONG,
      g_param_spec_object ("song", "song contruct prop",
          "Set song object, the wavetable belongs to", BT_TYPE_SONG,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, WAVETABLE_WAVES,
      g_param_spec_pointer ("waves",
          "waves list prop",
          "A copy of the list of waves",
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, WAVETABLE_MISSING_WAVES,
      g_param_spec_pointer ("missing-waves",
          "missing-waves list prop",
          "The list of missing waves, don't change",
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

//-- wavetable callback hack

static GstStructure *
get_wave_buffer (BtWavetable * self, guint wave_ix, guint wave_level_ix)
{
  BtWave *wave;
  BtWavelevel *wavelevel;
  GstStructure *s = NULL;

  if ((wave = bt_wavetable_get_wave_by_index (self, wave_ix))) {
    if ((wavelevel = bt_wave_get_level_by_index (wave, wave_level_ix))) {
      GstBuffer *buffer = NULL;
      gpointer *data;
      gulong length;
      guint channels;
      GstBtNote root_note;
      gsize size;

      g_object_get (wave, "channels", &channels, NULL);
      g_object_get (wavelevel, "data", &data, "length", &length, "root-note",
          &root_note, NULL);

      size = channels * length * sizeof (gint16);
      buffer =
          gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY, data, size, 0,
          size, NULL, NULL);

      s = gst_structure_new ("audio/x-raw",     // unused
          "format", G_TYPE_STRING, GST_AUDIO_NE (S16),  // unused
          "layout", G_TYPE_STRING, "interleaved",       // unused
          "rate", G_TYPE_INT, 44100,    // unused
          "channels", G_TYPE_INT, channels,
          "root-note", GSTBT_TYPE_NOTE, (guint) root_note,
          "buffer", GST_TYPE_BUFFER, buffer, NULL);

      g_object_unref (wavelevel);
    }
    g_object_unref (wave);
  }
  return s;
}

static gpointer callbacks[] = {
  /* user-data */
  NULL,
  /* callbacks */
  get_wave_buffer
};

/* bt_wavetable_get_callbacks:
 * @self: wabe table
 *
 * Get wavetable callbacks. The returned handle can be passed to plugin that
 * need wave-table access.
 *
 * Note: this is not a well defined interface yet and subject to change.
 *
 * Returns a static pointer array. The first entry will point to the wavetable
 * and the 2nd entry will be a pointer to a function get get wave buffers.
 */
gpointer
bt_wavetable_get_callbacks (BtWavetable * self)
{
  callbacks[0] = (gpointer) self;
  return &callbacks;
}
