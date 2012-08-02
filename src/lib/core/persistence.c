/* Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
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

//-- the iface

G_DEFINE_INTERFACE (BtPersistence, bt_persistence, 0);


//-- string formatting helper

/**
 * bt_persistence_strfmt_uchar:
 * @val: a value
 *
 * Convenience methods, that formats a value to be serialized as a string.
 *
 * Returns: a reference to static memory containing the formatted value.
 */
const gchar *
bt_persistence_strfmt_uchar (const guchar val)
{
  static gchar str[20];

  g_sprintf (str, "%u", val);
  return (str);
}

/**
 * bt_persistence_strfmt_long:
 * @val: a value
 *
 * Convenience methods, that formats a value to be serialized as a string.
 *
 * Returns: a reference to static memory containing the formatted value.
 */
const gchar *
bt_persistence_strfmt_long (const glong val)
{
  static gchar str[20];

  g_sprintf (str, "%ld", val);
  return (str);
}

/**
 * bt_persistence_strfmt_ulong:
 * @val: a value
 *
 * Convenience methods, that formats a value to be serialized as a string.
 *
 * Returns: a reference to static memory containing the formatted value.
 */
const gchar *
bt_persistence_strfmt_ulong (const gulong val)
{
  static gchar str[20];

  g_sprintf (str, "%lu", val);
  return (str);
}

/**
 * bt_persistence_strfmt_double:
 * @val: a value
 *
 * Convenience methods, that formats a value to be serialized as a string.
 *
 * Returns: a reference to static memory containing the formatted value.
 */
const gchar *
bt_persistence_strfmt_double (const gdouble val)
{
  static gchar str[G_ASCII_DTOSTR_BUF_SIZE + 1];

  g_ascii_dtostr (str, G_ASCII_DTOSTR_BUF_SIZE, val);
  return (str);
}

// string parsing helper

/**
 * bt_persistence_strfmt_enum:
 * @enum_type: the #GType for the enum
 * @value: the integer value for the enum
 *
 * Convenience methods, that formats a value to be serialized as a string.
 *
 * Returns: a reference to static memory containing the formatted value.
 */
const gchar *
bt_persistence_strfmt_enum (GType enum_type, gint value)
{
  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);

  GEnumClass *enum_class = g_type_class_ref (enum_type);
  GEnumValue *enum_value = g_enum_get_value (enum_class, value);
  g_type_class_unref (enum_class);
  return (enum_value ? enum_value->value_nick : NULL);
}

/**
 * bt_persistence_parse_enum:
 * @enum_type: the #GType for the enum
 * @str: the enum value name
 *
 * Convenience methods, that parses a enum value nick and to get the
 * corresponding integer value.
 *
 * Returns: the integer value for the enum, or -1 for invalid strings.
 */
gint
bt_persistence_parse_enum (GType enum_type, const gchar * str)
{
  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), -1);

  if (!str)
    return (-1);

  GEnumClass *enum_class = g_type_class_ref (enum_type);
  GEnumValue *enum_value = g_enum_get_value_by_nick (enum_class, str);
  g_type_class_unref (enum_class);
  return (enum_value ? enum_value->value : -1);
}

//-- list helper

/**
 * bt_persistence_save_list:
 * @list: a #GList
 * @doc; the xml-document
 * @node: the list xml node
 *
 * Iterates over a list of objects, which must implement the #BtPersistence
 * interface and calls bt_persistence_save() on each item.
 *
 * Returns: %TRUE if all elements have been serialized.
 */
gboolean
bt_persistence_save_list (const GList * list, xmlNodePtr const node)
{
  gboolean res = TRUE;

  for (; (list && res); list = g_list_next (list)) {
    if (BT_IS_PERSISTENCE (list->data)) {
      res &=
          (bt_persistence_save ((BtPersistence *) (list->data), node) != NULL);
    }
  }
  return (res);
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
 * @hashtable: a #GHashTable with strings
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

  return (res);
}

