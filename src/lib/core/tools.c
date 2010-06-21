/* $Id$
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

#define BT_CORE
#define BT_TOOLS_C

#include "core_private.h"

//-- gst registry

static gboolean bt_gst_registry_class_filter(GstPluginFeature * const feature, gpointer user_data) {
  const gchar ** categories=(const gchar **)user_data;
  gboolean res=FALSE;

  if(GST_IS_ELEMENT_FACTORY(feature)) {
    const gchar *klass=gst_element_factory_get_klass(GST_ELEMENT_FACTORY(feature));
    res=TRUE;
    while(res && *categories) {
      GST_LOG("  %s",*categories);
      // there is also strcasestr()
      res=(strstr(klass,*categories++)!=NULL);
    }
  }
  return(res);
}

/**
 * bt_gst_registry_get_element_names_matching_all_categories:
 * @class_filter: path for filtering (e.g. "Sink/Audio")
 *
 * Iterates over all available plugins and filters by categories given in
 * @class_filter.
 *
 * Returns: list of element names, g_list_free after use.
 */
GList *bt_gst_registry_get_element_names_matching_all_categories(const gchar *class_filter) {
  const GList *node;
  GList *res=NULL;

  g_return_val_if_fail(BT_IS_STRING(class_filter),NULL);

  GST_DEBUG("run filter for categories: %s",class_filter);

  gchar **categories=g_strsplit(class_filter,"/",0);
  GList * const list=gst_default_registry_feature_filter(bt_gst_registry_class_filter,FALSE,(gpointer)categories);

  GST_DEBUG("filtering done");

  for(node=list;node;node=g_list_next(node)) {
    GstPluginFeature * const feature=GST_PLUGIN_FEATURE(node->data);
    res=g_list_prepend(res,(gpointer)gst_plugin_feature_get_name(feature));
  }
  gst_plugin_feature_list_free((GList *)list);
  g_strfreev(categories);
  return(res);
}

/**
 * bt_gst_element_factory_can_sink_media_type:
 * @factory: element factory to check
 * @name: caps type name
 *
 * Check if the sink pads of the given @factory are compatible with the given 
 * @name. The @name can e.g. be "audio/x-raw-int".
 *
 * Returns: %TRUE if the pads are compatible.
 */
gboolean bt_gst_element_factory_can_sink_media_type(GstElementFactory *factory,const gchar *name) {
  GList *node;
  GstStaticPadTemplate *tmpl;
  GstCaps *caps;
  guint i,size;
  const GstStructure *s;

  g_assert(GST_IS_ELEMENT_FACTORY(factory));

  for(node=(GList *)gst_element_factory_get_static_pad_templates(factory);node;node=g_list_next(node)) {
    tmpl=node->data;
    if(tmpl->direction==GST_PAD_SINK) {
      caps=gst_static_caps_get(&tmpl->static_caps);
      size=gst_caps_get_size(caps);
      GST_INFO("  testing caps: %" GST_PTR_FORMAT, caps);
      for(i=0;i<size;i++) {
        s=gst_caps_get_structure(caps,i);
        if(gst_structure_has_name(s,name)) {
          gst_caps_unref(caps);
          return(TRUE);
        }
      }
      gst_caps_unref(caps);
    }
    else {
      GST_DEBUG("  skipping template, wrong dir: %d", tmpl->direction);
    }
  }
  return(FALSE);
}

/**
 * bt_gst_check_elements:
 * @list: a #GList with element names
 *
 * Check if the given elements exist.
 *
 * Returns: a list of element-names which do not exist, %NULL if all elements exist
 */
GList *bt_gst_check_elements(GList *list) {
  GList *res=NULL,*node;
  GstRegistry *registry;
  GstPluginFeature*feature;

  registry=gst_registry_get_default();

  for(node=list;node;node=g_list_next(node)) {
    if((feature=gst_registry_lookup_feature(registry,(const gchar *)node->data))) {
      gst_object_unref(feature);
    }
    else {
      res=g_list_prepend(res,node->data);
    }
  }

  return(res);
}

