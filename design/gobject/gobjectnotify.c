/* test gobject notify during construction
 *
 * building:
 * gcc -Wall -g `pkg-config glib-2.0 gobject-2.0 gthread-2.0 --cflags --libs` gobjectnotify.c -o gobjectnotify
 *
 * running:
 * ./gobjectnotify
 * break g_object_dispatch_properties_changed
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <glib.h>
#include <glib-object.h>

static GType gst_preset_test_get_type (void);

#define GST_TYPE_PRESET_TEST            (gst_preset_test_get_type ())
#define GST_PRESET_TEST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_PRESET_TEST, GstPresetTest))
#define GST_PRESET_TEST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_PRESET_TEST, GstPresetTestClass))
#define GST_IS_PRESET_TEST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_PRESET_TEST))
#define GST_IS_PRESET_TEST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_PRESET_TEST))
#define GST_PRESET_TEST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_PRESET_TEST, GstPresetTestClass))

enum {
  PROP_TEST=1,
  PROP_TEST_CONSTRUCT
};

typedef struct _GstPresetTest {
  GObject parent;

  gint test;
  gint test_construct;
} GstPresetTest;

typedef struct _GstPresetTestClass {
  GObjectClass parent_class;
} GstPresetTestClass;

static void
gst_preset_test_get_property (GObject *object, guint property_id,
  GValue *value, GParamSpec *pspec)
{
  GstPresetTest *self = GST_PRESET_TEST(object);

  switch (property_id) {
    case PROP_TEST:
      g_value_set_int(value, self->test);
      break;
    case PROP_TEST_CONSTRUCT:
      g_value_set_int(value, self->test_construct);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
      break;
  }
}

static void
gst_preset_test_set_property (GObject *object, guint property_id,
  const GValue *value, GParamSpec *pspec)
{
  GstPresetTest *self = GST_PRESET_TEST(object);
  
  switch (property_id) {
    case PROP_TEST:
      self->test = g_value_get_int(value);
      //g_print("setting 'test' = %d\n",self->test);
      break;
    case PROP_TEST_CONSTRUCT:
      self->test_construct = g_value_get_int(value);
      //g_print("setting 'test_construct' = %d\n",self->test_construct);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
      break;
  }
}

static void
gst_preset_test_class_init(GObjectClass *klass) {
  klass->set_property = gst_preset_test_set_property;
  klass->get_property = gst_preset_test_get_property;

  g_object_class_install_property(klass, PROP_TEST,
      g_param_spec_int("test",
          "test prop",
          "test parameter for test",
          G_MININT, G_MAXINT, 5, G_PARAM_READWRITE));
  g_object_class_install_property(klass, PROP_TEST_CONSTRUCT,
      g_param_spec_int("test-construct",
          "test construct prop",
          "test construct  parameter for test",
          G_MININT, G_MAXINT, 5, G_PARAM_READWRITE|G_PARAM_CONSTRUCT));
}

static GType 
gst_preset_test_get_type (void)
{
  static GType type = 0;
  if (type == 0) {
    const GTypeInfo info = {
      sizeof (GstPresetTestClass),
      NULL,   /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc)gst_preset_test_class_init, /* class_init */
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (GstPresetTest),
      0,    /* n_preallocs */
      NULL, /* instance_init */
      NULL  /* value_table */
    };

    type = g_type_register_static (G_TYPE_OBJECT, "GstPresetTest", &info, 0);
  }
  return type;
}

int
main(int argc, char **argv)
{
  GObject *obj;
  gulong i;  
  
  g_type_init();

  for(i=0;i<(1<<20);i++) {
    obj=g_object_new(GST_TYPE_PRESET_TEST,"test",i,NULL);
    g_object_unref(obj);
  }
  g_print("%lu test cycles\n",i);

  return 0;
}


/*
$ LD_LIBRARY_PATH=/home/ensonic/debug/lib time ./gobjectnotify
*/
/*
# only one property

# before
3.05user 0.02system 0:03.11elapsed 98%CPU (0avgtext+0avgdata 0maxresident)k
3.14user 0.01system 0:03.23elapsed 97%CPU (0avgtext+0avgdata 0maxresident)k
3.04user 0.00system 0:03.09elapsed 98%CPU (0avgtext+0avgdata 0maxresident)k
3.05user 0.00system 0:03.08elapsed 98%CPU (0avgtext+0avgdata 0maxresident)k

# skip notify totally
2.55user 0.00system 0:02.58elapsed 99%CPU (0avgtext+0avgdata 0maxresident)k
2.60user 0.00system 0:02.62elapsed 99%CPU (0avgtext+0avgdata 0maxresident)k
2.58user 0.00system 0:02.59elapsed 99%CPU (0avgtext+0avgdata 0maxresident)k
2.56user 0.01system 0:02.59elapsed 99%CPU (0avgtext+0avgdata 0maxresident)k

# skip notify if no handlers in _constructor and _newv
2.80user 0.02system 0:02.84elapsed 99%CPU (0avgtext+0avgdata 0maxresident)k
2.85user 0.00system 0:02.89elapsed 98%CPU (0avgtext+0avgdata 0maxresident)k
2.84user 0.00system 0:02.87elapsed 98%CPU (0avgtext+0avgdata 0maxresident)k
2.81user 0.00system 0:02.84elapsed 98%CPU (0avgtext+0avgdata 0maxresident)k

*/

/*
# one construct and one normal property

# before
4.66user 0.01system 0:04.76elapsed 98%CPU (0avgtext+0avgdata 0maxresident)k
4.72user 0.01system 0:04.75elapsed 99%CPU (0avgtext+0avgdata 0maxresident)k
4.72user 0.01system 0:04.78elapsed 98%CPU (0avgtext+0avgdata 0maxresident)k
4.68user 0.01system 0:04.73elapsed 99%CPU (0avgtext+0avgdata 0maxresident)k

# skip notify if no handlers in _newv
4.65user 0.00system 0:04.69elapsed 99%CPU (0avgtext+0avgdata 0maxresident)k
4.63user 0.01system 0:04.67elapsed 99%CPU (0avgtext+0avgdata 0maxresident)k
4.61user 0.01system 0:04.65elapsed 99%CPU (0avgtext+0avgdata 0maxresident)k
4.69user 0.01system 0:04.78elapsed 98%CPU (0avgtext+0avgdata 0maxresident)k

# skip notify if no handlers in _constructor and _newv
4.12user 0.00system 0:04.16elapsed 99%CPU (0avgtext+0avgdata 0maxresident)k
4.14user 0.01system 0:04.20elapsed 98%CPU (0avgtext+0avgdata 0maxresident)k
4.17user 0.01system 0:04.22elapsed 99%CPU (0avgtext+0avgdata 0maxresident)k
4.12user 0.02system 0:04.18elapsed 99%CPU (0avgtext+0avgdata 0maxresident)k

*/

