/* $Id: wire.c,v 1.36 2004-10-28 11:16:29 ensonic Exp $
 * class for a machine to machine connection
 */
 
#define BT_CORE
#define BT_WIRE_C

#include <libbtcore/core.h>

enum {
  WIRE_SONG=1,
	WIRE_SRC,
	WIRE_DST
};

struct _BtWirePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
	
	/* the song the wire belongs to */
	BtSong *song;
	/* the main gstreamer container element */
	GstBin *bin;

	/* which machines are linked */
	BtMachine *src,*dst;
	/* wire type adapter elements */
	GstElement *convert,*scale;
	/* convinience pointer */
	GstElement *dst_elem,*src_elem;
  
  /* the element to control the gain of a connection */
  GstElement *gain;
  
  /* the element to analyse the output level of the wire (Filter/Analyzer/Audio/Level)
   * \@todo which volume does one wants to know about? (see machine.c)
   *   a.) the machine output (before the volume control on the wire)
   *   b.) the machine input (after the volume control on the wire)
   */
  GstElement *level;
  /* the element to analyse the frequence spectrum of the wire (Filter/Analyzer/Audio/Spectrum) */
  GstElement *spectrum;
  
};

//-- helper methods

/**
 * bt_wire_link_machines:
 * @self: the wire that should be used for this connection
 *
 * Links the gst-element of this wire and tries a couple for conversion elements
 * if necessary.
 *
 * Returns: true for success
 */
static gboolean bt_wire_link_machines(const BtWire *self) {
  gboolean res=TRUE;
  BtMachine *src, *dst;
  BtSong *song=self->priv->song;

  g_assert(BT_IS_WIRE(self));
  
  src=self->priv->src;
  dst=self->priv->dst;

  g_assert(GST_IS_OBJECT(src->src_elem));
  g_assert(GST_IS_OBJECT(dst->dst_elem));

	GST_DEBUG("trying to link machines directly : %p -> %p",src->src_elem,dst->dst_elem);
	// try link src to dst {directly, with convert, with scale, with ...}
	if(!gst_element_link(src->src_elem, dst->dst_elem)) {
		if(!self->priv->convert) {
			self->priv->convert=gst_element_factory_make("audioconvert",g_strdup_printf("audioconvert_%p",self));
			g_assert(self->priv->convert!=NULL);
		}
		gst_bin_add(self->priv->bin, self->priv->convert);
    GST_DEBUG("trying to link machines with convert");
		if(!gst_element_link_many(src->src_elem, self->priv->convert, dst->dst_elem, NULL)) {
			if(!self->priv->scale) {
				self->priv->scale=gst_element_factory_make("audioscale",g_strdup_printf("audioscale_%p",self));
				g_assert(self->priv->scale!=NULL);
			}
			gst_bin_add(self->priv->bin, self->priv->scale);
      GST_DEBUG("trying to link machines with scale");
			if(!gst_element_link_many(src->src_elem, self->priv->scale, dst->dst_elem, NULL)) {
        GST_DEBUG("trying to link machines with convert and scale");
				if(!gst_element_link_many(src->src_elem, self->priv->convert, self->priv->scale, dst->dst_elem, NULL)) {
					// try harder (scale, convert)
					GST_DEBUG("failed to link the machines");
          res=FALSE;
				}
				else {
					self->priv->src_elem=self->priv->scale;
					self->priv->dst_elem=self->priv->convert;
					GST_DEBUG("  wire okay with convert and scale");
				}
			}
			else {
				self->priv->src_elem=self->priv->scale;
				self->priv->dst_elem=self->priv->scale;
        GST_DEBUG("  wire okay with scale");
			}
		}
		else {
			self->priv->src_elem=self->priv->convert;
			self->priv->dst_elem=self->priv->convert;
			GST_DEBUG("  wire okay with convert");
		}
	}
	else {
		self->priv->src_elem=src->src_elem;
		self->priv->dst_elem=dst->dst_elem;
		GST_DEBUG("  wire okay");
	}
	return(res);
}

/**
 * bt_wire_unlink_machines:
 * @self: the wire that should be used for this connection
 *
 * Unlinks gst-element of this wire and removes the conversion elemnts from the
 * song.
 *
 * Returns: true for success
 */
