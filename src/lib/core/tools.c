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

#define BT_CORE
#define BT_TOOLS_C

#include <math.h>
#include <time.h>
#include <sys/resource.h>

#include "core_private.h"
#include <glib/gprintf.h>
#include <gst/base/gstbasetransform.h>

//-- gst registry

static gboolean
bt_gst_registry_class_filter (GstPluginFeature * const feature,
    gpointer user_data)
{
  const gchar **categories = (const gchar **) user_data;
  gboolean res = FALSE;

  if (GST_IS_ELEMENT_FACTORY (feature)) {
    const gchar *klass =
        gst_element_factory_get_metadata (GST_ELEMENT_FACTORY (feature),
        GST_ELEMENT_METADATA_KLASS);
    res = TRUE;
    while (res && *categories) {
      GST_LOG ("  %s", *categories);
      // there is also strcasestr()
      res = (strstr (klass, *categories++) != NULL);
    }
  }
  return res;
}

/**
 * bt_gst_registry_get_element_factories_matching_all_categories:
 * @class_filter: path for filtering (e.g. "Sink/Audio")
 *
 * Iterates over all available elements and filters by categories given in
 * @class_filter.
 *
 * Returns: (element-type Gst.PluginFeature) (transfer full): list of element
 * factories, use gst_plugin_feature_list_free() after use.
 *
 * Since: 0.6
 */
GList *
bt_gst_registry_get_element_factories_matching_all_categories (const gchar *
    class_filter)
{

  g_return_val_if_fail (BT_IS_STRING (class_filter), NULL);

  GST_DEBUG ("run filter for categories: %s", class_filter);

  gchar **categories = g_strsplit (class_filter, "/", 0);
  GList *list = gst_registry_feature_filter (gst_registry_get (),
      bt_gst_registry_class_filter, FALSE, (gpointer) categories);

  GST_DEBUG ("filtering done");
  g_strfreev (categories);
  return list;
}

/**
 * bt_gst_element_factory_get_pad_template:
 * @factory: element factory
 * @name: name of the pad-template, e.g. "src" or "sink_%%u"
 *
 * Lookup a pad template by name.
 *
 * Returns: (transfer full): the pad template or %NULL if not found
 */
GstPadTemplate *
bt_gst_element_factory_get_pad_template (GstElementFactory * factory,
    const gchar * name)
{
  /* get pad templates */
  const GList *list = gst_element_factory_get_static_pad_templates (factory);
  for (; list; list = g_list_next (list)) {
    GstStaticPadTemplate *t = (GstStaticPadTemplate *) list->data;

    GST_INFO_OBJECT (factory, "Checking pad-template '%s'", t->name_template);
    if (g_strcmp0 (t->name_template, name) == 0) {
      return gst_static_pad_template_get (t);
    }
  }
  GST_WARNING_OBJECT (factory, "No pad-template '%s'", name);
  return NULL;
}

/**
 * bt_gst_element_factory_can_sink_media_type:
 * @factory: element factory to check
 * @name: caps type name
 *
 * Check if the sink pads of the given @factory are compatible with the given
 * @name. The @name can e.g. be "audio/x-raw".
 *
 * Returns: %TRUE if the pads are compatible.
 */
gboolean
bt_gst_element_factory_can_sink_media_type (GstElementFactory * factory,
    const gchar * name)
{
  GList *node;
  GstStaticPadTemplate *tmpl;
  GstCaps *caps;
  guint i, size;
  const GstStructure *s;

  g_assert (GST_IS_ELEMENT_FACTORY (factory));

  for (node = (GList *) gst_element_factory_get_static_pad_templates (factory);
      node; node = g_list_next (node)) {
    tmpl = node->data;
    if (tmpl->direction == GST_PAD_SINK) {
      caps = gst_static_caps_get (&tmpl->static_caps);
      GST_INFO ("  testing caps: %" GST_PTR_FORMAT, caps);
      if (gst_caps_is_any (caps)) {
        gst_caps_unref (caps);
        return TRUE;
      }
      size = gst_caps_get_size (caps);
      for (i = 0; i < size; i++) {
        s = gst_caps_get_structure (caps, i);
        if (gst_structure_has_name (s, name)) {
          gst_caps_unref (caps);
          return TRUE;
        }
      }
      gst_caps_unref (caps);
    } else {
      GST_DEBUG ("  skipping template, wrong dir: %d", tmpl->direction);
    }
  }
  return FALSE;
}

