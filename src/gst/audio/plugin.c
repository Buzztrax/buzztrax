/* GStreamer
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "audiodelay.h"
#include "simsyn.h"
#include "wavereplay.h"
#include "wavetabsyn.h"

#define GST_CAT_DEFAULT bt_audio_debug
GST_DEBUG_CATEGORY (GST_CAT_DEFAULT);

static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "bt-audio",
      GST_DEBUG_FG_WHITE | GST_DEBUG_BG_BLACK, "buzztrax audio elements");

  return (gst_element_register (plugin, "audiodelay", GST_RANK_NONE,
          GSTBT_TYPE_AUDIO_DELAY) &&
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
