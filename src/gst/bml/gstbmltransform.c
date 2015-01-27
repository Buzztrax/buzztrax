/* GStreamer
 * Copyright (C) 2005 Stefan Kost <ensonic at users.sf.net>
 *
 * gstbmltransform.c: BML transform plugin
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

// see also: gstidentity, gstvolume, gstlevel

#include "plugin.h"

#define GST_CAT_DEFAULT bml_debug
GST_DEBUG_CATEGORY_EXTERN (GST_CAT_DEFAULT);

static GstStaticPadTemplate bml_pad_caps_mono_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " GST_AUDIO_NE (F32) ", "
        "layout = (string) interleaved, "
        "rate = (int) [ 1, MAX ], " "channels = (int) 1")
    );

static GstStaticPadTemplate bml_pad_caps_stereo_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " GST_AUDIO_NE (F32) ", "
        "layout = (string) interleaved, "
        "rate = (int) [ 1, MAX ], " "channels = (int) 2")
    );

static GstStaticPadTemplate bml_pad_caps_mono_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " GST_AUDIO_NE (F32) ", "
        "layout = (string) interleaved, "
        "rate = (int) [ 1, MAX ], " "channels = (int) 1")
    );
static GstStaticPadTemplate bml_pad_caps_stereo_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw, "
        "format = (string) " GST_AUDIO_NE (F32) ", "
        "layout = (string) interleaved, "
        "rate = (int) [ 1, MAX ], " "channels = (int) 2")
    );

static GstBaseTransformClass *parent_class = NULL;

//-- child bin interface implementations


//-- child proxy interface implementations

static GObject *
gst_bml_child_proxy_get_child_by_index (GstChildProxy * child_proxy,
    guint index)
{
  GstBMLTransform *bml_transform = GST_BML_TRANSFORM (child_proxy);
  GstBML *bml = GST_BML (bml_transform);

  g_return_val_if_fail (index < bml->num_voices, NULL);

  return (gst_object_ref (g_list_nth_data (bml->voices, index)));
}

static guint
gst_bml_child_proxy_get_children_count (GstChildProxy * child_proxy)
{
  GstBMLTransform *bml_transform = GST_BML_TRANSFORM (child_proxy);
  GstBML *bml = GST_BML (bml_transform);

  return (bml->num_voices);
}


static void
gst_bml_child_proxy_interface_init (gpointer g_iface, gpointer iface_data)
{
  GstChildProxyInterface *iface = g_iface;

  GST_INFO ("initializing iface");

  iface->get_child_by_index = gst_bml_child_proxy_get_child_by_index;
  iface->get_children_count = gst_bml_child_proxy_get_children_count;
}

//-- property meta interface implementations

static gchar *
gst_bml_property_meta_describe_property (GstBtPropertyMeta * property_meta,
    guint prop_id, const GValue * value)
{
  GstBMLTransform *bml_transform = GST_BML_TRANSFORM (property_meta);
  GstBML *bml = GST_BML (bml_transform);
  GstBMLTransformClass *klass = GST_BML_TRANSFORM_GET_CLASS (bml_transform);
  GstBMLClass *bml_class = GST_BML_CLASS (klass);

  return (bml (gstbml_property_meta_describe_property (bml_class, bml,
              prop_id, value)));
}

static void
gst_bml_property_meta_interface_init (gpointer g_iface, gpointer iface_data)
{
  GstBtPropertyMetaInterface *iface = g_iface;

  GST_INFO ("initializing iface");

  iface->describe_property = gst_bml_property_meta_describe_property;
}

//-- tempo interface implementations

static void
gst_bml_tempo_change_tempo (GstBtTempo * tempo, glong beats_per_minute,
    glong ticks_per_beat, glong subticks_per_tick)
{
  GstBMLTransform *bml_transform = GST_BML_TRANSFORM (tempo);
  GstBML *bml = GST_BML (bml_transform);

  bml (gstbml_tempo_change_tempo (G_OBJECT (bml_transform), bml,
          beats_per_minute, ticks_per_beat, subticks_per_tick));
}

static void
gst_bml_tempo_interface_init (gpointer g_iface, gpointer iface_data)
{
  GstBtTempoInterface *iface = g_iface;

  GST_INFO ("initializing iface");

  iface->change_tempo = gst_bml_tempo_change_tempo;
}

//-- preset interface implementations

static gchar **
gst_bml_preset_get_preset_names (GstPreset * preset)
{
  GstBMLTransform *bml_transform = GST_BML_TRANSFORM (preset);
  GstBMLTransformClass *klass = GST_BML_TRANSFORM_GET_CLASS (bml_transform);
  GstBML *bml = GST_BML (bml_transform);
  GstBMLClass *bml_class = GST_BML_CLASS (klass);

  return (gstbml_preset_get_preset_names (bml, bml_class));
}

static gboolean
gst_bml_preset_load_preset (GstPreset * preset, const gchar * name)
{
  GstBMLTransform *bml_transform = GST_BML_TRANSFORM (preset);
  GstBMLTransformClass *klass = GST_BML_TRANSFORM_GET_CLASS (bml_transform);
  GstBML *bml = GST_BML (bml_transform);
  GstBMLClass *bml_class = GST_BML_CLASS (klass);

  return (gstbml_preset_load_preset (GST_OBJECT (preset), bml, bml_class,
          name));
}

static gboolean
gst_bml_preset_save_preset (GstPreset * preset, const gchar * name)
{
  GstBMLTransform *bml_transform = GST_BML_TRANSFORM (preset);
  GstBMLTransformClass *klass = GST_BML_TRANSFORM_GET_CLASS (bml_transform);
  GstBML *bml = GST_BML (bml_transform);
  GstBMLClass *bml_class = GST_BML_CLASS (klass);

  return (gstbml_preset_save_preset (GST_OBJECT (preset), bml, bml_class,
          name));
}

static gboolean
gst_bml_preset_rename_preset (GstPreset * preset, const gchar * old_name,
    const gchar * new_name)
{
  GstBMLTransform *bml_transform = GST_BML_TRANSFORM (preset);
  GstBMLTransformClass *klass = GST_BML_TRANSFORM_GET_CLASS (bml_transform);
  GstBMLClass *bml_class = GST_BML_CLASS (klass);

  return (gstbml_preset_rename_preset (bml_class, old_name, new_name));
}

static gboolean
gst_bml_preset_delete_preset (GstPreset * preset, const gchar * name)
{
  GstBMLTransform *bml_transform = GST_BML_TRANSFORM (preset);
  GstBMLTransformClass *klass = GST_BML_TRANSFORM_GET_CLASS (bml_transform);
  GstBMLClass *bml_class = GST_BML_CLASS (klass);

  return (gstbml_preset_delete_preset (bml_class, name));
}

static gboolean
gst_bml_set_meta (GstPreset * preset, const gchar * name, const gchar * tag,
    const gchar * value)
{
  GstBMLTransform *bml_transform = GST_BML_TRANSFORM (preset);
  GstBMLTransformClass *klass = GST_BML_TRANSFORM_GET_CLASS (bml_transform);
  GstBMLClass *bml_class = GST_BML_CLASS (klass);

  return (gstbml_preset_set_meta (bml_class, name, tag, value));
}

static gboolean
gst_bml_get_meta (GstPreset * preset, const gchar * name, const gchar * tag,
    gchar ** value)
{
  GstBMLTransform *bml_transform = GST_BML_TRANSFORM (preset);
  GstBMLTransformClass *klass = GST_BML_TRANSFORM_GET_CLASS (bml_transform);
  GstBMLClass *bml_class = GST_BML_CLASS (klass);

  return (gstbml_preset_get_meta (bml_class, name, tag, value));
}

static void
gst_bml_preset_interface_init (gpointer g_iface, gpointer iface_data)
{
  GstPresetInterface *iface = g_iface;

  GST_INFO ("initializing iface");

  iface->get_preset_names = gst_bml_preset_get_preset_names;
  iface->load_preset = gst_bml_preset_load_preset;
  iface->save_preset = gst_bml_preset_save_preset;
  iface->rename_preset = gst_bml_preset_rename_preset;
  iface->delete_preset = gst_bml_preset_delete_preset;
  iface->set_meta = gst_bml_set_meta;
  iface->get_meta = gst_bml_get_meta;
}

//-- gstbmltransform class implementation

/* get notified of caps and reject unsupported ones */
static gboolean
gst_bml_transform_set_caps (GstBaseTransform * base, GstCaps * incaps,
    GstCaps * outcaps)
{
  GstBMLTransform *bml_transform = GST_BML_TRANSFORM (base);
  GstBML *bml = GST_BML (bml_transform);
  GstStructure *structure;
  gboolean ret;
  gint samplerate = bml->samplerate;

  GST_DEBUG ("set_caps: in %" GST_PTR_FORMAT "  out %" GST_PTR_FORMAT, incaps,
      outcaps);
  structure = gst_caps_get_structure (incaps, 0);

  if ((ret = gst_structure_get_int (structure, "rate", &bml->samplerate))
      && (samplerate != bml->samplerate)) {
    bml (set_master_info (bml->beats_per_minute, bml->ticks_per_beat,
            bml->samplerate));
    // TODO(ensonic): irks, this resets all parameter to their default
    //bml(init(bml->bm,0,NULL));
  }

  return ret;
}

