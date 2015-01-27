/* GStreamer
 * Copyright (C) 2012 Stefan Sauer <ensonic@users.sf.net>
 *
 * filter-svf.c: state variable filter
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
 * SECTION:filter-svf
 * @title: GstBtFilterSVF
 * @include: libgstbuzztrax/filter-svf.h
 * @short_description: state variable filter
 *
 * An audio filter that can work in 4 modes (#GstBtFilterSVF:type).
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "filter-svf.h"

#define GST_CAT_DEFAULT envelope_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

enum
{
  // dynamic class properties
  PROP_FILTER = 1,
  PROP_CUTOFF,
  PROP_RESONANCE
};

//-- the class

G_DEFINE_TYPE (GstBtFilterSVF, gstbt_filter_svf, G_TYPE_OBJECT);

//-- enums

GType
gstbt_filter_svf_type_get_type (void)
{
  static GType type = 0;
  static const GEnumValue enums[] = {
    {GSTBT_FILTER_SVF_NONE, "None", "none"},
    {GSTBT_FILTER_SVF_LOWPASS, "LowPass", "lowpass"},
    {GSTBT_FILTER_SVF_HIPASS, "HiPass", "hipass"},
    {GSTBT_FILTER_SVF_BANDPASS, "BandPass", "bandpass"},
    {GSTBT_FILTER_SVF_BANDSTOP, "BandStop", "bandstop"},
    {0, NULL, NULL},
  };

  if (G_UNLIKELY (!type)) {
    type = g_enum_register_static ("GstBtFilterSVFType", enums);
  }
  return type;
}

//-- constructor methods

/**
 * gstbt_filter_svf_new:
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
GstBtFilterSVF *
gstbt_filter_svf_new (void)
{
  return GSTBT_FILTER_SVF (g_object_new (GSTBT_TYPE_FILTER_SVF, NULL));
}

//-- private methods

static void
gstbt_filter_svf_lowpass (GstBtFilterSVF * self, guint ct, gint16 * samples)
{
  guint i;
  gdouble flt_low = self->flt_low;
  gdouble flt_mid = self->flt_mid;
  gdouble flt_high = self->flt_high;
  gdouble flt_res = self->flt_res;
  gdouble cutoff = self->cutoff;

  for (i = 0; i < ct; i++) {
    flt_high = (gdouble) samples[i] - (flt_mid * flt_res) - flt_low;
    flt_mid += (flt_high * cutoff);
    flt_low += (flt_mid * cutoff);

    samples[i] = (gint16) CLAMP ((glong) flt_low, G_MININT16, G_MAXINT16);
  }
  self->flt_low = flt_low;
  self->flt_mid = flt_mid;
  self->flt_high = flt_high;
}

static void
gstbt_filter_svf_hipass (GstBtFilterSVF * self, guint ct, gint16 * samples)
{
  guint i;
  gdouble flt_low = self->flt_low;
  gdouble flt_mid = self->flt_mid;
  gdouble flt_high = self->flt_high;
  gdouble flt_res = self->flt_res;
  gdouble cutoff = self->cutoff;

  for (i = 0; i < ct; i++) {
    flt_high = (gdouble) samples[i] - (flt_mid * flt_res) - flt_low;
    flt_mid += (flt_high * cutoff);
    flt_low += (flt_mid * cutoff);

    samples[i] = (gint16) CLAMP ((glong) flt_high, G_MININT16, G_MAXINT16);
  }
  self->flt_low = flt_low;
  self->flt_mid = flt_mid;
  self->flt_high = flt_high;
}

static void
gstbt_filter_svf_bandpass (GstBtFilterSVF * self, guint ct, gint16 * samples)
{
  guint i;
  gdouble flt_low = self->flt_low;
  gdouble flt_mid = self->flt_mid;
  gdouble flt_high = self->flt_high;
  gdouble flt_res = self->flt_res;
  gdouble cutoff = self->cutoff;

  for (i = 0; i < ct; i++) {
    flt_high = (gdouble) samples[i] - (flt_mid * flt_res) - flt_low;
    flt_mid += (flt_high * cutoff);
    flt_low += (flt_mid * cutoff);

    samples[i] = (gint16) CLAMP ((glong) flt_mid, G_MININT16, G_MAXINT16);
  }
  self->flt_low = flt_low;
  self->flt_mid = flt_mid;
  self->flt_high = flt_high;
}

static void
gstbt_filter_svf_bandstop (GstBtFilterSVF * self, guint ct, gint16 * samples)
{
  guint i;
  gdouble flt_low = self->flt_low;
  gdouble flt_mid = self->flt_mid;
  gdouble flt_high = self->flt_high;
  gdouble flt_res = self->flt_res;
  gdouble cutoff = self->cutoff;

  for (i = 0; i < ct; i++) {
    flt_high = (gdouble) samples[i] - (flt_mid * flt_res) - flt_low;
    flt_mid += (flt_high * cutoff);
    flt_low += (flt_mid * cutoff);

    samples[i] =
        (gint16) CLAMP ((glong) (flt_low + flt_high), G_MININT16, G_MAXINT16);
  }
  self->flt_low = flt_low;
  self->flt_mid = flt_mid;
  self->flt_high = flt_high;
}

/*
 * gstbt_filter_svf_change_filter:
 * Assign filter function
 */
