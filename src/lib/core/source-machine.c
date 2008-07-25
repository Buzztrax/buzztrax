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
 * SECTION:btsourcemachine
 * @short_description: class for signal processing machines with outputs only
 *
 * Sources are machines that generate audio.
 */
 
#define BT_CORE
#define BT_SOURCE_MACHINE_C

#include "core_private.h"
#include <libbtcore/source-machine.h>
#include <libbtcore/machine-private.h>

struct _BtSourceMachinePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
};

static BtMachineClass *parent_class=NULL;

// signal handlers

static void bt_machine_on_length_changed(BtSequence * const sequence, GParamSpec * const arg, gconstpointer user_data) {
  BtSourceMachine * const self=BT_SOURCE_MACHINE(user_data);
  GstElement * const element;
  gulong length;
  
  g_object_get(self,"machine",&element,NULL);
  g_object_get(sequence,"length",&length,NULL);

  // set num-buffers, so that eos gets emitted
  g_object_set(element,"num-buffers",(gint)length,NULL);
  gst_object_unref(element);
  
  GST_WARNING("update num-buffers for src to %lu",length);
}


// helpers

static void bt_source_machine_post_init(BtSourceMachine * const self) {
  GstElement * const element;
  
  g_object_get(self,"machine",&element,NULL);
  if(GST_IS_BASE_SRC(element)) {
    BtSong * const song;
    BtSequence * const sequence;
    
    g_object_get(G_OBJECT(self),"song",&song,NULL);
    g_object_get(G_OBJECT(song),"sequence",&sequence,NULL);
    g_signal_connect(sequence,"notify::length",G_CALLBACK(bt_machine_on_length_changed),(gpointer)self);
    bt_machine_on_length_changed(sequence,NULL,self);
    g_object_unref(sequence);
    g_object_unref(song);
    
    gst_base_src_set_live(GST_BASE_SRC(element),FALSE);
  }
  gst_object_unref(element);
}

//-- constructor methods

