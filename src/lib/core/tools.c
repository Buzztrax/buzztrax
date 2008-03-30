/* $Id: tools.c,v 1.36 2007-12-07 15:44:02 ensonic Exp $
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

//-- registry

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
    res=g_list_prepend(res,(gchar *)gst_plugin_feature_get_name(feature));
  }
  g_list_free(list);
  g_strfreev(categories);
  return(res);
}

/**
 * bt_gst_element_factory_can_sink_media_type:
 * @factory: element factory to check
 * @name: caps type name
 *
 * Check if a factories sink pads are compatible with the given @name. The @name
 * can e.g. be "audio/x-raw-int".
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
      GST_INFO("  skipping template, wrong dir: %d", tmpl->direction);
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
 * Returns: a list of elements that does not exist, %NULL if all elements exist
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

static gboolean core_elements_checked=FALSE;
/**
 * bt_gst_check_core_elements:
 *
 * Check if all core elements exist.
 *
 * Returns: a list of elements that does not exist, %NULL if all elements exist
 */
GList *bt_gst_check_core_elements(void) {
  static GList *core_elements=NULL;
  static GList *res=NULL;

  if(!core_elements) {
    core_elements=g_list_prepend(core_elements,"volume");
    core_elements=g_list_prepend(core_elements,"tee");
    core_elements=g_list_prepend(core_elements,"audioconvert");
    core_elements=g_list_prepend(core_elements,"adder");
    /* if registry ever gets a 'changed' signal, we need to connect to that and
     * reset core_elements_checked to FALSE
     */
  }
  if(!core_elements_checked) {
    core_elements_checked=TRUE;
    res=bt_gst_check_elements(core_elements);
  }
  return(res);
}

//-- controller

/**
 * bt_gst_object_activate_controller:
 * @param_parent: the object to attach the controller to
 * @param_name: which parameter should be controlled
 * @is_trigger: is it a trigger parameter
 *
 * Create the controller object and set interpolation mode.
 *
 * Returns: the controller object if the parameter could be attached.
 */
GstController *bt_gst_object_activate_controller(GObject *param_parent,gchar *param_name,gboolean is_trigger) {
  GstController *ctrl;

  GST_INFO("activate controller for %s:%s", GST_IS_OBJECT(param_parent)?GST_OBJECT_NAME(param_parent):"-",param_name);

  if((ctrl=gst_object_control_properties(param_parent, param_name, NULL))) {
#ifdef HAVE_GST_0_10_14
    GstInterpolationControlSource *cs=gst_interpolation_control_source_new();
    gst_controller_set_control_source(ctrl,param_name,GST_CONTROL_SOURCE(cs));
    // set interpolation mode depending on param type
    gst_interpolation_control_source_set_interpolation_mode(cs,is_trigger?GST_INTERPOLATE_TRIGGER:GST_INTERPOLATE_NONE);
#else
    // set interpolation mode depending on param type
    gst_controller_set_interpolation_mode(ctrl,param_name,is_trigger?GST_INTERPOLATE_TRIGGER:GST_INTERPOLATE_NONE);
#endif
  }
  return(ctrl);
}

/**
 * bt_gst_object_deactivate_controller:
 * @param_parent: the object to dettach the controller from
 * @param_name: which parameter should be uncontrolled
 *
 * Remove the controller object.
 */
void bt_gst_object_deactivate_controller(GObject *param_parent,gchar *param_name) {
#ifdef HAVE_GST_0_10_14
  GstController *ctrl;
#endif

  GST_INFO("deactivate controller for %s:%s", GST_IS_OBJECT(param_parent)?GST_OBJECT_NAME(param_parent):"-",param_name);

#ifdef HAVE_GST_0_10_14
  if((ctrl=gst_object_get_controller(param_parent))) {
    GstControlSource *cs=gst_controller_get_control_source(ctrl,param_name);
    if(cs) {
      gst_controller_set_control_source(ctrl,param_name,NULL);
      // unref twice
      g_object_unref(cs);g_object_unref(cs);
    }
  }
#endif
  gst_object_uncontrol_properties(param_parent, param_name, NULL);
}


//-- gst compat

#ifndef HAVE_GST_0_10_11
/**
 * gst_element_state_change_return_get_name:
 * @state_ret: a #GstStateChangeReturn to get the name of.
 *
 * Gets a string representing the given state change result.
 *
 * Returns: a string with the name of the state change result.
 */