static GstFlowReturn
gst_bml_transform_transform_ip_mono (GstBaseTransform * base,
    GstBuffer * outbuf)
{
  GstMapInfo info;
  GstBMLTransform *bml_transform = GST_BML_TRANSFORM (base);
  GstBMLTransformClass *klass = GST_BML_TRANSFORM_GET_CLASS (bml_transform);
  GstBML *bml = GST_BML (bml_transform);
  GstBMLClass *bml_class = GST_BML_CLASS (klass);
  BMLData *data, *seg_data;
  gpointer bm = bml->bm;
  guint todo, seg_size, samples_per_buffer;
  gboolean has_data;
  guint mode = 3;               /*WM_READWRITE */

  bml->running_time =
      gst_segment_to_stream_time (&base->segment, GST_FORMAT_TIME,
      GST_BUFFER_TIMESTAMP (outbuf));

  if (GST_BUFFER_FLAG_IS_SET (outbuf, GST_BUFFER_FLAG_DISCONT)) {
    bml->subtick_count = (!bml->reverse) ? bml->subticks_per_tick : 1;
  }

  /* TODO(ensonic): sync on subticks ? */
  if (bml->subtick_count >= bml->subticks_per_tick) {
    bml (gstbml_reset_triggers (bml, bml_class));
    bml (gstbml_sync_values (bml, bml_class, GST_BUFFER_TIMESTAMP (outbuf)));
    bml (tick (bm));
    bml->subtick_count = 1;
  } else {
    bml->subtick_count++;
  }

  /* don't process data in passthrough-mode */
  if (gst_base_transform_is_passthrough (base))
    return GST_FLOW_OK;

  if (!gst_buffer_map (outbuf, &info, GST_MAP_READ | GST_MAP_WRITE)) {
    GST_WARNING_OBJECT (base, "unable to map buffer for read & write");
    return GST_FLOW_ERROR;
  }
  data = (BMLData *) info.data;
  samples_per_buffer = info.size / sizeof (BMLData);

  /* if buffer has only silence process with different mode */
  if (GST_BUFFER_FLAG_IS_SET (outbuf, GST_BUFFER_FLAG_GAP)) {
    mode = 2;                   /* WM_WRITE */
  } else {
    // buzz generates loud output
    gfloat fc = 32768.0;
    orc_scalarmultiply_f32_ns (data, data, fc, samples_per_buffer);
  }

  GST_DEBUG_OBJECT (bml_transform, "  calling work(%d,%d)", samples_per_buffer,
      mode);
  todo = samples_per_buffer;
  seg_data = data;
  has_data = FALSE;
  while (todo) {
    // 256 is MachineInterface.h::MAX_BUFFER_LENGTH
    seg_size = (todo > 256) ? 256 : todo;
    has_data |= bml (work (bm, seg_data, (int) seg_size, mode));
    seg_data = &seg_data[seg_size];
    todo -= seg_size;
  }
  if (gstbml_fix_data ((GstElement *) bml_transform, &info, has_data)) {
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_GAP);
  } else {
    GST_BUFFER_FLAG_UNSET (outbuf, GST_BUFFER_FLAG_GAP);
  }

  gst_buffer_unmap (outbuf, &info);

  return (GST_FLOW_OK);
}

