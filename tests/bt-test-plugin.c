/* $Id: bt-test-plugin.c,v 1.6 2005-07-26 20:39:44 ensonic Exp $
 * test gstreamer element for unit tests
 */

#include "bt-check.h"

//-- property ids

enum {
  ARG_BPM=1,	// tempo iface
  ARG_TPB,
  ARG_STPT,
  ARG_VOICES,	// child bin iface
  ARG_ULONG,
  ARG_DOUBLE,
  ARG_SWITCH,
	ARG_COUNT
};

//-- tempo interface implementation

static void bt_test_tempo_change_tempo(GstTempo *tempo, glong beats_per_minute, glong ticks_per_beat, glong subticks_per_tick) {  
  GST_INFO("changing tempo to %d BPM  %d TPB  %d STPT",beats_per_minute,ticks_per_beat,subticks_per_tick);
}

static void bt_test_tempo_interface_init(gpointer g_iface, gpointer iface_data) {
  GstTempoInterface *iface = g_iface;
  
  iface->change_tempo = bt_test_tempo_change_tempo;
}

//-- child proxy interface implementation

static GstObject *bt_test_child_proxy_get_child_by_index (GstChildProxy *child_proxy, guint index) {
  g_return_val_if_fail(index<BT_TEST_POLY_SOURCE(child_proxy)->num_voices,NULL);
  
  return(g_list_nth_data(BT_TEST_POLY_SOURCE(child_proxy)->voices,index));
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

static void bt_test_mono_source_class_init(BtTestMonoSourceClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  g_object_class_override_property(gobject_class, ARG_BPM, "beats-per-minute");
  g_object_class_override_property(gobject_class, ARG_TPB, "ticks-per-beat");
  g_object_class_override_property(gobject_class, ARG_STPT, "subticks-per-tick");

#ifdef USE_GST_DPARAMS
  g_object_class_install_property(gobject_class,ARG_ULONG,
                                  g_param_spec_ulong("ulong",
                                     "ulong prop",
                                     "ulong number parameter for the test_mono_source",
                                     0,
                                     G_MAXULONG,
                                     0,
                                     G_PARAM_READWRITE));
#endif
#ifdef USE_GST_CONTROLLER
  g_object_class_install_property(gobject_class,ARG_ULONG,
                                  g_param_spec_ulong("ulong",
                                     "ulong prop",
                                     "ulong number parameter for the test_mono_source",
                                     0,
                                     G_MAXULONG,
                                     0,
                                     G_PARAM_READWRITE|GST_PARAM_CONTROLLABLE));
#endif
}

static void bt_test_mono_source_base_init(BtTestMonoSourceClass *klass) {
  static const GstElementDetails details = {
		"Monophonic source for unit tests",
		"Source/Audio/MonoSource",
		"Use in unit tests",
		"Stefan Kost <ensonic@users.sf.net>"
  };
  GstElementClass *element_class = GST_ELEMENT_CLASS(klass);

  gst_element_class_set_details(element_class, &details);
}

GType bt_test_mono_source_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtTestMonoSourceClass),
      (GBaseInitFunc)bt_test_mono_source_base_init, 	// base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_test_mono_source_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtTestMonoSource),
      0,    // n_preallocs
	    NULL, // instance_init
			NULL  // value_table
    };
    static const GInterfaceInfo tempo_interface_info = {
      (GInterfaceInitFunc) bt_test_tempo_interface_init,          /* interface_init */
      NULL,               /* interface_finalize */
      NULL                /* interface_data */
    };
		type = g_type_register_static(GST_TYPE_ELEMENT,"BtTestMonoSource",&info,0);
    g_type_add_interface_static(type, GST_TYPE_TEMPO, &tempo_interface_info);
  }
  return type;
}

//-- test_poly_source

/* sets the given properties for this object */
static void bt_test_poly_source_set_property(GObject *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BtTestPolySource *self = BT_TEST_POLY_SOURCE(object);
  BtTestMonoSource *voice;
  gulong i,num_voices;
  
  switch (property_id) {
    case ARG_VOICES:
      num_voices=self->num_voices;
      self->num_voices = g_value_get_ulong(value);
      GST_INFO("changing voices from %d to %d",num_voices,self->num_voices);
      if(self->num_voices>num_voices) {
        for(i=num_voices;i<self->num_voices;i++) {
          self->voices=g_list_append(self->voices,g_object_new(BT_TYPE_TEST_MONO_SOURCE,NULL));
        }
      }
      else if(self->num_voices<num_voices) {
        for(i=num_voices;i>self->num_voices;i--) {
          voice=g_list_nth_data(self->voices,(i-1));
          self->voices=g_list_remove(self->voices,voice);
          g_object_unref(voice);
        }
      }
      break;
  }
}

