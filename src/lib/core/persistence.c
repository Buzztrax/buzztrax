// $Id: persistence.c,v 1.3 2006-03-01 16:47:08 ensonic Exp $
/**
 * SECTION:btpersistence
 * @short_description: object persistence interface
 *
 * Classes can implement this interface to make store their data as xml and
 * restore them from xml.
 *
 */ 
 
#define BT_CORE
#define BT_PERSISTENCE_C

#include <libbtcore/core.h>

//-- string formatting helper

/**
 * bt_persistence_strfmt_long:
 * @val: a value
 *
 * Convinience methods, that formats a value to be serialized as a string.
 *
 * Return: a reference to static memory containg the formatted value.
 */
const gchar *bt_persistence_strfmt_long(glong val) {
  static gchar str[20];

  g_sprintf(str,"%ld",val);
  return(str);
}

/**
 * bt_persistence_strfmt_ulong:
 * @val: a value
 *
 * Convinience methods, that formats a value to be serialized as a string.
 *
 * Return: a reference to static memory containg the formatted value.
 */
const gchar *bt_persistence_strfmt_ulong(gulong val) {
  static gchar str[20];

  g_sprintf(str,"%lu",val);
  return(str);
}

//-- list helper

/**
 * bt_persistence_save_list:
 * @list: a list
 * @doc; the xml-document
 * @parent_node: the parent xml node
 *
 * Iterates over a list of objects, which must implement the #BtPersistece
 * interface and calls bt_persistence_save() on each item.
 *
 * Return: %TRUE if all elements have been serialized.
 */
gboolean bt_persistence_save_list(const GList *list,xmlDocPtr doc, xmlNodePtr parent_node) {
  gboolean res=TRUE;

  for(;(list && res);list=g_list_next(list)) {
    res&=bt_persistence_save(BT_PERSISTENCE(list->data),doc,parent_node,NULL);
  }
  return(res);
}

//-- wrapper

/**
 * bt_persistence_save:
 * @self: a serialiable object
 * @doc; the xml-document
 * @parent_node: the parent xml node
 * @selection: an optional selection
 *
 * Serializes the given object as a children of @parent_node.
 *
 * Return: %TRUE if the element has been serialized.
 */
gboolean bt_persistence_save(BtPersistence *self, xmlDocPtr doc, xmlNodePtr parent_node, BtPersistenceSelection *selection) {
  g_return_val_if_fail (BT_IS_PERSISTENCE (self), FALSE);
  
  return (BT_PERSISTENCE_GET_INTERFACE (self)->save (self, doc, parent_node, selection));
}

/**
 * bt_persistence_load:
 * @self: a deserialiable object
 * @doc; the xml-document
 * @node: the xml node
 * @selection: an optional selection
 *
 * Deserializes the given object from the @node.
 *
 * Return: %TRUE if the element has been deserialized.
 */
gboolean bt_persistence_load(BtPersistence *self, xmlDocPtr doc, xmlNodePtr node, BtPersistenceLocation *location) {
  g_return_val_if_fail (BT_IS_PERSISTENCE (self), FALSE);
  
  return (BT_PERSISTENCE_GET_INTERFACE (self)->load (self, doc, node, location));
}

//-- interface internals

static void bt_persistence_base_init(gpointer g_class) {
  static gboolean initialized = FALSE;

  if (!initialized) {
    /* create interface signals and properties here. */
    initialized = TRUE;
  }
}

GType bt_persistence_get_type (void) {
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