static gboolean bt_wire_unlink_machines(const BtWire *self) {
  BtSong *song=self->priv->song;
  GstBin *bin;

  g_assert(BT_IS_WIRE(self));

	GST_DEBUG("trying to unlink machines");
	gst_element_unlink(self->priv->src->src_elem, self->priv->dst->dst_elem);
	if(self->priv->convert) {
		gst_element_unlink_many(self->priv->src->src_elem, self->priv->convert, self->priv->dst->dst_elem, NULL);
	}
	if(self->priv->scale) {
		gst_element_unlink_many(self->priv->src->src_elem, self->priv->scale, self->priv->dst->dst_elem, NULL);
	}
	if(self->priv->convert && self->priv->scale) {
		gst_element_unlink_many(self->priv->src->src_elem, self->priv->convert, self->priv->scale, self->priv->dst->dst_elem, NULL);
	}
	if(self->priv->convert) {
    GST_DEBUG("  removing convert from bin, obj->ref_count=%d",G_OBJECT(self->priv->convert)->ref_count);
		gst_bin_remove(self->priv->bin, self->priv->convert);
    //g_object_try_unref(self->priv->convert);
    self->priv->convert=NULL;
	}
	if(self->priv->scale) {
    GST_DEBUG("  removing scale from bin, obj->ref_count=%d",G_OBJECT(self->priv->scale)->ref_count);
		gst_bin_remove(self->priv->bin, self->priv->scale);
    //g_object_try_unref(self->priv->scale);
    self->priv->scale=NULL;
	}
}

/**
 * bt_wire_connect:
 * @self: the wire that should be used for this connection
 *
 * Connect two machines with a wire. The source and dsetination machine must be
 * passed upon construction. 
 * Each machine can be involved in multiple connections. The method
 * transparently add spreader or adder elements in such cases. It further
 * inserts elemnts for data-type conversion if neccesary.
 *
 * The machines must have been previously added to the setup using #bt_setup_add_machine().
 * Same way the resulting wire should be added to the setup using #bt_setup_add_wire().
 *
 * Returns: true for success
 */
static gboolean bt_wire_connect(BtWire *self) {
  gboolean res=FALSE;
  BtSong *song=self->priv->song;
  BtSetup *setup=NULL;
  BtWire *other_wire;
  BtMachine *src, *dst;
  GstElement *old_peer;

  g_assert(BT_IS_WIRE(self));

  if((!self->priv->src) || (!self->priv->dst)) goto Error;
  g_object_get(G_OBJECT(song),"bin",&self->priv->bin,"setup",&setup,NULL);
  src=self->priv->src;
  dst=self->priv->dst;

	GST_DEBUG("trying to link machines : %p (%p) -> %p (%p)",src,src->src_elem,dst,dst->dst_elem);

	// if there is already a connection from src && src has not yet an spreader (in use)
	if((other_wire=bt_setup_get_wire_by_src_machine(setup,src))
    && !bt_machine_has_active_spreader(src)) {
		GST_DEBUG("  other wire from src found");
		bt_wire_unlink_machines(other_wire);
		// create spreader (if needed)
    old_peer=src->src_elem;
    if(!bt_machine_activate_spreader(src)) {
      goto Error;
    }
    // if there is no converion element on the wire ..
		if(other_wire->priv->src_elem==old_peer) {
			// we need to fix the src_elem of the other connect, as we have inserted the spreader
			other_wire->priv->src_elem=src->src_elem;
		}
		// correct the link for the other wire
		if(!bt_wire_link_machines(other_wire)) {
		//if(!gst_element_link(other_wire->src->src_elem, other_wire->dst_elem)) {
			GST_ERROR("failed to re-link the machines after inserting internal spreaker");goto Error;
		}
	}
	
	// if there is already a wire to dst and dst has not yet an adder (in use)
	if((other_wire=bt_setup_get_wire_by_dst_machine(setup,dst))
    && !bt_machine_has_active_adder(dst)) {
		GST_DEBUG("  other wire to dst found");
		bt_wire_unlink_machines(other_wire);
		// create adder (if needed)
    old_peer=dst->dst_elem;
    if(!bt_machine_activate_adder(dst)) {
      goto Error;
    }
    // if there is no converion element on the wire ..
		if(other_wire->priv->dst_elem==old_peer) {
			// we need to fix the dst_elem of the other connect, as we have inserted the adder
			other_wire->priv->dst_elem=dst->dst_elem;
		}
		// correct the link for the other wire
		if(!bt_wire_link_machines(other_wire)) {
		//if(!gst_element_link(other_wire->src_elem, other_wire->dst->dst_elem)) {
			GST_ERROR("failed to re-link the machines after inserting internal adder");goto Error;
		}
	}
	
	if(!bt_wire_link_machines(self)) {
    GST_ERROR("linking machines failed");goto Error;
	}
  //bt_setup_add_wire(setup,self);
  res=TRUE;
  GST_DEBUG("linking machines succeded");
Error:
  g_object_try_unref(setup);
  if(!res) g_object_unref(self);
	return(res);
}

//-- constructor methods