/**
 * bt_gst_check_core_elements:
 *
 * Check if all core elements exist.
 *
 * Returns: a list of elements that does not exist, %NULL if all elements exist.
 * The list is static, don't free.
 */
GList *bt_gst_check_core_elements(void) {
  static GList *core_elements=NULL;
  static GList *res=NULL;
  static gboolean core_elements_checked=FALSE;

  /* @todo: if registry ever gets a 'changed' signal, we need to connect to that and
   * reset core_elements_checked to FALSE
   * There is gst_registry_get_feature_list_cookie() now
   */
  if(!core_elements) {
    core_elements=g_list_prepend(core_elements,"capsfilter");
    core_elements=g_list_prepend(core_elements,"queue");
    core_elements=g_list_prepend(core_elements,"tee");
    core_elements=g_list_prepend(core_elements,"audioconvert");
    core_elements=g_list_prepend(core_elements,"adder");
    core_elements=g_list_prepend(core_elements,"volume");
  }
  if(!core_elements_checked) {
    core_elements_checked=TRUE;
    res=bt_gst_check_elements(core_elements);
  }
  return(res);
}

//-- gst debugging

const gchar *bt_gst_debug_pad_link_return(GstPadLinkReturn link_res, GstPad *src_pad, GstPad *sink_pad) {
  static gchar msg1[5000];
  gchar msg2[4000];
  static gchar *link_res_desc[] = {
    "link succeeded",
    "pads have no common grandparent",
    "pad was already linked",
    "pads have wrong direction",
    "pads do not have common format",
    "pads cannot cooperate in scheduling",
    "refused for some reason"
  };
  
  if (!src_pad || !sink_pad) {
    /* one of the pads is NULL */
    msg2[0]='\0';
  }
  else {
    switch(link_res) {
      case GST_PAD_LINK_WRONG_HIERARCHY: {
        GstObject *src_parent = GST_OBJECT_PARENT(src_pad);
        GstObject *sink_parent = GST_OBJECT_PARENT(sink_pad);
        sprintf(msg2," : parent(src) = %s, parent(sink) = %s",
          (src_parent?GST_OBJECT_NAME(src_parent):"(NULL)"),
          (sink_parent?GST_OBJECT_NAME(sink_parent):"(NULL)"));
        break;
      }
      case GST_PAD_LINK_WAS_LINKED: {
        GstPad *src_peer = src_pad->peer;
        GstPad *sink_peer = sink_pad->peer;
        sprintf(msg2," : peer(src) = %s:%s, peer(sink) = %s:%s",
          GST_DEBUG_PAD_NAME(src_peer),
          GST_DEBUG_PAD_NAME(sink_peer));
        break;
      }
      case GST_PAD_LINK_WRONG_DIRECTION: {
        static gchar *dir_name[] = {"unknown", "src", "sink" };
        sprintf(msg2," : direction(src) = %s, direction(sink) = %s",
          ((src_pad->direction<3)?dir_name[src_pad->direction]:"invalid"),
          ((sink_pad->direction<3)?dir_name[sink_pad->direction]:"invalid"));
        break;
      }
      case GST_PAD_LINK_NOFORMAT: {
#if GST_CHECK_VERSION(0,10,26)
        GstCaps *srcc = gst_pad_get_caps_reffed(src_pad);
        GstCaps *sinkc = gst_pad_get_caps_reffed(sink_pad);
#else
        GstCaps *srcc = gst_pad_get_caps(src_pad);
        GstCaps *sinkc = gst_pad_get_caps(sink_pad);
#endif
        gchar *src_caps = gst_caps_to_string(srcc);
        gchar *sink_caps = gst_caps_to_string(sinkc);
        sprintf(msg2," : caps(src) = %s, caps(sink) = %s",src_caps,sink_caps);
        g_free(src_caps);g_free(sink_caps);
        gst_caps_unref(srcc);gst_caps_unref(sinkc);
        break;
      }
      case GST_PAD_LINK_NOSCHED:
      case GST_PAD_LINK_REFUSED:
      case GST_PAD_LINK_OK:
      default:
        msg2[0]='\0';
        break;
    }
  }
  
  sprintf(msg1, "%s:%s -> %s:%s : %s%s",
        GST_DEBUG_PAD_NAME(src_pad),GST_DEBUG_PAD_NAME(sink_pad),
        link_res_desc[-link_res], msg2);
  
  
  return msg1;
}

