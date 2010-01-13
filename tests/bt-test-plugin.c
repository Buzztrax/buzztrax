/* $Id$
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
 *
 * gstreamer test plugin for unit tests
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
 * SECTION::bttestplugin:
 * @short_description: test gstreamer element for unit tests
 *
 * Two stub elements for unit-tests. The Polyphonic elemnt use the monophonic
 * one for its voices. Thus prefix the parameter names with 'g-' for 'global'
 * (in the monophonic class) and 'v-' for 'voice' in the poly class.
 */
/* @todo:
 * - implement the tempo, property-meta ifaces in the test machines
 * - add a trigger param to the machines
 * - the trigger param counts how often it gets triggered
 * - the count is exposed as a read-only param
 */

#include "bt-check.h"

//-- property ids

enum {
  ARG_BPM=1,  // tempo iface
  ARG_TPB,
  ARG_STPT,
  ARG_VOICES,  // child bin iface
  ARG_ULONG,
  ARG_DOUBLE,
  ARG_SWITCH,
  ARG_COUNT
};

//-- pad templates

/*
static GstStaticPadTemplate sink_pad_template =
GST_STATIC_PAD_TEMPLATE (
  "sink",
  GST_PAD_SINK,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS ("ANY")
);
*/

static GstStaticPadTemplate src_pad_template =
GST_STATIC_PAD_TEMPLATE (
  "src",
  GST_PAD_SRC,
  GST_PAD_ALWAYS,
  GST_STATIC_CAPS ("ANY")
);

//-- tempo interface implementation

static void bt_test_tempo_change_tempo(GstBtTempo *tempo, glong beats_per_minute, glong ticks_per_beat, glong subticks_per_tick) {  
  GST_INFO("changing tempo to %lu BPM  %lu TPB  %lu STPT",beats_per_minute,ticks_per_beat,subticks_per_tick);
}

static void bt_test_tempo_interface_init(gpointer g_iface, gpointer iface_data) {
  GstBtTempoInterface *iface = g_iface;
  
  iface->change_tempo = bt_test_tempo_change_tempo;
}

//-- child proxy interface implementation

static GstObject *bt_test_child_proxy_get_child_by_index (GstChildProxy *child_proxy, guint index) {
  GST_INFO("machine %p, getting child %u of %lu",child_proxy,index,BT_TEST_POLY_SOURCE(child_proxy)->num_voices);
  g_return_val_if_fail(index<BT_TEST_POLY_SOURCE(child_proxy)->num_voices,NULL);
  
  return(g_object_ref(g_list_nth_data(BT_TEST_POLY_SOURCE(child_proxy)->voices,index)));
}

static guint bt_test_child_proxy_get_children_count (GstChildProxy *child_proxy) {
  return(BT_TEST_POLY_SOURCE(child_proxy)->num_voices);
}


static void bt_test_child_proxy_interface_init(gpointer g_iface, gpointer iface_data) {
  GstChildProxyInterface *iface = g_iface;
  
  iface->get_child_by_index = bt_test_child_proxy_get_child_by_index;
  iface->get_children_count = bt_test_child_proxy_get_children_count;
}


//-- test_mono_source

static void bt_test_mono_source_get_property(GObject *object,guint property_id,GValue *value,GParamSpec *pspec) {
  BtTestMonoSource *self = BT_TEST_MONO_SOURCE(object);

  switch (property_id) {
    case ARG_ULONG:
      g_value_set_ulong(value, self->ulong_val);
      break;
    case ARG_DOUBLE:
      g_value_set_double(value, self->double_val);
      break;
    case ARG_SWITCH:
      g_value_set_boolean(value, self->switch_val);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
      break;
  }
}

