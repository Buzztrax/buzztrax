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

#ifndef BT_MAIN_PAGE_MACHINES_H
#define BT_MAIN_PAGE_MACHINES_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

/**
 * BtMainPageMachines:
 *
 * the machines page for the editor application
 */
G_DECLARE_FINAL_TYPE(BtMainPageMachines, bt_main_page_machines, BT,
    MAIN_PAGE_MACHINES, GtkWidget);

#define BT_TYPE_MAIN_PAGE_MACHINES            (bt_main_page_machines_get_type ())

#define ICON_UPSAMPLING 1.5
// icon box
#define MACHINE_W (48.0*ICON_UPSAMPLING)
#define MACHINE_H (38.0*ICON_UPSAMPLING)
// label box
#define MACHINE_LABEL_BASE (12.0*ICON_UPSAMPLING)
#define MACHINE_LABEL_HEIGHT (7.0*ICON_UPSAMPLING)
#define MACHINE_LABEL_WIDTH (40.0*ICON_UPSAMPLING)
// meter boxes
#define MACHINE_METER_LEFT (7.0*ICON_UPSAMPLING)
#define MACHINE_METER_RIGHT (37.0*ICON_UPSAMPLING)
#define MACHINE_METER_BASE (31.0*ICON_UPSAMPLING)
#define MACHINE_METER_HEIGHT (12.0*ICON_UPSAMPLING)
#define MACHINE_METER_WIDTH (4.0*ICON_UPSAMPLING)
// wire pad
#define WIRE_PAD_W (35.0*ICON_UPSAMPLING)
#define WIRE_PAD_H (18.0*ICON_UPSAMPLING)
#define WIRE_PAD_METER_BASE (12.0*ICON_UPSAMPLING)
#define WIRE_PAD_METER_WIDTH (12.0*ICON_UPSAMPLING)
#define WIRE_PAD_METER_HEIGHT (4.0*ICON_UPSAMPLING)
#define WIRE_PAD_METER_VOL (4.0*ICON_UPSAMPLING)
#define WIRE_PAD_METER_PAN (10.0*ICON_UPSAMPLING)

//-- enums

#include "main-pages.h"

typedef enum {
  BT_MAIN_PAGE_MACHINES_GRID_DENSITY_OFF=0,
  BT_MAIN_PAGE_MACHINES_GRID_DENSITY_LOW,
  BT_MAIN_PAGE_MACHINES_GRID_DENSITY_MEDIUM,
  BT_MAIN_PAGE_MACHINES_GRID_DENSITY_HIGH,
} BtMainPageMachinesGridDensity;

GType bt_main_page_machines_grid_density_get_type(void) G_GNUC_CONST;


typedef struct _BtMachine BtMachine;
typedef struct _BtWire BtWire;

BtMainPageMachines *bt_main_page_machines_new();

gboolean bt_main_page_machines_wire_volume_popup(BtMainPageMachines *self, BtWire *wire, gint xpos, gint ypos);
gboolean bt_main_page_machines_wire_panorama_popup(BtMainPageMachines *self, BtWire *wire, gint xpos, gint ypos);

gboolean bt_main_page_machines_add_source_machine(BtMainPageMachines *self, const gchar *id, const gchar *plugin_name);
gboolean bt_main_page_machines_add_processor_machine(BtMainPageMachines *self, const gchar *id, const gchar *plugin_name);
void bt_main_page_machines_delete_machine(BtMainPageMachines *self, BtMachine *machine);
void bt_main_page_machines_delete_wire(BtMainPageMachines *self, BtWire *wire);
void bt_main_page_machines_rename_machine(BtMainPageMachines *self, BtMachine *machine);

void bt_main_page_machines_canvas_coords_to_relative(BtMainPageMachines * self, const gdouble xc, const gdouble yc, gdouble *xr, gdouble *yr);
void bt_main_page_machines_relative_coords_to_canvas(BtMainPageMachines * self, const gdouble xr, const gdouble yr, gdouble *xc, gdouble *yc);

void bt_main_page_machines_set_pages_parent(BtMainPageMachines * self, BtMainPages *pages);

#endif // BT_MAIN_PAGE_MACHINES_H