static void
gstbt_filter_svf_change_filter (GstBtFilterSVF * self)
{
  switch (self->type) {
    case GSTBT_FILTER_SVF_NONE:
      self->process = NULL;
      break;
    case GSTBT_FILTER_SVF_LOWPASS:
      self->process = gstbt_filter_svf_lowpass;
      break;
    case GSTBT_FILTER_SVF_HIPASS:
      self->process = gstbt_filter_svf_hipass;
      break;
    case GSTBT_FILTER_SVF_BANDPASS:
      self->process = gstbt_filter_svf_bandpass;
      break;
    case GSTBT_FILTER_SVF_BANDSTOP:
      self->process = gstbt_filter_svf_bandstop;
      break;
    default:
      GST_ERROR ("invalid filter-type: %d", self->type);
      break;
  }
}

//-- public methods

//-- virtual methods

static void
gstbt_filter_svf_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBtFilterSVF *self = GSTBT_FILTER_SVF (object);

  switch (prop_id) {
    case PROP_FILTER:
      //GST_INFO("change filter %d -> %d",g_value_get_enum (value),self->filter);
      self->type = g_value_get_enum (value);
      gstbt_filter_svf_change_filter (self);
      break;
    case PROP_CUTOFF:
      //GST_INFO("change cutoff %lf -> %lf",g_value_get_double (value),self->cutoff);
      self->cutoff = g_value_get_double (value);
      break;
    case PROP_RESONANCE:
      //GST_INFO("change resonance %lf -> %lf",g_value_get_double (value),self->resonance);
      self->resonance = g_value_get_double (value);
      self->flt_res = 1.0 / self->resonance;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_filter_svf_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstBtFilterSVF *self = GSTBT_FILTER_SVF (object);

  switch (prop_id) {
    case PROP_FILTER:
      g_value_set_enum (value, self->type);
      break;
    case PROP_CUTOFF:
      g_value_set_double (value, self->cutoff);
      break;
    case PROP_RESONANCE:
      g_value_set_double (value, self->resonance);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_filter_svf_init (GstBtFilterSVF * self)
{
  self->type = GSTBT_FILTER_SVF_LOWPASS;
  self->cutoff = 0.8;
  self->resonance = 0.8;
  self->flt_res = 1.0 / self->resonance;
  gstbt_filter_svf_change_filter (self);
}

static void
gstbt_filter_svf_class_init (GstBtFilterSVFClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "filter-svf",
      GST_DEBUG_FG_WHITE | GST_DEBUG_BG_BLACK, "state variable filter");

  gobject_class->set_property = gstbt_filter_svf_set_property;
  gobject_class->get_property = gstbt_filter_svf_get_property;

  // register own properties

  g_object_class_install_property (gobject_class, PROP_FILTER,
      g_param_spec_enum ("filter", "Filtertype", "Type of audio filter",
          GSTBT_TYPE_FILTER_SVF_TYPE, GSTBT_FILTER_SVF_LOWPASS,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CUTOFF,
      g_param_spec_double ("cut-off", "Cut-Off",
          "Audio filter cut-off frequency", 0.0, 1.0, 0.8,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_RESONANCE,
      g_param_spec_double ("resonance", "Resonance", "Audio filter resonance",
          0.7, 25.0, 0.8,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
}
