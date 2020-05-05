/* Buzztrax
 * Copyright (C) 2015 Stefan Sauer <ensonic@users.sf.net>
 *
 * combine.c: combine/mixing module
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTcombine.hICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
/**
 * SECTION:combine
 * @title: GstBtCombine
 * @include: libgstbuzztrax/combine.h
 * @short_description: combine/mixing module
 *
 * A module that combines 2 buffers using different (#GstBtCombine:combine)
 * modes.
 *
 * # Combine modes
 *
 * Examples of combining a saw-wave with another saw wave of double frequency.
 *
 * ![mix](lt-bt_gst_combine_mix__a+b.svg)
 * ![mul](lt-bt_gst_combine_mul__a*b.svg)
 * ![sub](lt-bt_gst_combine_sub__a-b.svg)
 * ![max](lt-bt_gst_combine_max__max_a,b_.svg)
 * ![min](lt-bt_gst_combine_min__min_a,b_.svg)
 * ![and](lt-bt_gst_combine_and__a_b.svg)
 * ![or](lt-bt_gst_combine_or__a_b.svg)
 * ![xor](lt-bt_gst_combine_xor__a_b.svg)
 * ![fold](lt-bt_gst_combine_fold__a_b.svg)
 */
/* TODO:
 * mul: is ring-mod, also add AM (a*((b+1)/2), see
 * https://en.wikibooks.org/wiki/Sound_Synthesis_Theory/Modulation_Synthesis
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "combine.h"

#define GST_CAT_DEFAULT combine_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

enum
{
  // dynamic class properties
  PROP_COMBINE = 1
};

//-- the class

G_DEFINE_TYPE (GstBtCombine, gstbt_combine, GST_TYPE_OBJECT);

//-- enums

GType
gstbt_combine_type_get_type (void)
{
  static GType type = 0;
  static const GEnumValue enums[] = {
    {GSTBT_COMBINE_MIX, "Mix: A+B", "add both signals (A+B)"},
    {GSTBT_COMBINE_MUL, "Mul: A*B",
        "multiply signals (A*B) (amplitude modulation)"},
    {GSTBT_COMBINE_SUB, "Sub: A-B", "subtract signals (A-B)"},
    {GSTBT_COMBINE_MAX, "Max: Max(A,B)", "max of both signals (max(A,B))"},
    {GSTBT_COMBINE_MIN, "Min: Min(A,B)", "min of both signals (min(A,B))"},
    {GSTBT_COMBINE_AND, "And: A&B", "logical and of both signals (A&B)"},
    {GSTBT_COMBINE_OR, "Or: A|B", "logical or of both signals (A|B)"},
    {GSTBT_COMBINE_XOR, "Xor: A^B", "logical xor of both signals (A^B)"},
    {GSTBT_COMBINE_FOLD, "fold: A?B", "wrap a inside b"},
    {0, NULL, NULL},
  };

  if (G_UNLIKELY (!type)) {
    type = g_enum_register_static ("GstBtCombineType", enums);
  }
  return type;
}

//-- constructor methods

/**
 * gstbt_combine_new:
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
GstBtCombine *
gstbt_combine_new (void)
{
  return GSTBT_COMBINE (g_object_new (GSTBT_TYPE_COMBINE, NULL));
}

//-- private methods

static void
gstbt_combine_mix (GstBtCombine * self, guint ct, gint16 * d1, gint16 * d2)
{
  guint i;
  glong mix;

  for (i = 0; i < ct; i++) {
    mix = ((glong) d1[i] + (glong) d2[i]) >> 1;
    d1[i] = (gint16) CLAMP (mix, G_MININT16, G_MAXINT16);
  }
}

static void
gstbt_combine_mul (GstBtCombine * self, guint ct, gint16 * d1, gint16 * d2)
{
  guint i;
  glong mix;

  for (i = 0; i < ct; i++) {
    mix = ((glong) d1[i] * (glong) d2[i]) >> 15;
    d1[i] = (gint16) CLAMP (mix, G_MININT16, G_MAXINT16);
  }
}

static void
gstbt_combine_sub (GstBtCombine * self, guint ct, gint16 * d1, gint16 * d2)
{
  guint i;
  glong mix;

  for (i = 0; i < ct; i++) {
    mix = ((glong) d1[i] - (glong) d2[i]) >> 1;
    d1[i] = (gint16) CLAMP (mix, G_MININT16, G_MAXINT16);
  }
}

static void
gstbt_combine_max (GstBtCombine * self, guint ct, gint16 * d1, gint16 * d2)
{
  guint i;

  for (i = 0; i < ct; i++) {
    d1[i] = MAX (d1[i], d2[i]);
  }
}

static void
gstbt_combine_min (GstBtCombine * self, guint ct, gint16 * d1, gint16 * d2)
{
  guint i;

  for (i = 0; i < ct; i++) {
    d1[i] = MIN (d1[i], d2[i]);
  }
}

static void
gstbt_combine_and (GstBtCombine * self, guint ct, gint16 * d1, gint16 * d2)
{
  guint i;

  for (i = 0; i < ct; i++) {
    d1[i] &= d2[i];
  }
}

static void
gstbt_combine_or (GstBtCombine * self, guint ct, gint16 * d1, gint16 * d2)
{
  guint i;

  for (i = 0; i < ct; i++) {
    d1[i] |= d2[i];
  }
}

static void
gstbt_combine_xor (GstBtCombine * self, guint ct, gint16 * d1, gint16 * d2)
{
  guint i;

  for (i = 0; i < ct; i++) {
    d1[i] ^= d2[i];
  }
}

static void
gstbt_combine_fold (GstBtCombine * self, guint ct, gint16 * d1, gint16 * d2)
{
  guint i;
  gint16 d2a;

  for (i = 0; i < ct; i++) {
    // we fold around +/- d2
    d2a = abs (d2[i]);
    if (d1[i] > 0) {
      d1[i] = (d1[i] > d2a) ? (d2a - (d1[i] - d2a)) : d1[i];
    } else {
      d1[i] = (d1[i] < -d2a) ? (-d2a - (d1[i] + d2a)) : d1[i];
    }
  }
}

/*
 * gstbt_combine_change_type:
 * Assign combine function
 */
