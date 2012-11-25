/* propertyproxy
 *
 * building:
 * gcc -Wall -g propertyproxy.c -o propertyproxy `pkg-config glib-2.0 gobject-2.0 --cflags --libs`
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <glib.h>
#include <glib-object.h>

enum
{
  PROP_PROXY = 1,
};


typedef struct _BtChild
{
  GObject parent;
} BtChild;
typedef struct _BtChildClass
{
  GObjectClass parent;
} BtChildClass;
G_DEFINE_TYPE (BtChild, bt_child, G_TYPE_OBJECT)

     static void
         bt_child_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
}

static void
bt_child_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
}

static void
bt_child_init (BtChild * self)
{
}

static void
bt_child_class_init (BtChildClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;

  gobject_class->set_property = bt_child_set_property;
  gobject_class->get_property = bt_child_get_property;

  g_object_class_install_property (gobject_class, PROP_PROXY,
      g_param_spec_int ("test",
          "test prop",
          "test parameter for test", G_MININT, G_MAXINT, 5, G_PARAM_READWRITE));

}


typedef struct _BtComposite
{
  GObject parent;
} BtComposite;
typedef struct _BtCompositeClass
{
  GObjectClass parent;
} BtCompositeClass;
G_DEFINE_TYPE (BtComposite, bt_composite, G_TYPE_OBJECT)

     static void
         bt_composite_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
}

static void
bt_composite_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
}

static void
bt_composite_init (BtComposite * self)
{
}

static void
bt_composite_class_init (BtCompositeClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GObjectClass *child_klass = g_type_class_ref (bt_child_get_type ());

  gobject_class->set_property = bt_composite_set_property;
  gobject_class->get_property = bt_composite_get_property;

  g_object_class_install_property (gobject_class, PROP_PROXY,
      g_param_spec_override ("test",
          g_object_class_find_property (child_klass, "test")));
  //g_type_class_unref (child_klass);
}


gint
main (gint argc, gchar * argv[])
{
  GObject *o;
  GObjectClass *k;
  GParamSpec **props;
  guint ct, i;

  g_type_init ();

  o = g_object_new (bt_composite_get_type (), NULL);
  k = G_OBJECT_GET_CLASS (o);

  if ((props = g_object_class_list_properties (k, &ct))) {
    printf ("num-properties: %d\n", ct);
    for (i = 0; i < ct; i++) {
      puts (props[i]->name);
    }
    g_free (props);
  }

  g_object_unref (o);
  return 0;
}