static GstFlowReturn
gst_bml_transform_transform_ip_stereo (GstBaseTransform * base,
    GstBuffer * outbuf)
{
  GstMapInfo info;
  GstBMLTransform *bml_transform = GST_BML_TRANSFORM (base);
  GstBMLTransformClass *klass = GST_BML_TRANSFORM_GET_CLASS (bml_transform);
  GstBML *bml = GST_BML (bml_transform);
  GstBMLClass *bml_class = GST_BML_CLASS (klass);
  BMLData *data, *seg_data;
  gpointer bm = bml->bm;
  guint todo, seg_size, samples_per_buffer;
  gboolean has_data;
  guint mode = 3;               /*WM_READWRITE */

  bml->running_time =
      gst_segment_to_stream_time (&base->segment, GST_FORMAT_TIME,
      GST_BUFFER_TIMESTAMP (outbuf));

  if (GST_BUFFER_FLAG_IS_SET (outbuf, GST_BUFFER_FLAG_DISCONT)) {
    bml->subtick_count = (!bml->reverse) ? bml->subticks_per_tick : 1;
  }

  /* TODO(ensonic): sync on subticks ? */
  if (bml->subtick_count >= bml->subticks_per_tick) {
    bml (gstbml_reset_triggers (bml, bml_class));
    bml (gstbml_sync_values (bml, bml_class, GST_BUFFER_TIMESTAMP (outbuf)));
    bml (tick (bm));
    bml->subtick_count = 1;
  } else {
    bml->subtick_count++;
  }

  /* don't process data in passthrough-mode */
  if (gst_base_transform_is_passthrough (base))
    return GST_FLOW_OK;

  if (!gst_buffer_map (outbuf, &info, GST_MAP_READ | GST_MAP_WRITE)) {
    GST_WARNING_OBJECT (base, "unable to map buffer for read & write");
    return GST_FLOW_ERROR;
  }
  data = (BMLData *) info.data;
  samples_per_buffer = info.size / (sizeof (BMLData) * 2);

  /* if buffer has only silence process with different mode */
  if (GST_BUFFER_FLAG_IS_SET (outbuf, GST_BUFFER_FLAG_GAP)) {
    mode = 2;                   /* WM_WRITE */
  } else {
    gfloat fc = 32768.0;
    orc_scalarmultiply_f32_ns (data, data, fc, samples_per_buffer * 2);
  }

  GST_DEBUG_OBJECT (bml_transform, "  calling work_m2s(%d,%d)",
      samples_per_buffer, mode);
  todo = samples_per_buffer;
  seg_data = data;
  has_data = FALSE;
  while (todo) {
    // 256 is MachineInterface.h::MAX_BUFFER_LENGTH
    seg_size = (todo > 256) ? 256 : todo;
    // first seg_data can be NULL, its ignored
    has_data |= bml (work_m2s (bm, seg_data, seg_data, (int) seg_size, mode));
    seg_data = &seg_data[seg_size * 2];
    todo -= seg_size;
  }
  if (gstbml_fix_data ((GstElement *) bml_transform, &info, has_data)) {
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_GAP);
  } else {
    GST_BUFFER_FLAG_UNSET (outbuf, GST_BUFFER_FLAG_GAP);
  }

  gst_buffer_unmap (outbuf, &info);

  return (GST_FLOW_OK);
}

