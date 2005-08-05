/* $Id: bt-test-plugin.h,v 1.6 2005-08-05 17:13:01 ensonic Exp $
 * test gstreamer element for unit tests
 */

#ifndef BT_TEST_PLUGIN_H
#define BT_TEST_PLUGIN_H

#include <glib.h>
#include <glib-object.h>

//-- test_mono_source

#define BT_TYPE_TEST_MONO_SOURCE            (bt_test_mono_source_get_type ())
#define BT_TEST_MONO_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_TEST_MONO_SOURCE, BtTestMonoSource))
#define BT_TEST_MONO_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_TEST_MONO_SOURCE, BtTestMonoSourceClass))
#define BT_IS_TEST_MONO_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_TEST_MONO_SOURCE))
#define BT_IS_TEST_MONO_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_TEST_MONO_SOURCE))
#define BT_TEST_MONO_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_TEST_MONO_SOURCE, BtTestMonoSourceClass))

/* type macros */

typedef struct _BtTestMonoSource BtTestMonoSource;
typedef struct _BtTestMonoSourceClass BtTestMonoSourceClass;

/* monophonic source element */
struct _BtTestMonoSource {
  GstElement parent;
};
/* structure of the test_mono_source class */
struct _BtTestMonoSourceClass {
  GstElementClass parent_class;
};

/* used by TEST_MONO_SOURCE_TYPE */
GType bt_test_mono_source_get_type(void);

//-- test_poly_source

#define BT_TYPE_TEST_POLY_SOURCE            (bt_test_poly_source_get_type ())
#define BT_TEST_POLY_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_TEST_POLY_SOURCE, BtTestPolySource))
#define BT_TEST_POLY_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_TEST_POLY_SOURCE, BtTestPolySourceClass))
#define BT_IS_TEST_POLY_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_TEST_POLY_SOURCE))
#define BT_IS_TEST_POLY_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_TEST_POLY_SOURCE))
#define BT_TEST_POLY_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_TEST_POLY_SOURCE, BtTestPolySourceClass))

/* type macros */

typedef struct _BtTestPolySource BtTestPolySource;
typedef struct _BtTestPolySourceClass BtTestPolySourceClass;

/* polyphonic source element */
struct _BtTestPolySource {
  GstElement parent;
  
  // the number of voices the plugin has
  gulong num_voices;
  GList *voices;
};
/* structure of the test_poly_source class */
struct _BtTestPolySourceClass {
  GstElementClass parent_class;
};

/* used by TEST_POLY_SOURCE_TYPE */
GType bt_test_poly_source_get_type(void);

//-- test_mono_processor

//-- test_poly_processor

#endif /* BT_TEST_PLUGIN_H */
