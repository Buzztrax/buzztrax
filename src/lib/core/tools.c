/* $Id: tools.c,v 1.24 2006-01-13 18:06:12 ensonic Exp $
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
  GstPluginFeature *feature;

  g_return_val_if_fail(BT_IS_STRING(class_filter),NULL);
    
  list=gst_default_registry_feature_filter(bt_gst_registry_class_filter,FALSE,class_filter);
  for(node=list;node;node=g_list_next(node)) {
    feature=GST_PLUGIN_FEATURE(node->data);
    res=g_list_append(res,(gchar *)gst_plugin_feature_get_name(feature));
  }
  g_list_free(list);
  return(res);
}

/**
 * gst_element_dbg_caps:
 * @elem: a #GstElement
 *
 * Write out a list of pads for the given element
 *
 */
void gst_element_dbg_pads(GstElement *elem) {
  GstIterator *it;
  GstPad *pad;
  GstCaps *caps;
  GstStructure *structure;
  GstPadDirection dir;
  gchar *dirs[]={"unkonow","src","sink",NULL};
  gchar *str;
  gboolean done;
  guint i,size;

  GST_DEBUG("machine: %s",GST_ELEMENT_NAME(elem));
  it=gst_element_iterate_pads(elem);
  done = FALSE;
  while (!done) {
    switch (gst_iterator_next (it, (gpointer)&pad)) {
      case GST_ITERATOR_OK:
        dir=gst_pad_get_direction(pad);
        caps=gst_pad_get_caps(pad);
        size=gst_caps_get_size(caps);
        GST_DEBUG("  pad: %s:%s, dir: %s, nr-caps: %d",GST_DEBUG_PAD_NAME(pad),dirs[dir],size);
        // iterate over structures and print
        for(i=0;i<size;i++) {
          structure=gst_caps_get_structure(caps,i);
          str=gst_structure_to_string(structure);
          GST_DEBUG("    caps[%2d]: %s : %s",i,GST_STR_NULL(gst_structure_get_name(structure)),str);
          g_free(str);
          //gst_object_unref(structure);
        }
        //gst_object_unref(caps);
        gst_object_unref(pad);
        break;
      case GST_ITERATOR_RESYNC:
        gst_iterator_resync (it);
        break;
      case GST_ITERATOR_ERROR:
      case GST_ITERATOR_DONE:
        done = TRUE;
        break;
    }
  }
  gst_iterator_free(it);
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

/**
 * bt_g_type_get_base_type:
 * @type: a GType
 *
 * Call g_type_parent() as long as it returns a parent.
 *
 * Returns: the super parent type, aka base type. 
 */
GType bt_g_type_get_base_type(GType type) {
  GType base;

  while((base=g_type_parent(type))) type=base;
  return(type);
}

