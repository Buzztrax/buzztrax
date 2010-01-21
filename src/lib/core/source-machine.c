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
#include <libbuzztard-core/source-machine.h>

struct _BtSourceMachinePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;
};

static BtMachineClass *parent_class=NULL;

//-- pad templates
static GstStaticPadTemplate machine_src_template =
GST_STATIC_PAD_TEMPLATE ("src%d",
    GST_PAD_SRC,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS_ANY);

//-- constructor methods

/**
 * bt_source_machine_new:
 * @song: the song the new instance belongs to
 * @id: the id, we can use to lookup the machine
 * @plugin_name: the name of the gst-plugin the machine is using
 * @voices: the number of voices the machine should initially have
 * @err: inform about failed instance creation
 *
 * Create a new instance
 * The machine is automaticly added to the setup from the given song object. You
 * don't need to add the machine with
 * <code>#bt_setup_add_machine(setup,BT_MACHINE(machine));</code>.
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtSourceMachine *bt_source_machine_new(const BtSong * const song, const gchar * const id, const gchar * const plugin_name, const glong voices, GError **err) {
  return(BT_SOURCE_MACHINE(g_object_new(BT_TYPE_SOURCE_MACHINE,"construction-error",err,"song",song,"id",id,"plugin-name",plugin_name,"voices",voices,NULL)));
}

//-- methods

//-- io interface

static xmlNodePtr bt_source_machine_persistence_save(const BtPersistence * const persistence, xmlNodePtr const parent_node) {
  const BtSourceMachine * const self = BT_SOURCE_MACHINE(persistence);
  const BtPersistenceInterface * const parent_iface=g_type_interface_peek_parent(BT_PERSISTENCE_GET_INTERFACE(persistence));
  xmlNodePtr node=NULL;
  gchar * const plugin_name;
  gulong voices;

  GST_DEBUG("PERSISTENCE::source-machine");

  // save parent class stuff
  if((node=parent_iface->save(persistence,parent_node))) {
    xmlNewProp(node,XML_CHAR_PTR("type"),XML_CHAR_PTR("source"));

    g_object_get(G_OBJECT(self),"plugin-name",&plugin_name,"voices",&voices,NULL);
    xmlNewProp(node,XML_CHAR_PTR("plugin-name"),XML_CHAR_PTR(plugin_name));
    xmlNewProp(node,XML_CHAR_PTR("voices"),XML_CHAR_PTR(bt_persistence_strfmt_ulong(voices)));
    g_free(plugin_name);
  }
  return(node);
}

static BtPersistence *bt_source_machine_persistence_load(const GType type, const BtPersistence * const persistence, xmlNodePtr node, GError **err, va_list var_args) {
  BtSourceMachine *self;
  BtPersistence *result;
  BtPersistenceInterface *parent_iface;

  xmlChar * const id=xmlGetProp(node,XML_CHAR_PTR("id"));
  xmlChar * const plugin_name=xmlGetProp(node,XML_CHAR_PTR("plugin-name"));
  xmlChar * const voices_str=xmlGetProp(node,XML_CHAR_PTR("voices"));
  const gulong voices=voices_str?atol((char *)voices_str):0;
  
  if(!persistence) {
    BtSong *song=NULL;
    gchar *param_name;

    // we need to get parameters from var_args
    param_name=va_arg(var_args,gchar*);
    while(param_name) {
      if(!strcmp(param_name,"song")) {
        song=va_arg(var_args, gpointer);
      }
      else {
        GST_WARNING("unhandled argument: %s",param_name);
        break;
      }
      param_name=va_arg(var_args,gchar*);
    }
    
    self=bt_source_machine_new(song,(gchar*)id,(gchar *)plugin_name,voices,err);
    result=BT_PERSISTENCE(self);
  }
  else {
    self=BT_SOURCE_MACHINE(persistence);
    result=BT_PERSISTENCE(persistence);

    g_object_set(self,"plugin-name",plugin_name,"voices",voices,NULL);
  }
  xmlFree(id);
  xmlFree(plugin_name);
  xmlFree(voices_str);
  
  // load parent class stuff
  parent_iface=g_type_interface_peek_parent(BT_PERSISTENCE_GET_INTERFACE(result));
  parent_iface->load(BT_TYPE_MACHINE,result,node,NULL,NULL);

  return(result);
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
    GST_ERROR("  plugin \"%s\" is has %lu src pads instead of >0 and %lu sink pads instead of 0",
      plugin_name,pad_src_ct,pad_sink_ct);
    g_free(plugin_name);
    return(FALSE);
  }
  return(TRUE);
}

//-- g_object overrides

static void bt_source_machine_constructed(GObject *object) {
  BtSourceMachine * const self=BT_SOURCE_MACHINE(object);
  GError **err;
  
  GST_INFO("source-machine constructed");

  G_OBJECT_CLASS(parent_class)->constructed(object);

  g_object_get(G_OBJECT(self),"construction-error",&err,NULL);
  if(err==NULL || *err==NULL) {
    GstElement * const element;
    BtSong * const song;
    BtSetup *setup;
    BtPattern *pattern;
    
    g_object_get(G_OBJECT(self),"machine",&element,"song",&song,NULL);
    if(GST_IS_BASE_SRC(element)) {
      // don't ever again get the idea to turn of is_live for basesrc elements
      // if they are live, they are, no matter what we want
      //  we get "can't record audio fast enough"
      if(gst_base_src_is_live(GST_BASE_SRC(element))) {
        if(GST_IS_BASE_AUDIO_SRC(element)) {
          g_object_set(element,"buffer-time",150*GST_MSECOND,NULL);
        }
      }
    }
    gst_object_unref(element);
    pattern=bt_pattern_new_with_event(song,BT_MACHINE(self),BT_PATTERN_CMD_SOLO);
    g_object_unref(pattern);
    
    bt_machine_activate_spreader(BT_MACHINE(self));
    bt_machine_enable_output_gain(BT_MACHINE(self));

    // add the machine to the setup of the song
    g_object_get(song,"setup",&setup,NULL);
    bt_setup_add_machine(setup,BT_MACHINE(self));
    g_object_unref(setup);

    g_object_unref(song);
  }
}

/* returns a property for the given property_id for this object */
static void bt_source_machine_get_property(GObject * const object, const guint property_id, GValue * const value, GParamSpec * const pspec) {
  const BtSourceMachine * const self = BT_SOURCE_MACHINE(object);
  return_if_disposed();
  switch (property_id) {
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void bt_source_machine_set_property(GObject * const object, const guint property_id, const GValue * const value, GParamSpec * const pspec) {
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

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

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
  GstElementClass * const gstelement_klass = GST_ELEMENT_CLASS(klass);
  BtMachineClass * const machine_class = BT_MACHINE_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtSourceMachinePrivate));

  gobject_class->constructed  = bt_source_machine_constructed;
  gobject_class->set_property = bt_source_machine_set_property;
  gobject_class->get_property = bt_source_machine_get_property;
  gobject_class->dispose      = bt_source_machine_dispose;
  gobject_class->finalize     = bt_source_machine_finalize;

  machine_class->check_type   = bt_source_machine_check_type;
  
  gst_element_class_add_pad_template(gstelement_klass, gst_static_pad_template_get(&machine_src_template));
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