G_CONST_RETURN gchar *
gst_element_state_change_return_get_name (GstStateChangeReturn state_ret)
{
  switch (state_ret) {
#ifdef GST_DEBUG_COLOR
    case GST_STATE_CHANGE_FAILURE:
      return "\033[01;31mFAILURE\033[00m";
    case GST_STATE_CHANGE_SUCCESS:
      return "\033[01;32mSUCCESS\033[00m";
    case GST_STATE_CHANGE_ASYNC:
      return "\033[01;33mASYNC\033[00m";
    case GST_STATE_CHANGE_NO_PREROLL:
      return "\033[01;34mNO_PREROLL\033[00m";
    default:
      /* This is a memory leak */
      return g_strdup_printf ("\033[01;35;41mUNKNOWN!\033[00m(%d)", state_ret);
#else
    case GST_STATE_CHANGE_FAILURE:
      return "FAILURE";
    case GST_STATE_CHANGE_SUCCESS:
      return "SUCCESS";
    case GST_STATE_CHANGE_ASYNC:
      return "ASYNC";
    case GST_STATE_CHANGE_NO_PREROLL:
      return "NO PREROLL";
    default:
      /* This is a memory leak */
      return g_strdup_printf ("UNKNOWN!(%d)", state_ret);
#endif
  }
}
#endif

//-- debugging

/**
 * bt_gst_element_dbg_pads:
 * @elem: a #GstElement
 *
 * Write out a list of pads for the given element
 *
 */
void bt_gst_element_dbg_pads(GstElement * const elem) {
  GstIterator *it;
  GstPad * const pad;
  GstPadDirection dir;
  const gchar *dirs[]={"unknown","src","sink",NULL};
  gboolean done;
  guint i;

  GST_DEBUG("machine: %s",GST_ELEMENT_NAME(elem));
  it=gst_element_iterate_pads(elem);
  done = FALSE;
  while (!done) {
    switch (gst_iterator_next (it, (gpointer)&pad)) {
      case GST_ITERATOR_OK:
        dir=gst_pad_get_direction(pad);
        GstCaps * const caps=gst_pad_get_caps(pad);
        const guint size=gst_caps_get_size(caps);
        GST_DEBUG("  pad: %s:%s, dir: %s, nr-caps: %d",GST_DEBUG_PAD_NAME(pad),dirs[dir],size);
        // iterate over structures and print
        for(i=0;i<size;i++) {
          const GstStructure * const structure=gst_caps_get_structure(caps,i);
          gchar * const str=gst_structure_to_string(structure);
          GST_DEBUG("    caps[%2d]: %s : %s",i,GST_STR_NULL(gst_structure_get_name(structure)),str);
          g_free(str);
         }
        gst_caps_unref(caps);
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

/*
 * bt_cpu_load_init:
 *
 * Initializes cpu usage monitoring.
 */
static void GST_GNUC_CONSTRUCTOR bt_cpu_load_init(void) {
  struct timeval now;

  gettimeofday(&now,NULL);
  treal_last=GST_TIMEVAL_TO_TIME(now);
  //clk=sysconf(_SC_CLK_TCK);
}

/**
 * bt_cpu_load_get_current:
 *
 * Determines the current CPU load.
 *
 * Returns: CPU usage as integer ranging from 0% to 100%
 */
guint bt_cpu_load_get_current(void) {
  struct rusage rus,ruc;
  //struct tms tms;
  GstClockTime tnow,treal,tuser,tsys;
  struct timeval now;
  guint cpuload;

  gettimeofday(&now,NULL);
  tnow=GST_TIMEVAL_TO_TIME(now);
  treal=tnow-treal_last;
  treal_last=tnow;
  // version 1
  getrusage(RUSAGE_SELF,&rus);
  getrusage(RUSAGE_CHILDREN,&ruc);
  tnow=GST_TIMEVAL_TO_TIME(rus.ru_utime)+GST_TIMEVAL_TO_TIME(ruc.ru_utime);
  tuser=tnow-tuser_last;
  tuser_last=tnow;
  tnow=GST_TIMEVAL_TO_TIME(rus.ru_stime)+GST_TIMEVAL_TO_TIME(ruc.ru_stime);
  tsys=tnow-tsys_last;
  tsys_last=tnow;
  // version 2
  //times(&tms);
  // percentage
  cpuload=(guint)gst_util_uint64_scale(tuser+tsys,G_GINT64_CONSTANT(100),treal);
  GST_LOG("real %"GST_TIME_FORMAT", user %"GST_TIME_FORMAT", sys %"GST_TIME_FORMAT" => cpuload %d",GST_TIME_ARGS(treal),GST_TIME_ARGS(tuser),GST_TIME_ARGS(tsys),cpuload);
  //printf("user %6.4lf, sys %6.4lf\n",(double)tms.tms_utime/(double)clk,(double)tms.tms_stime/(double)clk);

  return(cpuload);
}
