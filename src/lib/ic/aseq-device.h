/* Buzztrax
 * Copyright (C) 2014 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#ifndef BTIC_ASEQ_DEVICE_H
#define BTIC_ASEQ_DEVICE_H

#include <glib.h>
#include <glib-object.h>
#if USE_ALSA
#include <alsa/asoundlib.h>
#endif

#include "device.h"

/* type macros */

#define BTIC_TYPE_ASEQ_DEVICE            (btic_aseq_device_get_type ())
#define BTIC_ASEQ_DEVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTIC_TYPE_ASEQ_DEVICE, BtIcASeqDevice))
#define BTIC_ASEQ_DEVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTIC_TYPE_ASEQ_DEVICE, BtIcASeqDeviceClass))
#define BTIC_IS_ASEQ_DEVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTIC_TYPE_ASEQ_DEVICE))
#define BTIC_IS_ASEQ_DEVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTIC_TYPE_ASEQ_DEVICE))
#define BTIC_ASEQ_DEVICE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTIC_TYPE_ASEQ_DEVICE, BtIcASeqDeviceClass))

typedef struct _BtIcASeqDevice BtIcASeqDevice;
typedef struct _BtIcASeqDeviceClass BtIcASeqDeviceClass;
typedef struct _BtIcASeqDevicePrivate BtIcASeqDevicePrivate;

/**
 * BtIcASeqDevice:
 *
 * buzztraxs interaction controller registry
 */
struct _BtIcASeqDevice {
  const BtIcDevice parent;
  
  /*< private >*/
  BtIcASeqDevicePrivate *priv;
};

struct _BtIcASeqDeviceClass {
  const BtIcDeviceClass parent;
  
  snd_seq_t *seq;
};

GType btic_aseq_device_get_type(void) G_GNUC_CONST;

BtIcASeqDevice *btic_aseq_device_new(const gchar *udi,const gchar *name,gint client,gint port);

#endif // BTIC_ASEQ_DEVICE_H
