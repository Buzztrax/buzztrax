/* Buzztard
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

#ifndef BTIC_MIDI_DEVICE_H
#define BTIC_MIDI_DEVICE_H

#include <glib.h>
#include <glib-object.h>

#include "device.h"

/* type macros */

#define BTIC_TYPE_MIDI_DEVICE            (btic_midi_device_get_type ())
#define BTIC_MIDI_DEVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTIC_TYPE_MIDI_DEVICE, BtIcMidiDevice))
#define BTIC_MIDI_DEVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTIC_TYPE_MIDI_DEVICE, BtIcMidiDeviceClass))
#define BTIC_IS_MIDI_DEVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTIC_TYPE_MIDI_DEVICE))
#define BTIC_IS_MIDI_DEVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTIC_TYPE_MIDI_DEVICE))
#define BTIC_MIDI_DEVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTIC_TYPE_MIDI_DEVICE, BtIcMidiDeviceClass))

typedef struct _BtIcMidiDevice BtIcMidiDevice;
typedef struct _BtIcMidiDeviceClass BtIcMidiDeviceClass;
typedef struct _BtIcMidiDevicePrivate BtIcMidiDevicePrivate;

/**
 * BtIcMidiDevice:
 *
 * buzztards interaction controller registry
 */
struct _BtIcMidiDevice {
  const BtIcDevice parent;
  
  /*< private >*/
  BtIcMidiDevicePrivate *priv;
};

struct _BtIcMidiDeviceClass {
  const BtIcDeviceClass parent;
  
};

GType btic_midi_device_get_type(void) G_GNUC_CONST;

BtIcMidiDevice *btic_midi_device_new(const gchar *udi,const gchar *name,const gchar *devnode);

#endif // BTIC_MIDI_DEVICE_H
