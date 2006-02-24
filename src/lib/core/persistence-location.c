// $Id: persistence-location.c,v 1.1 2006-02-24 19:51:49 ensonic Exp $
/**
 * SECTION:btpersistence-location
 * @short_description: base-class for object deserialisation filters
 *
 */

#define BT_CORE
#define BT_PERSISTENCE_LOCATION_C

#include <libbtcore/core.h>

GType bt_persistence_location_get_type(void) {
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      G_STRUCT_SIZE(BtPersistenceLocationClass),
      NULL, // base_init
      NULL, // base_finalize
      NULL, // class_init
      NULL, // class_finalize
      NULL, // class_data
      G_STRUCT_SIZE(BtPersistenceLocation),
      0,   // n_preallocs
      NULL, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(G_TYPE_OBJECT,"BtPersistenceLocation",&info,G_TYPE_FLAG_ABSTRACT);
  }
  return type;
}