/**
 * bt_wire_new:
 * @song: the song the new instance belongs to
 * @src_machine: the data source (#BtSourceMachine or #BtProcessorMachine)
 * @dst_machine: the data sink (#BtSinkMachine or #BtProcessorMachine)
 *
 * Create a new instance.
 * A wire should be added to a songs setup using
 * <code>#bt_setup_add_wire(setup,wire);</code>.
 * Both #BtMachine instances have to be added
 * (using <code>#bt_setup_add_machine(setup,BT_MACHINE(machine));</code>)
 * to the setup before as well
 *
 * Returns: the new instance or NULL in case of an error
 */
BtWire *bt_wire_new(const BtSong *song, const BtMachine *src_machine, const BtMachine *dst_machine) {
  BtWire *self;
  
  g_assert(BT_IS_SONG(song));
  g_assert(BT_IS_MACHINE(src_machine));
  g_assert(!BT_IS_SINK_MACHINE(src_machine));
  g_assert(BT_IS_MACHINE(dst_machine));
  g_assert(!BT_IS_SOURCE_MACHINE(dst_machine));
  g_assert(src_machine!=dst_machine);

  self=BT_WIRE(g_object_new(BT_TYPE_WIRE,"song",song,"src",src_machine,"dst",dst_machine,NULL));
  if(self) {
    // @todo check result
    bt_wire_connect(self);
  }
  return(self);
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void bt_wire_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtWire *self = BT_WIRE(object);
  return_if_disposed();
  switch (property_id) {
    case WIRE_SONG: {
      g_value_set_object(value, self->priv->song);
    } break;
    case WIRE_SRC: {
      g_value_set_object(value, self->priv->src);
    } break;
    case WIRE_DST: {
      g_value_set_object(value, self->priv->dst);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_wire_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtWire *self = BT_WIRE(object);
  return_if_disposed();
  switch (property_id) {
    case WIRE_SONG: {
      g_object_try_weak_unref(self->priv->song);
      self->priv->song = BT_SONG(g_value_get_object(value));
      g_object_try_weak_ref(self->priv->song);
      //GST_DEBUG("set the song for wire: %p",self->priv->song);
    } break;
		case WIRE_SRC: {
      g_object_try_unref(self->priv->src);
			self->priv->src = g_object_try_ref(g_value_get_object(value));
      GST_DEBUG("set the source element for the wire: %p",self->priv->src);
		} break;
		case WIRE_DST: {
      g_object_try_unref(self->priv->dst);
			self->priv->dst = g_object_try_ref(g_value_get_object(value));
      GST_DEBUG("set the target element for the wire: %p",self->priv->dst);
		} break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_wire_dispose(GObject *object) {
  BtWire *self = BT_WIRE(object);

	return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  
  // remove the GstElements from the bin
  if(self->priv->bin) {
    bt_wire_unlink_machines(self); // removes convert and scale if in use
    // @todo add the rest
    GST_DEBUG("  elements removed from bin");
    g_object_try_unref(self->priv->bin);
  }

	g_object_try_weak_unref(self->priv->song);
  //gstreamer uses floating references, therefore elements are destroyed, when removed from the bin
	g_object_try_unref(self->priv->dst);
	g_object_try_unref(self->priv->src);
  //g_object_try_unref(self->priv->convert);
  //g_object_try_unref(self->priv->scale);
  //g_object_try_unref(self->priv->gain);
  //g_object_try_unref(self->priv->level);
  //g_object_try_unref(self->priv->spectrum);
}

static void bt_wire_finalize(GObject *object) {
  BtWire *self = BT_WIRE(object);

  GST_DEBUG("!!!! self=%p",self);

  g_free(self->priv);
}

static void bt_wire_init(GTypeInstance *instance, gpointer g_class) {
  BtWire *self = BT_WIRE(instance);
  self->priv = g_new0(BtWirePrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_wire_class_init(BtWireClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  
  gobject_class->set_property = bt_wire_set_property;
  gobject_class->get_property = bt_wire_get_property;
  gobject_class->dispose      = bt_wire_dispose;
  gobject_class->finalize     = bt_wire_finalize;

  g_object_class_install_property(gobject_class,WIRE_SONG,
																	g_param_spec_object("song",
                                     "song construct prop",
                                     "the song object, the wire belongs to",
                                     BT_TYPE_SONG, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class,WIRE_SRC,
																	g_param_spec_object("src",
                                     "src ro prop",
                                     "src machine object, the wire links to",
                                     BT_TYPE_MACHINE, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class,WIRE_DST,
																	g_param_spec_object("dst",
                                     "dst ro prop",
                                     "dst machine object, the wire links to",
                                     BT_TYPE_MACHINE, /* object type */
                                     G_PARAM_CONSTRUCT_ONLY|G_PARAM_READWRITE));
}

GType bt_wire_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtWireClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_wire_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof (BtWire),
      0,   // n_preallocs
	    (GInstanceInitFunc)bt_wire_init, // instance_init
    };
		type = g_type_register_static(G_TYPE_OBJECT,"BtWire",&info,0);
  }
  return type;
}