static GstFlowReturn
gst_bml_transform_transform_mono_to_stereo (GstBaseTransform * base,
    GstBuffer * inbuf, GstBuffer * outbuf)
{
  GstMapInfo infoi, infoo;
  GstBMLTransform *bml_transform = GST_BML_TRANSFORM (base);
  GstBMLTransformClass *klass = GST_BML_TRANSFORM_GET_CLASS (bml_transform);
  GstBML *bml = GST_BML (bml_transform);
  GstBMLClass *bml_class = GST_BML_CLASS (klass);
  BMLData *datai, *datao, *seg_datai, *seg_datao;
  gpointer bm = bml->bm;
  guint todo, seg_size, samples_per_buffer;
  gboolean has_data;
  guint mode = 3;               /*WM_READWRITE */

  bml->running_time =
      gst_segment_to_stream_time (&base->segment, GST_FORMAT_TIME,
      GST_BUFFER_TIMESTAMP (inbuf));

  if (GST_BUFFER_FLAG_IS_SET (outbuf, GST_BUFFER_FLAG_DISCONT)) {
    bml->subtick_count = (!bml->reverse) ? bml->subticks_per_tick : 1;
  }

  if (bml->subtick_count >= bml->subticks_per_tick) {
    bml (gstbml_reset_triggers (bml, bml_class));
    bml (gstbml_sync_values (bml, bml_class, GST_BUFFER_TIMESTAMP (outbuf)));
    bml (tick (bm));
    bml->subtick_count = 1;
  } else {
    bml->subtick_count++;
  }

  /* don't process data in passthrough-mode */
  if (gst_base_transform_is_passthrough (base)) {
    // we would actually need to convert mono to stereo here
    // but this is not even called
    GST_WARNING_OBJECT (bml_transform, "m2s in passthrough mode");
    //return GST_FLOW_OK;
  }

  if (!gst_buffer_map (inbuf, &infoi, GST_MAP_READ)) {
    GST_WARNING_OBJECT (base, "unable to map input buffer for read");
    return GST_FLOW_ERROR;
  }
  datai = (BMLData *) infoi.data;
  samples_per_buffer = infoi.size / sizeof (BMLData);
  if (!gst_buffer_map (outbuf, &infoo, GST_MAP_READ | GST_MAP_WRITE)) {
    GST_WARNING_OBJECT (base, "unable to map output buffer for read & write");
    return GST_FLOW_ERROR;
  }
  datao = (BMLData *) infoo.data;

  // some buzzmachines expect a cleared buffer
  //for(i=0;i<samples_per_buffer*2;i++) datao[i]=0.0f;
  memset (datao, 0, samples_per_buffer * 2 * sizeof (BMLData));

  /* if buffer has only silence process with different mode */
  if (GST_BUFFER_FLAG_IS_SET (outbuf, GST_BUFFER_FLAG_GAP)) {
    mode = 2;                   /* WM_WRITE */
  } else {
    gfloat fc = 32768.0;
    orc_scalarmultiply_f32_ns (datai, datai, fc, samples_per_buffer);
  }

  GST_DEBUG_OBJECT (bml_transform, "  calling work_m2s(%d,%d)",
      samples_per_buffer, mode);
  todo = samples_per_buffer;
  seg_datai = datai;
  seg_datao = datao;
  has_data = FALSE;
  while (todo) {
    // 256 is MachineInterface.h::MAX_BUFFER_LENGTH
    seg_size = (todo > 256) ? 256 : todo;
    has_data |= bml (work_m2s (bm, seg_datai, seg_datao, (int) seg_size, mode));
    seg_datai = &seg_datai[seg_size];
    seg_datao = &seg_datao[seg_size * 2];
    todo -= seg_size;
  }
  if (gstbml_fix_data ((GstElement *) bml_transform, &infoo, has_data)) {
    GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_GAP);
  } else {
    GST_BUFFER_FLAG_UNSET (outbuf, GST_BUFFER_FLAG_GAP);
  }

  gst_buffer_unmap (inbuf, &infoi);
  gst_buffer_unmap (outbuf, &infoo);
  return (GST_FLOW_OK);
}

