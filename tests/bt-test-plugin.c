/* $Id: bt-test-plugin.c,v 1.1 2005-07-18 16:07:36 ensonic Exp $
 * test gstreamer element for unit tests
 */

#include "bt-check.h"

GType bt_test_mono_source_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtTestMonoSourceClass),
      NULL, // base_init
      NULL, // base_finalize
      NULL, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtTestMonoSource),
      0,    // n_preallocs
	    NULL, // instance_init
			NULL  // value_table
    };
		type = g_type_register_static(GST_TYPE_ELEMENT,"BtTestMonoSource",&info,0);
  }
  return type;
}

//-- plugin handling

static gboolean plugin_init (GstPlugin * plugin) {

	gst_element_register(plugin,"buzztard-test-mono-source",GST_RANK_NONE,BT_TYPE_TEST_MONO_SOURCE);
	//gst_element_register(plugin,"buzztard-test-poly-source",GST_RANK_NONE,BT_TYPE_TEST_POLY_SOURCE);
	//gst_element_register(plugin,"buzztard-test-mono-processor",GST_RANK_NONE,BT_TYPE_TEST_MONO_PROCESSOR);
	//gst_element_register(plugin,"buzztard-test-poly-processor",GST_RANK_NONE,BT_TYPE_TEST_POLY_PROCESSOR);
  return TRUE;
}

GST_PLUGIN_DEFINE_STATIC(GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "bt-test",
    "buzztard test plugin - several unit test support elements",
    plugin_init, VERSION, "LGPL", PACKAGE_NAME, "http://www.buzztard.org"
)
