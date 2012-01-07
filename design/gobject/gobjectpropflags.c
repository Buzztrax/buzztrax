/* test gobject property flags
 *
 * building:
 * gcc -Wall -g `pkg-config glib-2.0 gobject-2.0 --cflags --libs` gobjectpropflags.c -o gobjectpropflags
 *
 * running:
 * ./gobjectpropflags
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
      g_print("setting 'test' = %d\n",self->test);
      break;
    case PROP_TEST_CONSTRUCT:
      self->test_construct = g_value_get_int(value);
      g_print("setting 'test_construct' = %d\n",self->test_construct);
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
  gint t1,t2;
  
  g_type_init();
  obj=g_object_new(GST_TYPE_PRESET_TEST,NULL);

  g_object_get(obj,"test",&t1,"test-construct",&t2,NULL);
  printf("get : 'test' = %d, 'test-construct' = %d\n",t1,t2);

  g_object_unref(obj);
  return 0;
}


/*
*/
