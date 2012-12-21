/* Buzztard
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BT_MAIN_PAGE_MACHINES_H
#define BT_MAIN_PAGE_MACHINES_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_MAIN_PAGE_MACHINES            (bt_main_page_machines_get_type ())
#define BT_MAIN_PAGE_MACHINES(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_MAIN_PAGE_MACHINES, BtMainPageMachines))
#define BT_MAIN_PAGE_MACHINES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_MAIN_PAGE_MACHINES, BtMainPageMachinesClass))
#define BT_IS_MAIN_PAGE_MACHINES(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_MAIN_PAGE_MACHINES))
#define BT_IS_MAIN_PAGE_MACHINES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_MAIN_PAGE_MACHINES))
#define BT_MAIN_PAGE_MACHINES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_MAIN_PAGE_MACHINES, BtMainPageMachinesClass))

/* type macros */

typedef struct _BtMainPageMachines BtMainPageMachines;
typedef struct _BtMainPageMachinesClass BtMainPageMachinesClass;
typedef struct _BtMainPageMachinesPrivate BtMainPageMachinesPrivate;

/**
 * BtMainPageMachines:
 *
 * the machines page for the editor application
 */
struct _BtMainPageMachines {
  GtkVBox parent;
  
  /*< private >*/
  BtMainPageMachinesPrivate *priv;
};

struct _BtMainPageMachinesClass {
  GtkVBoxClass parent;
  
};


// machine view area (4:3 aspect ratio)
// TODO(ensonic): should we check screens aspect ratio?
#define MACHINE_VIEW_ZOOM_X (400.0*1.2)
#define MACHINE_VIEW_ZOOM_Y (300.0*1.2)
#define MACHINE_VIEW_ZOOM_FC  1.0

#define MACHINE_VIEW_GRID_FC  4.0

//#define MACHINE_VIEW_MACHINE_SIZE_X 35.0
//#define MACHINE_VIEW_MACHINE_SIZE_Y 23.0
#define MACHINE_VIEW_MACHINE_SIZE_X 42.0
#define MACHINE_VIEW_MACHINE_SIZE_Y 31.5

// 6.0 is too small
#define MACHINE_VIEW_FONT_SIZE 6.5

#define MACHINE_VIEW_WIRE_PAD_SIZE 6.0

GType bt_main_page_machines_get_type(void) G_GNUC_CONST;

#include "main-pages.h"

BtMainPageMachines *bt_main_page_machines_new(const BtMainPages *pages);

gboolean bt_main_page_machines_wire_volume_popup(const BtMainPageMachines *self, BtWire *wire, gint xpos, gint ypos);
gboolean bt_main_page_machines_wire_panorama_popup(const BtMainPageMachines *self, BtWire *wire, gint xpos, gint ypos);

gboolean bt_main_page_machines_add_source_machine(const BtMainPageMachines *self, const gchar *id, const gchar *plugin_name);
gboolean bt_main_page_machines_add_processor_machine(const BtMainPageMachines *self, const gchar *id, const gchar *plugin_name);
void bt_main_page_machines_delete_machine(const BtMainPageMachines *self, BtMachine *machine);
void bt_main_page_machines_delete_wire(const BtMainPageMachines *self, BtWire *wire);
void bt_main_page_machines_rename_machine(const BtMainPageMachines *self, BtMachine *machine);

#endif // BT_MAIN_PAGE_MACHINES_H
