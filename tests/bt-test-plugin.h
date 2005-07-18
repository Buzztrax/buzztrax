/* $Id: bt-test-plugin.h,v 1.1 2005-07-18 16:07:36 ensonic Exp $
 * test gstreamer element for unit tests
 */

#ifndef BT_TEST_PLUGIN_H
#define BT_TEST_PLUGIN_H

#include <glib.h>
#include <glib-object.h>

//-- test_mono_source

#define BT_TYPE_TEST_MONO_SOURCE    				(bt_sequence_get_type ())
#define BT_TEST_MONO_SOURCE(obj)		        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_TEST_MONO_SOURCE, BtTestMonoSource))
#define BT_TEST_MONO_SOURCE_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_TEST_MONO_SOURCE, BtTestMonoSourceClass))
#define BT_IS_TEST_MONO_SOURCE(obj)	        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_TEST_MONO_SOURCE))
#define BT_IS_TEST_MONO_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_TEST_MONO_SOURCE))
#define BT_TEST_MONO_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_TEST_MONO_SOURCE, BtTestMonoSourceClass))

/* type macros */

typedef struct _BtTestMonoSource BtTestMonoSource;
typedef struct _BtTestMonoSourceClass BtTestMonoSourceClass;
typedef struct _BtTestMonoSourcePrivate BtTestMonoSourcePrivate;

/* monophonic source element */
struct _BtTestMonoSource {
  GObject parent;
  
  /*< private >*/
  BtTestMonoSourcePrivate *priv;
};
/* structure of the test_mono_source class */
struct _BtTestMonoSourceClass {
  GObjectClass parent_class;
};

/* used by TEST_MONO_SOURCE_TYPE */
GType bt_test_mono_source_get_type(void);

//-- test_poly_source

//-- test_mono_processor

//-- test_poly_processor

#endif /* BT_TEST_PLUGIN_H */
