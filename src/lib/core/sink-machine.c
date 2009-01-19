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
 * SECTION:btsinkmachine
 * @short_description: class for signal processing machines with inputs only
 *
 * Sinks are machines that do playback or recording of the song. The
 * sink-machine utilizes the #BtSinkBin to transparently switch elements between
 * record (encoding) and playback.
 */ 
 
#define BT_CORE
#define BT_SINK_MACHINE_C

#include "core_private.h"
#include <libbuzztard-core/sink-machine.h>

struct _BtSinkMachinePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
};

static BtMachineClass *parent_class=NULL;

//-- constructor methods

/**
 * bt_sink_machine_new:
 * @song: the song the new instance belongs to
 * @id: the id, we can use to lookup the machine
 * @err: inform about failed instance creation
 *
 * Create a new instance.
 * The machine is automaticly added to the setup from the given song object. You
 * don't need to add the machine with
 * <code>#bt_setup_add_machine(setup,BT_MACHINE(machine));</code>.
 * The element used for this machine is #BtSinkBin which is configured according
 * to the use-case (playback, recordfing). The playback device is taken from the
 * #BtSettings.
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtSinkMachine *bt_sink_machine_new(const BtSong * const song, const gchar * const id, GError **err) {
  return(BT_SINK_MACHINE(g_object_new(BT_TYPE_SINK_MACHINE,"construction-error",err,"song",song,"id",id,"plugin-name","bt-sink-bin",NULL)));
}

#if 0
/* @todo: bt_sink_machine_new_dummy:
 * @song: the song the new instance belongs to
 * @id: the id, we can use to lookup the machine
 *
 * Create a new instance.
 * The machine is automaticly added to the setup from the given song object. You
 * don't need to add the machine with
 * <code>#bt_setup_add_machine(setup,BT_MACHINE(machine));</code>.
 * The element used for this machine is identity. This sink-machine can be used
 * to play multiple songs simultaneously.
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtSinkMachine *bt_sink_machine_new_dummy(const BtSong * const song, const gchar * const id) {
  g_return_val_if_fail(BT_IS_SONG(song),NULL);
  g_return_val_if_fail(BT_IS_STRING(id),NULL);

  BtSinkMachine * const self=BT_SINK_MACHINE(g_object_new(BT_TYPE_SINK_MACHINE,"song",song,"id",id,"plugin-name","identity",NULL));

  if(!self) {
    goto Error;
  }
  if(!bt_machine_new(BT_MACHINE(self))) {
    goto Error;
  }
  bt_sink_machine_post_init(self);
  
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}
#endif

//-- methods

//-- io interface

static xmlNodePtr bt_sink_machine_persistence_save(const BtPersistence * const persistence, xmlNodePtr const parent_node, const BtPersistenceSelection * const selection) {
  //BtSinkMachine *self = BT_SINK_MACHINE(persistence);
  BtPersistenceInterface * const parent_iface=g_type_interface_peek_parent(BT_PERSISTENCE_GET_INTERFACE(persistence));
  xmlNodePtr node=NULL;

  GST_DEBUG("PERSISTENCE::sink-machine");

  // save parent class stuff
  if((node=parent_iface->save(persistence,parent_node,selection))) {
    /* @todo: save own stuff */
    xmlNewProp(node,XML_CHAR_PTR("type"),XML_CHAR_PTR("sink"));
  }
  return(node);
}

static BtPersistence *bt_sink_machine_persistence_load(const GType type, const BtPersistence * const persistence, xmlNodePtr node, const BtPersistenceLocation * const location, GError **err, va_list var_args) {
  BtSinkMachine *self;
  BtPersistence *result;
  BtPersistenceInterface *parent_iface;

  GST_DEBUG("PERSISTENCE::sink_machine");
  g_assert(node);
  
  xmlChar * const id=xmlGetProp(node,XML_CHAR_PTR("id"));

  if(!persistence) {
    BtSong *song=NULL;
    gchar *param_name;

    // we need to get parameters from var_args
    // @todo: this is duplicated code among the subclasses
    param_name=va_arg(var_args,gchar*);
    while(param_name) {
      if(!strcmp(param_name,"song")) {
        song=va_arg(var_args,gpointer);
      }
      else {
        GST_WARNING("unhandled argument: %s",param_name);
        break;
      }
      param_name=va_arg(var_args,gchar*);
    }
 
    self=bt_sink_machine_new(song,(gchar*)id,err);
    result=BT_PERSISTENCE(self);
  }
  else {
    self=BT_SINK_MACHINE(persistence); 
    result=BT_PERSISTENCE(persistence);

    g_object_set(G_OBJECT(self),"plugin-name","bt-sink-bin",NULL);
  }
  xmlFree(id);
  
  // load parent class stuff
  parent_iface=g_type_interface_peek_parent(BT_PERSISTENCE_GET_INTERFACE(result));
  parent_iface->load(BT_TYPE_MACHINE,result,node,location,NULL,NULL);

  return(result);
}