/**
 * bt_gst_check_elements:
 * @list: (element-type utf8): a #GList with element names
 *
 * Check if the given elements exist.
 *
 * Returns: (element-type utf8) (transfer full): a list of element-names which
 * do not exist, %NULL if all elements exist, g_list_free after use.
 */
GList *
bt_gst_check_elements (GList * list)
{
  GList *res = NULL, *node;
  GstRegistry *registry;
  GstPluginFeature *feature;

  g_return_val_if_fail (gst_is_initialized (), NULL);

  if ((registry = gst_registry_get ())) {
    for (node = list; node; node = g_list_next (node)) {
      if ((feature =
              gst_registry_lookup_feature (registry,
                  (const gchar *) node->data))) {
        gst_object_unref (feature);
      } else {
        res = g_list_prepend (res, node->data);
      }
    }
  }
  return res;
}

/**
 * bt_gst_check_core_elements:
 *
 * Check if all core elements exist.
 *
 * Returns: (element-type utf8) (transfer none): a list of elements that does
 * not exist, %NULL if all elements exist. The list is static, don't free or
 * modify.
 */
GList *
bt_gst_check_core_elements (void)
{
  static GList *core_elements = NULL;
  static GList *res = NULL;
  static gboolean core_elements_checked = FALSE;

  /* TODO(ensonic): if registry ever gets a 'changed' signal, we need to connect to that and
   * reset core_elements_checked to FALSE
   * There is gst_registry_get_feature_list_cookie() now
   */
  if (!core_elements_checked) {
    // gstreamer
    core_elements = g_list_prepend (core_elements, "capsfilter");
    core_elements = g_list_prepend (core_elements, "fdsrc");
    core_elements = g_list_prepend (core_elements, "fdsink");
    core_elements = g_list_prepend (core_elements, "filesink");
    core_elements = g_list_prepend (core_elements, "identity");
    core_elements = g_list_prepend (core_elements, "queue");
    core_elements = g_list_prepend (core_elements, "tee");
    // gst-plugins-base
    core_elements = g_list_prepend (core_elements, "audioconvert");
    core_elements = g_list_prepend (core_elements, "audioresample");
    core_elements = g_list_prepend (core_elements, "audiotestsrc");
    core_elements = g_list_prepend (core_elements, "adder");
    core_elements = g_list_prepend (core_elements, "volume");
    core_elements_checked = TRUE;
    res = bt_gst_check_elements (core_elements);
    g_list_free (core_elements);
    core_elements = res;
  } else {
    res = core_elements;
  }
  return res;
}

//-- gst safe linking

/*
 * bt_gst_get_peer_pad:
 * @elem: a gstreamer element
 *
 * Get the peer pad connected to the first pad in the given iter.
 *
 * Returns: the pad or %NULL
 */
static GstPad *
bt_gst_get_peer_pad (GstIterator * it)
{
  gboolean done = FALSE;
  GValue item = { 0, };
  GstPad *pad, *peer_pad = NULL;

  while (!done) {
    switch (gst_iterator_next (it, &item)) {
      case GST_ITERATOR_OK:
        pad = GST_PAD (g_value_get_object (&item));
        peer_pad = gst_pad_get_peer (pad);
        done = TRUE;
        break;
      case GST_ITERATOR_RESYNC:
        gst_iterator_resync (it);
        break;
      case GST_ITERATOR_ERROR:
        done = TRUE;
        break;
      case GST_ITERATOR_DONE:
        done = TRUE;
        break;
    }
  }
  g_value_unset (&item);
  gst_iterator_free (it);
  return peer_pad;
}

/**
 * bt_gst_try_element:
 * @factory: plugin feature to try
 * @format: required media format
 *
 * Create an instance of the element and try to set it to %GST_STATE_READY.
 *
 * Returns: %TRUE, if the element is usable
 */
