/* $Id: tools.h,v 1.3 2004-07-19 17:37:47 ensonic Exp $
 */

#ifndef BT_TOOLS_H
#define BT_TOOLS_H

#ifndef BT_TOOLS_C
	extern GObject* bt_g_object_get_object_property(GObject *object, const gchar *property_name);
  extern const gchar* bt_g_object_get_string_property(GObject *object, const gchar *property_name);
  extern glong bt_g_object_get_long_property(GObject *object, const gchar *property_name);

	extern void bt_g_object_set_object_property(GObject *object, const gchar *property_name, GObject *data);
  extern void bt_g_object_set_string_property(GObject *object, const gchar *property_name, const gchar *data);
  extern void bt_g_object_set_long_property(GObject *object, const gchar *property_name, glong data);
#endif

#endif // BT_TOOLS_H

