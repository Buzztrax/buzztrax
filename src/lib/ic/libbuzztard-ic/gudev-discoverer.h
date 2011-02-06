/* $Id$
 *
 * Buzztard
 * Copyright (C) 2011 Buzztard team <buzztard-devel@lists.sf.net>
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

#ifndef BTIC_GUDEV_DISCOVERER_H
#define BTIC_GUDEV_DISCOVERER_H

#include <glib.h>
#include <glib-object.h>

/* type macros */

#define BTIC_TYPE_GUDEV_DISCOVERER            (btic_gudev_discoverer_get_type ())
#define BTIC_GUDEV_DISCOVERER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTIC_TYPE_GUDEV_DISCOVERER, BtIcGudevDiscoverer))
#define BTIC_GUDEV_DISCOVERER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTIC_TYPE_GUDEV_DISCOVERER, BtIcRGudevDiscovererClass))
#define BTIC_IS_GUDEV_DISCOVERER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTIC_TYPE_GUDEV_DISCOVERER))
#define BTIC_IS_GUDEV_DISCOVERER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTIC_TYPE_GUDEV_DISCOVERER))
#define BTIC_GUDEV_DISCOVERER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTIC_TYPE_GUDEV_DISCOVERER, BtIcGudevDiscovererClass))

typedef struct _BtIcGudevDiscoverer BtIcGudevDiscoverer;
typedef struct _BtIcGudevDiscovererClass BtIcGudevDiscovererClass;
typedef struct _BtIcGudevDiscovererPrivate BtIcGudevDiscovererPrivate;

/**
 * BtIcGudevDiscoverer:
 *
 * gudev based device discovery for buzztards interaction controller
 */
struct _BtIcGudevDiscoverer {
  const GObject parent;

  /*< private >*/
  BtIcGudevDiscovererPrivate *priv;
};
struct _BtIcGudevDiscovererClass {
  const GObjectClass parent;

};

/* used by GUDEV_DISCOVERER_TYPE */
GType btic_gudev_discoverer_get_type(void) G_GNUC_CONST;

#endif // BTIC_GUDEV_DISCOVERER