/**
 * bt_persistence_load_hashtable:
 * @hashtable: a #GHashTable
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
  return (TRUE);
}

//-- gvalue helper

/**
 * bt_persistence_set_value:
 * @gvalue: a #GValue
 * @svalue: the string representation of the value to store
 *
 * Stores the supplied value into the given @gvalue.
 *
 * Returns: %TRUE for success
 */
gboolean
bt_persistence_set_value (GValue * const gvalue, const gchar * svalue)
{
  gboolean res = TRUE;
  GType value_type, base_type;

  g_return_val_if_fail (G_IS_VALUE (gvalue), FALSE);

  value_type = G_VALUE_TYPE (gvalue);
  base_type = bt_g_type_get_base_type (value_type);
  // depending on the type, set the GValue
  switch (base_type) {
    case G_TYPE_DOUBLE:{
      //gdouble val=atof(svalue); // this is dependend on the locale
      const gdouble val = svalue ? g_ascii_strtod (svalue, NULL) : 0.0;
      g_value_set_double (gvalue, val);
    } break;
    case G_TYPE_FLOAT:{
      const gfloat val = svalue ? (gfloat) g_ascii_strtod (svalue, NULL) : 0.0;
      g_value_set_float (gvalue, val);
    } break;
    case G_TYPE_BOOLEAN:{
      const gboolean val = svalue ? atoi (svalue) : FALSE;
      g_value_set_boolean (gvalue, val);
    } break;
    case G_TYPE_STRING:{
      g_value_set_string (gvalue, svalue);
    }
      break;
    case G_TYPE_ENUM:{
      if (value_type == GSTBT_TYPE_NOTE) {
        GEnumClass *enum_class = g_type_class_peek_static (GSTBT_TYPE_NOTE);
        GEnumValue *enum_value;

        if ((enum_value = g_enum_get_value_by_nick (enum_class, svalue))) {
          //GST_INFO("mapping '%s' -> %d", svalue, enum_value->value);
          g_value_set_enum (gvalue, enum_value->value);
        } else {
          GST_INFO ("-> %s out of range", svalue);
          res = FALSE;
        }
      } else {
        const gint val = svalue ? atoi (svalue) : 0;
        GEnumClass *enum_class = g_type_class_peek_static (value_type);
        GEnumValue *enum_value = g_enum_get_value (enum_class, val);

        if (enum_value) {
          //GST_INFO("-> %d",val);
          g_value_set_enum (gvalue, val);
        } else {
          GST_INFO ("-> %d out of range", val);
          res = FALSE;
        }
      }
    }
      break;
    case G_TYPE_INT:{
      const gint val = svalue ? atoi (svalue) : 0;
      g_value_set_int (gvalue, val);
    } break;
    case G_TYPE_UINT:{
      const guint val = svalue ? atoi (svalue) : 0;
      g_value_set_uint (gvalue, val);
    } break;
    case G_TYPE_INT64:{
      const gint64 val = svalue ? g_ascii_strtoll (svalue, NULL, 10) : 0;
      g_value_set_int64 (gvalue, val);
    } break;
    case G_TYPE_UINT64:{
      const guint64 val = svalue ? g_ascii_strtoull (svalue, NULL, 10) : 0;
      g_value_set_uint64 (gvalue, val);
    } break;
    case G_TYPE_LONG:{
      const glong val = svalue ? atol (svalue) : 0;
      g_value_set_long (gvalue, val);
    } break;
    case G_TYPE_ULONG:{
      const gulong val = svalue ? atol (svalue) : 0;
      g_value_set_ulong (gvalue, val);
    } break;
    default:
      GST_ERROR ("unsupported GType=%lu:'%s' for value=\"%s\"",
          (gulong) G_VALUE_TYPE (gvalue), G_VALUE_TYPE_NAME (gvalue), svalue);
      return (FALSE);
  }
  return (res);
}

/**
 * bt_persistence_get_value:
 * @gvalue: the event cell
 *
 * Returns the string representation of the given @gvalue. Free it when done.
 *
 * Returns: a newly allocated string with the data or %NULL on error
 */