gboolean
bt_gst_try_element (GstElementFactory * factory, const gchar * format)
{
  GstElement *e;
  GstStateChangeReturn ret;
  gboolean valid = FALSE;

  if (!bt_gst_element_factory_can_sink_media_type (factory, format)) {
    GST_INFO_OBJECT (factory, "has no compatible caps");
    return FALSE;
  }
  if (!(e = gst_element_factory_create (factory, NULL))) {
    GST_INFO_OBJECT (factory, "cannot create instance");
    return FALSE;
  }

  ret = gst_element_set_state (e, GST_STATE_READY);
  if (ret != GST_STATE_CHANGE_FAILURE) {
    GST_DEBUG_OBJECT (e, "this worked!");
    valid = TRUE;
  } else {
    GST_INFO_OBJECT (e, "fails to go to ready");
  }
  gst_element_set_state (e, GST_STATE_NULL);
  gst_object_unref (e);
  return valid;
}

#if 0
/*
 * bt_gst_element_get_src_peer_pad:
 * @elem: a gstreamer element
 *
 * Get the peer pad connected to the given elements first source pad.
 *
 * Returns: the pad or %NULL
 */
static GstPad *
bt_gst_element_get_src_peer_pad (GstElement * const elem)
{
  return bt_gst_get_peer_pad (gst_element_iterate_src_pads (elem));
}
#endif

/*
 * bt_gst_element_get_sink_peer_pad:
 * @elem: a gstreamer element
 *
 * Get the peer pad connected to the given elements first sink pad.
 *
 * Returns: the pad or %NULL
 */
static GstPad *
bt_gst_element_get_sink_peer_pad (GstElement * const elem)
{
  return bt_gst_get_peer_pad (gst_element_iterate_sink_pads (elem));
}

static GstPadProbeReturn
async_add (GstPad * tee_src, GstPadProbeInfo * info, gpointer user_data)
{
  GList *elements = (GList *) user_data;
  GstPad *sink_pad =
      gst_element_get_static_pad (GST_ELEMENT (elements->data), "sink");
  const GList *node;
  GstElement *next;
  GstPadLinkReturn plr;

  GST_INFO_OBJECT (tee_src, "sync state");
  for (node = elements; node; node = g_list_next (node)) {
    next = GST_ELEMENT (node->data);
    if (!(gst_element_sync_state_with_parent (next))) {
      GST_INFO_OBJECT (tee_src, "cannot sync state for elements \"%s\"",
          GST_OBJECT_NAME (next));
    }
  }

  GST_INFO_OBJECT (tee_src, "linking to tee");
  if (GST_PAD_LINK_FAILED (plr = gst_pad_link (tee_src, sink_pad))) {
    GST_INFO_OBJECT (tee_src, "cannot link analyzers to tee");
    GST_WARNING_OBJECT (tee_src, "%s", bt_gst_debug_pad_link_return (plr,
            tee_src, sink_pad));
  }
  gst_object_unref (sink_pad);
  g_list_free (elements);

  GST_INFO_OBJECT (tee_src, "add analyzers done");

  return GST_PAD_PROBE_REMOVE;
}

/**
 * bt_bin_activate_tee_chain:
 * @bin: the bin
 * @tee: the tee to connect the chain to
 * @elements: (element-type Gst.Element): the list of elements to activate
 * @is_playing: whether the pipeline is streaming data
 *
 * Add the @elements to the @bin and link them. Handle pad blocking in playing
 * mode.
 *
 * Return: %TRUE for success
 */
gboolean
bt_bin_activate_tee_chain (GstBin * bin, GstElement * tee, GList * elements,
    gboolean is_playing)
{
  gboolean res = TRUE;
  GstElementClass *tee_class = GST_ELEMENT_GET_CLASS (tee);
  GstPad *tee_src;
  const GList *node;
  GstElement *prev = NULL, *next;

  g_return_val_if_fail (GST_IS_BIN (bin), FALSE);
  g_return_val_if_fail (GST_IS_ELEMENT (tee), FALSE);
  g_return_val_if_fail (elements, FALSE);

  GST_INFO_OBJECT (bin, "add analyzers (%d elements)",
      g_list_length (elements));

  // do final link afterwards
  for (node = elements; (node && res); node = g_list_next (node)) {
    next = GST_ELEMENT (node->data);

    if (!(res = gst_bin_add (bin, next))) {
      GST_INFO_OBJECT (bin, "cannot add element \"%s\" to bin",
          GST_OBJECT_NAME (next));
      return FALSE;
    }
    if (prev) {
      if (!(res = gst_element_link (prev, next))) {
        GST_INFO_OBJECT (bin, "cannot link elements \"%s\" -> \"%s\"",
            GST_OBJECT_NAME (prev), GST_OBJECT_NAME (next));
        return FALSE;
      }
    }
    prev = next;
  }
  GST_INFO_OBJECT (bin, "blocking tee.src");
  tee_src = gst_element_request_pad (tee,
      gst_element_class_get_pad_template (tee_class, "src_%u"), NULL, NULL);
  elements = g_list_copy (elements);
  if (is_playing) {
    gst_pad_add_probe (tee_src, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, async_add,
        elements, NULL);
  } else {
    async_add (tee_src, NULL, elements);
  }
  return res;
}

