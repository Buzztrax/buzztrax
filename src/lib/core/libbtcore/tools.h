/* $Id: tools.h,v 1.2 2004-07-13 16:52:11 ensonic Exp $
 */

#ifndef BT_TOOLS_H
#define BT_TOOLS_H

#ifndef BT_TOOLS_C
	extern GObject* bt_g_object_get_object_property(GObject *object,gchar *property_name);
  extern const gchar* bt_g_object_get_string_property(GObject *object,gchar *property_name);
  extern glong bt_g_object_get_long_property(GObject *object,gchar *property_name);
#endif

#endif // BT_TOOLS_H

