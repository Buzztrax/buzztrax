/* $Id: tools.c,v 1.1 2004-07-12 16:38:49 ensonic Exp $
 */
 
#define BT_CORE
#define BT_TOOLS_C
#include <libbtcore/core.h>

/**
 * bt_g_object_get_object_property:
 * @object: the object instance
 * @property_name: the property name
 *
 * Fetches the names property from the given object and returns it as a GObject*.
 *
 * Returns: the GObject value or NULL
 */
GObject* bt_g_object_get_object_property(GObject *object,gchar *property_name) {
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
 * @obj: the object instance
 * @property_name: the property name
 *
 * Fetches the names property from the given object and returns it as a gchar*.
 *
 * Returns: the string value or NULL
 */
gchar* bt_g_object_get_string_property(GObject *object,gchar *property_name) {
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
 * @obj: the object instance
 * @property_name: the property name
 *
 * Fetches the names property from the given object and returns it as a glong.
 *
 * Returns: the long value
 */
glong bt_g_object_get_long_property(GObject *object,gchar *property_name) {
  static GValue *val=NULL;
  
  if(val==NULL) {
    val=g_new0(GValue,1);
    g_value_init(val,G_TYPE_LONG);
  }
  g_object_get_property(G_OBJECT(object),property_name,val);
  return(g_value_get_long(val));
}