/**
 * bt_source_machine_new:
 * @song: the song the new instance belongs to
 * @id: the id, we can use to lookup the machine
 * @plugin_name: the name of the gst-plugin the machine is using
 * @voices: the number of voices the machine should initially have
 *
 * Create a new instance
 * The machine is automaticly added to the setup from the given song object. You
 * don't need to add the machine with
 * <code>#bt_setup_add_machine(setup,BT_MACHINE(machine));</code>.
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtSourceMachine *bt_source_machine_new(const BtSong * const song, const gchar * const id, const gchar * const plugin_name, const glong voices) {
  g_return_val_if_fail(BT_IS_SONG(song),NULL);
  g_return_val_if_fail(BT_IS_STRING(id),NULL);
  g_return_val_if_fail(BT_IS_STRING(plugin_name),NULL);
  
  BtSourceMachine * const self=BT_SOURCE_MACHINE(g_object_new(BT_TYPE_SOURCE_MACHINE,"song",song,"id",id,"plugin-name",plugin_name,"voices",voices,NULL));
  
  if(!self) {
    goto Error;
  }
  if(!bt_machine_new(BT_MACHINE(self))) {
    goto Error;
  }
  bt_source_machine_post_init(self);
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods

//-- io interface

static xmlNodePtr bt_source_machine_persistence_save(const BtPersistence * const persistence, xmlNodePtr const parent_node, const BtPersistenceSelection * const selection) {
  const BtSourceMachine * const self = BT_SOURCE_MACHINE(persistence);
  const BtPersistenceInterface * const parent_iface=g_type_interface_peek_parent(BT_PERSISTENCE_GET_INTERFACE(persistence));
  xmlNodePtr node=NULL;
  gchar * const plugin_name;
  gulong voices;

  GST_DEBUG("PERSISTENCE::source-machine");

  // save parent class stuff
  if((node=parent_iface->save(persistence,parent_node,selection))) {
    xmlNewProp(node,XML_CHAR_PTR("type"),XML_CHAR_PTR("source"));

    g_object_get(G_OBJECT(self),"plugin-name",&plugin_name,"voices",&voices,NULL);
    xmlNewProp(node,XML_CHAR_PTR("plugin-name"),XML_CHAR_PTR(plugin_name));
    xmlNewProp(node,XML_CHAR_PTR("voices"),XML_CHAR_PTR(bt_persistence_strfmt_ulong(voices)));
    g_free(plugin_name);
  }
  return(node);
}

static gboolean bt_source_machine_persistence_load(const BtPersistence * const persistence, xmlNodePtr node, const BtPersistenceLocation * const location) {
  BtSourceMachine * const self = BT_SOURCE_MACHINE(persistence);
  const BtPersistenceInterface * const parent_iface=g_type_interface_peek_parent(BT_PERSISTENCE_GET_INTERFACE(persistence));
  gboolean res;

  xmlChar * const plugin_name=xmlGetProp(node,XML_CHAR_PTR("plugin-name"));
  xmlChar * const voices_str=xmlGetProp(node,XML_CHAR_PTR("voices"));
  const gulong voices=voices_str?atol((char *)voices_str):0;
  g_object_set(G_OBJECT(self),"plugin-name",plugin_name,"voices",voices,NULL);
  xmlFree(plugin_name);
  xmlFree(voices_str);
  
  // load parent class stuff
  if((res=parent_iface->load(persistence,node,location))) {  
    bt_source_machine_post_init(self);
  }
  
  return(res);
}

static void bt_source_machine_persistence_interface_init(gpointer const g_iface, gpointer const iface_data) {
  BtPersistenceInterface * const iface = g_iface;
  
  iface->load = bt_source_machine_persistence_load;
  iface->save = bt_source_machine_persistence_save;
}

//-- wrapper

//-- bt_machine overrides

static gboolean bt_source_machine_check_type(const BtMachine * const self, const gulong pad_src_ct, const gulong pad_sink_ct) {
  if(pad_src_ct==0 || pad_sink_ct>0) {
    gchar * const plugin_name;
    
    g_object_get(G_OBJECT(self),"plugin-name",&plugin_name,NULL);
    GST_ERROR("  plugin \"%s\" is has %d src pads instead of >0 and %d sink pads instead of 0",
      plugin_name,pad_src_ct,pad_sink_ct);
    g_free(plugin_name);
    return(FALSE);
  }
  return(TRUE);
}

static void bt_source_machine_setup(const BtMachine * const self) {
  BtSong * const song;
  
  g_object_get(G_OBJECT(self),"song",&song,NULL);
  BtPattern * const pattern=bt_pattern_new_with_event(song,self,BT_PATTERN_CMD_SOLO);
  if(pattern) {
    g_object_unref(pattern);
  }
  g_object_unref(song);
  bt_machine_enable_output_gain(BT_MACHINE(self));
}

//-- g_object overrides

/* returns a property for the given property_id for this object */
static void bt_source_machine_get_property(GObject      * const object,
                               const guint         property_id,
                               GValue       * const value,
                               GParamSpec   * const pspec)
{
  const BtSourceMachine * const self = BT_SOURCE_MACHINE(object);
  return_if_disposed();
  switch (property_id) {
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_source_machine_set_property(GObject      * const object,
                              const guint         property_id,
                              const GValue * const value,
                              GParamSpec   * const pspec)
{
  const BtSourceMachine * const self = BT_SOURCE_MACHINE(object);
  return_if_disposed();
  switch (property_id) {
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_source_machine_dispose(GObject * const object) {
  const BtSourceMachine * const self = BT_SOURCE_MACHINE(object);
  BtSong * const song;

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;
  
  g_object_get(G_OBJECT(self),"song",&song,NULL);
  if(song) {
    BtSequence * const sequence;

    g_object_get(G_OBJECT(song),"sequence",&sequence,NULL);
    if(sequence) {
      g_signal_handlers_disconnect_matched(sequence,G_SIGNAL_MATCH_FUNC|G_SIGNAL_MATCH_DATA,0,0,NULL,bt_machine_on_length_changed,(gpointer)self);
  
      g_object_unref(sequence);
    }
    g_object_unref(song);   
  }

  GST_DEBUG("!!!! self=%p",self);
  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_source_machine_finalize(GObject * const object) {
  const BtSourceMachine * const self = BT_SOURCE_MACHINE(object);

  GST_DEBUG("!!!! self=%p",self);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

//-- class internals

static void bt_source_machine_init(GTypeInstance * const instance, gconstpointer g_class) {
  BtSourceMachine * const self = BT_SOURCE_MACHINE(instance);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_SOURCE_MACHINE, BtSourceMachinePrivate);
}

static void bt_source_machine_class_init(BtSourceMachineClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);
  BtMachineClass * const machine_class = BT_MACHINE_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtSourceMachinePrivate));
  
  gobject_class->set_property = bt_source_machine_set_property;
  gobject_class->get_property = bt_source_machine_get_property;
  gobject_class->dispose      = bt_source_machine_dispose;
  gobject_class->finalize     = bt_source_machine_finalize;

  machine_class->check_type   = bt_source_machine_check_type;
  machine_class->setup        = bt_source_machine_setup;
}

GType bt_source_machine_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      sizeof(BtSourceMachineClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_source_machine_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtSourceMachine),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_source_machine_init, // instance_init
      NULL // value_table
    };
    const GInterfaceInfo persistence_interface_info = {
      (GInterfaceInitFunc) bt_source_machine_persistence_interface_init,  // interface_init
      NULL, // interface_finalize
      NULL  // interface_data
    };
    type = g_type_register_static(BT_TYPE_MACHINE,"BtSourceMachine",&info,0);
    g_type_add_interface_static(type, BT_TYPE_PERSISTENCE, &persistence_interface_info);
  }
  return type;
}