//-- gst compat

//-- glib compat & helper

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


//-- cpu load monitoring

static GstClockTime treal_last=G_GINT64_CONSTANT(0),tuser_last=G_GINT64_CONSTANT(0),tsys_last=G_GINT64_CONSTANT(0);
//long clk=1;
//long num_cpus;

#if WIN32
#include <windows.h> //For GetSystemInfo
#endif


/*
 * bt_cpu_load_init:
 *
 * Initializes cpu usage monitoring.
 */
static void GST_GNUC_CONSTRUCTOR bt_cpu_load_init(void) {
#ifdef HAVE_CLOCK_GETTIME
  struct timespec now;
#else
  struct timeval now;
#endif

#ifdef HAVE_CLOCK_GETTIME
  clock_gettime (CLOCK_MONOTONIC, &now);
  treal_last=GST_TIMESPEC_TO_TIME(now);
#else
  gettimeofday(&now,NULL);
  treal_last=GST_TIMEVAL_TO_TIME(now);
#endif
  //clk=sysconf(_SC_CLK_TCK);
  
  /* @todo: get number of CPUS's
#ifndef WIN32
   num_cpus = sysconf (_SC_NPROCESSORS_ONLN);
#else
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  num_cpus = si.dwNumberOfProcessors;
#endif
   */
}

/**
 * bt_cpu_load_get_current:
 *
 * Determines the current CPU load.
 *
 * Returns: CPU usage as integer ranging from 0% to 100%
 */
guint bt_cpu_load_get_current(void) {
#ifdef HAVE_CLOCK_GETTIME
  struct timespec now;
#else
  struct timeval now;
#endif
  struct rusage rus,ruc;
  GstClockTime tnow,treal,tuser,tsys;
  guint cpuload;

  // check real time elapsed
#ifdef HAVE_CLOCK_GETTIME
  clock_gettime (CLOCK_MONOTONIC, &now);
  tnow=GST_TIMESPEC_TO_TIME(now);
#else
  gettimeofday(&now,NULL);
  tnow=GST_TIMEVAL_TO_TIME(now);
#endif
  treal=tnow-treal_last;
  treal_last=tnow;
  // check time spent load
  getrusage(RUSAGE_SELF,&rus);
  getrusage(RUSAGE_CHILDREN,&ruc);
  tnow=GST_TIMEVAL_TO_TIME(rus.ru_utime)+GST_TIMEVAL_TO_TIME(ruc.ru_utime);
  tuser=tnow-tuser_last;
  tuser_last=tnow;
  tnow=GST_TIMEVAL_TO_TIME(rus.ru_stime)+GST_TIMEVAL_TO_TIME(ruc.ru_stime);
  tsys=tnow-tsys_last;
  tsys_last=tnow;
  // percentage
  cpuload=(guint)gst_util_uint64_scale(tuser+tsys,G_GINT64_CONSTANT(100),treal);
  GST_LOG("real %"GST_TIME_FORMAT", user %"GST_TIME_FORMAT", sys %"GST_TIME_FORMAT" => cpuload %d",GST_TIME_ARGS(treal),GST_TIME_ARGS(tuser),GST_TIME_ARGS(tsys),cpuload);
  //printf("user %6.4lf, sys %6.4lf\n",(double)tms.tms_utime/(double)clk,(double)tms.tms_stime/(double)clk);

  return(cpuload);
}
