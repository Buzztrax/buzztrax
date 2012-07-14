/* test gobject construction/destruction order
 *
 * building:
 * gcc -Wall -g `pkg-config glib-2.0 gobject-2.0 --cflags --libs` gobjectlife.c -o gobjectlife
 *
 * running:
 * LD_LIBRARY_PATH=/home/ensonic/debug/lib ./gobjectlife
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <glib.h>
#include <glib-object.h>


static void
stamp (const gchar * func_name)
{
  printf ("  %s\n", func_name);
}

/* test class 1 */

static GType g_life_test1_get_type (void);
#define G_TYPE_LIFE_TEST1            (g_life_test1_get_type ())

typedef struct _GLifeTest1
{
  GObject parent;
} GLifeTest1;

typedef struct _GLifeTest1Class
{
  GObjectClass parent_class;
} GLifeTest1Class;

static GObjectClass *g_life_test1_parent_class = NULL;

static GObject *
g_life_test1_constructor (GType type, guint n_cp, GObjectConstructParam * cp)
{
  GObject *self;

  self =
      G_OBJECT_CLASS (g_life_test1_parent_class)->constructor (type, n_cp, cp);
  stamp (__PRETTY_FUNCTION__);
  return self;
}

static void
g_life_test1_constructed (GObject * object)
{
  stamp (__PRETTY_FUNCTION__);
  if (G_OBJECT_CLASS (g_life_test1_parent_class)->constructed)
    G_OBJECT_CLASS (g_life_test1_parent_class)->constructed (object);
}

static void
g_life_test1_dispose (GObject * object)
{
  stamp (__PRETTY_FUNCTION__);
  G_OBJECT_CLASS (g_life_test1_parent_class)->dispose (object);
}

static void
g_life_test1_finalize (GObject * object)
{
  stamp (__PRETTY_FUNCTION__);
  G_OBJECT_CLASS (g_life_test1_parent_class)->finalize (object);
}

static void
g_life_test1_base_init (gpointer g_class)
{
  stamp (__PRETTY_FUNCTION__);
}

static void
g_life_test1_base_finalize (gpointer g_class)
{
  stamp (__PRETTY_FUNCTION__);
}

static void
g_life_test1_class_init (gpointer g_class, gpointer class_data)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);

  g_life_test1_parent_class = g_type_class_peek_parent (g_class);

  gobject_class->constructor = g_life_test1_constructor;
  gobject_class->constructed = g_life_test1_constructed;
  gobject_class->dispose = g_life_test1_dispose;
  gobject_class->finalize = g_life_test1_finalize;
  stamp (__PRETTY_FUNCTION__);
}

static void
g_life_test1_init (GTypeInstance * instance, gpointer g_class)
{
  stamp (__PRETTY_FUNCTION__);
}

static GType
g_life_test1_get_type (void)
{
  static GType type = 0;
  if (type == 0) {
    const GTypeInfo info = {
      sizeof (GLifeTest1Class),
      g_life_test1_base_init,
      g_life_test1_base_finalize,       /* not called for static types */
      g_life_test1_class_init,
      NULL,                     /* class finalizer (not allowed for static types) */
      NULL,                     /* class_data */
      sizeof (GLifeTest1),
      0,                        /* n_preallocs */
      g_life_test1_init,
      NULL                      /* value_table */
    };

    type = g_type_register_static (G_TYPE_OBJECT, "GLifeTest1", &info, 0);
  }
  return type;
}

/* test class 2 */

static GType g_life_test2_get_type (void);
#define G_TYPE_LIFE_TEST2            (g_life_test2_get_type ())

typedef struct _GLifeTest2
{
  GLifeTest1 parent;
} GLifeTest2;

typedef struct _GLifeTest2Class
{
  GLifeTest1Class parent_class;
} GLifeTest2Class;

static GObjectClass *g_life_test2_parent_class = NULL;

static GObject *
g_life_test2_constructor (GType type, guint n_cp, GObjectConstructParam * cp)
{
  GObject *self;

  self =
      G_OBJECT_CLASS (g_life_test2_parent_class)->constructor (type, n_cp, cp);
  stamp (__PRETTY_FUNCTION__);
  return self;
}

static void
g_life_test2_constructed (GObject * object)
{
  stamp (__PRETTY_FUNCTION__);
  if (G_OBJECT_CLASS (g_life_test2_parent_class)->constructed)
    G_OBJECT_CLASS (g_life_test2_parent_class)->constructed (object);
}

static void
g_life_test2_dispose (GObject * object)
{
  stamp (__PRETTY_FUNCTION__);
  G_OBJECT_CLASS (g_life_test2_parent_class)->dispose (object);
}

static void
g_life_test2_finalize (GObject * object)
{
  stamp (__PRETTY_FUNCTION__);
  G_OBJECT_CLASS (g_life_test2_parent_class)->finalize (object);
}

static void
g_life_test2_base_init (gpointer g_class)
{
  stamp (__PRETTY_FUNCTION__);
}

static void
g_life_test2_base_finalize (gpointer g_class)
{
  stamp (__PRETTY_FUNCTION__);
}

static void
g_life_test2_class_init (gpointer g_class, gpointer class_data)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);

  g_life_test2_parent_class = g_type_class_peek_parent (g_class);

  gobject_class->constructor = g_life_test2_constructor;
  gobject_class->constructed = g_life_test2_constructed;
  gobject_class->dispose = g_life_test2_dispose;
  gobject_class->finalize = g_life_test2_finalize;
  stamp (__PRETTY_FUNCTION__);
}

static void
g_life_test2_init (GTypeInstance * instance, gpointer g_class)
{
  stamp (__PRETTY_FUNCTION__);
}

static GType
g_life_test2_get_type (void)
{
  static GType type = 0;
  if (type == 0) {
    const GTypeInfo info = {
      sizeof (GLifeTest2Class),
      g_life_test2_base_init,
      g_life_test2_base_finalize,       /* not called for static types */
      g_life_test2_class_init,
      NULL,                     /* class finalizer (not allowed for static types) */
      NULL,                     /* class_data */
      sizeof (GLifeTest2),
      0,                        /* n_preallocs */
      g_life_test2_init,
      NULL                      /* value_table */
    };

    type = g_type_register_static (G_TYPE_LIFE_TEST1, "GLifeTest2", &info, 0);
  }
  return type;
}

/* test driver */

int
main (int argc, char **argv)
{
  GObject *obj1, *obj2, *obj3;

  g_type_init ();

  puts ("= Object #1-1 new =");
  obj1 = g_object_new (G_TYPE_LIFE_TEST1, NULL);

  puts ("= Object #1-2 new  =");
  obj2 = g_object_new (G_TYPE_LIFE_TEST1, NULL);

  puts ("= Object #2-1 new  =");
  obj3 = g_object_new (G_TYPE_LIFE_TEST2, NULL);

  puts ("= Object #2-1 unref  =");
  g_object_unref (obj3);

  puts ("= Object #1-2 unref  =");
  g_object_unref (obj2);

  puts ("= Object #1-1 unref =");
  g_object_unref (obj1);
  return 0;
}