static GstPadProbeReturn
async_remove (GstPad * tee_src, GstPadProbeInfo * info, gpointer user_data)
{
  GList *elements = (GList *) user_data;
  GstElement *tee = (GstElement *) gst_pad_get_parent (tee_src);
  GstBin *bin = (GstBin *) gst_element_get_parent (tee);
  const GList *node;
  GstElement *prev, *next;

  prev = tee;
  for (node = elements; node; node = g_list_next (node)) {
    next = GST_ELEMENT (node->data);

    if (gst_element_set_state (next, GST_STATE_NULL) ==
        GST_STATE_CHANGE_FAILURE) {
      GST_INFO ("cannot set state to NULL for '%s'", GST_OBJECT_NAME (next));
    }
    gst_element_unlink (prev, next);
    prev = next;
  }
  for (node = elements; node; node = g_list_next (node)) {
    next = GST_ELEMENT (node->data);
    if (!gst_bin_remove (bin, next)) {
      GST_INFO ("cannot remove element '%s' from bin", GST_OBJECT_NAME (next));
    }
  }

  GST_INFO ("releasing request pad for tee");
  gst_element_release_request_pad (tee, tee_src);
  gst_object_unref (tee_src);
  gst_object_unref (tee);
  gst_object_unref (bin);
  g_list_free (elements);

  return GST_PAD_PROBE_REMOVE;
}

/**
 * bt_bin_deactivate_tee_chain:
 * @bin: the bin
 * @tee: the tee to connect the chain to
 * @elements: (element-type Gst.Element): the list of elements to deactivate
 * @is_playing: wheter the pipeline is streaming data
 *
 * Add the @elements to the @bin and link them. Handle pad blocking in playing
 * mode.
 *
 * Return: %TRUE for success
 */
gboolean
bt_bin_deactivate_tee_chain (GstBin * bin, GstElement * tee, GList * elements,
    gboolean is_playing)
{
  gboolean res = FALSE;
  GstPad *tee_src;

  g_return_val_if_fail (GST_IS_BIN (bin), FALSE);
  g_return_val_if_fail (GST_IS_ELEMENT (tee), FALSE);
  g_return_val_if_fail (elements, FALSE);

  GST_INFO ("remove analyzers (%d elements)", g_list_length (elements));

  if ((tee_src =
          bt_gst_element_get_sink_peer_pad (GST_ELEMENT (elements->data)))) {
    elements = g_list_copy (elements);
    // block src_pad (of tee)
    if (is_playing) {
      gst_pad_add_probe (tee_src, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
          async_remove, elements, NULL);
    } else {
      async_remove (tee_src, NULL, elements);
    }
    res = TRUE;
  }
  return res;
}

//-- gst debugging

#if !GST_CHECK_VERSION(1,3,0)
static const gchar *
gst_pad_link_get_name (GstPadLinkReturn ret)
{
  static gchar *link_res_desc[] = {
    "link succeeded",
    "pads have no common grandparent",
    "pad was already linked",
    "pads have wrong direction",
    "pads do not have common format",
    "pads cannot cooperate in scheduling",
    "refused for some reason"
  };
  return link_res_desc[-ret];
}
#endif

/**
 * bt_gst_debug_pad_link_return:
 * @link_res: pad link result
 * @src_pad: the source pad
 * @sink_pad: the sink pad
 *
 * Format a nice debug message from failed pad links.
 *
 * Returns: (transfer none): the message. The returned string has to be used
 * before the can be called again, otherwise the previous reult will be
 * overwritten.
 */
