#include <gtk/gtk.h>

#include "my-test-object.h"
#include "my-list-store.h"

#define MY_TEST_OBJECT_GET_PRIVATE(obj)    (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MY_TYPE_TEST_OBJECT, MyTestObjectPrivate))

G_DEFINE_TYPE (MyTestObject, my_test_object, G_TYPE_OBJECT);

/* count how many of these objects have been created */
static int my_test_object_counter = 0;

/* private data for this object */
typedef struct _MyTestObjectPrivate MyTestObjectPrivate;
struct _MyTestObjectPrivate
{
	int id;
};

/* this method is called when the object is finalized (being destroyed) */
static void
my_test_object_finalize (GObject *self)
{
	g_print ("finalizing object %p\n", self);

	/* call our parent method (always do this!) */
	G_OBJECT_CLASS (my_test_object_parent_class)->finalize (self);
}

/* this method is called once to set up the class */
static void
my_test_object_class_init (MyTestObjectClass *class)
{
	GObjectClass *gobject_class;

	gobject_class = G_OBJECT_CLASS (class);
	gobject_class->finalize = my_test_object_finalize;

	g_type_class_add_private (gobject_class, sizeof (MyTestObjectPrivate));
}

/* this method is called every time an instance of the class is created */
static void
my_test_object_init (MyTestObject *self)
{
	MyTestObjectPrivate *priv = MY_TEST_OBJECT_GET_PRIVATE (self);

	/* assign a unique number to this object as its name */
	priv->id = my_test_object_counter++;
}

/**
 * my_test_object_get_id:
 * @self: the #MyTestObject
 *
 * Returns the object's internal id
 *
 * Returns: the id
 */
int
my_test_object_get_id (MyTestObject *self)
{
	MyTestObjectPrivate *priv;

	/* validate our parameters */
	g_return_val_if_fail (MY_IS_TEST_OBJECT (self), -1);

	priv = MY_TEST_OBJECT_GET_PRIVATE (self);

	return priv->id;
}

/* set up a test */
int main (int argc, char **argv)
{
	MyListStore *store;
	MyTestObject *obj;
	GtkTreeIter iter;
	int i;

	gtk_init (&argc, &argv);

	/* create a list store */
	store = g_object_new (MY_TYPE_LIST_STORE, NULL);

	/* create some objects */
	for (i = 0; i < 5; i++)
	{
		obj = g_object_new (MY_TYPE_TEST_OBJECT, NULL);
		g_print ("adding object = %p\n", obj);
		my_list_store_add (store, obj, NULL);
		g_object_unref (obj); /* release our own ref */
	}

	/* print out the objects */
	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter)) do
	{
		int id;

		gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
				OBJECT_COLUMN, &obj, ID_COLUMN, &id, -1);
		
		g_print ("got object %i = %p\n", id, obj);

		g_object_unref (obj);
	}
	while (gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter));

	/* clean up -- destroy the store */
	g_object_unref (store);
}
