/* test gobject setter/getters
 *
 * building:
 * gcc -Wall -g gobjectprops.c -o gobjectprops `pkg-config glib-2.0 gobject-2.0 --cflags --libs`
 * gcc -Wall -g -DG_DISABLE_ASSERT -DG_DISABLE_CHECKS -DG_DISABLE_CAST_CHECKS gobjectprops.c -o gobjectprops `pkg-config glib-2.0 gobject-2.0 --cflags --libs`
 * only set or get
 * gcc -Wall -g -DSKIP_SET gobjectprops.c -o gobjectprops `pkg-config glib-2.0 gobject-2.0 --cflags --libs`
 * gcc -Wall -g -DSKIP_GET gobjectprops.c -o gobjectprops `pkg-config glib-2.0 gobject-2.0 --cflags --libs`
 *
 * running:
 * LD_LIBRARY_PATH=/home/ensonic/debug/lib ./gobjectprops
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

enum
{
  PROP_TEST = 1,
};

typedef struct _GstPresetTest
{
  GObject parent;

  gint test;
} GstPresetTest;

typedef struct _GstPresetTestClass
{
  GObjectClass parent_class;
} GstPresetTestClass;

static void
gst_preset_test_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstPresetTest *self = GST_PRESET_TEST (object);

  switch (property_id) {
    case PROP_TEST:
      g_value_set_int (value, self->test);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
gst_preset_test_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstPresetTest *self = GST_PRESET_TEST (object);

  switch (property_id) {
    case PROP_TEST:
      self->test = g_value_get_int (value);
      break;
  }
}

static void
gst_preset_test_class_init (GObjectClass * klass)
{
  klass->set_property = gst_preset_test_set_property;
  klass->get_property = gst_preset_test_get_property;

  g_object_class_install_property (klass, PROP_TEST,
      g_param_spec_int ("test",
          "test prop",
          "test parameter for preset test",
          G_MININT, G_MAXINT, 0, G_PARAM_READWRITE));
}

static GType
gst_preset_test_get_type (void)
{
  static GType type = 0;
  if (type == 0) {
    const GTypeInfo info = {
      sizeof (GstPresetTestClass),
      NULL,                     /* base_init */
      NULL,                     /* base_finalize */
      (GClassInitFunc) gst_preset_test_class_init,      /* class_init */
      NULL,                     /* class_finalize */
      NULL,                     /* class_data */
      sizeof (GstPresetTest),
      0,                        /* n_preallocs */
      NULL,                     /* instance_init */
      NULL                      /* value_table */
    };

    type = g_type_register_static (G_TYPE_OBJECT, "GstPresetTest", &info, 0);
  }
  return type;
}

void
test_set_test (GstPresetTest * self, gint i)
{
  g_return_if_fail (GST_IS_PRESET_TEST (self));

  self->test = i;
  g_object_notify (G_OBJECT (self), "test");
}

gint
test_get_test (GstPresetTest * self)
{
  g_return_val_if_fail (GST_IS_PRESET_TEST (self), 0);

  return self->test;
}

#ifndef SKIP_MON
static void
on_test_changed (GObject * obj, GParamSpec * arg, gpointer user_data)
{
  gint *notify_monitor = user_data;
  (*notify_monitor)++;
}
#endif


