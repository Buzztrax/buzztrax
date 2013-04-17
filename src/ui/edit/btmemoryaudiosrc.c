/* Buzztard
 * Copyright (C) 2008 Buzztard team <buzztard-devel@lists.sf.net>
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
 * SECTION:btmemoryaudiosrc
 * @short_description: play samples from memory
 *
 * GStreamer audio source to play samples from memory. Supports interleaved
 * multi-channels audio. Supports trickmode playback (resampling and ping-pong
 * loops).
 */
/* seeking
                              forward    backward
   start =  0     
loop_beg =  3       n_samples=3          =15-7= 8
loop_end =  7  n_samples_stop=7          =15-3=12
  length = 15
*/

/* TODO(ensonic): we could also use giostreamsrc + a GMemoryInputStream.
 * See this example here:
 * http://cgit.freedesktop.org/gstreamer/gst-plugins-base/tree/tests/check/pipelines/gio.c#n60
 *
 * - or port to baseaudiosrc (see to focus on hardware sources)
 * - or audiosynth
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "btmemoryaudiosrc.h"
#include "tools.h"

GST_DEBUG_CATEGORY_STATIC (memory_audio_src_debug);
#define GST_CAT_DEFAULT memory_audio_src_debug

enum
{
  PROP_0,
  PROP_CAPS,
  PROP_DATA,
  PROP_LENGTH
};


static GstStaticPadTemplate bt_memory_audio_src_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " GST_AUDIO_NE (S16) ", "
        "layout = (string) interleaved, "
        "rate = (int) [ 1, MAX ], " "channels = (int) [1, 2]")
    );


G_DEFINE_TYPE (BtMemoryAudioSrc, bt_memory_audio_src, GST_TYPE_BASE_SRC);


static void bt_memory_audio_src_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void bt_memory_audio_src_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);
static void bt_memory_audio_src_dispose (GObject * object);


static GstCaps *bt_memory_audio_src_get_caps (GstBaseSrc * basesrc,
    GstCaps * filter);
static gboolean bt_memory_audio_src_is_seekable (GstBaseSrc * basesrc);
static gboolean bt_memory_audio_src_do_seek (GstBaseSrc * basesrc,
    GstSegment * segment);
static gboolean bt_memory_audio_src_query (GstBaseSrc * basesrc,
    GstQuery * query);

static GstFlowReturn bt_memory_audio_src_create (GstBaseSrc * basesrc,
    guint64 offset, guint length, GstBuffer ** buffer);

static void
bt_memory_audio_src_class_init (BtMemoryAudioSrcClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *element_class = (GstElementClass *) klass;
  GstBaseSrcClass *gstbasesrc_class = (GstBaseSrcClass *) klass;

  gobject_class = (GObjectClass *) klass;
  gstbasesrc_class = (GstBaseSrcClass *) klass;

  gobject_class->set_property = bt_memory_audio_src_set_property;
  gobject_class->get_property = bt_memory_audio_src_get_property;
  gobject_class->dispose = bt_memory_audio_src_dispose;

  g_object_class_install_property (gobject_class, PROP_CAPS,
      g_param_spec_boxed ("caps", "Output caps",
          "Data format of sample.",
          GST_TYPE_CAPS, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_DATA,
      g_param_spec_pointer ("data", "Sample data",
          "The sample data (interleaved for multi-channel)",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_LENGTH,
      g_param_spec_ulong ("length", "Length in samples",
          "Length of the sample in number of samples",
          0, G_MAXLONG, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gstbasesrc_class->get_caps = GST_DEBUG_FUNCPTR (bt_memory_audio_src_get_caps);
  gstbasesrc_class->is_seekable =
      GST_DEBUG_FUNCPTR (bt_memory_audio_src_is_seekable);
  gstbasesrc_class->do_seek = GST_DEBUG_FUNCPTR (bt_memory_audio_src_do_seek);
  gstbasesrc_class->query = GST_DEBUG_FUNCPTR (bt_memory_audio_src_query);
  gstbasesrc_class->create = GST_DEBUG_FUNCPTR (bt_memory_audio_src_create);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&bt_memory_audio_src_src_template));
  gst_element_class_set_static_metadata (element_class,
      "Memory audio source",
      "Source/Audio",
      "Plays audio from memory", "Stefan Kost <ensonic@users.sf.net>");
}

static void
bt_memory_audio_src_init (BtMemoryAudioSrc * src)
{
  /* some defaults */
  src->samplerate = GST_AUDIO_DEF_RATE;
  src->channels = 1;

  /* we operate in time */
  gst_base_src_set_format (GST_BASE_SRC (src), GST_FORMAT_TIME);
  gst_base_src_set_live (GST_BASE_SRC (src), FALSE);

  src->caps = gst_caps_new_any ();
  // this gives us max 15 position updates per sec.
  src->samples_per_buffer = GST_AUDIO_DEF_RATE / 15;
  src->generate_samples_per_buffer = src->samples_per_buffer;
}