gchar *
bt_persistence_get_value (GValue * const gvalue)
{
  GType base_type;
  gchar *res = NULL;

  g_return_val_if_fail (G_IS_VALUE (gvalue), NULL);

  base_type = bt_g_type_get_base_type (G_VALUE_TYPE (gvalue));
  // depending on the type, set the result
  switch (base_type) {
    case G_TYPE_DOUBLE:{
      gchar str[G_ASCII_DTOSTR_BUF_SIZE + 1];
      // this is dependend on the locale
      //res=g_strdup_printf("%lf",g_value_get_double(gvalue));
      res =
          g_strdup (g_ascii_dtostr (str, G_ASCII_DTOSTR_BUF_SIZE,
              g_value_get_double (gvalue)));
    }
      break;
    case G_TYPE_FLOAT:{
      gchar str[G_ASCII_DTOSTR_BUF_SIZE + 1];
      // this is dependend on the locale
      //res=g_strdup_printf("%f",g_value_get_float(gvalue));
      res =
          g_strdup (g_ascii_dtostr (str, G_ASCII_DTOSTR_BUF_SIZE,
              g_value_get_float (gvalue)));
    }
      break;
    case G_TYPE_BOOLEAN:
      res = g_strdup_printf ("%d", g_value_get_boolean (gvalue));
      break;
    case G_TYPE_STRING:
      res = g_value_dup_string (gvalue);
      break;
    case G_TYPE_ENUM:
      if (G_VALUE_TYPE (gvalue) == GSTBT_TYPE_NOTE) {
        GEnumClass *enum_class = g_type_class_peek_static (GSTBT_TYPE_NOTE);
        GEnumValue *enum_value;
        gint val = g_value_get_enum (gvalue);

        if ((enum_value = g_enum_get_value (enum_class, val))) {
          res = g_strdup (enum_value->value_nick);
        } else {
          res = g_strdup ("");
        }
      } else {
        res = g_strdup_printf ("%d", g_value_get_enum (gvalue));
      }
      break;
    case G_TYPE_INT:
      res = g_strdup_printf ("%d", g_value_get_int (gvalue));
      break;
    case G_TYPE_UINT:
      res = g_strdup_printf ("%u", g_value_get_uint (gvalue));
      break;
    case G_TYPE_INT64:
      res = g_strdup_printf ("%" G_GINT64_FORMAT, g_value_get_int64 (gvalue));
      break;
    case G_TYPE_UINT64:
      res = g_strdup_printf ("%" G_GUINT64_FORMAT, g_value_get_uint64 (gvalue));
      break;
    case G_TYPE_LONG:
      res = g_strdup_printf ("%ld", g_value_get_long (gvalue));
      break;
    case G_TYPE_ULONG:
      res = g_strdup_printf ("%lu", g_value_get_ulong (gvalue));
      break;
    default:
      GST_ERROR ("unsupported GType=%lu:'%s'", (gulong) G_VALUE_TYPE (gvalue),
          G_VALUE_TYPE_NAME (gvalue));
      return (NULL);
  }
  return (res);
}


//-- wrapper

/**
 * bt_persistence_save:
 * @self: a serialiable object
 * @parent_node: the parent xml node
 *
 * Serializes the given object into @node.
 *
 * Returns: the new node if the object has been serialized, else %NULL.
 */
xmlNodePtr
bt_persistence_save (const BtPersistence * const self,
    xmlNodePtr const parent_node)
{
  g_return_val_if_fail (BT_IS_PERSISTENCE (self), FALSE);

  return (BT_PERSISTENCE_GET_INTERFACE (self)->save (self, parent_node));
}

/**
 * bt_persistence_load:
 * @type: a #GObject type
 * @self: a deserialiable object
 * @node: the xml node
 * @err: a GError for deserialisation errors
 * @...: extra parameters NULL terminated name/value pairs.
 *
 * Deserializes the given object from the @node. If @self is NULL and a @type is
 * given it constructs a new object.
 *
 * Returns: the deserialized object or %NULL.
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
  return (result);
}

//-- interface internals

static void
bt_persistence_default_init (BtPersistenceInterface * klass)
{
}
