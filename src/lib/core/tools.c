/* $Id: tools.c,v 1.18 2005-09-19 16:14:06 ensonic Exp $
 */
 
#define BT_CORE
#define BT_TOOLS_C
#include <libbtcore/core.h>

/**
 * bt_gst_registry_get_element_names_by_class:
 * @class_filter: path for filtering (e.g. "Sink/Audio")
 *
 * Iterates over all available plugins and filters by class-name.
 * The matching uses right-truncation, e.g. "Sink/" matches all sorts of sinks.
 *
 * Returns: list of element names, g_list_free after use.
 */
GList *bt_gst_registry_get_element_names_by_class(gchar *class_filter) {
  GList *elements,*node;
  GList *res=NULL;
  GstElementFactory *element;
  gsize sl;

  g_return_val_if_fail(is_string(class_filter),NULL);
  
  sl=(gsize)(strlen(class_filter));
  
  elements=gst_registry_pool_feature_list(GST_TYPE_ELEMENT_FACTORY);
  for(node=elements;node;node=g_list_next(node)) {
    element=GST_ELEMENT_FACTORY(node->data);
    //GST_DEBUG("  %s: %s", GST_OBJECT_NAME(element),gst_element_factory_get_klass(element));
    if(!g_ascii_strncasecmp(gst_element_factory_get_klass(element),class_filter,sl)) {
      // @todo: consider  gst_plugin_feature_get_rank() for sorting
      res=g_list_append(res,(gchar *)gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(element)));
    }
  }
  g_list_free(elements);
  return(res);
}

#ifndef HAVE_GLIB_2_8
gpointer g_try_malloc0(gulong n_bytes) {
  gpointer mem;

  if((mem=g_try_malloc(n_bytes))) {
    memset(mem,0,n_bytes);
  }
  return(mem);
}
#endif