const gchar *
bt_gst_debug_pad_link_return (GstPadLinkReturn link_res, GstPad * src_pad,
    GstPad * sink_pad)
{
  static gchar msg1[5000];
  gchar msg2[4000];

  if (!src_pad || !sink_pad) {
    /* one of the pads is NULL */
    msg2[0] = '\0';
  } else {
    switch (link_res) {
      case GST_PAD_LINK_WRONG_HIERARCHY:{
        GstObject *src_parent = GST_OBJECT_PARENT (src_pad);
        GstObject *sink_parent = GST_OBJECT_PARENT (sink_pad);
        g_snprintf (msg2, sizeof(msg2), " : parent(src) = %s, parent(sink) = %s",
            (src_parent ? GST_OBJECT_NAME (src_parent) : "(NULL)"),
            (sink_parent ? GST_OBJECT_NAME (sink_parent) : "(NULL)"));
        break;
      }
      case GST_PAD_LINK_WAS_LINKED:{
        GstPad *src_peer = src_pad->peer;
        GstPad *sink_peer = sink_pad->peer;
        g_snprintf (msg2, sizeof(msg2), " : peer(src) = %s:%s, peer(sink) = %s:%s",
            GST_DEBUG_PAD_NAME (src_peer), GST_DEBUG_PAD_NAME (sink_peer));
        break;
      }
      case GST_PAD_LINK_WRONG_DIRECTION:{
        static gchar *dir_name[] = { "unknown", "src", "sink" };
        g_snprintf (msg2, sizeof(msg2), " : direction(src) = %s, direction(sink) = %s",
            ((src_pad->direction <
                    3) ? dir_name[src_pad->direction] : "invalid"),
            ((sink_pad->direction <
                    3) ? dir_name[sink_pad->direction] : "invalid"));
        break;
      }
      case GST_PAD_LINK_NOFORMAT:{
        GstCaps *srcc = gst_pad_query_caps (src_pad, NULL);
        GstCaps *sinkc = gst_pad_query_caps (sink_pad, NULL);
        gchar *src_caps = gst_caps_to_string (srcc);
        gchar *sink_caps = gst_caps_to_string (sinkc);
        g_snprintf (msg2, sizeof(msg2), " : caps(src) = %s, caps(sink) = %s", src_caps,
            sink_caps);
        g_free (src_caps);
        g_free (sink_caps);
        gst_caps_unref (srcc);
        gst_caps_unref (sinkc);
        break;
      }
      case GST_PAD_LINK_NOSCHED:
      case GST_PAD_LINK_REFUSED:
      case GST_PAD_LINK_OK:
      default:
        msg2[0] = '\0';
        break;
    }
  }

  g_snprintf (msg1, sizeof(msg1), "%s:%s -> %s:%s : %s%s",
      GST_DEBUG_PAD_NAME (src_pad), GST_DEBUG_PAD_NAME (sink_pad),
      gst_pad_link_get_name (link_res), msg2);

  return msg1;
}

//-- gst element messages

/**
 * bt_gst_level_message_get_aggregated_field:
 * @structure: the message structure
 * @field_name: the field, such as 'decay' or 'peak'
 * @default_value: a default, in the case of inf/nan levels
 *
 * Aggregate the levels per channel and return the averaged level.
 *
 * Returns: the average level field for all channels
 */
gdouble
bt_gst_level_message_get_aggregated_field (const GstStructure * structure,
    const gchar * field_name, gdouble default_value)
{
  guint i, size;
  gdouble sum = 0.0;
  GValueArray *arr;
  GValue *values;

  arr =
      (GValueArray *) g_value_get_boxed (gst_structure_get_value (structure,
          field_name));
  size = arr->n_values;
  values = arr->values;
  for (i = 0; i < size; i++) {
    sum += g_value_get_double (&values[i]);
  }
  if (G_UNLIKELY (isinf (sum) || isnan (sum))) {
    //GST_WARNING("level.%s was INF or NAN, %lf",field_name,sum);
    return default_value;
  }
  return (sum / size);
}

