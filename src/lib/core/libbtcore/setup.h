/* $Id$
 *
 * Buzztard
 * Copyright (C) 2006 Buzztard team <buzztard-devel@lists.sf.net>
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

#ifndef BT_SETUP_H
#define BT_SETUP_H

#include <glib.h>
#include <glib-object.h>
#include "machine.h"
#include "wire.h"

#define BT_TYPE_SETUP             (bt_setup_get_type ())
#define BT_SETUP(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SETUP, BtSetup))
#define BT_SETUP_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SETUP, BtSetupClass))
#define BT_IS_SETUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SETUP))
#define BT_IS_SETUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SETUP))
#define BT_SETUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SETUP, BtSetupClass))

/* type macros */

typedef struct _BtSetup BtSetup;
typedef struct _BtSetupClass BtSetupClass;
typedef struct _BtSetupPrivate BtSetupPrivate;

/**
 * BtSetup:
 *
 * virtual hardware setup
 * (contains #BtMachine and #BtWire objects)
 */
struct _BtSetup {
  const GObject parent;
  
  /*< private >*/
  BtSetupPrivate *priv;
};
/* structure of the setup class */
struct _BtSetupClass {
  const GObjectClass parent;

  void (*machine_added_event)(const BtSetup * const setup, const BtMachine * const machine, gconstpointer const user_data);
  void (*wire_added_event)(const BtSetup * const setup, const BtWire * const wire, gconstpointer const user_data);
  void (*machine_removed_event)(const BtSetup * const setup, const BtMachine * const machine, gconstpointer const user_data);
  void (*wire_removed_event)(const BtSetup * const setup, const BtWire * const wire, gconstpointer const user_data);
};

/* used by SETUP_TYPE */
GType bt_setup_get_type(void) G_GNUC_CONST;


#endif // BT_SETUP_H
