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

#ifndef BTIC_ASEQ_DISCOVERER_H
#define BTIC_ASEQ_DISCOVERER_H

#include <glib.h>
#include <glib-object.h>

/* type macros */

#define BTIC_TYPE_ASEQ_DISCOVERER            (btic_aseq_discoverer_get_type ())
#define BTIC_ASEQ_DISCOVERER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTIC_TYPE_ASEQ_DISCOVERER, BtIcASeqDiscoverer))
#define BTIC_ASEQ_DISCOVERER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTIC_TYPE_ASEQ_DISCOVERER, BtIcRGudevDiscovererClass))
#define BTIC_IS_ASEQ_DISCOVERER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTIC_TYPE_ASEQ_DISCOVERER))
#define BTIC_IS_ASEQ_DISCOVERER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTIC_TYPE_ASEQ_DISCOVERER))
#define BTIC_ASEQ_DISCOVERER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTIC_TYPE_ASEQ_DISCOVERER, BtIcASeqDiscovererClass))

typedef struct _BtIcASeqDiscoverer BtIcASeqDiscoverer;
typedef struct _BtIcASeqDiscovererClass BtIcASeqDiscovererClass;
typedef struct _BtIcASeqDiscovererPrivate BtIcASeqDiscovererPrivate;

/**
 * BtIcASeqDiscoverer:
 *
 * alsa based device discovery for buzztraxs interaction controller
 */
struct _BtIcASeqDiscoverer {
  const GObject parent;

  /*< private >*/
  BtIcASeqDiscovererPrivate *priv;
};
struct _BtIcASeqDiscovererClass {
  const GObjectClass parent;

};

GType btic_aseq_discoverer_get_type(void) G_GNUC_CONST;

BtIcASeqDiscoverer *btic_aseq_discoverer_new(void);

#endif // BTIC_ASEQ_DISCOVERER