static void bt_test_mono_source_set_property(GObject *object,guint property_id,const GValue *value,GParamSpec *pspec) {
  BtTestMonoSource *self = BT_TEST_MONO_SOURCE(object);
  
  switch (property_id) {
    case ARG_ULONG:
      self->ulong_val = g_value_get_ulong(value);
      break;
    case ARG_DOUBLE:
      self->double_val = g_value_get_double(value);
      break;
    case ARG_SWITCH:
      self->switch_val = g_value_get_boolean(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
      break;
  }
}


static void bt_test_mono_source_init(GTypeInstance *instance, gpointer g_class) {
  BtTestMonoSource *self = BT_TEST_MONO_SOURCE(instance);
  GstElementClass *klass = GST_ELEMENT_GET_CLASS(instance);
  GstPad *src_pad;

  src_pad = gst_pad_new_from_template(gst_element_class_get_pad_template(klass,"src"),"src");
  gst_element_add_pad(GST_ELEMENT(self),src_pad);
}

static void bt_test_mono_source_class_init(BtTestMonoSourceClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->set_property = bt_test_mono_source_set_property;
  gobject_class->get_property = bt_test_mono_source_get_property;
  
  g_object_class_override_property(gobject_class, ARG_BPM, "beats-per-minute");
  g_object_class_override_property(gobject_class, ARG_TPB, "ticks-per-beat");
  g_object_class_override_property(gobject_class, ARG_STPT, "subticks-per-tick");

  g_object_class_install_property(gobject_class,ARG_ULONG,
                                  g_param_spec_ulong("g-ulong",
                                     "ulong prop",
                                     "ulong number parameter for the test_mono_source",
                                     0,
                                     G_MAXULONG,
                                     0,
                                     G_PARAM_READWRITE|GST_PARAM_CONTROLLABLE));

  g_object_class_install_property(gobject_class,ARG_DOUBLE,
                                  g_param_spec_double("g-double",
                                     "double prop",
                                     "double number parameter for the test_mono_source",
                                     -1000.0,
                                     1000.0,
                                     0,
                                     G_PARAM_READWRITE|GST_PARAM_CONTROLLABLE));

  g_object_class_install_property(gobject_class,ARG_SWITCH,
                                  g_param_spec_boolean("g-switch",
                                     "switch prop",
                                     "switch parameter for the test_mono_source",
                                     FALSE,
                                     G_PARAM_READWRITE|GST_PARAM_CONTROLLABLE));
}

static void bt_test_mono_source_base_init(BtTestMonoSourceClass *klass) {
  static const GstElementDetails details = {
    "Monophonic source for unit tests",
    "Source/Audio/MonoSource",
    "Use in unit tests",
    "Stefan Kost <ensonic@users.sf.net>"
  };
  GstElementClass *element_class = GST_ELEMENT_CLASS(klass);

  gst_element_class_add_pad_template (element_class,
        gst_static_pad_template_get (&src_pad_template));
    
  gst_element_class_set_details(element_class, &details);
}

GType bt_test_mono_source_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    const GTypeInfo info = {
      sizeof(BtTestMonoSourceClass),
      (GBaseInitFunc)bt_test_mono_source_base_init,   // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_test_mono_source_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtTestMonoSource),
      0,    // n_preallocs
      (GInstanceInitFunc)bt_test_mono_source_init, // instance_init
      NULL  // value_table
    };
    const GInterfaceInfo tempo_interface_info = {
      (GInterfaceInitFunc) bt_test_tempo_interface_init,          /* interface_init */
      NULL,               /* interface_finalize */
      NULL                /* interface_data */
    };
    type = g_type_register_static(GST_TYPE_ELEMENT,"BtTestMonoSource",&info,0);
    g_type_add_interface_static(type, GSTBT_TYPE_TEMPO, &tempo_interface_info);
  }
  return type;
}

//-- test_poly_source

static GObjectClass *poly_parent_class=NULL;

static void bt_test_poly_source_get_property(GObject *object,guint property_id,GValue *value,GParamSpec *pspec) {
  BtTestPolySource *self = BT_TEST_POLY_SOURCE(object);

  switch (property_id) {
    case ARG_ULONG:
      g_value_set_ulong(value, self->ulong_val);
      break;
    case ARG_DOUBLE:
      g_value_set_double(value, self->double_val);
      break;
    case ARG_SWITCH:
      g_value_set_boolean(value, self->switch_val);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
      break;
  }
}