static void bt_sink_machine_persistence_interface_init(gpointer const g_iface, gpointer const iface_data) {
  BtPersistenceInterface * const iface = g_iface;
  
  iface->load = bt_sink_machine_persistence_load;
  iface->save = bt_sink_machine_persistence_save;
}

//-- wrapper

//-- bt_machine overrides

static gboolean bt_sink_machine_check_type(const BtMachine * const self, const gulong pad_src_ct, const gulong pad_sink_ct) {
  if(pad_src_ct>0 || pad_sink_ct==0) {
    gchar * const plugin_name;
    
    g_object_get(G_OBJECT(self),"plugin-name",&plugin_name,NULL);
    GST_ERROR("  plugin \"%s\" is has %lu src pads instead of 0 and %lu sink pads instead of >0",
      plugin_name,pad_src_ct,pad_sink_ct);
    g_free(plugin_name);
    return(FALSE);
  }
  return(TRUE);
}

//-- g_object overrides

static void bt_sink_machine_constructed(GObject *object) {
  BtSinkMachine * const self=BT_SINK_MACHINE(object);
  GError **err;
    
  GST_INFO("sink-machine constructed");

  G_OBJECT_CLASS(parent_class)->constructed(object);

  g_object_get(G_OBJECT(self),"construction-error",&err,NULL);
  if(*err==NULL) {
    BtSong * const song;
    GstElement * const element;
    GstElement * const gain;
    BtSetup *setup;
    
    bt_machine_enable_input_gain(BT_MACHINE(self));
    g_object_get(G_OBJECT(self),"machine",&element,"song",&song,"input-gain",&gain, NULL);
    g_object_set(G_OBJECT(element),"input-gain",gain,NULL);
    g_object_set(G_OBJECT(song),"master",G_OBJECT(self),NULL);
    gst_object_unref(element);
    gst_object_unref(gain);
    
    // add the machine to the setup of the song
    g_object_get(G_OBJECT(song),"setup",&setup,NULL);
    bt_setup_add_machine(setup,BT_MACHINE(self));
    g_object_unref(setup);
    
    g_object_unref(song);
  }
}

/* returns a property for the given property_id for this object */
static void bt_sink_machine_get_property(GObject * const object, const guint property_id, GValue * const value, GParamSpec * const pspec) {
  const BtSinkMachine * const self = BT_SINK_MACHINE(object);
  return_if_disposed();
  switch (property_id) {
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_sink_machine_set_property(GObject * const object, const guint property_id, const GValue * const value, GParamSpec * const pspec) {
  const BtSinkMachine * const self = BT_SINK_MACHINE(object);
  return_if_disposed();
  switch (property_id) {
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_sink_machine_dispose(GObject * const object) {
  const BtSinkMachine * const self = BT_SINK_MACHINE(object);
  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_sink_machine_finalize(GObject * const object) {
  const BtSinkMachine * const self = BT_SINK_MACHINE(object);

  GST_DEBUG("!!!! self=%p",self);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

//-- class internals

static void bt_sink_machine_init(GTypeInstance * const instance, gconstpointer g_class) {
  BtSinkMachine *self = BT_SINK_MACHINE(instance);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_SINK_MACHINE, BtSinkMachinePrivate);
}

static void bt_sink_machine_class_init(BtSinkMachineClass *klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);
  BtMachineClass * const machine_class = BT_MACHINE_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtSinkMachinePrivate));

  gobject_class->constructed  = bt_sink_machine_constructed;
  gobject_class->set_property = bt_sink_machine_set_property;
  gobject_class->get_property = bt_sink_machine_get_property;
  gobject_class->dispose      = bt_sink_machine_dispose;
  gobject_class->finalize     = bt_sink_machine_finalize;

  machine_class->check_type   = bt_sink_machine_check_type;
}

GType bt_sink_machine_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof(BtSinkMachineClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_sink_machine_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtSinkMachine),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_sink_machine_init, // instance_init
      NULL // value_table
    };
    const GInterfaceInfo persistence_interface_info = {
      (GInterfaceInitFunc) bt_sink_machine_persistence_interface_init,  // interface_init
      NULL, // interface_finalize
      NULL  // interface_data
    };
    type = g_type_register_static(BT_TYPE_MACHINE,"BtSinkMachine",&info,0);
    g_type_add_interface_static(type, BT_TYPE_PERSISTENCE, &persistence_interface_info);
  }
  return type;
}