int
main (int argc, char **argv)
{
  GTimer *timer;
  GObject *obj;
  GObjectClass *klass;
  GValue val = { 0, };
  GParamSpec *pspec;
  GQuark detail_quark;
  guint signal_id;
  //gchar *signal_name;
  gint i, j;
  gdouble elapsed;
  gint range = G_MAXINT >> 10;
  gint notify_monitor;

  g_type_init ();
  obj = g_object_new (GST_TYPE_PRESET_TEST, NULL);
  j = 0;
  printf ("doing %d loops\n", range << 1);

#ifndef SKIP_MON
  g_signal_connect (obj, "notify::test", G_CALLBACK (on_test_changed),
      &notify_monitor);
#endif

  notify_monitor = 0;
  timer = g_timer_new ();
  for (i = -range; i < range; i++) {
#ifndef SKIP_SET
    g_object_set (obj, "test", i, NULL);
#endif
#ifndef SKIP_GET
    g_object_get (obj, "test", &j, NULL);
#endif
  }
  g_timer_stop (timer);
  elapsed = g_timer_elapsed (timer, NULL);
  printf ("set/get          : time spent %lf (%d change notification)\n",
      elapsed, notify_monitor);


  notify_monitor = 0;
  g_value_init (&val, G_TYPE_INT);
  timer = g_timer_new ();
  for (i = -range; i < range; i++) {
#ifndef SKIP_SET
    g_value_set_int (&val, i);
    g_object_set_property (obj, "test", &val);
#endif
#ifndef SKIP_GET
    g_object_get_property (obj, "test", &val);
    j = g_value_get_int (&val);
#endif
  }
  g_timer_stop (timer);
  elapsed = g_timer_elapsed (timer, NULL);
  printf ("set/get_property : time spent %lf (%d change notification)\n",
      elapsed, notify_monitor);
  g_value_unset (&val);


  notify_monitor = 0;
  g_timer_start (timer);
  for (i = -range; i < range; i++) {
#ifndef SKIP_SET
    test_set_test (GST_PRESET_TEST (obj), i);
#endif
#ifndef SKIP_GET
    j = test_get_test (GST_PRESET_TEST (obj));
#endif
  }
  g_timer_stop (timer);
  elapsed = g_timer_elapsed (timer, NULL);
  printf ("api              : time spent %lf (%d change notification)\n",
      elapsed, notify_monitor);


  notify_monitor = 0;
  g_value_init (&val, G_TYPE_INT);
  klass = G_OBJECT_CLASS (GST_PRESET_TEST_GET_CLASS (obj));
  while (klass && !(pspec = g_object_class_find_property (klass, "test"))) {
    klass = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
  }
  g_assert (klass);
  signal_id = g_signal_lookup ("notify", G_TYPE_OBJECT);
  detail_quark = g_quark_from_string ("test");
  g_timer_start (timer);
  for (i = -range; i < range; i++) {
#ifndef SKIP_SET
    g_value_set_int (&val, i);
    klass->set_property (obj, pspec->param_id, &val, pspec);
    g_signal_emit (obj, signal_id, detail_quark, pspec);
#endif
#ifndef SKIP_GET
    klass->get_property (obj, pspec->param_id, &val, pspec);
    j = g_value_get_int (&val);
#endif
  }
  g_timer_stop (timer);
  elapsed = g_timer_elapsed (timer, NULL);
  printf ("direct           : time spent %lf (%d change notification)\n",
      elapsed, notify_monitor);
  g_value_unset (&val);


  g_object_unref (obj);
  return 0;
}


/*
== both set/get: =
doing 2097151 loops
set/get          : time spent 9.663750
set/get_property : time spent 8.459261
api              : time spent 4.279584
direct           : time spent 0.857308

== only set: ==
doing 2097151 loops
set/get          : time spent 6.850322
set/get_property : time spent 6.047467
api              : time spent 3.879935
direct           : time spent 0.640669

== only get: ==
doing 2097151 loops
set/get          : time spent 2.341246
set/get_property : time spent 1.891384
api              : time spent 0.171456
direct           : time spent 0.242385

== oprofile runs ==
opreport -l ~ensonic/debug/lib/libgobject-2.0.so | head -n5
13467    11.5737  g_type_check_instance_is_a
11856    10.1892  g_object_set_valist
9018      7.7502  g_type_value_table_peek
8684      7.4631  .plt
8657      7.4399  g_type_check_instance_cast

opreport -l ~ensonic/debug/lib/libglib-2.0.so | head -n5
11000    24.2125  g_hash_table_lookup
5008     11.0233  g_atomic_pointer_compare_and_exchange
4907     10.8010  g_datalist_id_set_data_full
3509      7.7238  g_atomic_int_compare_and_exchange
3169      6.9754  g_slice_alloc
2934      6.4581  g_slice_free1

after applying: http://bugzilla.gnome.org/show_bug.cgi?id=526456 
opreport -l ~ensonic/debug/lib/libglib-2.0.so | head -n5
14137    13.7216  g_hash_table_lookup
13296    12.9054  g_slice_free1
12233    11.8736  g_slice_alloc
11068    10.7428  g_datalist_id_set_data_full
8189      7.9484  g_atomic_pointer_compare_and_exchange
6201      6.0188  g_slice_free_chain_with_offset

compiling with -DG_DISABLE_ASSERT -DG_DISABLE_CHECKS -DG_DISABLE_CAST_CHECKS results in more or less constant saving


*/