static void
gstbt_combine_change_type (GstBtCombine * self)
{
  switch (self->type) {
    case GSTBT_COMBINE_MIX:
      self->process = gstbt_combine_mix;
      break;
    case GSTBT_COMBINE_MUL:
      self->process = gstbt_combine_mul;
      break;
    case GSTBT_COMBINE_SUB:
      self->process = gstbt_combine_sub;
      break;
    case GSTBT_COMBINE_MAX:
      self->process = gstbt_combine_max;
      break;
    case GSTBT_COMBINE_MIN:
      self->process = gstbt_combine_min;
      break;
    case GSTBT_COMBINE_AND:
      self->process = gstbt_combine_and;
      break;
    case GSTBT_COMBINE_OR:
      self->process = gstbt_combine_or;
      break;
    case GSTBT_COMBINE_XOR:
      self->process = gstbt_combine_xor;
      break;
    case GSTBT_COMBINE_FOLD:
      self->process = gstbt_combine_fold;
      break;
    default:
      GST_ERROR ("invalid combine-type: %d", self->type);
      break;
  }
}

//-- public methods

/**
 * gstbt_combine_trigger:
 * @self: the combine module
 *
 * Reset state. Typically called for new notes.
 */
void
gstbt_combine_trigger (GstBtCombine * self)
{
  self->offset = 0;
}

/**
 * gstbt_combine_process:
 * @self: the oscillator
 * @size: number of sample to filter
 * @d1: buffer with the audio
 * @d2: buffer with the audio
 *
 * Process @size samples of audio from @d1 and @d2. Stores the result into @d1.
 */
void
gstbt_combine_process (GstBtCombine * self, guint size, gint16 * d1,
    gint16 * d2)
{
  gst_object_sync_values ((GstObject *) self, self->offset);
  self->process (self, size, d1, d2);
  self->offset += size;
}

//-- virtual methods

static void
gstbt_combine_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBtCombine *self = GSTBT_COMBINE (object);

  switch (prop_id) {
    case PROP_COMBINE:
      //GST_INFO("change type %d -> %d",g_value_get_enum (value),self->filter);
      self->type = g_value_get_enum (value);
      gstbt_combine_change_type (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_combine_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstBtCombine *self = GSTBT_COMBINE (object);

  switch (prop_id) {
    case PROP_COMBINE:
      g_value_set_enum (value, self->type);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_combine_init (GstBtCombine * self)
{
  self->type = GSTBT_COMBINE_MIX;
  gstbt_combine_change_type (self);
}

static void
gstbt_combine_class_init (GstBtCombineClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "combine",
      GST_DEBUG_FG_WHITE | GST_DEBUG_BG_BLACK, "combine/mixing module");

  gobject_class->set_property = gstbt_combine_set_property;
  gobject_class->get_property = gstbt_combine_get_property;

  // register own properties

  g_object_class_install_property (gobject_class, PROP_COMBINE,
      g_param_spec_enum ("combine", "Combinetype", "Type of combine operation",
          GSTBT_TYPE_COMBINE_TYPE, GSTBT_COMBINE_MIX,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
}