static void bt_test_poly_source_finalize(GObject *object) {
  BtTestPolySource *self = BT_TEST_POLY_SOURCE(object);

  if(self->voices) {
    GList* node;
    for(node=self->voices;node;node=g_list_next(node)) {
      g_object_unref(node->data);
      node->data=NULL;
    }
    g_list_free(self->voices);
    self->voices=NULL;
  }
}

static void bt_test_poly_source_class_init(BtTestPolySourceClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  gobject_class->set_property = bt_test_poly_source_set_property;
  gobject_class->finalize     = bt_test_poly_source_finalize;
  
  g_object_class_override_property(gobject_class, ARG_BPM, "beats-per-minute");
  g_object_class_override_property(gobject_class, ARG_TPB, "ticks-per-beat");
  g_object_class_override_property(gobject_class, ARG_STPT, "subticks-per-tick");
  g_object_class_override_property(gobject_class, ARG_VOICES, "voices");

#ifdef USE_GST_DPARAMS
  g_object_class_install_property(gobject_class,ARG_ULONG,
                                  g_param_spec_ulong("ulong",
                                     "ulong prop",
                                     "ulong number parameter for the test_mono_source",
                                     0,
                                     G_MAXULONG,
                                     0,
                                     G_PARAM_READWRITE));
#endif
#ifdef USE_GST_CONTROLLER
  g_object_class_install_property(gobject_class,ARG_ULONG,
                                  g_param_spec_ulong("ulong",
                                     "ulong prop",
                                     "ulong number parameter for the test_mono_source",
                                     0,
                                     G_MAXULONG,
                                     0,
                                     G_PARAM_READWRITE|GST_PARAM_CONTROLLABLE));
#endif
}

static void bt_test_poly_source_base_init(BtTestPolySourceClass *klass) {
  static const GstElementDetails details = {
		"Polyphonic source for unit tests",
		"Source/Audio/PolySource",
		"Use in unit tests",
		"Stefan Kost <ensonic@users.sf.net>"
  };
  GstElementClass *element_class = GST_ELEMENT_CLASS(klass);

  gst_element_class_set_details(element_class, &details);
}

GType bt_test_poly_source_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtTestPolySourceClass),
      (GBaseInitFunc)bt_test_poly_source_base_init, 	// base_init
      NULL, // base_finalize
      (GClassInitFunc)bt_test_poly_source_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtTestPolySource),
      0,    // n_preallocs
	    NULL, // instance_init
			NULL  // value_table
    };
    static const GInterfaceInfo tempo_interface_info = {
      (GInterfaceInitFunc) bt_test_tempo_interface_init,          /* interface_init */
      NULL,               /* interface_finalize */
      NULL                /* interface_data */
    };
    static const GInterfaceInfo child_proxy_interface_info = {
      (GInterfaceInitFunc) bt_test_child_proxy_interface_init,    /* interface_init */
      NULL,               /* interface_finalize */
      NULL                /* interface_data */
    };
    static const GInterfaceInfo child_bin_interface_info = {
      NULL,               /* interface_init */
      NULL,               /* interface_finalize */
      NULL                /* interface_data */
    };

		type = g_type_register_static(GST_TYPE_ELEMENT,"BtTestPolySource",&info,0);
    g_type_add_interface_static(type, GST_TYPE_TEMPO, &tempo_interface_info);
    g_type_add_interface_static(type, GST_TYPE_CHILD_PROXY, &child_proxy_interface_info);
    g_type_add_interface_static(type, GST_TYPE_CHILD_BIN, &child_bin_interface_info);
  }
  return type;
}

//-- test_mono_processor

//-- test_poly_processor

//-- plugin handling

static gboolean bt_test_plugin_init (GstPlugin * plugin) {
  //GST_INFO("registering unit test plugin");

	gst_element_register(plugin,"buzztard-test-mono-source",GST_RANK_NONE,BT_TYPE_TEST_MONO_SOURCE);
	gst_element_register(plugin,"buzztard-test-poly-source",GST_RANK_NONE,BT_TYPE_TEST_POLY_SOURCE);
	//gst_element_register(plugin,"buzztard-test-mono-processor",GST_RANK_NONE,BT_TYPE_TEST_MONO_PROCESSOR);
	//gst_element_register(plugin,"buzztard-test-poly-processor",GST_RANK_NONE,BT_TYPE_TEST_POLY_PROCESSOR);
  
	// it not looks like we need to do it 
  //gst_registry_pool_add_plugin(plugin);
  return TRUE;
}

GST_PLUGIN_DEFINE_STATIC(GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "bt-test",
    "buzztard test plugin - several unit test support elements",
    bt_test_plugin_init, VERSION, "LGPL", PACKAGE_NAME, "http://www.buzztard.org"
)
