/* $Id: tools.c,v 1.4 2004-07-19 17:37:47 ensonic Exp $
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
 *
 * Returns: the GObject value or NULL
 */
GObject* bt_g_object_get_object_property(GObject *object, const gchar *property_name) {
  static GValue *val=NULL;
  
  if(val==NULL) {
    val=g_new0(GValue,1);
    g_value_init(val,G_TYPE_OBJECT);
  }
  g_object_get_property(G_OBJECT(object),property_name,val);
  return(g_value_get_object(val));
}

/**
 * bt_g_object_get_string_property:
 * @object: the object instance
 * @property_name: the property name
 *
 * Fetches the named property from the given object and returns it as a gchar*.
 *
 * Returns: the string value or NULL
 */
const gchar* bt_g_object_get_string_property(GObject *object, const gchar *property_name) {
  static GValue *val=NULL;
  
  if(val==NULL) {
    val=g_new0(GValue,1);
    g_value_init(val,G_TYPE_STRING);
  }
  g_object_get_property(G_OBJECT(object),property_name,val);
  return(g_value_get_string(val));
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
  static GValue *val=NULL;
  
  if(val==NULL) {
    val=g_new0(GValue,1);
    g_value_init(val,G_TYPE_LONG);
  }
  g_object_get_property(G_OBJECT(object),property_name,val);
  return(g_value_get_long(val));
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
  static GValue *val;
  
  if(val==NULL) {
    val=g_new0(GValue,1);
    g_value_init(val,G_TYPE_OBJECT);
  }
  g_value_set_object(val,data);
  g_object_set_property(G_OBJECT(object),property_name, val);
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
  static GValue *val;
  
  if(val==NULL) {
    val=g_new0(GValue,1);
    g_value_init(val,G_TYPE_STRING);
  }
  g_value_set_string(val,data);
  g_object_set_property(G_OBJECT(object),property_name, val);
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
  static GValue *val;
  
  if(val==NULL) {
    val=g_new0(GValue,1);
    g_value_init(val,G_TYPE_LONG);
  }
  g_value_set_long(val,data);
  g_object_set_property(G_OBJECT(object),property_name, val);
}

