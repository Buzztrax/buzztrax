/* $Id: wavelevel.c,v 1.1 2004-12-18 20:43:20 ensonic Exp $
 * class for wavelevel
 */

#define BT_CORE
#define BT_WAVELEVEL_C

#include <libbtcore/core.h>

//-- property ids

enum {
  WAVELEVEL_SONG=1
};

struct _BtWavelevelPrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
	
	/* the song the wavelevel belongs to */
	BtSong *song;
	
	GList *waves;		// each entry points to BtWave
};

static GObjectClass *parent_class=NULL;

//-- constructor methods

/**
 * bt_wavelevel_new:
 * @song: the song the new instance belongs to
 *
 * Create a new instance
 *
 * Returns: the new instance or NULL in case of an error
 */
BtWavelevel *bt_wavelevel_new(const BtSong *song) {
  BtWavelevel *self;

	g_return_val_if_fail(BT_IS_SONG(song),NULL);

  self=BT_WAVELEVEL(g_object_new(BT_TYPE_WAVELEVEL,"song",song,NULL));
  return(self);
}

//-- private methods

//-- public methods

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_wavelevel_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtWavelevel *self = BT_WAVELEVEL(object);
  return_if_disposed();
  switch (property_id) {
    case WAVELEVEL_SONG: {
      g_value_set_object(value, self->priv->song);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_wavelevel_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtWavelevel *self = BT_WAVELEVEL(object);
  return_if_disposed();
  switch (property_id) {
    case WAVELEVEL_SONG: {
      g_object_try_weak_unref(self->priv->song);
      self->priv->song = BT_SONG(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->song);
      //GST_DEBUG("set the song for wavelevel: %p",self->priv->song);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_wavelevel_dispose(GObject *object) {
  BtWavelevel *self = BT_WAVELEVEL(object);
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

static void bt_wavelevel_finalize(GObject *object) {
  BtWavelevel *self = BT_WAVELEVEL(object);

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

static void bt_wavelevel_init(GTypeInstance *instance, gpointer g_class) {
  BtWavelevel *self = BT_WAVELEVEL(instance);
  self->priv = g_new0(BtWavelevelPrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_wavelevel_class_init(BtWavelevelClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(G_TYPE_OBJECT);

  gobject_class->set_property = bt_wavelevel_set_property;
  gobject_class->get_property = bt_wavelevel_get_property;
  gobject_class->dispose      = bt_wavelevel_dispose;
  gobject_class->finalize     = bt_wavelevel_finalize;

	g_object_class_install_property(gobject_class,WAVELEVEL_SONG,
                                  g_param_spec_object("song",
                                     "song contruct prop",
                                     "Set song object, the wavelevel belongs to",
                                     BT_TYPE_SONG, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));
}

GType bt_wavelevel_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtWavelevelClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_wavelevel_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtWavelevel),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_wavelevel_init, // instance_init
			NULL // value_table
    };
		type = g_type_register_static(G_TYPE_OBJECT,"BtWavelevel",&info,0);
  }
  return type;
}