static GstCaps *
bt_memory_audio_src_get_caps (GstBaseSrc * basesrc, GstCaps * filter)
{
  BtMemoryAudioSrc *src = BT_MEMORY_AUDIO_SRC (basesrc);
  GstCaps *res = gst_caps_copy (src->caps);

  if (filter) {
    GstCaps *tmp =
        gst_caps_intersect_full (filter, res, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (res);
    res = tmp;
  }

  GST_INFO_OBJECT (src, "caps: %" GST_PTR_FORMAT, res);

  return res;
}

static gboolean
bt_memory_audio_src_query (GstBaseSrc * basesrc, GstQuery * query)
{
  BtMemoryAudioSrc *src = BT_MEMORY_AUDIO_SRC (basesrc);
  gboolean res = FALSE;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_CONVERT:
    {
      GstFormat src_fmt, dest_fmt;
      gint64 src_val, dest_val;

      gst_query_parse_convert (query, &src_fmt, &src_val, &dest_fmt, &dest_val);
      if (src_fmt == dest_fmt) {
        dest_val = src_val;
        goto done;
      }

      switch (src_fmt) {
        case GST_FORMAT_DEFAULT:
          switch (dest_fmt) {
            case GST_FORMAT_TIME:
              /* samples to time */
              dest_val =
                  gst_util_uint64_scale_int (src_val, GST_SECOND,
                  src->samplerate);
              break;
            default:
              goto error;
          }
          break;
        case GST_FORMAT_TIME:
          switch (dest_fmt) {
            case GST_FORMAT_DEFAULT:
              /* time to samples */
              dest_val =
                  gst_util_uint64_scale_int (src_val, src->samplerate,
                  GST_SECOND);
              break;
            default:
              goto error;
          }
          break;
        default:
          goto error;
      }
    done:
      gst_query_set_convert (query, src_fmt, src_val, dest_fmt, dest_val);
      res = TRUE;
      break;
    }
    default:
      res =
          GST_BASE_SRC_CLASS (bt_memory_audio_src_parent_class)->query (basesrc,
          query);
      break;
  }

  return res;
  /* ERROR */
error:
  {
    GST_WARNING_OBJECT (src, "query %s failed",
        gst_query_type_get_name (GST_QUERY_TYPE (query)));
    return FALSE;
  }
}

static gboolean
bt_memory_audio_src_do_seek (GstBaseSrc * basesrc, GstSegment * segment)
{
  BtMemoryAudioSrc *src = BT_MEMORY_AUDIO_SRC (basesrc);
  GstClockTime time;

  src->rate = segment->rate;

  GST_LOG_OBJECT (src, "rates : %lf %lf -------------------------------",
      segment->rate, segment->applied_rate);

  time = segment->position;
  /* now move to the time indicated */
  src->n_samples =
      gst_util_uint64_scale_int (time, src->samplerate, GST_SECOND);

  if (src->rate > 0.0) {
    segment->time = segment->start;
    src->running_time =
        gst_util_uint64_scale_int (src->n_samples, GST_SECOND, src->samplerate);

    g_assert (src->running_time <= time);

    if (GST_CLOCK_TIME_IS_VALID (segment->stop)) {
      time = segment->stop;
      src->n_samples_stop = gst_util_uint64_scale_int (time, src->samplerate,
          GST_SECOND);
    } else {
      src->n_samples_stop = src->length;
    }
  } else {
    segment->time = segment->stop;
    src->n_samples = src->length - src->n_samples;
    src->running_time =
        gst_util_uint64_scale_int (src->n_samples, GST_SECOND, src->samplerate);

    if (GST_CLOCK_TIME_IS_VALID (segment->start)) {
      time = segment->start;
      src->n_samples_stop =
          src->length - gst_util_uint64_scale_int (time, src->samplerate,
          GST_SECOND);
    } else {
      src->n_samples_stop = src->length;
    }
  }
  src->eos_reached = FALSE;

  GST_LOG_OBJECT (src, "range : %" G_GUINT64_FORMAT " - %" G_GUINT64_FORMAT,
      src->n_samples, src->n_samples_stop);

  return TRUE;
}

static gboolean
bt_memory_audio_src_is_seekable (GstBaseSrc * basesrc)
{
  /* we're seekable... */
  return TRUE;
}

