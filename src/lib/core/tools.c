/* $Id: tools.c,v 1.19 2005-09-19 21:23:07 ensonic Exp $
 */
 
#define BT_CORE
#define BT_TOOLS_C
#include <libbtcore/core.h>

static gboolean bt_gst_registry_class_filter(GstPluginFeature *feature, gpointer user_data) {
  gchar *class_filter=(gchar *)user_data;
  gsize sl=(gsize)(strlen(class_filter));
  
  if(GST_IS_ELEMENT_FACTORY(feature)) {
    if(!g_ascii_strncasecmp(gst_element_factory_get_klass(GST_ELEMENT_FACTORY(feature)),class_filter,sl)) {
      return(TRUE);
    }
  }
  return(FALSE);
}

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
  GList *list,*node;
  GList *res=NULL;
  //GstElementFactory *element;
  //gsize sl;
  GstPluginFeature *feature;

  g_return_val_if_fail(is_string(class_filter),NULL);
    
  /* deprecated API
  sl=(gsize)(strlen(class_filter));

  list=gst_registry_pool_feature_list(GST_TYPE_ELEMENT_FACTORY);
  for(node=list;node;node=g_list_next(node)) {
    element=GST_ELEMENT_FACTORY(node->data);
    //GST_DEBUG("  %s: %s", GST_OBJECT_NAME(element),gst_element_factory_get_klass(element));
    if(!g_ascii_strncasecmp(gst_element_factory_get_klass(element),class_filter,sl)) {
      // @todo: consider  gst_plugin_feature_get_rank() for sorting
      res=g_list_append(res,(gchar *)gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(element)));
    }
  }
  g_list_free(list);
  */
  list=gst_registry_feature_filter(gst_registry_get_default(),bt_gst_registry_class_filter,FALSE,class_filter);
  for(node=list;node;node=g_list_next(node)) {
    feature=GST_PLUGIN_FEATURE(node->data);
    res=g_list_append(res,(gchar *)gst_plugin_feature_get_name(feature));
  }
  g_list_free(list);
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
