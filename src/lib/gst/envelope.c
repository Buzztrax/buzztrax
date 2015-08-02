/* GStreamer
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
 *
 * envelope.c: decay envelope object
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
 * SECTION:envelope
 * @title: GstBtEnvelope
 * @include: libgstbuzztrax/envelope.h
 * @short_description: envelope base class
 *
 * Base class for envelopes. The are specialized control sources. Subclsses
 * provide constructors and configure a #GstInterpolationControlSource according
 * to the parameters given.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "envelope.h"

#define GST_CAT_DEFAULT envelope_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

enum
{
  // class properties
  PROP_LENGTH = 1,
};

//-- the class

G_DEFINE_ABSTRACT_TYPE (GstBtEnvelope, gstbt_envelope,
    GST_TYPE_INTERPOLATION_CONTROL_SOURCE);

//-- constructor methods

//-- private methods

//-- public methods

/**
 * gstbt_envelope_is_running:
 * @self: the envelope
 * @offset: the current offset
 *
 * Checks if the end of the envelop has reached. Can be used to skip audio
 * rendering once the end is reached.
 *
 * Returns: if the envelope is still running
 */
gboolean
gstbt_envelope_is_running (GstBtEnvelope * self, guint64 offset)
{
  return offset < self->length;
}

//-- virtual methods

static void
gstbt_envelope_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBtEnvelope *self = GSTBT_ENVELOPE (object);

  if (self->dispose_has_run)
    return;

  switch (prop_id) {
    case PROP_LENGTH:
      self->length = g_value_get_uint64 (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_envelope_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstBtEnvelope *self = GSTBT_ENVELOPE (object);

  if (self->dispose_has_run)
    return;

  switch (prop_id) {
    case PROP_LENGTH:
      g_value_set_uint64 (value, self->length);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_envelope_dispose (GObject * object)
{
  GstBtEnvelope *self = GSTBT_ENVELOPE (object);

  if (self->dispose_has_run)
    return;
  self->dispose_has_run = TRUE;

  G_OBJECT_CLASS (gstbt_envelope_parent_class)->dispose (object);
}

static void
gstbt_envelope_init (GstBtEnvelope * self)
{
  self->length = G_GUINT64_CONSTANT (0);
  g_object_set (self, "mode", GST_INTERPOLATION_MODE_LINEAR, NULL);

}

static void
gstbt_envelope_class_init (GstBtEnvelopeClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "envelope",
      GST_DEBUG_FG_WHITE | GST_DEBUG_BG_BLACK, "parameter envelope");

  gobject_class->set_property = gstbt_envelope_set_property;
  gobject_class->get_property = gstbt_envelope_get_property;
  gobject_class->dispose = gstbt_envelope_dispose;

  // register own properties

  g_object_class_install_property (gobject_class, PROP_LENGTH,
      g_param_spec_uint64 ("length", "Length",
          "Length of the envelope in samples", 0, G_MAXUINT64, 0,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

}