static GstFlowReturn
bt_memory_audio_src_create (GstBaseSrc * basesrc, guint64 offset,
    guint length, GstBuffer ** buffer)
{
  BtMemoryAudioSrc *src = BT_MEMORY_AUDIO_SRC (basesrc);
  GstBuffer *buf;
  gpointer data;
  GstClockTime next_time;
  gint64 n_samples;
  gsize size;

  if (src->eos_reached) {
    GST_WARNING ("have eos");
    return GST_FLOW_EOS;
  }

  /* check for eos */
  if ((src->n_samples_stop > src->n_samples) &&
      (src->n_samples_stop < src->n_samples + src->samples_per_buffer)
      ) {
    /* calculate only partial buffer */
    src->generate_samples_per_buffer =
        (src->n_samples_stop - src->n_samples) * src->channels;
    n_samples = src->n_samples_stop;
    src->eos_reached = TRUE;
  } else {
    /* calculate full buffer */
    src->generate_samples_per_buffer = src->samples_per_buffer * src->channels;
    n_samples = src->n_samples + src->samples_per_buffer;
  }
  next_time = gst_util_uint64_scale (n_samples, GST_SECOND,
      (guint64) src->samplerate);

  if (src->rate > 0.0) {
    data = (gpointer) & src->data[src->n_samples * src->channels];
  } else {
    data =
        (gpointer) & src->data[(src->length - src->n_samples) * src->channels];
  }
  size = src->generate_samples_per_buffer * sizeof (gint16);
  buf =
      gst_buffer_new_wrapped_full (GST_MEMORY_FLAG_READONLY, data, size, 0,
      size, NULL, NULL);

  if (src->rate > 0.0) {
    GST_BUFFER_TIMESTAMP (buf) = src->running_time;
    GST_BUFFER_OFFSET (buf) = src->n_samples;
    GST_BUFFER_OFFSET_END (buf) = n_samples;
  } else {
    GstClockTime end_time = gst_util_uint64_scale (src->length, GST_SECOND,
        (guint64) src->samplerate);
    GST_BUFFER_TIMESTAMP (buf) = end_time - src->running_time;
    GST_BUFFER_OFFSET (buf) = src->length - src->n_samples;
    GST_BUFFER_OFFSET_END (buf) = src->length - n_samples;
  }
  GST_BUFFER_DURATION (buf) = next_time - src->running_time;

  GST_LOG_OBJECT (src, "play from ts %" GST_TIME_FORMAT
      " with duration %" GST_TIME_FORMAT ", size %" G_GSIZE_FORMAT
      ", and data %p", GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (buf)),
      GST_TIME_ARGS (GST_BUFFER_DURATION (buf)), size, data);

  src->running_time = next_time;
  src->n_samples = n_samples;

  *buffer = buf;

  return GST_FLOW_OK;
}

static void
bt_memory_audio_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  BtMemoryAudioSrc *src = BT_MEMORY_AUDIO_SRC (object);

  switch (prop_id) {
    case PROP_CAPS:{
      GstCaps *new_caps, *old_caps = src->caps;
      const GstCaps *new_caps_val = gst_value_get_caps (value);

      if (new_caps_val == NULL) {
        new_caps = gst_caps_new_any ();
      } else {
        const GstStructure *structure;

        new_caps = gst_caps_ref ((GstCaps *) new_caps_val);

        structure = gst_caps_get_structure (new_caps, 0);
        gst_structure_get_int (structure, "rate", &src->samplerate);
        gst_structure_get_int (structure, "channels", &src->channels);
      }

      src->caps = new_caps;
      gst_caps_unref (old_caps);
      GST_DEBUG_OBJECT (src, "set new caps %" GST_PTR_FORMAT, new_caps);
    } break;
    case PROP_DATA:
      src->data = g_value_get_pointer (value);
      break;
    case PROP_LENGTH:
      src->length = g_value_get_ulong (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
bt_memory_audio_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  BtMemoryAudioSrc *src = BT_MEMORY_AUDIO_SRC (object);

  switch (prop_id) {
    case PROP_CAPS:
      gst_value_set_caps (value, src->caps);
      break;
    case PROP_DATA:
      g_value_set_pointer (value, src->data);
      break;
    case PROP_LENGTH:
      g_value_set_ulong (value, src->length);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
bt_memory_audio_src_dispose (GObject * object)
{
  BtMemoryAudioSrc *src = BT_MEMORY_AUDIO_SRC (object);

  gst_caps_replace (&src->caps, NULL);

  G_OBJECT_CLASS (bt_memory_audio_src_parent_class)->dispose (object);
}

//-- plugin handling

gboolean
bt_memory_audio_src_plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (memory_audio_src_debug, "memoryaudiosrc", 0,
      "Memory audio source");

  return gst_element_register (plugin, "memoryaudiosrc",
      GST_RANK_NONE, BT_TYPE_MEMORY_AUDIO_SRC);
}