/**
 * bt_gst_analyzer_get_waittime:
 * @analyzer: the analyzer
 * @structure: the message data
 * @endtime_is_running_time: some elements (level) report endtime as running
 *   time and therefore need segment correction
 *
 * Get the time to wait for audio corresponding to the analyzed data to be
 * rendered.
 *
 * Returns: the wait time in ns.
 */
GstClockTime
bt_gst_analyzer_get_waittime (GstElement * analyzer,
    const GstStructure * structure, gboolean endtime_is_running_time)
{
  GstClockTime timestamp, duration;
  GstClockTime waittime = GST_CLOCK_TIME_NONE;

  if (gst_structure_get_clock_time (structure, "running-time", &timestamp)
      && gst_structure_get_clock_time (structure, "duration", &duration)) {
    /* wait for start of buffer (middle of buffer), this helps a bit as we also
     * have a little extra delay from thread switching
     */
    waittime = timestamp /*+ (duration / 2) */ ;
  } else if (gst_structure_get_clock_time (structure, "endtime", &timestamp)) {
    /* level send endtime as stream_time and not as running_time */
    if (endtime_is_running_time) {
      waittime =
          gst_segment_to_running_time (&GST_BASE_TRANSFORM (analyzer)->segment,
          GST_FORMAT_TIME, timestamp);
    } else {
      waittime = timestamp;
    }
  }
  return waittime;
}

/**
 * bt_gst_log_message_error:
 * @cat: category
 * @file: source file
 * @func: function name
 * @line: source code line
 * @msg: a #GstMessage of type %GST_MESSAGE_ERROR
 * @err_msg: optional output variable for the error message
 * @err_desc: optional output variable for the error description
 *
 * Low level helper that logs the message to the debug log. If @description is
 * given, it is set to the error detail (free after use).
 *
 * Use the #BT_GST_LOG_MESSAGE_ERROR() macro instead.
 *
 * Returns: %TRUE if the message could be parsed
 */
gboolean
bt_gst_log_message_error (GstDebugCategory * cat, const gchar * file,
    const gchar * func, const int line, GstMessage * msg, gchar ** err_msg,
    gchar ** err_desc)
{
  GError *err;
  gchar *desc, *dbg = NULL;

  g_return_val_if_fail (msg, FALSE);
  g_return_val_if_fail (msg->type == GST_MESSAGE_ERROR, FALSE);

  gst_message_parse_error (msg, &err, &dbg);
  desc = gst_error_get_message (err->domain, err->code);
  gst_debug_log (cat, GST_LEVEL_WARNING, file, func, line,
      (GObject *) GST_MESSAGE_SRC (msg), "ERROR: %s (%s) (%s)",
      err->message, desc, (dbg ? dbg : "no debug"));

  if (err_msg)
    *err_msg = g_strdup (err->message);
  if (err_desc)
    *err_desc = desc;
  else
    g_free (desc);
  g_error_free (err);
  g_free (dbg);
  return TRUE;
}

/**
 * bt_gst_log_message_warning:
 * @cat: category
 * @file: source file
 * @func: function name
 * @line: source code line
 * @msg: a #GstMessage of type %GST_MESSAGE_ERROR
 * @warn_msg: optional output variable for the error message
 * @warn_desc: optional output variable for the error description
 *
 * Low level helper that logs the message to the debug log. If @description is
 * given, it is set to the error detail (free after use).
 *
 * Use the #BT_GST_LOG_MESSAGE_WARNING() macro instead.
 *
 * Returns: %TRUE if the message could be parsed
 */
