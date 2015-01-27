/* GStreamer
 * Copyright (C) 2014 Stefan Sauer <ensonic@users.sf.net>
 *
 * delay.c: delay line object
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
 * SECTION:delay
 * @title: GstBtDelay
 * @include: libgstbuzztrax/delay.h
 * @short_description: delay line class
 *
 * A delay line. 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "delay.h"

#define GST_CAT_DEFAULT envelope_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

enum
{
  // class properties
  PROP_DELAYTIME = 1
};

//-- the class

G_DEFINE_TYPE (GstBtDelay, gstbt_delay, G_TYPE_OBJECT);

//-- constructor methods

/**
 * gstbt_delay_new:
 *
 * Create a new instance.
 *
 * Returns: the new instance or %NULL in case of an error
 */
GstBtDelay *
gstbt_delay_new (void)
{
  return GSTBT_DELAY (g_object_new (GSTBT_TYPE_DELAY, NULL));
}


//-- private methods

//-- public methods

/**
 * gstbt_delay_start:
 * @self: the delay
 * @samplerate: the new sampling rate
 *
 * Initialize the delay line.
 */
void
gstbt_delay_start (GstBtDelay * self, gint samplerate)
{
  self->samplerate = samplerate;
  self->max_delaytime = (2 + (self->delaytime * samplerate) / 100);
  self->ring_buffer = (gint16 *) g_new0 (gint16, self->max_delaytime);
  self->rb_ptr = 0;
  GST_INFO ("max_delaytime %d at %d Hz sampling rate", self->max_delaytime,
      samplerate);
}

/**
 * gstbt_delay_flush:
 * @self: the delay
 *
 * Zero pending data in the delay.
 */
void
gstbt_delay_flush (GstBtDelay * self)
{
  memset (self->ring_buffer, 0, sizeof (gint16) * self->max_delaytime);
  self->rb_ptr = 0;
}

/**
 * gstbt_delay_stop:
 * @self: the delay
 *
 * Stop and release the delay line.
 */
void
gstbt_delay_stop (GstBtDelay * self)
{
  if (self->ring_buffer) {
    g_free (self->ring_buffer);
    self->ring_buffer = NULL;
  }
}

//-- virtual methods

static void
gstbt_delay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBtDelay *self = GSTBT_DELAY (object);

  switch (prop_id) {
    case PROP_DELAYTIME:
      self->delaytime = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_delay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstBtDelay *self = GSTBT_DELAY (object);

  switch (prop_id) {
    case PROP_DELAYTIME:
      g_value_set_uint (value, self->delaytime);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gstbt_delay_finalize (GObject * object)
{
  GstBtDelay *self = GSTBT_DELAY (object);

  gstbt_delay_stop (self);

  G_OBJECT_CLASS (gstbt_delay_parent_class)->finalize (object);
}

static void
gstbt_delay_init (GstBtDelay * self)
{
  self->delaytime = 100;

  self->ring_buffer = NULL;
}

static void
gstbt_delay_class_init (GstBtDelayClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "delay",
      GST_DEBUG_FG_WHITE | GST_DEBUG_BG_BLACK, "delay line");

  gobject_class->set_property = gstbt_delay_set_property;
  gobject_class->get_property = gstbt_delay_get_property;
  gobject_class->finalize = gstbt_delay_finalize;

  // register own properties

  g_object_class_install_property (gobject_class, PROP_DELAYTIME,
      g_param_spec_uint ("delaytime", "Delay time",
          "Time difference between two echos as milliseconds", 1,
          1000, 100, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));
}
