/* $Id: wave.c,v 1.8 2005-06-14 07:19:54 ensonic Exp $
 * class for wave
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
	WAVE_FILE_NAME
};

struct _BtWavePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
	
	/* the song the wave belongs to */
	BtSong *song;

	/* each wave has an index number, the list of waves can have empty slots */
	gulong index;	
	/* the name of the wave and the the sample file */
	gchar *name;
	gchar *file_name;
	
	GList *wavelevels;		// each entry points to a BtWavelevel
};

static GObjectClass *parent_class=NULL;

//static guint signals[LAST_SIGNAL]={0,};

//-- constructor methods

/**
 * bt_wave_new:
 * @song: the song the new instance belongs to
 * @name: the display name for the new wave
 * @file_name: the file system path of the sample data
 * @index: the list slot for the new wave
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtWave *bt_wave_new(const BtSong *song,const gchar *name,const gchar *file_name,gulong index) {
  BtWave *self;
	BtWavetable *wavetable;

	g_return_val_if_fail(BT_IS_SONG(song),NULL);

  if(!(self=BT_WAVE(g_object_new(BT_TYPE_WAVE,"song",song,"name",name,"file-name",file_name,"index",index,NULL)))) {
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
gboolean bt_wave_add_wavelevel(const BtWave *self, const BtWavelevel *wavelevel) {
	gboolean ret=FALSE;
	
	g_return_val_if_fail(BT_IS_WAVE(self),FALSE);
	g_return_val_if_fail(BT_IS_WAVELEVEL(wavelevel),FALSE);

  if(!g_list_find(self->priv->wavelevels,wavelevel)) {
		ret=TRUE;
    self->priv->wavelevels=g_list_append(self->priv->wavelevels,g_object_ref(G_OBJECT(wavelevel)));
		//g_signal_emit(G_OBJECT(self),signals[WAVELEVEL_ADDED_EVENT], 0, wavelevel);
	}
  else {
    GST_WARNING("trying to add wavelevel again"); 
  }
	return ret;
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_wave_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtWave *self = BT_WAVE(object);
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
    case WAVE_FILE_NAME: {
      g_value_set_string(value, self->priv->file_name);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_wave_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtWave *self = BT_WAVE(object);
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
    case WAVE_FILE_NAME: {
      g_free(self->priv->file_name);
      self->priv->file_name = g_value_dup_string(value);
      GST_DEBUG("set the file-name for wave: %s",self->priv->file_name);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_wave_dispose(GObject *object) {
  BtWave *self = BT_WAVE(object);
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
        GObject *obj=node->data;
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

static void bt_wave_finalize(GObject *object) {
  BtWave *self = BT_WAVE(object);

  GST_DEBUG("!!!! self=%p",self);

	// free list of wavelevels
	if(self->priv->wavelevels) {
		g_list_free(self->priv->wavelevels);
		self->priv->wavelevels=NULL;
	}
	g_free(self->priv->name);
	g_free(self->priv->file_name);
  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_wave_init(GTypeInstance *instance, gpointer g_class) {
  BtWave *self = BT_WAVE(instance);
  self->priv = g_new0(BtWavePrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_wave_class_init(BtWaveClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(G_TYPE_OBJECT);

  gobject_class->set_property = bt_wave_set_property;
  gobject_class->get_property = bt_wave_get_property;
  gobject_class->dispose      = bt_wave_dispose;
  gobject_class->finalize     = bt_wave_finalize;

	g_object_class_install_property(gobject_class,WAVE_SONG,
                                  g_param_spec_object("song",
                                     "song contruct prop",
                                     "Set song object, the wave belongs to",
                                     BT_TYPE_SONG, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY |G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,WAVE_WAVELEVELS,
                                  g_param_spec_pointer("wavelevels",
                                     "wavelevels list prop",
                                     "A copy of the list of wavelevels",
                                     G_PARAM_READABLE));

	g_object_class_install_property(gobject_class,WAVE_INDEX,
                                  g_param_spec_ulong("index",
                                     "index prop",
                                     "The index of the wave in the wavtable",
																		 0,
																		 G_MAXULONG,
                                     0, /* default value */
                                     G_PARAM_READWRITE));

	g_object_class_install_property(gobject_class,WAVE_NAME,
                                  g_param_spec_string("name",
                                     "name prop",
                                     "The name of the wave",
                                     "unamed wave", /* default value */
                                     G_PARAM_READWRITE));

	g_object_class_install_property(gobject_class,WAVE_FILE_NAME,
                                  g_param_spec_string("file-name",
                                     "file-name prop",
                                     "The file-name of the wave",
                                     NULL, /* default value */
                                     G_PARAM_READWRITE));
}

GType bt_wave_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtWaveClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_wave_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtWave),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_wave_init, // instance_init
			NULL // value_table
    };
		type = g_type_register_static(G_TYPE_OBJECT,"BtWave",&info,0);
  }
  return type;
}