static void bt_test_poly_source_set_property(GObject *object,guint property_id,const GValue *value,GParamSpec *pspec) {
  BtTestPolySource *self = BT_TEST_POLY_SOURCE(object);
  BtTestMonoSource *voice;
  gulong i,num_voices;
  
  switch (property_id) {
    case ARG_VOICES:
      num_voices=self->num_voices;
      self->num_voices = g_value_get_ulong(value);
      GST_INFO("machine %p, changing voices from %lu to %lu",object,num_voices,self->num_voices);
      if(self->num_voices>num_voices) {
        for(i=num_voices;i<self->num_voices;i++) {
          voice=g_object_new(BT_TYPE_TEST_MONO_SOURCE,NULL);
          self->voices=g_list_append(self->voices,voice);
          GST_DEBUG(" adding voice %p, ref_ct=%d",voice, G_OBJECT(voice)->ref_count);
          gst_object_set_parent(GST_OBJECT(voice), GST_OBJECT(self));
          GST_DEBUG(" parented voice %p, ref_ct=%d",voice, G_OBJECT(voice)->ref_count);
        }
      }
      else if(self->num_voices<num_voices) {
        for(i=num_voices;i>self->num_voices;i--) {
          voice=g_list_nth_data(self->voices,(i-1));
          self->voices=g_list_remove(self->voices,voice);
          gst_object_unparent(GST_OBJECT(voice));
        }
      }
      break;
    case ARG_ULONG:
      self->ulong_val = g_value_get_ulong(value);
      break;
    case ARG_DOUBLE:
      self->double_val = g_value_get_double(value);
      break;
    case ARG_SWITCH:
      self->switch_val = g_value_get_boolean(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
      break;
  }
}

static void bt_test_poly_source_finalize(GObject *object) {
  BtTestPolySource *self = BT_TEST_POLY_SOURCE(object);

  GST_DEBUG(" !!! self=%p",object);

  if(self->voices) {
    GList* node;
    for(node=self->voices;node;node=g_list_next(node)) {
      GST_DEBUG(" free voice %p, ref_ct=%d",node->data, G_OBJECT(node->data)->ref_count);
      gst_object_unparent(node->data);
      node->data=NULL;
    }
    g_list_free(self->voices);
    self->voices=NULL;
  }
  G_OBJECT_CLASS(poly_parent_class)->finalize(object);
}

static void bt_test_poly_source_init(GTypeInstance *instance, gpointer g_class) {
  BtTestPolySource *self = BT_TEST_POLY_SOURCE(instance);
  GstElementClass *klass = GST_ELEMENT_GET_CLASS(instance);
  GstPad *src_pad;

  src_pad = gst_pad_new_from_template(gst_element_class_get_pad_template(klass,"src"),"src");
  gst_element_add_pad(GST_ELEMENT(self),src_pad);
}

static void bt_test_poly_source_class_init(BtTestPolySourceClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  
  poly_parent_class=g_type_class_peek_parent(klass);

  gobject_class->set_property = bt_test_poly_source_set_property;
  gobject_class->get_property = bt_test_poly_source_get_property;
  //gobject_class->dispose      = bt_test_poly_source_dispose;
  gobject_class->finalize     = bt_test_poly_source_finalize;
  
  g_object_class_override_property(gobject_class, ARG_BPM, "beats-per-minute");
  g_object_class_override_property(gobject_class, ARG_TPB, "ticks-per-beat");
  g_object_class_override_property(gobject_class, ARG_STPT, "subticks-per-tick");
  g_object_class_override_property(gobject_class, ARG_VOICES, "children");

  g_object_class_install_property(gobject_class,ARG_ULONG,
                                  g_param_spec_ulong("v-ulong",
                                     "ulong prop",
                                     "ulong number parameter for the test_poly_source",
                                     0,
                                     G_MAXULONG,
                                     0,
                                     G_PARAM_READWRITE|GST_PARAM_CONTROLLABLE));

  g_object_class_install_property(gobject_class,ARG_DOUBLE,
                                  g_param_spec_double("g-double",
                                     "double prop",
                                     "double number parameter for the test_poly_source",
                                     -1000.0,
                                     1000.0,
                                     0,
                                     G_PARAM_READWRITE|GST_PARAM_CONTROLLABLE));

  g_object_class_install_property(gobject_class,ARG_SWITCH,
                                  g_param_spec_boolean("g-switch",
                                     "switch prop",
                                     "switch parameter for the test_poly_source",
                                     FALSE,
                                     G_PARAM_READWRITE|GST_PARAM_CONTROLLABLE));
}

static void bt_test_poly_source_base_init(BtTestPolySourceClass *klass) {
  static const GstElementDetails details = {
    "Polyphonic source for unit tests",
    "Source/Audio/PolySource",
    "Use in unit tests",
    "Stefan Kost <ensonic@users.sf.net>"
  };
  GstElementClass *element_class = GST_ELEMENT_CLASS(klass);

  gst_element_class_add_pad_template (element_class,
        gst_static_pad_template_get (&src_pad_template));

  gst_element_class_set_details(element_class, &details);
}

GType bt_test_poly_source_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    const GTypeInfo info = {
      sizeof(BtTestPolySourceClass),
      (GBaseInitFunc)bt_test_poly_source_base_init,   // base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_test_poly_source_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      sizeof(BtTestPolySource),
      0,    // n_preallocs
      (GInstanceInitFunc)bt_test_poly_source_init, // instance_init
      NULL  // value_table
    };
    const GInterfaceInfo tempo_interface_info = {
      (GInterfaceInitFunc) bt_test_tempo_interface_init,          /* interface_init */
      NULL,               /* interface_finalize */
      NULL                /* interface_data */
    };
    const GInterfaceInfo child_proxy_interface_info = {
      (GInterfaceInitFunc) bt_test_child_proxy_interface_init,    /* interface_init */
      NULL,               /* interface_finalize */
      NULL                /* interface_data */
    };
    const GInterfaceInfo child_bin_interface_info = {
      NULL,               /* interface_init */
      NULL,               /* interface_finalize */
      NULL                /* interface_data */
    };

    type = g_type_register_static(GST_TYPE_ELEMENT,"BtTestPolySource",&info,0);
    g_type_add_interface_static(type, GSTBT_TYPE_TEMPO, &tempo_interface_info);
    g_type_add_interface_static(type, GST_TYPE_CHILD_PROXY, &child_proxy_interface_info);
    g_type_add_interface_static(type, GSTBT_TYPE_CHILD_BIN, &child_bin_interface_info);
  }
  return type;
}

