#ifndef __MY_TEST_OBJECT_H__
#define __MY_TEST_OBJECT_H__

#include <glib.h>

G_BEGIN_DECLS

#define MY_TYPE_TEST_OBJECT	(my_test_object_get_type ())
#define MY_TEST_OBJECT(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), MY_TYPE_TEST_OBJECT, MyTestObject))
#define MY_TEST_OBJECT_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), MY_TYPE_TEST_OBJECT, MyTestObjectClass))
#define MY_IS_TEST_OBJECT(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MY_TYPE_TEST_OBJECT))
#define MY_IS_TEST_OBJECT_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), MY_TYPE_TEST_OBJECT))
#define MY_TEST_OBJECT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), MY_TYPE_TEST_OBJECT, MyTestObjectClass))

typedef struct _MyTestObject MyTestObject;
typedef struct _MyTestObjectClass MyTestObjectClass;

struct _MyTestObject
{
	GObject parent;
};

struct _MyTestObjectClass
{
	GObjectClass parent_class;
};

GType my_test_object_get_type (void);
int my_test_object_get_id (MyTestObject *self);

G_END_DECLS

#endif
