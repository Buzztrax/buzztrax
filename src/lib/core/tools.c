/* $Id: tools.c,v 1.7 2004-09-22 16:05:11 ensonic Exp $
 */
 
#define BT_CORE
#define BT_TOOLS_C
#include <libbtcore/core.h>

/**
 * bt_g_object_get_object_property:
 * @object: the object instance
 * @property_name: the property name
 *
 * Fetches the named property from the given object and returns it as a GObject*.
 * Do not forget to call g_object_unref() on the returned object.
 *
 * Returns: the GObject value or NULL
 */
GObject* bt_g_object_get_object_property(GObject *object, const gchar *property_name) {
  static GObject *res;
  
  g_assert(object);
  g_assert(property_name);
  
  g_object_get(object,property_name,&res,NULL);
  return(res);
}

/**
 * bt_g_object_get_string_property:
 * @object: the object instance
 * @property_name: the property name
 *
 * Fetches the named property from the given object and returns it as a gchar*.
 * Do not forget to call g_free() on the returned string.
 *
 * Returns: the string value or NULL
 */
gchar* bt_g_object_get_string_property(GObject *object, const gchar *property_name) {
  static gchar *res;
  
  g_assert(object);
  g_assert(property_name);

  g_object_get(object,property_name,&res,NULL);
  return(res);
}

/**
 * bt_g_object_get_long_property:
 * @object: the object instance
 * @property_name: the property name
 *
 * Fetches the named property from the given object and returns it as a glong.
 *
 * Returns: the glong value
 */
glong bt_g_object_get_long_property(GObject *object, const gchar *property_name) {
  static glong res;
  
  g_assert(object);
  g_assert(property_name);

  g_object_get(object,property_name,&res,NULL);
  return(res);
}


/**
 * bt_g_object_set_object_property:
 * @object: the object instance
 * @property_name: the property name
 * @data: the new GObject value
 *
 * Stores the new object value for the named property of the given object
 *
 */
void bt_g_object_set_object_property(GObject *object, const gchar *property_name, GObject *data) {

  g_assert(object);
  g_assert(property_name);

  g_object_set(object,property_name,data,NULL);
}

/**
 * bt_g_object_set_string_property:
 * @object: the object instance
 * @property_name: the property name
 * @data: the new gchar* value
 *
 * Stores the new string value for the named property of the given object
 *
 */
void bt_g_object_set_string_property(GObject *object, const gchar *property_name, const gchar *data) {

  g_assert(object);
  g_assert(property_name);

  g_object_set(object,property_name,data,NULL);
}

/**
 * bt_g_object_set_long_property:
 * @object: the object instance
 * @property_name: the property name
 * @data: the new glong value
 *
 * Stores the new long value for the named property of the given object
 *
 */
void bt_g_object_set_long_property(GObject *object, const gchar *property_name, glong data) {

  g_assert(object);
  g_assert(property_name);

  g_object_set(object,property_name,data,NULL);
}

