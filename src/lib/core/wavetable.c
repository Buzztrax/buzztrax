/* $Id: wavetable.c,v 1.6 2005-04-25 14:50:26 ensonic Exp $
 * class for wavetable
 */

#define BT_CORE
#define BT_WAVETABLE_C

#include <libbtcore/core.h>

//-- signal ids

enum {
  WAVE_ADDED_EVENT,
  LAST_SIGNAL
};

//-- property ids

enum {
  WAVETABLE_SONG=1,
	WAVETABLE_WAVES
};

struct _BtWavetablePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
	
	/* the song the wavetable belongs to */
	BtSong *song;
	
	GList *waves;		// each entry points to a BtWave
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
 * Returns: the new instance or NULL in case of an error
 */
BtWavetable *bt_wavetable_new(const BtSong *song) {
  BtWavetable *self;

	g_return_val_if_fail(BT_IS_SONG(song),NULL);

  if(!(self=BT_WAVETABLE(g_object_new(BT_TYPE_WAVETABLE,"song",song,NULL)))) {
		goto Error;
	}
  return(self);
Error:
	g_object_try_unref(self);
	return(NULL);
}

//-- private methods

//-- public methods

gboolean bt_wavetable_add_wave(const BtWavetable *self, const BtWave *wave) {
	gboolean ret=FALSE;
	
	g_return_val_if_fail(BT_IS_WAVETABLE(self),FALSE);
	g_return_val_if_fail(BT_IS_WAVE(wave),FALSE);

  if(!g_list_find(self->priv->waves,wave)) {
		ret=TRUE;
    self->priv->waves=g_list_append(self->priv->waves,g_object_ref(G_OBJECT(wave)));
		//g_signal_emit(G_OBJECT(self),signals[WAVE_ADDED_EVENT], 0, wave);
	}
  else {
    GST_WARNING("trying to add wave again"); 
  }
	return ret;
}

BtWave *bt_wavetable_get_wave_by_index(const BtWavetable *self, gulong index) {
	// @todo implement 
	return(NULL);
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_wavetable_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtWavetable *self = BT_WAVETABLE(object);
  return_if_disposed();
  switch (property_id) {
    case WAVETABLE_SONG: {
      g_value_set_object(value, self->priv->song);
    } break;
		case WAVETABLE_WAVES: {
			g_value_set_pointer(value,g_list_copy(self->priv->waves));
		} break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_wavetable_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtWavetable *self = BT_WAVETABLE(object);
  return_if_disposed();
  switch (property_id) {
    case WAVETABLE_SONG: {
      g_object_try_weak_unref(self->priv->song);
      self->priv->song = BT_SONG(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->song);
      //GST_DEBUG("set the song for wavetable: %p",self->priv->song);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_wavetable_dispose(GObject *object) {
  BtWavetable *self = BT_WAVETABLE(object);
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
        GObject *obj=node->data;
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

static void bt_wavetable_finalize(GObject *object) {
  BtWavetable *self = BT_WAVETABLE(object);

  GST_DEBUG("!!!! self=%p",self);

	// free list of waves
	if(self->priv->waves) {
		g_list_free(self->priv->waves);
		self->priv->waves=NULL;
	}
  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_wavetable_init(GTypeInstance *instance, gpointer g_class) {
  BtWavetable *self = BT_WAVETABLE(instance);
  self->priv = g_new0(BtWavetablePrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_wavetable_class_init(BtWavetableClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(G_TYPE_OBJECT);

  gobject_class->set_property = bt_wavetable_set_property;
  gobject_class->get_property = bt_wavetable_get_property;
  gobject_class->dispose      = bt_wavetable_dispose;
  gobject_class->finalize     = bt_wavetable_finalize;

	g_object_class_install_property(gobject_class,WAVETABLE_SONG,
                                  g_param_spec_object("song",
                                     "song contruct prop",
                                     "Set song object, the wavetable belongs to",
                                     BT_TYPE_SONG, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,WAVETABLE_WAVES,
                                  g_param_spec_pointer("waves",
                                     "waves list prop",
                                     "A copy of the list of waves",
                                     G_PARAM_READABLE));
}

GType bt_wavetable_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtWavetableClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_wavetable_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtWavetable),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_wavetable_init, // instance_init
			NULL // value_table
    };
		type = g_type_register_static(G_TYPE_OBJECT,"BtWavetable",&info,0);
  }
  return type;
}
