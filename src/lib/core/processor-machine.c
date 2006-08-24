/* $Id: processor-machine.c,v 1.43 2006-08-24 20:00:51 ensonic Exp $
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
 * SECTION:btprocessormachine
 * @short_description: class for signal processing machines with inputs and 
 * outputs
 *
 * Processors are machines that alter incomming audio.
 */ 

#define BT_CORE
#define BT_PROCESSOR_MACHINE_C

#include <libbtcore/core.h>
#include <libbtcore/machine.h>
#include <libbtcore/machine-private.h>

struct _BtProcessorMachinePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
};

static BtMachineClass *parent_class=NULL;

//-- helper methods

static void bt_processor_machine_post_init(BtProcessorMachine *self) {
  GstElement *element;
  
  g_object_get(self,"machine",&element,NULL);
  if(GST_IS_BASE_TRANSFORM(element)) {
    gst_base_transform_set_passthrough(GST_BASE_TRANSFORM(element),FALSE);
  }
  gst_object_unref(element);
}

//-- constructor methods

/**
 * bt_processor_machine_new:
 * @song: the song the new instance belongs to
 * @id: the id, we can use to lookup the machine
 * @plugin_name: the name of the gst-plugin the machine is using
 * @voices: the number of voices the machine should initially have
 *
 * Create a new instance.
 * The machine is automaticly added to the setup of the given song. You don't 
 * need to call <code>#bt_setup_add_machine(setup,BT_MACHINE(machine));</code>.
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtProcessorMachine *bt_processor_machine_new(const BtSong *song, const gchar *id, const gchar *plugin_name, glong voices) {
  BtProcessorMachine *self;
  
  g_return_val_if_fail(BT_IS_SONG(song),NULL);
  g_return_val_if_fail(BT_IS_STRING(id),NULL);
  g_return_val_if_fail(BT_IS_STRING(plugin_name),NULL);
  
  if(!(self=BT_PROCESSOR_MACHINE(g_object_new(BT_TYPE_PROCESSOR_MACHINE,"song",song,"id",id,"plugin-name",plugin_name,"voices",voices,NULL)))) {
    goto Error;
  }
  if(!bt_machine_new(BT_MACHINE(self))) {
    goto Error;
  }
  bt_processor_machine_post_init(self);
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods

//-- io interface

static xmlNodePtr bt_processor_machine_persistence_save(BtPersistence *persistence, xmlNodePtr parent_node, BtPersistenceSelection *selection) {
  BtProcessorMachine *self = BT_PROCESSOR_MACHINE(persistence);
  BtPersistenceInterface *parent_iface=g_type_interface_peek_parent(BT_PERSISTENCE_GET_INTERFACE(persistence));
  xmlNodePtr node=NULL;
  gchar *plugin_name;
  gulong voices;

  GST_DEBUG("PERSISTENCE::processor-machine");

  // save parent class stuff
  if((node=parent_iface->save(persistence,parent_node,selection))) {
    xmlNewProp(node,XML_CHAR_PTR("type"),XML_CHAR_PTR("processor"));

    g_object_get(G_OBJECT(self),"plugin-name",&plugin_name,"voices",&voices,NULL);
    xmlNewProp(node,XML_CHAR_PTR("plugin-name"),XML_CHAR_PTR(plugin_name));
    xmlNewProp(node,XML_CHAR_PTR("voices"),XML_CHAR_PTR(bt_persistence_strfmt_ulong(voices)));
    g_free(plugin_name);
  }
  return(node);
}

static gboolean bt_processor_machine_persistence_load(BtPersistence *persistence, xmlNodePtr node, BtPersistenceLocation *location) {
  BtProcessorMachine *self = BT_PROCESSOR_MACHINE(persistence);
  BtPersistenceInterface *parent_iface=g_type_interface_peek_parent(BT_PERSISTENCE_GET_INTERFACE(persistence));
  xmlChar *plugin_name,*voices_str;
  gulong voices;
  gboolean res;

  plugin_name=xmlGetProp(node,XML_CHAR_PTR("plugin-name"));
  voices_str=xmlGetProp(node,XML_CHAR_PTR("voices"));
  voices=voices_str?atol((char *)voices_str):0;
  g_object_set(G_OBJECT(self),"plugin-name",plugin_name,"voices",voices,NULL);
  xmlFree(plugin_name);xmlFree(voices_str);
  
  // load parent class stuff
  res=parent_iface->load(persistence,node,location);
  
  bt_processor_machine_post_init(self);
  
  return(res);
}

static void bt_processor_machine_persistence_interface_init(gpointer g_iface, gpointer iface_data) {
  BtPersistenceInterface *iface = g_iface;
  
  iface->load = bt_processor_machine_persistence_load;
  iface->save = bt_processor_machine_persistence_save;
}


//-- wrapper

//-- bt_machine overrides

static gboolean bt_processor_machine_check_type(const BtMachine *self,gulong pad_src_ct,gulong pad_sink_ct) {
  if(pad_src_ct==0 || pad_sink_ct==0) {
    gchar *plugin_name;
    
    g_object_get(G_OBJECT(self),"plugin-name",&plugin_name,NULL);
    GST_ERROR("  plugin \"%s\" is has %d src pads instead of >0 and %d sink pads instead of >0",
      plugin_name,pad_src_ct,pad_sink_ct);
    g_free(plugin_name);
    return(FALSE);
  }
  return(TRUE);
}

static void bt_processor_machine_setup(const BtMachine *self) {
  BtPattern *pattern;
  BtSong *song;
  
  g_object_get(G_OBJECT(self),"song",&song,NULL);
  if((pattern=bt_pattern_new_with_event(song,self,BT_PATTERN_CMD_BYPASS))) {
    g_object_unref(pattern);
  }
  g_object_unref(song);
  bt_machine_enable_output_gain(BT_MACHINE(self));
}


//-- g_object overrides

/* returns a property for the given property_id for this object */
static void bt_processor_machine_get_property(GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BtProcessorMachine *self = BT_PROCESSOR_MACHINE(object);
  return_if_disposed();
  switch (property_id) {
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_processor_machine_set_property(GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtProcessorMachine *self = BT_PROCESSOR_MACHINE(object);
  return_if_disposed();
  switch (property_id) {
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void bt_processor_machine_dispose(GObject *object) {
  BtProcessorMachine *self = BT_PROCESSOR_MACHINE(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p",self);
  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void bt_processor_machine_finalize(GObject *object) {
  BtProcessorMachine *self = BT_PROCESSOR_MACHINE(object);

  GST_DEBUG("!!!! self=%p",self);

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void bt_processor_machine_init(GTypeInstance *instance, gpointer g_class) {
  BtProcessorMachine *self = BT_PROCESSOR_MACHINE(instance);
  
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BT_TYPE_PROCESSOR_MACHINE, BtProcessorMachinePrivate);
}

//-- class internals

static void bt_processor_machine_class_init(BtProcessorMachineClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  BtMachineClass *machine_class = BT_MACHINE_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtProcessorMachinePrivate));
  
  gobject_class->set_property = bt_processor_machine_set_property;
  gobject_class->get_property = bt_processor_machine_get_property;
  gobject_class->dispose      = bt_processor_machine_dispose;
  gobject_class->finalize     = bt_processor_machine_finalize;

  machine_class->check_type   = bt_processor_machine_check_type;
  machine_class->setup        = bt_processor_machine_setup;
}

GType bt_processor_machine_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtProcessorMachineClass),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_processor_machine_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtProcessorMachine),
      0,   // n_preallocs
      (GInstanceInitFunc)bt_processor_machine_init, // instance_init
      NULL // value_table
    };
    static const GInterfaceInfo persistence_interface_info = {
      (GInterfaceInitFunc) bt_processor_machine_persistence_interface_init,  // interface_init
      NULL, // interface_finalize
      NULL  // interface_data
    };
    type = g_type_register_static(BT_TYPE_MACHINE,"BtProcessorMachine",&info,0);
    g_type_add_interface_static(type, BT_TYPE_PERSISTENCE, &persistence_interface_info);
  }
  return type;
}