gboolean
bt_gst_log_message_warning (GstDebugCategory * cat, const gchar * file,
    const gchar * func, const int line, GstMessage * msg, gchar ** warn_msg,
    gchar ** warn_desc)
{
  GError *err;
  gchar *desc, *dbg = NULL;

  g_return_val_if_fail (msg, FALSE);
  g_return_val_if_fail (msg->type == GST_MESSAGE_WARNING, FALSE);

  gst_message_parse_warning (msg, &err, &dbg);
  desc = gst_error_get_message (err->domain, err->code);
  gst_debug_log (cat, GST_LEVEL_WARNING, file, func, line,
      (GObject *) GST_MESSAGE_SRC (msg), "ERROR: %s (%s) (%s)",
      err->message, desc, (dbg ? dbg : "no debug"));

  if (warn_msg)
    *warn_msg = g_strdup (err->message);
  if (warn_desc)
    *warn_desc = desc;
  else
    g_free (desc);
  g_error_free (err);
  g_free (dbg);
  return TRUE;
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
GType
bt_g_type_get_base_type (GType type)
{
  GType base;

  while ((base = g_type_parent (type)))
    type = base;
  return type;
}

/*
 * on_object_weak_unref:
 * @user_data: the #GSource id
 * @obj: the old #GObject
 *
 * Use this with g_idle_add to detach handlers when the object is disposed.
 */
static void
on_object_weak_unref (gpointer user_data, GObject * obj)
{
  g_source_remove (GPOINTER_TO_UINT (user_data));
}

/**
 * bt_g_object_idle_add:
 * @obj: the old #GObject
 * @pri: the priority of the idle source, e.g. %G_PRIORITY_DEFAULT_IDLE
 * @func: (scope async): the callback
 *
 * A g_idle_add_full() variant, that passes @obj as user_data and detaches the
 * handler when @obj gets destroyed.
 *
 * Returns: the handler id
 */
guint
bt_g_object_idle_add (GObject * obj, gint pri, GSourceFunc func)
{
  guint id = g_idle_add_full (pri, func, obj, NULL);

  g_object_weak_ref (obj, on_object_weak_unref, GUINT_TO_POINTER (id));
  return id;
}

/**
 * bt_g_signal_connect_object:
 * @instance: (type GObject.Object): the instance to connect to.
 * @detailed_signal: a string of the form "signal-name::detail".
 * @c_handler: (scope async): the GCallback to connect.
 * @data: data to pass to c_handler calls.
 * @connect_flags: a combination of GConnectFlags
 *
 * Like g_signal_connect_object(), but checks first if the handler is already
 * connected.
 *
 * Returns: the handler id
 */
gulong
bt_g_signal_connect_object (gpointer instance, const gchar * detailed_signal,
    GCallback c_handler, gpointer data, GConnectFlags connect_flags)
{
  gulong id = g_signal_handler_find (instance,
      G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA, 0, 0, NULL, c_handler, data);
  if (!id) {
    id = g_signal_connect_object (instance, detailed_signal, c_handler, data,
        connect_flags);
  }
  return id;
}

//-- cpu load monitoring

static GstClockTime treal_last = G_GINT64_CONSTANT (0), tuser_last =
G_GINT64_CONSTANT (0), tsys_last = G_GINT64_CONSTANT (0);
//long clk=1;
//long num_cpus;

#if WIN32
#include <windows.h>            //For GetSystemInfo
#endif


/*
 * bt_cpu_load_init:
 *
 * Initializes cpu usage monitoring.
 */
static void GST_GNUC_CONSTRUCTOR
bt_cpu_load_init (void)
{
  treal_last = gst_util_get_timestamp ();
  //clk=sysconf(_SC_CLK_TCK);

  /* TODO(ensonic): we might need to handle multi core cpus and divide by num-cores
   * http://lxr.linux.no/linux/include/linux/cpumask.h
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
 * Determines the current CPU load. Run this from a timeout handler (with e.g. a
 * 1 second interval).
 *
 * Returns: CPU usage as integer ranging from 0% to 100%
 */
guint
bt_cpu_load_get_current (void)
{
  struct rusage rus;
  GstClockTime tnow, treal, tuser, tsys;
  guint cpuload;

  // check real time elapsed
  tnow = gst_util_get_timestamp ();
  treal = tnow - treal_last;
  treal_last = tnow;
  // check time spent in our process
  getrusage (RUSAGE_SELF, &rus);
  /* children are child processes, which we don't have
   * getrusage(RUSAGE_CHILDREN,&ruc); */
  tnow = GST_TIMEVAL_TO_TIME (rus.ru_utime);
  tuser = tnow - tuser_last;
  tuser_last = tnow;
  tnow = GST_TIMEVAL_TO_TIME (rus.ru_stime);
  tsys = tnow - tsys_last;
  tsys_last = tnow;
  // percentage
  cpuload =
      (guint) gst_util_uint64_scale (tuser + tsys, G_GINT64_CONSTANT (100),
      treal);
  cpuload = (cpuload < 100) ? cpuload : 100;
  GST_LOG ("real %" GST_TIME_FORMAT ", user %" GST_TIME_FORMAT ", sys %"
      GST_TIME_FORMAT " => cpuload %d", GST_TIME_ARGS (treal),
      GST_TIME_ARGS (tuser), GST_TIME_ARGS (tsys), cpuload);
  //printf("user %6.4lf, sys %6.4lf\n",(double)tms.tms_utime/(double)clk,(double)tms.tms_stime/(double)clk);

  return cpuload;
}

//-- string formatting helper

/**
 * bt_str_format_uchar:
 * @val: a value
 *
 * Convenience methods, that formats a value to be serialized as a string.
 *
 * Returns: (transfer none): a reference to static memory containing the
 * formatted value.
 */
const gchar *
bt_str_format_uchar (const guchar val)
{
  static gchar str[20];

  g_snprintf (str, sizeof(str), "%u", val);
  return str;
}

/**
 * bt_str_format_long:
 * @val: a value
 *
 * Convenience methods, that formats a value to be serialized as a string.
 *
 * Returns: (transfer none): a reference to static memory containing the
 * formatted value.
 */
const gchar *
bt_str_format_long (const glong val)
{
  static gchar str[20];

  g_snprintf (str, sizeof(str), "%ld", val);
  return str;
}

/**
 * bt_str_format_ulong:
 * @val: a value
 *
 * Convenience methods, that formats a value to be serialized as a string.
 *
 * Returns: (transfer none): a reference to static memory containing the
 * formatted value.
 */
const gchar *
bt_str_format_ulong (const gulong val)
{
  static gchar str[20];

  g_snprintf (str, sizeof(str), "%lu", val);
  return str;
}

/**
 * bt_str_format_double:
 * @val: a value
 *
 * Convenience methods, that formats a value to be serialized as a string.
 *
 * Returns: (transfer none): a reference to static memory containing the
 * formatted value.
 */
const gchar *
bt_str_format_double (const gdouble val)
{
  static gchar str[G_ASCII_DTOSTR_BUF_SIZE + 1];

  g_ascii_dtostr (str, G_ASCII_DTOSTR_BUF_SIZE, val);
  return str;
}

/**
 * bt_str_format_enum:
 * @enum_type: the #GType for the enum
 * @value: the integer value for the enum
 *
 * Convenience methods, that formats a value to be serialized as a string.
 *
 * Returns: (transfer none): a reference to static memory containing the
 * formatted value.
 */
const gchar *
bt_str_format_enum (GType enum_type, gint value)
{
  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);

  GEnumClass *enum_class = g_type_class_ref (enum_type);
  GEnumValue *enum_value = g_enum_get_value (enum_class, value);
  g_type_class_unref (enum_class);
  return (enum_value ? enum_value->value_nick : NULL);
}

// string parsing helper

/**
 * bt_str_parse_enum:
 * @enum_type: the #GType for the enum
 * @str: the enum value name
 *
 * Convenience methods, that parses a enum value nick and to get the
 * corresponding integer value.
 *
 * Returns: the integer value for the enum, or -1 for invalid strings.
 */
gint
bt_str_parse_enum (GType enum_type, const gchar * str)
{
  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), -1);

  if (!str)
    return -1;

  GEnumClass *enum_class = g_type_class_ref (enum_type);
  GEnumValue *enum_value = g_enum_get_value_by_nick (enum_class, str);
  g_type_class_unref (enum_class);
  return (enum_value ? enum_value->value : -1);
}

//-- gvalue helper

/**
 * bt_str_parse_gvalue:
 * @gvalue: a #GValue
 * @svalue: the string representation of the value to store
 *
 * Stores the supplied value into the given @gvalue.
 *
 * Returns: %TRUE for success
 */
gboolean
bt_str_parse_gvalue (GValue * const gvalue, const gchar * svalue)
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
      return FALSE;
  }
  return res;
}

/**
 * bt_str_format_gvalue:
 * @gvalue: the event cell
 *
 * Returns the string representation of the given @gvalue. Free it when done.
 *
 * Returns: (transfer full): a newly allocated string with the data or %NULL on
 * error
 */
gchar *
bt_str_format_gvalue (GValue * const gvalue)
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
      return NULL;
  }
  return res;
}
