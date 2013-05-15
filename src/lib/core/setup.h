/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
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

struct _BtSetupClass {
  const GObjectClass parent;
};

GType bt_setup_get_type(void) G_GNUC_CONST;

BtSetup *bt_setup_new(const BtSong * const song);

gboolean bt_setup_add_machine(const BtSetup * const self, const BtMachine * const machine);
gboolean bt_setup_add_wire(const BtSetup * const self, const BtWire * const wire);

void bt_setup_remove_machine(const BtSetup * const self, const BtMachine * const machine);
void bt_setup_remove_wire(const BtSetup * const self, const BtWire * const wire);

BtMachine *bt_setup_get_machine_by_id(const BtSetup * const self, const gchar * const id);
BtMachine *bt_setup_get_machine_by_type(const BtSetup * const self, const GType type);

GList *bt_setup_get_machines_by_type(const BtSetup * const self, const GType type);

BtWire *bt_setup_get_wire_by_src_machine(const BtSetup * const self, const BtMachine * const src);
BtWire *bt_setup_get_wire_by_dst_machine(const BtSetup * const self, const BtMachine * const dst);
BtWire *bt_setup_get_wire_by_machines(const BtSetup * const self, const BtMachine * const src, const BtMachine * const dst);

GList *bt_setup_get_wires_by_src_machine(const BtSetup * const self, const BtMachine * const src);
GList *bt_setup_get_wires_by_dst_machine(const BtSetup * const self, const BtMachine * const dst);

gchar *bt_setup_get_unique_machine_id(const BtSetup * const self, const gchar * const base_name);

void bt_setup_remember_missing_machine(const BtSetup * const self, const gchar * const str);

#endif // BT_SETUP_H
