/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
/**
 * SECTION:btpersistence
 * @short_description: object persistence interface
 *
 * Classes can implement this interface to store their data as xml and
 * restore them from xml. They should call the interface methods on their
 * children objects (which also implement the interface) to serialize/
 * deserialize a whole object hierarchy.
 *
 */
/* IDEA(ensonic): what if we pass an object instance (ctx) to bt_persistence_{load,save}
 * instead of the xmlNodePtr.
 * - loading
 *   const gchar *          bt_persistence_context_get_attribute (BtPersistenceContext *ctx, const gchar *name);
 *   GList *                bt_persistence_context_get_children (BtPersistenceContext *ctx);
 * - saving
 *                          bt_persistence_context_set_attribute (BtPersistenceContext *ctx, const gchar *name, const gchar *attribute);
 *   BtPersistenceContext * bt_persistence_context_add_child (BtPersistenceContext *ctx, const gchar *name);
 *
 * - need some parameters for constructor (e.g. uri)
 * - BtPersistenceContext would be an abstract baseclass, we would then implement
 *   BtPersistenceContextLibXML2
 *   - this way we have the libxml2 hidden, we then can support different backends (e.g. for clipboard)
 */

#define BT_CORE
#define BT_PERSISTENCE_C

#include "core_private.h"
#include <glib/gprintf.h>

//-- the iface

G_DEFINE_INTERFACE (BtPersistence, bt_persistence, 0);

//-- list helper

/**
 * bt_persistence_save_list:
 * @list: (element-type BuzztraxCore.Persistence): a #GList
 * @doc; the xml-document
 * @node: the list xml node
 *
 * Iterates over a list of objects, which must implement the #BtPersistence
 * interface and calls bt_persistence_save() on each item.
 *
 * Returns: %TRUE if all elements have been serialized.
 */
gboolean
bt_persistence_save_list (const GList * list, xmlNodePtr const node, gpointer const userdata)
{
  gboolean res = TRUE;

  for (; (list && res); list = g_list_next (list)) {
    if (BT_IS_PERSISTENCE (list->data)) {
      res &=
          (bt_persistence_save ((BtPersistence *) (list->data), node, userdata) != NULL);
    }
  }
  return res;
}

/*
 * this isn't as useful as the _save_list version
 *
gboolean bt_persistence_load_list(const GList *list,xmlNodePtr node,const gchar const *name) {
  gboolean res=TRUE;
  gulong name_len=strlen(name);

  for(node=node->children;node;node=node->next) {
    if((!xmlNodeIsText(node)) && (!strncmp((char *)node->name,name,strlen))) {
      ...
    }
  }
*/

//-- hashtable helper

/**
 * bt_persistence_collect_hashtable_entries:
 * @key: hashtable key
 * @value: hashtable value
 * @user_data: GList **list
 *
 * Gather #GHashTable entries in a list. Should be used with g_hash_table_foreach().
 */
void
bt_persistence_collect_hashtable_entries (gpointer const key,
    gpointer const value, gpointer const user_data)
{
  GList **list = (GList **) user_data;

  *list = g_list_prepend (*list, value);
}

static void
bt_persistence_save_hashtable_entries (gpointer const key, gpointer const value,
    gpointer const user_data)
{
  xmlNodePtr node;

  node = xmlNewChild (user_data, NULL, XML_CHAR_PTR ("property"), NULL);
  xmlNewProp (node, XML_CHAR_PTR ("key"), XML_CHAR_PTR (key));
  xmlNewProp (node, XML_CHAR_PTR ("value"), XML_CHAR_PTR (value));
}

/**
 * bt_persistence_save_hashtable:
 * @hashtable: (element-type utf8 utf8): a #GHashTable with strings
 * @node: the list xml node
 *
 * Iterates over a hashtable with strings and serializes them.
 *
 * Returns: %TRUE if all elements have been serialized.
 */
gboolean
bt_persistence_save_hashtable (GHashTable * hashtable, xmlNodePtr const node)
{
  gboolean res = TRUE;

  g_hash_table_foreach (hashtable, bt_persistence_save_hashtable_entries,
      (gpointer) node);

  return res;
}

/**
 * bt_persistence_load_hashtable:
 * @hashtable: (element-type utf8 utf8): a #GHashTable
 * @node: the list xml node
 *
 * Iterates over the xml-node and deserializes elements into the hashtable.
 *
 * Returns: %TRUE if all elements have been deserialized.
 */
gboolean
bt_persistence_load_hashtable (GHashTable * hashtable, xmlNodePtr node)
{
  xmlChar *key, *value;

  // iterate over children
  for (node = node->children; node; node = node->next) {
    if (!xmlNodeIsText (node)
        && !strncmp ((char *) node->name, "property\0", 9)) {
      key = xmlGetProp (node, XML_CHAR_PTR ("key"));
      value = xmlGetProp (node, XML_CHAR_PTR ("value"));
      //GST_DEBUG("    [%s] => [%s]",key,value);
      g_hash_table_insert (hashtable, key, value);
      // do not free, as the hastable now owns the memory
      //xmlFree(key);xmlFree(value);
    }
  }
  return TRUE;
}

//-- wrapper

/**
 * bt_persistence_save:
 * @self: a serialiable object
 * @parent_node: the parent xml node
 *
 * Serializes the given object into an xmlNode.
 *
 * Returns: (transfer full) (skip): the new node if the object has been serialized, else %NULL.
 */
xmlNodePtr
bt_persistence_save (const BtPersistence * const self,
    xmlNodePtr const parent_node, gpointer const userdata)
{
  g_return_val_if_fail (BT_IS_PERSISTENCE (self), FALSE);

  return BT_PERSISTENCE_GET_INTERFACE (self)->save (self, parent_node, userdata);
}

/**
 * bt_persistence_load:
 * @type: a #GObject type
 * @self: a deserialiable object
 * @node: the xml node
 * @err: a GError for deserialisation errors
 * @...: extra parameters NULL terminated name/value pairs.
 *
 * Deserializes the given object from the @node. If @self is %NULL and a @type
 * is given it constructs a new object.
 *
 * Returns: (transfer none): the deserialized object or %NULL.
 */
BtPersistence *
bt_persistence_load (const GType type, const BtPersistence * const self,
    xmlNodePtr node, GError ** err, ...)
{
  BtPersistence *result;
  va_list var_args;

  if (!self) {
    GObjectClass *klass;
    BtPersistenceInterface *iface;

    g_return_val_if_fail (G_TYPE_IS_OBJECT (type), NULL);

    klass = g_type_class_ref (type);
    iface = g_type_interface_peek (klass, BT_TYPE_PERSISTENCE);
    va_start (var_args, err);
    result = iface->load (type, self, node, err, var_args);
    va_end (var_args);
    g_type_class_unref (klass);
  } else {
    g_return_val_if_fail (BT_IS_PERSISTENCE (self), NULL);

    va_start (var_args, err);
    result =
        BT_PERSISTENCE_GET_INTERFACE (self)->load (type, self, node, err,
        var_args);
    va_end (var_args);
  }
  return result;
}

//-- interface internals

static void
bt_persistence_default_init (BtPersistenceInterface * klass)
{
}
