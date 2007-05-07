/* $Id: midi-device.c,v 1.5 2007-05-07 14:45:40 ensonic Exp $
 *
 * Buzztard
 * Copyright (C) 2007 Buzztard team <buzztard-devel@lists.sf.net>
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/**
 * SECTION:bticmididevice
 * @short_description: buzztards interaction controller midi device
 *
 * Event handling for midi devices.
 */

#define BTIC_CORE
#define BTIC_MIDI_DEVICE_C

#include <libbtic/ic.h>

enum {
  DEVICE_DEVNODE=1
};

struct _BtIcMidiDevicePrivate {
  /* used to validate if dispose has run */
  gboolean dispose_has_run;

  gchar *devnode;
};

static GObjectClass *parent_class=NULL;

//-- helper

//-- handler

//-- constructor methods

/**
 * btic_midi_device_new:
 * @udi: the udi of the device
 * @name: human readable name
 * @devnode: device node in filesystem
 *
 * Create a new instance
 *
 * Returns: the new instance or %NULL in case of an error
 */
BtIcMidiDevice *btic_midi_device_new(const gchar *udi,const gchar *name,const gchar *devnode) {
  BtIcMidiDevice *self=BTIC_MIDI_DEVICE(g_object_new(BTIC_TYPE_MIDI_DEVICE,"udi",udi,"name",name,"devnode",devnode,NULL));
  if(!self) {
    goto Error;
  }
  return(self);
Error:
  g_object_try_unref(self);
  return(NULL);
}

//-- methods

static gboolean btic_midi_device_start(gconstpointer self) {
  // @todo: start the io-loop
  return(FALSE);
}

static gboolean btic_midi_device_stop(gconstpointer self) {
  // @todo: stop the io-loop
  return(FALSE);
}

//-- wrapper

//-- class internals

/* returns a property for the given property_id for this object */
static void btic_midi_device_get_property(GObject      * const object,
                               const guint         property_id,
                               GValue       * const value,
                               GParamSpec   * const pspec)
{
  const BtIcMidiDevice * const self = BTIC_MIDI_DEVICE(object);
  return_if_disposed();
  switch (property_id) {
    case DEVICE_DEVNODE: {
      g_value_set_string(value, self->priv->devnode);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

/* sets the given properties for this object */
static void btic_midi_device_set_property(GObject      * const object,
                              const guint         property_id,
                              const GValue * const value,
                              GParamSpec   * const pspec)
{
  const BtIcMidiDevice * const self = BTIC_MIDI_DEVICE(object);
  return_if_disposed();
  switch (property_id) {
    case DEVICE_DEVNODE: {
      g_free(self->priv->devnode);
      self->priv->devnode = g_value_dup_string(value);
    } break;
    default: {
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
    } break;
  }
}

static void btic_midi_device_dispose(GObject * const object) {
  const BtIcMidiDevice * const self = BTIC_MIDI_DEVICE(object);

  return_if_disposed();
  self->priv->dispose_has_run = TRUE;

  GST_DEBUG("!!!! self=%p, self->ref_ct=%d",self,G_OBJECT(self)->ref_count);

  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(parent_class)->dispose(object);
  GST_DEBUG("  done");
}

static void btic_midi_device_finalize(GObject * const object) {
  const BtIcMidiDevice * const self = BTIC_MIDI_DEVICE(object);

  GST_DEBUG("!!!! self=%p",self);

  g_free(self->priv->devnode);

  GST_DEBUG("  chaining up");
  G_OBJECT_CLASS(parent_class)->finalize(object);
  GST_DEBUG("  done");
}

static void btic_midi_device_init(const GTypeInstance * const instance, gconstpointer const g_class) {
  BtIcMidiDevice * const self = BTIC_MIDI_DEVICE(instance);

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, BTIC_TYPE_MIDI_DEVICE, BtIcMidiDevicePrivate);
}

static void btic_midi_device_class_init(BtIcMidiDeviceClass * const klass) {
  GObjectClass * const gobject_class = G_OBJECT_CLASS(klass);
  BtIcDeviceClass * const bticdevice_class = BTIC_DEVICE_CLASS(klass);

  parent_class=g_type_class_peek_parent(klass);
  g_type_class_add_private(klass,sizeof(BtIcMidiDevicePrivate));

  gobject_class->set_property = btic_midi_device_set_property;
  gobject_class->get_property = btic_midi_device_get_property;
  gobject_class->dispose      = btic_midi_device_dispose;
  gobject_class->finalize     = btic_midi_device_finalize;

  bticdevice_class->start     = btic_midi_device_start;
  bticdevice_class->stop      = btic_midi_device_stop;

  g_object_class_install_property(gobject_class,DEVICE_DEVNODE,
                                  g_param_spec_string("devnode",
                                     "devnode prop",
                                     "device node path",
                                     NULL, /* default value */
                                     G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS));
}

GType btic_midi_device_get_type(void) {
  static GType type = 0;
  if (G_UNLIKELY(type == 0)) {
    const GTypeInfo info = {
      (guint16)(sizeof(BtIcMidiDeviceClass)),
      NULL, // base_init
      NULL, // base_finalize
      (GClassInitFunc)btic_midi_device_class_init, // class_init
      NULL, // class_finalize
      NULL, // class_data
      (guint16)(sizeof(BtIcMidiDevice)),
      0,   // n_preallocs
      (GInstanceInitFunc)btic_midi_device_init, // instance_init
      NULL // value_table
    };
    type = g_type_register_static(BTIC_TYPE_DEVICE,"BtIcMidiDevice",&info,0);
  }
  return type;
}
