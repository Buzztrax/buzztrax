/* $Id$
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

#ifndef BTIC_REGISTRY_H
#define BTIC_REGISTRY_H

#include <glib.h>
#include <glib-object.h>

#include "device.h"

/* type macros */

#define BTIC_TYPE_REGISTRY            (btic_registry_get_type ())
#define BTIC_REGISTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTIC_TYPE_REGISTRY, BtIcRegistry))
#define BTIC_REGISTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTIC_TYPE_REGISTRY, BtIcRegistryClass))
#define BTIC_IS_REGISTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTIC_TYPE_REGISTRY))
#define BTIC_IS_REGISTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTIC_TYPE_REGISTRY))
#define BTIC_REGISTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTIC_TYPE_REGISTRY, BtIcRegistryClass))

typedef struct _BtIcRegistry BtIcRegistry;
typedef struct _BtIcRegistryClass BtIcRegistryClass;
typedef struct _BtIcRegistryPrivate BtIcRegistryPrivate;

/**
 * BtIcRegistry:
 *
 * buzztards interaction controller registry
 */
struct _BtIcRegistry {
  const GObject parent;
  
  /*< private >*/
  BtIcRegistryPrivate *priv;
};
/* structure of the registry class */
struct _BtIcRegistryClass {
  const GObjectClass parent;
  
};

/* used by REGISTRY_TYPE */
GType btic_registry_get_type(void) G_GNUC_CONST;

BtIcRegistry *btic_registry_new(void);

void btic_registry_remove_device_by_udi(const gchar *udi);
void btic_registry_add_device(BtIcDevice *device);

#endif // BTIC_REGISTRY_H