//-- test_mono_processor

//-- test_poly_processor

//-- plugin handling

#if !GST_CHECK_VERSION(0,10,16)
static
#endif
gboolean bt_test_plugin_init (GstPlugin * plugin) {
  //GST_INFO("registering unit test plugin");

  gst_element_register(plugin,"buzztard-test-mono-source",GST_RANK_NONE,BT_TYPE_TEST_MONO_SOURCE);
  gst_element_register(plugin,"buzztard-test-poly-source",GST_RANK_NONE,BT_TYPE_TEST_POLY_SOURCE);
  //gst_element_register(plugin,"buzztard-test-mono-processor",GST_RANK_NONE,BT_TYPE_TEST_MONO_PROCESSOR);
  //gst_element_register(plugin,"buzztard-test-poly-processor",GST_RANK_NONE,BT_TYPE_TEST_POLY_PROCESSOR);
  return TRUE;
}

#if !GST_CHECK_VERSION(0,10,16)
GST_PLUGIN_DEFINE_STATIC(
  GST_VERSION_MAJOR,
  GST_VERSION_MINOR,
  "bt-test",
  "buzztard test plugin - several unit test support elements",
  bt_test_plugin_init, VERSION, "LGPL", PACKAGE_NAME, "http://www.buzztard.org"
);
#endif

