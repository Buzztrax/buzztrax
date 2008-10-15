#include <gtk/gtk.h>

#include "my-list-store.h"
#include "my-test-object.h"

static void my_list_store_tree_model_iface_init (GtkTreeModelIface *iface);
static int my_list_store_get_n_columns (GtkTreeModel *self);
static GType my_list_store_get_column_type (GtkTreeModel *self, int column);
static void my_list_store_get_value (GtkTreeModel *self, GtkTreeIter *iter,
		int column, GValue *value);

/* MyListStore inherits from GtkListStore, and implements the GtkTreeStore
 * interface */
G_DEFINE_TYPE_EXTENDED (MyListStore, my_list_store, GTK_TYPE_LIST_STORE, 0,
		G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
			my_list_store_tree_model_iface_init));

/* our parent's model iface */
static GtkTreeModelIface parent_iface = { 0, };

/* this method is called once to set up the class */
static void
my_list_store_class_init (MyListStoreClass *class)
{
}

/* this method is called once to set up the interface */
static void
my_list_store_tree_model_iface_init (GtkTreeModelIface *iface)
{
	/* this is where we override the interface methods */
	/* first make a copy of our parent's interface to call later */
	parent_iface = *iface;

	/* now put in our own overriding methods */
	iface->get_n_columns = my_list_store_get_n_columns;
	iface->get_column_type = my_list_store_get_column_type;
	iface->get_value = my_list_store_get_value;
}

/* this method is called every time an instance of the class is created */
static void
my_list_store_init (MyListStore *self)
{
	/* initialise the underlying storage for the GtkListStore */
	GType types[] = { MY_TYPE_TEST_OBJECT };

	gtk_list_store_set_column_types (GTK_LIST_STORE (self), 1, types);
}

/**
 * my_list_store_add:
 * @self: the #MyListStore
 * @obj: a #MyTestObject to add to the list
 * @iter: a pointer to a #GtkTreeIter for the row (or NULL)
 *
 * Appends @obj to the list.
 */
void
my_list_store_add (MyListStore *self, MyTestObject *obj, GtkTreeIter *iter)
{
	GtkTreeIter iter1;

	/* validate our parameters */
	g_return_if_fail (MY_IS_LIST_STORE (self));
	g_return_if_fail (MY_IS_TEST_OBJECT (obj));

	/* put this object in our data storage */
	gtk_list_store_append (GTK_LIST_STORE (self), &iter1);
	gtk_list_store_set (GTK_LIST_STORE (self), &iter1, 0, obj, -1);

	/* here you would connect up signals (e.g. ::notify) to notice when your
	 * object has changed */

	/* return the iter if the user cares */
	if (iter) *iter = iter1;
}

/* retreive an object from our parent's data storage,
 * unref the returned object when done */
static MyTestObject *
my_list_store_get_object (MyListStore *self, GtkTreeIter *iter)
{
	GValue value = { 0, };
	MyTestObject *obj;

	/* validate our parameters */
	g_return_val_if_fail (MY_IS_LIST_STORE (self), NULL);
	g_return_val_if_fail (iter != NULL, NULL);

	/* retreive the object using our parent's interface, take our own
	 * reference to it */
	parent_iface.get_value (GTK_TREE_MODEL (self), iter, 0, &value);
	obj = MY_TEST_OBJECT (g_value_dup_object (&value));

	g_value_unset (&value);

	return obj;
}

/* this method returns the number of columns in our tree model */
static int
my_list_store_get_n_columns (GtkTreeModel *self)
{
	return N_COLUMNS;
}

/* this method returns the type of each column in our tree model */
static GType
my_list_store_get_column_type (GtkTreeModel *self, int column)
{
	GType types[] = {
		MY_TYPE_TEST_OBJECT,
		G_TYPE_INT,
	};

	/* validate our parameters */
	g_return_val_if_fail (MY_IS_LIST_STORE (self), G_TYPE_INVALID);
	g_return_val_if_fail (column >= 0 && column < N_COLUMNS, G_TYPE_INVALID);

	return types[column];
}

/* this method retrieves the value for a particular column */
static void
my_list_store_get_value (GtkTreeModel *self, GtkTreeIter *iter, int column,
		GValue *value)
{
	MyTestObject *obj;

	/* validate our parameters */
	g_return_if_fail (MY_IS_LIST_STORE (self));
	g_return_if_fail (iter != NULL);
	g_return_if_fail (column >= 0 && column < N_COLUMNS);
	g_return_if_fail (value != NULL);

	/* get the object from our parent's storage */
	obj = my_list_store_get_object (MY_LIST_STORE (self), iter);

	/* initialise our GValue to the required type */
	g_value_init (value,
		my_list_store_get_column_type (GTK_TREE_MODEL (self), column));

	switch (column)
	{
		case OBJECT_COLUMN:
			/* the object itself was requested */
			g_value_set_object (value, obj);
			break;

		case ID_COLUMN:
			/* the objects id was requested */
			g_value_set_int (value, my_test_object_get_id (obj));
			break;

		default:
			g_assert_not_reached ();
	}

	/* release the reference gained from my_list_store_get_object() */
	g_object_unref (obj);
}
