// $Id: persistence.c,v 1.1 2006-02-24 19:51:49 ensonic Exp $
/**
 * SECTION:btpersistence
 * @short_description: object persistence interface
 *
 */ 
 
#define BT_CORE
#define BT_PERSISTENCE_C

#include <libbtcore/core.h>

gboolean bt_persistence_save(BtPersistence *self, xmlDocPtr doc, xmlNodePtr parent_node, BtPersistenceSelection *selection) {
  g_return_val_if_fail (BT_IS_PERSISTENCE (self), FALSE);
  
  return (BT_PERSISTENCE_GET_INTERFACE (self)->save (self, doc, parent_node, selection));
}

gboolean bt_persistence_load(BtPersistence *self, xmlDocPtr doc, xmlNodePtr parent_node, BtPersistenceLocation *location) {
  g_return_val_if_fail (BT_IS_PERSISTENCE (self), FALSE);
  
  return (BT_PERSISTENCE_GET_INTERFACE (self)->load (self, doc, parent_node, location));
}

static void bt_persistence_base_init(gpointer g_class)
{
  static gboolean initialized = FALSE;

  if (!initialized) {
    /* create interface signals and properties here. */
    initialized = TRUE;
  }
}

GType
bt_persistence_get_type (void)
{
  static GType type = 0;
  
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (BtPersistenceInterface),
      bt_persistence_base_init,   /* base_init */
      NULL,   /* base_finalize */
      NULL,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      0,
      0,      /* n_preallocs */
      NULL    /* instance_init */
    };
    type = g_type_register_static (G_TYPE_INTERFACE,"BtPersistence",&info,0);
  }
  return type;
}