static gboolean
gst_bml_transform_get_unit_size (GstBaseTransform * base, GstCaps * caps,
    gsize * size)
{
  GstAudioInfo info;

  g_assert (size);

  if (!gst_audio_info_from_caps (&info, caps))
    return FALSE;

  *size = GST_AUDIO_INFO_BPF (&info);

  return TRUE;
}

static GstCaps *
gst_bml_transform_transform_caps (GstBaseTransform * base,
    GstPadDirection direction, GstCaps * caps, GstCaps * filter)
{
  GstBMLTransformClass *klass = GST_BML_TRANSFORM_GET_CLASS (base);
  GstBMLClass *bml_class = GST_BML_CLASS (klass);
  GstCaps *res = gst_caps_copy (caps);
  GstStructure *structure;
  gint i, n = gst_caps_get_size (res);

  for (i = 0; i < n; i++) {
    structure = gst_caps_get_structure (res, i);
    /* if we should produce this output, what can we accept */
    if (direction == GST_PAD_SRC) {
      GST_INFO_OBJECT (base, "allow %d input channel",
          bml_class->input_channels);
      gst_structure_set (structure, "channels", G_TYPE_INT,
          bml_class->input_channels, NULL);
      gst_structure_remove_field (structure, "channel-mask");
    } else {
      GST_INFO_OBJECT (base, "allow %d output channels",
          bml_class->output_channels);
      gst_structure_set (structure, "channels", G_TYPE_INT,
          bml_class->output_channels, NULL);
    }
  }

  if (filter) {
    GstCaps *tmp =
        gst_caps_intersect_full (filter, res, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (res);
    res = tmp;
  }

  return res;
}

static gboolean
gst_bml_transform_stop (GstBaseTransform * base)
{
  GstBMLTransform *bml_transform = GST_BML_TRANSFORM (base);
  GstBML *bml = GST_BML (bml_transform);
  gpointer bm = bml->bm;

  bml (stop (bm));
  return TRUE;
}

static void
gst_bml_transform_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBMLTransform *bml_transform = GST_BML_TRANSFORM (object);
  GstBMLTransformClass *klass = GST_BML_TRANSFORM_GET_CLASS (bml_transform);
  GstBML *bml = GST_BML (bml_transform);
  GstBMLClass *bml_class = GST_BML_CLASS (klass);

  bml (gstbml_set_property (bml, bml_class, prop_id, value, pspec));
}

