/* Buzztrax
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
 *
 * plugin.c: audio synthesizer elements for gstreamer
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
 * SECTION:plugin
 * @short_description: helpers for composition of gobjects
 *
 * Helper functions to compose gobjects.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "plugin.h"
#include "audiodelay.h"
#include "ebeats.h"
#include "simsyn.h"
#include "wavereplay.h"
#include "wavetabsyn.h"

#define GST_CAT_DEFAULT bt_audio_debug
GST_DEBUG_CATEGORY (GST_CAT_DEFAULT);

// common helpers

/* https://bugzilla.gnome.org/show_bug.cgi?id=744011
 *
 * the problem is that g_object_class_list_properties() will remove the
 * g_param_spec_override() - see pspec_list_remove_overridden_and_redirected()
 * as it also goes up the hierarchy in order to try to supply the redirect
 * target instead. Here the filter_klass is not a parent, nor an interface and
 * thus the pspec is lost.
 */
GParamSpec *
bt_g_param_spec_clone (GObjectClass * src_class, const gchar * src_name)
{
  GParamSpec *pspec;
  GTypeQuery query;
  GParamSpec *src = g_object_class_find_property (src_class, src_name);

  g_return_val_if_fail (src, NULL);

  g_type_query (G_PARAM_SPEC_TYPE (src), &query);
  /* this is pretty lame, we have no idea if we copy e.g. pointer fields */
#if GLIB_CHECK_VERSION(2, 68, 0)
  pspec = g_memdup2 (src, (gsize) query.instance_size);
#else
  pspec = g_memdup (src, query.instance_size);
#endif
  /* reset known fields, see g_param_spec_init() */
  pspec->owner_type = 0;
  pspec->qdata = NULL;
  g_datalist_set_flags (&pspec->qdata, /*PARAM_FLOATING_FLAG */ 0x2);
  pspec->ref_count = 1;
  pspec->param_id = 0;

  if (!(pspec->flags & G_PARAM_STATIC_NICK)) {
    pspec->_nick = g_strdup (pspec->_nick);
  }
  if (!(pspec->flags & G_PARAM_STATIC_BLURB)) {
    pspec->_blurb = g_strdup (pspec->_blurb);
  }

  return pspec;
}

GParamSpec *
bt_g_param_spec_clone_as (GObjectClass * src_class, const gchar * src_name,
    gchar * new_name, gchar * new_nick, gchar * new_blurb)
{
  GParamSpec *pspec = bt_g_param_spec_clone (src_class, src_name);
  gchar *p = new_name;

  g_return_val_if_fail (new_name, NULL);

  // validate
  while (*p != 0) {
    gchar c = *p;

    if (c != '-' && (c < '0' || c > '9') && (c < 'A' || c > 'Z') &&
        (c < 'a' || c > 'z')) {
      g_warning ("non-canonical pspec name: %s", new_name);
      break;
    }
    p++;
  }

  if (pspec->flags & G_PARAM_STATIC_NAME) {
    pspec->name = (gchar *) g_intern_static_string (new_name);
  } else {
    pspec->name = (gchar *) g_intern_string (new_name);
  }
  if (new_nick) {
    if (!(pspec->flags & G_PARAM_STATIC_NICK)) {
      g_free (pspec->_nick);
      pspec->_nick = g_strdup (new_nick);
    } else {
      pspec->_nick = new_nick;
    }
  }
  if (new_blurb) {
    if (!(pspec->flags & G_PARAM_STATIC_BLURB)) {
      g_free (pspec->_nick);
      pspec->_blurb = g_strdup (new_blurb);
    } else {
      pspec->_blurb = new_blurb;
    }
  }
  return pspec;
}

// the plugin

static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "bt-audio",
      GST_DEBUG_FG_WHITE | GST_DEBUG_BG_BLACK, "buzztrax audio elements");

  return (gst_element_register (plugin, "audiodelay", GST_RANK_NONE,
          GSTBT_TYPE_AUDIO_DELAY) &&
      gst_element_register (plugin, "ebeats", GST_RANK_NONE,
          GSTBT_TYPE_E_BEATS) &&
      gst_element_register (plugin, "simsyn", GST_RANK_NONE,
          GSTBT_TYPE_SIM_SYN) &&
      gst_element_register (plugin, "wavereplay", GST_RANK_NONE,
          GSTBT_TYPE_WAVE_REPLAY) &&
      gst_element_register (plugin, "wavetabsyn", GST_RANK_NONE,
          GSTBT_TYPE_WAVE_TAB_SYN));
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    buzztraxaudio,
    "Buzztrax audio elements",
    plugin_init, VERSION, "LGPL", PACKAGE_NAME, PACKAGE_ORIGIN);
