// $Id: persistence.c,v 1.6 2006-03-08 21:37:54 ensonic Exp $
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
 * @node: the list xml node
 *
 * Iterates over a list of objects, which must implement the #BtPersistece
 * interface and calls bt_persistence_save() on each item.
 *
 * Return: %TRUE if all elements have been serialized.
 */
gboolean bt_persistence_save_list(const GList *list,xmlDocPtr doc, xmlNodePtr node) {
  gboolean res=TRUE;

  for(;(list && res);list=g_list_next(list)) {
    res&=(bt_persistence_save(BT_PERSISTENCE(list->data),doc,node,NULL)!=NULL);
  }
  return(res);
}

//-- gvalue helper

/**
 * bt_persistence_set_value:
 * @gvalue: a #GValue
 * @svalue: the string representation of the value to store
 *
 * Stores the supplied value into the given @gvalue.
 *
 * Return: %TRUE for success
 */
gboolean bt_persistence_set_value(GValue *gvalue, const gchar *svalue) {
  GType base_type;
  
  g_return_val_if_fail(G_IS_VALUE(gvalue),FALSE);
  g_return_val_if_fail(svalue,FALSE);

  base_type=bt_g_type_get_base_type(G_VALUE_TYPE(gvalue));
  // depending on the type, set the GValue
  switch(base_type) {
    case G_TYPE_DOUBLE: {
      //gdouble val=atof(svalue); // this is dependend on the locale
      gdouble val=g_ascii_strtod(svalue,NULL);
      g_value_set_double(gvalue,val);
    } break;
    case G_TYPE_BOOLEAN: {
      gint val=atoi(svalue);
      g_value_set_boolean(gvalue,val);
    } break;
    case G_TYPE_STRING: {
      g_value_set_string(gvalue,svalue);
    } break;
    case G_TYPE_ENUM: {
      gint val=atoi(svalue);
      g_value_set_enum(gvalue,val);
    } break;
    case G_TYPE_INT: {
      gint val=atoi(svalue);
      g_value_set_int(gvalue,val);
    } break;
    case G_TYPE_UINT: {
      guint val=atoi(svalue);
      g_value_set_uint(gvalue,val);
    } break;
    case G_TYPE_LONG: {
      glong val=atol(svalue);
      g_value_set_long(gvalue,val);
    } break;
    case G_TYPE_ULONG: {
      gulong val=atol(svalue);
      g_value_set_ulong(gvalue,val);
    } break;
    default:
      GST_ERROR("unsupported GType=%d:'%s' for value=\"%s\"",G_VALUE_TYPE(gvalue),G_VALUE_TYPE_NAME(gvalue),svalue);
      return(FALSE);
  }
  return(TRUE);
}

/*
 * bt_persistence_get_value:
 * @gvalue: the event cell
 *
 * Returns the string representation of the given @gvalue. Free it when done.
 *
 * Returns: a newly allocated string with the data or %NULL on error
 */
gchar *bt_persistence_get_value(GValue *gvalue) {
  GType base_type;
  gchar *res=NULL;

  g_return_val_if_fail(G_IS_VALUE(gvalue),NULL);
  
  base_type=bt_g_type_get_base_type(G_VALUE_TYPE(gvalue));
  // depending on the type, set the result
  switch(base_type) {
    case G_TYPE_DOUBLE:
      res=g_strdup_printf("%lf",g_value_get_double(gvalue));
      break;
    case G_TYPE_BOOLEAN:
      res=g_strdup_printf("%d",g_value_get_boolean(gvalue));
      break;
    case G_TYPE_STRING:
      res=g_value_dup_string(gvalue);
      break;
    case G_TYPE_ENUM:
      res=g_strdup_printf("%d",g_value_get_enum(gvalue));
      break;
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
 * @doc; the xml-document
 * @parent_node: the parent xml node
 * @selection: an optional selection
 *
 * Serializes the given object into @node.
 *
 * Return: the new node if the element has been serialized, ellse %NULL.
 */
/* @todo: give parent_node to _save()
 * return new xmlNodePtr (or NULL for error)
 * subclassed objects call super first and use the returned node to save their own data
 */
xmlNodePtr bt_persistence_save(BtPersistence *self, xmlDocPtr doc, xmlNodePtr parent_node, BtPersistenceSelection *selection) {
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
