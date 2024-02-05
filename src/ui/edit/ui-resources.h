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

#ifndef BT_UI_RESOURCES_H
#define BT_UI_RESOURCES_H

#include <glib.h>
#include <glib-object.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#define BT_TYPE_UI_RESOURCES            (bt_ui_resources_get_type ())
#define BT_UI_RESOURCES(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_UI_RESOURCES, BtUIResources))
#define BT_UI_RESOURCES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_UI_RESOURCES, BtUIResourcesClass))
#define BT_IS_UI_RESOURCES(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_UI_RESOURCES))
#define BT_IS_UI_RESOURCES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_UI_RESOURCES))
#define BT_UI_RESOURCES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_UI_RESOURCES, BtUIResourcesClass))

/* type macros */

typedef struct _BtUIResources BtUIResources;
typedef struct _BtUIResourcesClass BtUIResourcesClass;
typedef struct _BtUIResourcesPrivate BtUIResourcesPrivate;

/**
 * BtUIResources:
 *
 * a collection of shared ui resources
 */
struct _BtUIResources {
  GObject parent;

  /*< private >*/
  BtUIResourcesPrivate *priv;
};

struct _BtUIResourcesClass {
  GObjectClass parent;
};

/**
 * BtUIResourcesMachineColors:
 * @BT_UI_RES_COLOR_MACHINE_BASE: base color
 * @BT_UI_RES_COLOR_MACHINE_BRIGHT1: lighter variant
 * @BT_UI_RES_COLOR_MACHINE_BRIGHT2: even lighter variant
 * @BT_UI_RES_COLOR_MACHINE_DARK1: darker variant
 * @BT_UI_RES_COLOR_MACHINE_DARK2: even darker variant
 *
 * Symbolic color names for machines.
 */
typedef enum {
  BT_UI_RES_COLOR_MACHINE_BASE=0,     /* machine view normal */
  BT_UI_RES_COLOR_MACHINE_BRIGHT1,    /* list view odd */
  BT_UI_RES_COLOR_MACHINE_BRIGHT2,    /* list view even */
  BT_UI_RES_COLOR_MACHINE_DARK1,      /* --- */
  BT_UI_RES_COLOR_MACHINE_DARK2       /* --- */
} BtUIResourcesMachineColors;


GType bt_ui_resources_get_type(void) G_GNUC_CONST;

//-- forward declarations
typedef struct _BtMachine BtMachine;
typedef struct _BtWire BtWire;

//-- methods

BtUIResources *bt_ui_resources_new(GdkDisplay *display);

GdkPaintable *bt_ui_resources_get_icon_paintable_by_machine(const BtMachine *machine);
GdkPaintable *bt_ui_resources_get_machine_graphics_texture_by_machine(const BtMachine *machine, gdouble zoom);
GtkWidget *bt_ui_resources_get_icon_image_by_machine(const BtMachine *machine);
GtkWidget *bt_ui_resources_get_icon_image_by_machine_type(GType machine_type);

GdkPaintable *bt_ui_resources_get_wire_graphics_paintable_by_wire(const BtWire *wire, gdouble zoom);

/// GTK4 GtkAccelGroup *bt_ui_resources_get_accel_group(void);

#endif // BT_UI_RESOURCES_H
