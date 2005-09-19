// $Id: wire.c,v 1.62 2005-09-19 16:14:06 ensonic Exp $
/**
 * SECTION:btwire
 * @short_description: class for a connection of two #BtMachines
 */ 
 
/*
 * @todo try to derive this from GstBin!
 *  then put the machines into itself (and not into the songs bin, but insert the machine directly into the song->bin
 *  when adding internal machines we need to fix the ghost pads (this may require relinking)
 *    gst_element_add_ghost_pad and gst_element_remove_ghost_pad
 *  What if we can directly connect src->dst, then the wire can't be an element (nothing there to use as ghost-pads).
 *  On the other hand this is only the case, if we do not need converters and have no volume and no monitors.
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
  /* convenience pointer */
  GstElement *dst_elem,*src_elem;
  
  /* the element to control the gain of a connection */
  GstElement *gain;
  
  /* the element to analyze the output level of the wire (Filter/Analyzer/Audio/Level)
   * \@todo which volume does one wants to know about? (see machine.c)
   *   a.) the machine output (before the volume control on the wire)
   *   b.) the machine input (after the volume control on the wire)
   */
  GstElement *level;
  /* the element to Analise the frequency spectrum of the wire (Filter/Analyzer/Audio/Spectrum) */
  GstElement *spectrum;
  
};

static GObjectClass *parent_class=NULL;

//-- helper methods

/**
 * bt_wire_link_machines:
 * @self: the wire that should be used for this connection
 *
 * Links the gst-element of this wire and tries a couple for conversion elements
 * if necessary.
 *
 * Returns: %TRUE for success
 */
