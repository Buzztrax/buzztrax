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

#ifndef BTIC_HAL_DISCOVERER_H
#define BTIC_HAL_DISCOVERER_H

#include <glib.h>
#include <glib-object.h>

/* type macros */

#define BTIC_TYPE_HAL_DISCOVERER            (btic_hal_discoverer_get_type ())
#define BTIC_HAL_DISCOVERER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTIC_TYPE_HAL_DISCOVERER, BtIcHalDiscoverer))
#define BTIC_HAL_DISCOVERER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTIC_TYPE_HAL_DISCOVERER, BtIcRHalDiscovererClass))
#define BTIC_IS_HAL_DISCOVERER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTIC_TYPE_HAL_DISCOVERER))
#define BTIC_IS_HAL_DISCOVERER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTIC_TYPE_HAL_DISCOVERER))
#define BTIC_HAL_DISCOVERER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTIC_TYPE_HAL_DISCOVERER, BtIcHalDiscovererClass))

typedef struct _BtIcHalDiscoverer BtIcHalDiscoverer;
typedef struct _BtIcHalDiscovererClass BtIcHalDiscovererClass;
typedef struct _BtIcHalDiscovererPrivate BtIcHalDiscovererPrivate;

/**
 * BtIcHalDiscoverer:
 *
 * hal based device discovery for buzztards interaction controller
 */
struct _BtIcHalDiscoverer {
  const GObject parent;

  /*< private >*/
  BtIcHalDiscovererPrivate *priv;
};
struct _BtIcHalDiscovererClass {
  const GObjectClass parent;

};

/* used by HAL_DISCOVERER_TYPE */
GType btic_hal_discoverer_get_type(void) G_GNUC_CONST;

BtIcHalDiscoverer *btic_hal_discoverer_new(void);

#endif // BTIC_HAL_DISCOVERER
