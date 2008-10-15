#ifndef __MY_LIST_STORE_H__
#define __MY_LIST_STORE_H__

#include <gtk/gtk.h>
#include "my-test-object.h"

G_BEGIN_DECLS

#define MY_TYPE_LIST_STORE	(my_list_store_get_type ())
#define MY_LIST_STORE(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), MY_TYPE_LIST_STORE, MyListStore))
#define MY_LIST_STORE_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), MY_TYPE_LIST_STORE, MyListStoreClass))
#define MY_IS_LIST_STORE(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MY_TYPE_LIST_STORE))
#define MY_IS_LIST_STORE_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), MY_TYPE_LIST_STORE))
#define MY_LIST_STORE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), MY_TYPE_LIST_STORE, MyListStoreClass))

typedef struct _MyListStore MyListStore;
typedef struct _MyListStoreClass MyListStoreClass;
typedef enum _MyListStoreColumn MyListStoreColumn;

struct _MyListStore
{
	GtkListStore parent;
};

struct _MyListStoreClass
{
	GtkListStoreClass parent_class;
};

enum _MyListStoreColumn
{
	OBJECT_COLUMN,
	ID_COLUMN,
	N_COLUMNS
};

GType my_list_store_get_type (void);
void my_list_store_add (MyListStore *self, MyTestObject *obj, GtkTreeIter *iter);

G_END_DECLS

#endif
