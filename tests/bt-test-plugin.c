/* $Id: bt-test-plugin.c,v 1.2 2005-07-18 22:46:43 ensonic Exp $
 * test gstreamer element for unit tests
 */

#include "bt-check.h"

//-- property ids

enum {
  TEST_PAR_ULONG=1,
  TEST_PAR_DOUBLE,
  TEST_PAR_SWITCH
};

//-- tempo interface implementations

static void gst_test_tempo_change_tempo(GstTempo *tempo, glong beats_per_minute, glong ticks_per_beat, glong subticks_per_tick) {
  
  GST_INFO("changing tempo to %d BPM  %d TPB  %d STPT",beats_per_minute,ticks_per_beat,subticks_per_tick);
}

static void gst_test_tempo_interface_init(gpointer g_iface, gpointer iface_data) {
  GstTempoInterface *iface = g_iface;
  
  iface->change_tempo = gst_test_tempo_change_tempo;
}


//-- test_mono_source

static void bt_test_mono_source_class_init(BtTestMonoSourceClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

  g_object_class_install_property(gobject_class,TEST_PAR_ULONG,
                                  g_param_spec_ulong("ulong",
                                     "ulong prop",
                                     "ulong number parameter for the test_mono_source",
                                     0,
                                     G_MAXULONG,
                                     0,
                                     G_PARAM_READWRITE|GST_PARAM_CONTROLLABLE));
}

GType bt_test_mono_source_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtTestMonoSourceClass),
      NULL, // base_init
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
      (GInterfaceInitFunc) gst_test_tempo_interface_init,          /* interface_init */
      NULL,               /* interface_finalize */
      NULL                /* interface_data */
    };
		type = g_type_register_static(GST_TYPE_ELEMENT,"BtTestMonoSource",&info,0);
    g_type_add_interface_static(type, GST_TYPE_TEMPO, &tempo_interface_info);
  }
  return type;
}

//-- test_poly_source

//-- test_mono_processor

//-- test_poly_processor

//-- plugin handling

static gboolean plugin_init (GstPlugin * plugin) {
  GST_INFO("registering unit test plugin");

	gst_element_register(plugin,"buzztard-test-mono-source",GST_RANK_NONE,BT_TYPE_TEST_MONO_SOURCE);
	//gst_element_register(plugin,"buzztard-test-poly-source",GST_RANK_NONE,BT_TYPE_TEST_POLY_SOURCE);
	//gst_element_register(plugin,"buzztard-test-mono-processor",GST_RANK_NONE,BT_TYPE_TEST_MONO_PROCESSOR);
	//gst_element_register(plugin,"buzztard-test-poly-processor",GST_RANK_NONE,BT_TYPE_TEST_POLY_PROCESSOR);
  
  gst_registry_pool_add_plugin(plugin);
  return TRUE;
}

GST_PLUGIN_DEFINE_STATIC(GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "bt-test",
    "buzztard test plugin - several unit test support elements",
    plugin_init, VERSION, "LGPL", PACKAGE_NAME, "http://www.buzztard.org"
)