static void
gst_bml_transform_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstBMLTransform *bml_transform = GST_BML_TRANSFORM (object);
  GstBMLTransformClass *klass = GST_BML_TRANSFORM_GET_CLASS (bml_transform);
  GstBML *bml = GST_BML (bml_transform);
  GstBMLClass *bml_class = GST_BML_CLASS (klass);

  bml (gstbml_get_property (bml, bml_class, prop_id, value, pspec));
}

static void
gst_bml_transform_dispose (GObject * object)
{
  GstBMLTransform *bml_transform = GST_BML_TRANSFORM (object);
  GstBML *bml = GST_BML (bml_transform);

  gstbml_dispose (bml);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_bml_transform_finalize (GObject * object)
{
  GstBMLTransform *bml_transform = GST_BML_TRANSFORM (object);
  GstBML *bml = GST_BML (bml_transform);

  bml (gstbml_finalize (bml));

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_bml_transform_base_finalize (GstBMLTransformClass * klass)
{
  GstBMLClass *bml_class = GST_BML_CLASS (klass);

  gstbml_preset_finalize (bml_class);
  bml (gstbml_base_finalize (bml_class));
}

static void
gst_bml_transform_init (GstBMLTransform * bml_transform)
{
  GstBMLTransformClass *klass = GST_BML_TRANSFORM_GET_CLASS (bml_transform);
  GstBMLClass *bml_class = GST_BML_CLASS (klass);
  GstBML *bml = GST_BML (bml_transform);

  GST_INFO ("initializing instance: elem=%p, bml=%p, bml_class=%p",
      bml_transform, bml, bml_class);
  GST_INFO ("bmh=0x%p, src=%d, sink=%d", bml_class->bmh, bml_class->numsrcpads,
      bml_class->numsinkpads);

  bml (gstbml_init (bml, bml_class, GST_ELEMENT (bml_transform)));
  /* this is not nedded when using the base class
     bml(gstbml_init_pads(GST_ELEMENT(bml_transform),bml,gst_bml_transform_link));

     if (sinkcount == 1) {
     // with one sink (input ports) we can use the chain function
     // effects
     GST_DEBUG_OBJECT(bml, "chain mode");
     gst_pad_set_chain_function(bml->sinkpads[0], gst_bml_transform_chain);
     }
     else if (sinkcount > 1) {
     // more than one sink (input ports) pad needs loop mode
     // auxbus based effects
     GST_DEBUG_OBJECT(bml, "loop mode with %d sink pads and %d src pads", sinkcount, srccount);
     gst_element_set_loop_function(GST_ELEMENT(bml_transform), gst_bml_transform_loop);
     }
   */

  gst_base_transform_set_gap_aware (GST_BASE_TRANSFORM (bml_transform), TRUE);

  GST_DEBUG ("  done");
}

static void
gst_bml_transform_class_init (GstBMLTransformClass * klass)
{
  GstBMLClass *bml_class = GST_BML_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseTransformClass *gstbasetransform_class =
      GST_BASE_TRANSFORM_CLASS (klass);

  GST_INFO ("initializing class");
  parent_class = g_type_class_peek_parent (klass);

  // override methods
  gobject_class->set_property =
      GST_DEBUG_FUNCPTR (gst_bml_transform_set_property);
  gobject_class->get_property =
      GST_DEBUG_FUNCPTR (gst_bml_transform_get_property);
  gobject_class->dispose = GST_DEBUG_FUNCPTR (gst_bml_transform_dispose);
  gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_bml_transform_finalize);
  gstbasetransform_class->set_caps =
      GST_DEBUG_FUNCPTR (gst_bml_transform_set_caps);
  gstbasetransform_class->stop = GST_DEBUG_FUNCPTR (gst_bml_transform_stop);
  if (bml_class->output_channels == 1) {
    gstbasetransform_class->transform_ip =
        GST_DEBUG_FUNCPTR (gst_bml_transform_transform_ip_mono);
  } else {
    if (bml_class->input_channels == 1) {
      gstbasetransform_class->transform =
          GST_DEBUG_FUNCPTR (gst_bml_transform_transform_mono_to_stereo);
      gstbasetransform_class->get_unit_size =
          GST_DEBUG_FUNCPTR (gst_bml_transform_get_unit_size);
      gstbasetransform_class->transform_caps =
          GST_DEBUG_FUNCPTR (gst_bml_transform_transform_caps);
    } else {
      gstbasetransform_class->transform_ip =
          GST_DEBUG_FUNCPTR (gst_bml_transform_transform_ip_stereo);
    }
  }

  // override interface properties and register parameters as gobject properties
  bml (gstbml_class_prepare_properties (gobject_class, bml_class));
}

static void
gst_bml_transform_base_init (GstBMLTransformClass * klass)
{
  GstBMLClass *bml_class = GST_BML_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  //GstPadTemplate *templ;
  gpointer bmh;
  static GstPadTemplate *mono_src_pad_template = NULL;
  static GstPadTemplate *stereo_src_pad_template = NULL;
  static GstPadTemplate *mono_sink_pad_template = NULL;
  static GstPadTemplate *stereo_sink_pad_template = NULL;

  GST_INFO ("initializing base");

  bmh =
      bml (gstbml_class_base_init (bml_class, G_TYPE_FROM_CLASS (klass), 1, 1));

  if (bml_class->output_channels == 1) {
    if (G_UNLIKELY (!mono_src_pad_template))
      mono_src_pad_template =
          gst_static_pad_template_get (&bml_pad_caps_mono_src_template);
    gst_element_class_add_pad_template (element_class, mono_src_pad_template);
    GST_INFO ("  added mono src pad template");
  } else {
    if (G_UNLIKELY (!stereo_src_pad_template))
      stereo_src_pad_template =
          gst_static_pad_template_get (&bml_pad_caps_stereo_src_template);
    gst_element_class_add_pad_template (element_class, stereo_src_pad_template);
    GST_INFO ("  added stereo src pad template");
  }
  if (bml_class->input_channels == 1) {
    if (G_UNLIKELY (!mono_sink_pad_template))
      mono_sink_pad_template =
          gst_static_pad_template_get (&bml_pad_caps_mono_sink_template);
    gst_element_class_add_pad_template (element_class, mono_sink_pad_template);
    GST_INFO ("  added mono sink pad template");
  } else {
    if (G_UNLIKELY (!stereo_sink_pad_template))
      stereo_sink_pad_template =
          gst_static_pad_template_get (&bml_pad_caps_stereo_sink_template);
    gst_element_class_add_pad_template (element_class,
        stereo_sink_pad_template);
    GST_INFO ("  added stereo sink pad template");
  }

  bml (gstbml_class_set_details (element_class, bml_class, bmh,
          "Filter/Effect/Audio/BML"));
}


GType
bml (transform_get_type (const char *element_type_name, gboolean is_polyphonic))
{
  const GTypeInfo element_type_info = {
    sizeof (GstBMLTransformClass),
    (GBaseInitFunc) gst_bml_transform_base_init,
    (GBaseFinalizeFunc) gst_bml_transform_base_finalize,
    (GClassInitFunc) gst_bml_transform_class_init,
    NULL,
    NULL,
    sizeof (GstBMLTransform),
    0,
    (GInstanceInitFunc) gst_bml_transform_init,
  };
  const GInterfaceInfo child_proxy_interface_info = {
    (GInterfaceInitFunc) gst_bml_child_proxy_interface_init,    /* interface_init */
    NULL,                       /* interface_finalize */
    NULL                        /* interface_data */
  };
  const GInterfaceInfo child_bin_interface_info = {
    NULL,                       /* interface_init */
    NULL,                       /* interface_finalize */
    NULL                        /* interface_data */
  };
  const GInterfaceInfo property_meta_interface_info = {
    (GInterfaceInitFunc) gst_bml_property_meta_interface_init,  /* interface_init */
    NULL,                       /* interface_finalize */
    NULL                        /* interface_data */
  };
  const GInterfaceInfo tempo_interface_info = {
    (GInterfaceInitFunc) gst_bml_tempo_interface_init,  /* interface_init */
    NULL,                       /* interface_finalize */
    NULL                        /* interface_data */
  };
  const GInterfaceInfo preset_interface_info = {
    (GInterfaceInitFunc) gst_bml_preset_interface_init, /* interface_init */
    NULL,                       /* interface_finalize */
    NULL                        /* interface_data */
  };
  GType element_type;

  GST_INFO ("registering transform-plugin: \"%s\"", element_type_name);

  // create the element type now
  element_type =
      g_type_register_static (GST_TYPE_BASE_TRANSFORM, element_type_name,
      &element_type_info, 0);
  GST_INFO ("succefully registered new type : \"%s\"", element_type_name);
  g_type_add_interface_static (element_type, GSTBT_TYPE_PROPERTY_META,
      &property_meta_interface_info);
  g_type_add_interface_static (element_type, GSTBT_TYPE_TEMPO,
      &tempo_interface_info);

  // check if this plugin can do multiple voices
  if (is_polyphonic) {
    g_type_add_interface_static (element_type, GST_TYPE_CHILD_PROXY,
        &child_proxy_interface_info);
    g_type_add_interface_static (element_type, GSTBT_TYPE_CHILD_BIN,
        &child_bin_interface_info);
  }
  // add presets iface
  g_type_add_interface_static (element_type, GST_TYPE_PRESET,
      &preset_interface_info);

  GST_INFO ("successfully registered type interfaces");

  return (element_type);
}
