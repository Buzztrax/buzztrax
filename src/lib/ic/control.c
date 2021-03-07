/* Buzztrax
 * Copyright (C) 2007 Buzztrax team <buzztrax-devel@buzztrax.org>
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
 * SECTION:bticcontrol
 * @short_description: buzztraxs interaction controller single control
 *
 * Abstract base class for controls.
 */
/* TODO(ensonic): need flags to quickly filter e.g. for the controller menu
 * - BTIC_CONTROL_ABS,BTIC_CONTROL_TRIGGER,... checking types right now
 * - keyboard, trigger pad: to know which ones are useful for live-play
 *   - a range control can be a slider/knob (not instant) or
 *     a velocity, aftertouch from a pad/key (instant)
 *     - 'instant' it is implicit for note-on/off
 *   - maybe add this as a flag for range-controls and let the user-override
 *     - e.g. for a ribbon-controller (AN-1x) we might see a normal controller,
 *       but one can tap it anywhere
 */
/* TODO(ensonic): new subclasses:
 * - BTIC_CONTROL_KEY (midi keyboard, computer keyboard)
 *   - value would be a guint with the key-number (see GSTBT_NOTE_*)
 *   - from midi, we get key-press and release events for each key
 *   - if a midi keyboard is polyphonic, should it register multiple controls?
 *     - how large can polyphony be?
 *     - also what if we want to use this on a monophonic synth
 *     - if we register multiple controls they could have a shared group-id
 *   - when we bind it, we need to smartly assign the controls to voices
 *     - if a synth has 4 voices we assign key0->voice0, key1->voice1, ...
 *     - a max number of e.g. 16 voices shold be enough in practise (given that
 *       one has 10 fingers)
 *    - the device class would implement the voice allocation
 *      - e.g. round-robin, ev. take key-release or velocity into account
 */
/* TODO(ensonic): virtual devices:
 * we could have virtual devices that remap controls from other devices:
 * - define key-zones
 *   if the device is called "midi", the virtual ones could be named
 *   "midi(C-0 B-1)" and "midi(C-1 B-2)"
 * - scaled range controls (offset + factor)
 */
#define BTIC_CORE
#define BTIC_CONTROL_C

#include "ic_private.h"

enum
{
  CONTROL_DEVICE = 1,
  CONTROL_NAME,
  CONTROL_ID,
  CONTROL_BOUND
};

struct _BtIcControlPrivate
{
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  BtIcDevice *device;
  gchar *name;
  guint id;
  gboolean bound;
};

//-- the class

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (BtIcControl, btic_control, G_TYPE_OBJECT, G_ADD_PRIVATE(BtIcControl));

//-- helper

//-- handler

//-- constructor methods

//-- methods

//-- wrapper

//-- class internals

static void
btic_control_get_property (GObject * const object, const guint property_id,
    GValue * const value, GParamSpec * const pspec)
{
  const BtIcControl *const self = BTIC_CONTROL (object);
  return_if_disposed ();
  switch (property_id) {
    case CONTROL_DEVICE:
      g_value_set_object (value, self->priv->device);
      break;
    case CONTROL_NAME:
      g_value_set_string (value, self->priv->name);
      break;
    case CONTROL_ID:
      g_value_set_uint (value, self->priv->id);
      break;
    case CONTROL_BOUND:
      g_value_set_boolean (value, self->priv->bound);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
btic_control_set_property (GObject * const object, const guint property_id,
    const GValue * const value, GParamSpec * const pspec)
{
  const BtIcControl *const self = BTIC_CONTROL (object);
  return_if_disposed ();
  switch (property_id) {
    case CONTROL_DEVICE:
      self->priv->device = BTIC_DEVICE (g_value_get_object (value));
      g_object_try_weak_ref (self->priv->device);
      break;
    case CONTROL_NAME:{
      gchar *new_name = g_value_dup_string (value);
      if (!self->priv->name) {
        self->priv->name = new_name;
        GST_DEBUG ("setting initial name '%s'!", self->priv->name);
      } else {
        if (BTIC_IS_LEARN (self->priv->device)) {
          gchar *old_name = self->priv->name;
          self->priv->name = new_name;
          GST_DEBUG ("updating name '%s'!", self->priv->name);
          btic_learn_store_controller_map (BTIC_LEARN (self->priv->device));
          g_free (old_name);
        } else {
          GST_WARNING ("can't change control name '%s'!", self->priv->name);
          g_free (new_name);
        }
      }
      break;
    }
    case CONTROL_ID:
      self->priv->id = g_value_get_uint (value);
      break;
    case CONTROL_BOUND:
      self->priv->bound = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
btic_control_dispose (GObject * const object)
{
  const BtIcControl *const self = BTIC_CONTROL (object);

  return_if_disposed ();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG ("!!!! self=%p", self);

  g_object_try_weak_unref (self->priv->device);

  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (btic_control_parent_class)->dispose (object);
  GST_DEBUG ("  done");
}

static void
btic_control_finalize (GObject * const object)
{
  const BtIcControl *const self = BTIC_CONTROL (object);

  GST_DEBUG ("!!!! self=%p", self);

  g_free (self->priv->name);

  GST_DEBUG ("  chaining up");
  G_OBJECT_CLASS (btic_control_parent_class)->finalize (object);
  GST_DEBUG ("  done");
}

static void
btic_control_init (BtIcControl * self)
{
  self->priv = btic_control_get_instance_private(self);
}

static void
btic_control_class_init (BtIcControlClass * const klass)
{
  GObjectClass *const gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = btic_control_set_property;
  gobject_class->get_property = btic_control_get_property;
  gobject_class->dispose = btic_control_dispose;
  gobject_class->finalize = btic_control_finalize;

  g_object_class_install_property (gobject_class, CONTROL_DEVICE,
      g_param_spec_object ("device", "device prop", "parent device object",
          BTIC_TYPE_DEVICE,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, CONTROL_NAME,
      g_param_spec_string ("name", "name prop", "control name", NULL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, CONTROL_ID,
      g_param_spec_uint ("id", "id prop", "control id (for lookups)",
          0, G_MAXUINT, 0,
          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, CONTROL_BOUND,
      g_param_spec_boolean ("bound", "bound prop", "wheter a control is in use",
          FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}
