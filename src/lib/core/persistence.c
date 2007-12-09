/* $Id: persistence.c,v 1.20 2007-07-05 21:07:35 ensonic Exp $
 *
 * Buzztard
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

#define BT_CORE
#define BT_PERSISTENCE_C

#include <libbtcore/core.h>

//-- string formatting helper

/**
 * bt_persistence_strfmt_uchar:
 * @val: a value
 *
 * Convinience methods, that formats a value to be serialized as a string.
 *
 * Returns: a reference to static memory containg the formatted value.
 */
const gchar *bt_persistence_strfmt_uchar(const guchar val) {
  static gchar str[20];

  g_sprintf(str,"%u",val);
  return(str);
}

/**
 * bt_persistence_strfmt_long:
 * @val: a value
 *
 * Convinience methods, that formats a value to be serialized as a string.
 *
 * Returns: a reference to static memory containg the formatted value.
 */
const gchar *bt_persistence_strfmt_long(const glong val) {
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
 * Returns: a reference to static memory containg the formatted value.
 */
const gchar *bt_persistence_strfmt_ulong(const gulong val) {
  static gchar str[20];

  g_sprintf(str,"%lu",val);
  return(str);
}

/**
 * bt_persistence_strfmt_double:
 * @val: a value
 *
 * Convinience methods, that formats a value to be serialized as a string.
 *
 * Returns: a reference to static memory containg the formatted value.
 */
const gchar *bt_persistence_strfmt_double(const gdouble val) {
  static gchar str[G_ASCII_DTOSTR_BUF_SIZE+1];

  g_ascii_dtostr(str,G_ASCII_DTOSTR_BUF_SIZE,val);
  return(str);
}

//-- list helper

/**
 * bt_persistence_save_list:
 * @list: a #GList
 * @doc; the xml-document
 * @node: the list xml node
 *
 * Iterates over a list of objects, which must implement the #BtPersistece
 * interface and calls bt_persistence_save() on each item.
 *
 * Returns: %TRUE if all elements have been serialized.
 */
gboolean bt_persistence_save_list(const GList *list,xmlNodePtr const node) {
  gboolean res=TRUE;

  for(;(list && res);list=g_list_next(list)) {
    res&=(bt_persistence_save(BT_PERSISTENCE(list->data),node,NULL)!=NULL);
  }
  return(res);
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

static void bt_persistence_save_hashtable_entries(gpointer const key, gpointer const value, gpointer const user_data) {
  xmlNodePtr node;

  node=xmlNewChild(user_data,NULL,XML_CHAR_PTR("property"),NULL);
  xmlNewProp(node,XML_CHAR_PTR("key"),XML_CHAR_PTR(key));
  xmlNewProp(node,XML_CHAR_PTR("value"),XML_CHAR_PTR(value));
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
gboolean bt_persistence_save_hashtable(GHashTable *hashtable, xmlNodePtr const node) {
  gboolean res=TRUE;

  g_hash_table_foreach(hashtable,bt_persistence_save_hashtable_entries,(gpointer)node);

  return(res);
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
gboolean bt_persistence_load_hashtable(GHashTable *hashtable, xmlNodePtr node) {
  xmlChar *key,*value;

  // iterate over children
  for(node=node->children;node;node=node->next) {
    if(!xmlNodeIsText(node) && !strncmp((char *)node->name,"property\0",9)) {
      key=xmlGetProp(node,XML_CHAR_PTR("key"));
      value=xmlGetProp(node,XML_CHAR_PTR("value"));
      //GST_DEBUG("    [%s] => [%s]",key,value);
      g_hash_table_insert(hashtable,key,value);
      // do not free, as the hastable now owns the memory
      //xmlFree(key);xmlFree(value);
    }
  }
  return(TRUE);
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
gboolean bt_persistence_set_value(GValue* const gvalue, const gchar *svalue) {
  GType base_type;

  g_return_val_if_fail(G_IS_VALUE(gvalue),FALSE);

  base_type=bt_g_type_get_base_type(G_VALUE_TYPE(gvalue));
  // depending on the type, set the GValue
  switch(base_type) {
    case G_TYPE_DOUBLE: {
      //gdouble val=atof(svalue); // this is dependend on the locale
      const gdouble val=svalue?g_ascii_strtod(svalue,NULL):0.0;
      g_value_set_double(gvalue,val);
    } break;
    case G_TYPE_FLOAT: {
      const gfloat val=svalue?(gfloat)g_ascii_strtod(svalue,NULL):0.0;
      g_value_set_float(gvalue,val);
    } break;
    case G_TYPE_BOOLEAN: {
      const gint val=svalue?atoi(svalue):0;
      g_value_set_boolean(gvalue,val);
    } break;
    case G_TYPE_STRING: {
      g_value_set_string(gvalue,svalue);
    } break;
    case G_TYPE_ENUM: {
      GEnumClass *enum_class=g_type_class_peek_static(G_VALUE_TYPE(gvalue));
      GEnumValue *enum_value=g_enum_get_value_by_nick(enum_class,svalue);
      GST_DEBUG("'%s', %p, %p, [%s]",G_VALUE_TYPE_NAME(gvalue),enum_class,enum_value,svalue);
      if(enum_value) {
        //GST_INFO("-> %d",enum_value->value);
        g_value_set_enum(gvalue,enum_value->value);
      }
      else if(svalue && isdigit(*svalue)) {
        // legacy
        const gint val=atoi(svalue);
        //GST_INFO("-> %d",val);
        g_value_set_enum(gvalue,val);
      }
    } break;
    case G_TYPE_INT: {
      const gint val=svalue?atoi(svalue):0;
      g_value_set_int(gvalue,val);
    } break;
    case G_TYPE_UINT: {
      const guint val=svalue?atoi(svalue):0;
      g_value_set_uint(gvalue,val);
    } break;
    case G_TYPE_LONG: {
      const glong val=svalue?atol(svalue):0;
      g_value_set_long(gvalue,val);
    } break;
    case G_TYPE_ULONG: {
      const gulong val=svalue?atol(svalue):0;
      g_value_set_ulong(gvalue,val);
    } break;
    default:
      GST_ERROR("unsupported GType=%d:'%s' for value=\"%s\"",G_VALUE_TYPE(gvalue),G_VALUE_TYPE_NAME(gvalue),svalue);
      return(FALSE);
  }
  return(TRUE);
}

/**
 * bt_persistence_get_value:
 * @gvalue: the event cell
 *
 * Returns the string representation of the given @gvalue. Free it when done.
 *
 * Returns: a newly allocated string with the data or %NULL on error
 */
gchar *bt_persistence_get_value(GValue * const gvalue) {
// we're not showing more digits in the pattern view right now
#define FLOAT_BUFFER_LEN 8
  GType base_type;
  gchar *res=NULL;

  g_return_val_if_fail(G_IS_VALUE(gvalue),NULL);

  base_type=bt_g_type_get_base_type(G_VALUE_TYPE(gvalue));
  // depending on the type, set the result
  switch(base_type) {
    case G_TYPE_DOUBLE: {
      gchar str[FLOAT_BUFFER_LEN+1];
      // this is dependend on the locale
      //res=g_strdup_printf("%lf",g_value_get_double(gvalue));
      res=g_strdup(g_ascii_dtostr(str,FLOAT_BUFFER_LEN,g_value_get_double(gvalue)));
      } break;
    case G_TYPE_FLOAT: {
      gchar str[FLOAT_BUFFER_LEN+1];
      // this is dependend on the locale
      //res=g_strdup_printf("%f",g_value_get_float(gvalue));
      res=g_strdup(g_ascii_dtostr(str,FLOAT_BUFFER_LEN,g_value_get_float(gvalue)));
      } break;
    case G_TYPE_BOOLEAN:
      res=g_strdup_printf("%d",g_value_get_boolean(gvalue));
      break;
    case G_TYPE_STRING:
      res=g_value_dup_string(gvalue);
      break;
    case G_TYPE_ENUM: {
      GEnumClass *enum_class=g_type_class_peek_static(G_VALUE_TYPE(gvalue));
      GEnumValue *enum_value=g_enum_get_value(enum_class,g_value_get_enum(gvalue));
      if(enum_value) {
        res=g_strdup_printf("%s",enum_value->value_nick);
        //GST_INFO("-> %s",res);
      }
      else {
        // legacy
        res=g_strdup_printf("%d",g_value_get_enum(gvalue));
        //GST_INFO("-> %s",res);
      }
    } break;
    case G_TYPE_INT:
      res=g_strdup_printf("%d",g_value_get_int(gvalue));
      break;
    case G_TYPE_UINT:
      res=g_strdup_printf("%u",g_value_get_uint(gvalue));
      break;
    case G_TYPE_LONG:
      res=g_strdup_printf("%ld",g_value_get_long(gvalue));
      break;
    case G_TYPE_ULONG:
      res=g_strdup_printf("%lu",g_value_get_ulong(gvalue));
      break;
    default:
      GST_ERROR("unsupported GType=%d:'%s'",G_VALUE_TYPE(gvalue),G_VALUE_TYPE_NAME(gvalue));
      return(NULL);
  }
  return(res);
}


//-- wrapper

/**
 * bt_persistence_save:
 * @self: a serialiable object
 * @parent_node: the parent xml node
 * @selection: an optional selection
 *
 * Serializes the given object into @node.
 *
 * Returns: the new node if the element has been serialized, ellse %NULL.
 */
xmlNodePtr bt_persistence_save(const BtPersistence * const self, xmlNodePtr const parent_node, const BtPersistenceSelection * const selection) {
  g_return_val_if_fail (BT_IS_PERSISTENCE (self), FALSE);

  return (BT_PERSISTENCE_GET_INTERFACE (self)->save (self, parent_node, selection));
}

/**
 * bt_persistence_load:
 * @self: a deserialiable object
 * @node: the xml node
 * @location: an optional location
 *
 * Deserializes the given object from the @node.
 *
 * Returns: %TRUE if the element has been deserialized.
 */
gboolean bt_persistence_load(const BtPersistence * const self, xmlNodePtr node, const BtPersistenceLocation * const location) {
  g_return_val_if_fail (BT_IS_PERSISTENCE (self), FALSE);

  return (BT_PERSISTENCE_GET_INTERFACE (self)->load (self, node, location));
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
    const GTypeInfo info = {
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