static gboolean bt_wire_link_machines(const BtWire *self) {
  gboolean res=TRUE;
  BtMachine *src, *dst;

  g_assert(BT_IS_WIRE(self));
  
  src=self->priv->src;
  dst=self->priv->dst;

  g_assert(GST_IS_OBJECT(src->src_elem));
  g_assert(GST_IS_OBJECT(dst->dst_elem));

  GST_DEBUG("trying to link machines directly : %p '%s' -> %p '%s'",src->src_elem,GST_OBJECT_NAME(src->src_elem),dst->dst_elem,GST_OBJECT_NAME(dst->dst_elem));
  // try link src to dst {directly, with convert, with scale, with ...}
  if(!gst_element_link(src->src_elem, dst->dst_elem)) {
    if(!self->priv->convert) {
      gchar *name=g_strdup_printf("audioconvert_%p",self);
      self->priv->convert=gst_element_factory_make("audioconvert",name);
      g_assert(self->priv->convert!=NULL);
      g_free(name);
    }
    gst_bin_add(self->priv->bin, self->priv->convert);
    GST_DEBUG("trying to link machines with convert");
    if(!gst_element_link_many(src->src_elem, self->priv->convert, dst->dst_elem, NULL)) {
      gst_element_unlink_many(src->src_elem, self->priv->convert, dst->dst_elem, NULL);
			/*
      if(!self->priv->scale) {
        gchar *name=g_strdup_printf("audioresample_%p",self);
        self->priv->scale=gst_element_factory_make("audioresample",name);
        g_assert(self->priv->scale!=NULL);
        g_free(name);
      }
      gst_bin_add(self->priv->bin, self->priv->scale);
      GST_DEBUG("trying to link machines with resample");
      if(!gst_element_link_many(src->src_elem, self->priv->scale, dst->dst_elem, NULL)) {
        gst_element_unlink_many(src->src_elem, self->priv->scale, dst->dst_elem, NULL);
        GST_DEBUG("trying to link machines with convert and resample");
        if(!gst_element_link_many(src->src_elem, self->priv->convert, self->priv->scale, dst->dst_elem, NULL)) {
          gst_element_unlink_many(src->src_elem, self->priv->convert, self->priv->scale, dst->dst_elem, NULL);
          GST_DEBUG("trying to link machines with scale and convert");
          if(!gst_element_link_many(src->src_elem, self->priv->scale, self->priv->convert, dst->dst_elem, NULL)) {
            gst_element_unlink_many(src->src_elem, self->priv->scale, self->priv->convert, dst->dst_elem, NULL);
			*/
            GST_DEBUG("failed to link the machines");
            // print out the content of both machines (using GST_DEBUG)
            bt_machine_dbg_print_parts(src);bt_machine_dbg_print_parts(dst);
            res=FALSE;
			/*
          }
          else {
            self->priv->src_elem=self->priv->convert;
            self->priv->dst_elem=self->priv->scale;
            GST_DEBUG("  wire okay with scale and convert");
          }
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
			*/
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
 * Unlinks gst-element of this wire and removes the conversion elements from the
 * song.
 */
static void bt_wire_unlink_machines(const BtWire *self) {

  g_assert(BT_IS_WIRE(self));

  GST_DEBUG("unlink machines '%s' -> '%s'",GST_OBJECT_NAME(self->priv->src->src_elem),GST_OBJECT_NAME(self->priv->dst->dst_elem));
  if(self->priv->convert && self->priv->scale) {
    gst_element_unlink_many(self->priv->src->src_elem, self->priv->convert, self->priv->scale, self->priv->dst->dst_elem, NULL);
  }
  else if(self->priv->convert) {
    gst_element_unlink_many(self->priv->src->src_elem, self->priv->convert, self->priv->dst->dst_elem, NULL);
  }
  else if(self->priv->scale) {
    gst_element_unlink_many(self->priv->src->src_elem, self->priv->scale, self->priv->dst->dst_elem, NULL);
  }
  else {
    gst_element_unlink(self->priv->src->src_elem, self->priv->dst->dst_elem);
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
 * Connect two machines with a wire. The source and destination machine must be
 * passed upon construction. 
 * Each machine can be involved in multiple connections. The method
 * transparently add spreader or adder elements in such cases. It further
 * inserts elements for data-type conversion if necessary.
 *
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
  GST_DEBUG("about to link machines, bin->ref_count=%d",G_OBJECT(self->priv->bin)->ref_count);
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
    // if there is no conversion element on the wire ..
    if(other_wire->priv->src_elem==old_peer) {
      // we need to fix the src_elem of the other connect, as we have inserted the spreader
      other_wire->priv->src_elem=src->src_elem;
    }
    // correct the link for the other wire
    if(!bt_wire_link_machines(other_wire)) {
      GST_ERROR("failed to re-link the machines after inserting internal spreader");goto Error;
    }
    g_object_unref(other_wire);
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
    // if there is no conversion element on the wire ..
    if(other_wire->priv->dst_elem==old_peer) {
      // we need to fix the dst_elem of the other connect, as we have inserted the adder
      other_wire->priv->dst_elem=dst->dst_elem;
    }
    // correct the link for the other wire
    if(!bt_wire_link_machines(other_wire)) {
      GST_ERROR("failed to re-link the machines after inserting internal adder");goto Error;
    }
    g_object_unref(other_wire);
  }
  
  if(!bt_wire_link_machines(self)) {
    GST_ERROR("linking machines failed");goto Error;
  }
  res=TRUE;
  GST_DEBUG("linking machines succeeded, bin->ref_count=%d",G_OBJECT(self->priv->bin)->ref_count);
Error:
  g_object_try_unref(setup);
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
 * The new wire is automaticall added to a songs setup. You don't need to call
 * <code>#bt_setup_add_wire(setup,wire);</code>.
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtWire *bt_wire_new(const BtSong *song, const BtMachine *src_machine, const BtMachine *dst_machine) {
  BtWire *self;
  BtSetup *setup;
  
  g_return_val_if_fail(BT_IS_SONG(song),NULL);
  g_return_val_if_fail(BT_IS_MACHINE(src_machine),NULL);
  g_return_val_if_fail(!BT_IS_SINK_MACHINE(src_machine),NULL);
  g_return_val_if_fail(BT_IS_MACHINE(dst_machine),NULL);
  g_return_val_if_fail(!BT_IS_SOURCE_MACHINE(dst_machine),NULL);
  g_return_val_if_fail(src_machine!=dst_machine,NULL);
  
  GST_INFO("create wire between %p and %p (%d)",src_machine,dst_machine,(src_machine!=dst_machine));

  if(!(self=BT_WIRE(g_object_new(BT_TYPE_WIRE,"song",song,"src",src_machine,"dst",dst_machine,NULL)))) {
    goto Error;
  }
  if(!bt_wire_connect(self)) {
    goto Error;
  }
  // add the wire to the setup of the song
  g_object_get(G_OBJECT(song),"setup",&setup,NULL);
  bt_setup_add_wire(setup,self);
  g_object_unref(setup);

  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods

/**
 * bt_wire_reconnect:
 * @self: the wire to re-link
 *
 * Call this method after internal elements in a #BtMachine have changed, but
 * failed to link.
 *
 * Returns: %TRUE for success and %FALSE otherwise
 */
gboolean bt_wire_reconnect(BtWire *self) {
  g_return_val_if_fail(BT_IS_WIRE(self),FALSE);
  
  GST_DEBUG("relinking machines '%s' -> '%s'",GST_OBJECT_NAME(self->priv->src->src_elem),GST_OBJECT_NAME(self->priv->dst->dst_elem));
  bt_wire_unlink_machines(self);
  return(bt_wire_link_machines(self));
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
      self->priv->src=BT_MACHINE(g_value_dup_object(value));
      GST_DEBUG("set the source element for the wire: %p",self->priv->src);
    } break;
    case WIRE_DST: {
      g_object_try_unref(self->priv->dst);
      self->priv->dst=BT_MACHINE(g_value_dup_object(value));
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
    // @todo add the remaining elements to remove (which?)
    GST_DEBUG("  releasing the bin, bin->ref_count=%d",(G_OBJECT(self->priv->bin))->ref_count);
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

  if(G_OBJECT_CLASS(parent_class)->dispose) {
    (G_OBJECT_CLASS(parent_class)->dispose)(object);
  }
}

static void bt_wire_finalize(GObject *object) {
  BtWire *self = BT_WIRE(object);

  GST_DEBUG("!!!! self=%p",self);

  g_free(self->priv);

  if(G_OBJECT_CLASS(parent_class)->finalize) {
    (G_OBJECT_CLASS(parent_class)->finalize)(object);
  }
}

static void bt_wire_init(GTypeInstance *instance, gpointer g_class) {
  BtWire *self = BT_WIRE(instance);
  self->priv = g_new0(BtWirePrivate,1);
  self->priv->dispose_has_run = FALSE;
}

static void bt_wire_class_init(BtWireClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  parent_class=g_type_class_ref(G_TYPE_OBJECT);
  
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
      G_STRUCT_SIZE(BtWireClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_wire_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtWire),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_wire_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(G_TYPE_OBJECT,"BtWire",&info,0);
  }
  return type;
}
