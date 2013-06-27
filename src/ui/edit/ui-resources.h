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
 * BtUIResourcesColors:
 * @BT_UI_RES_COLOR_CURSOR: cursor color
 * @BT_UI_RES_COLOR_SELECTION1: selection color
 * @BT_UI_RES_COLOR_SELECTION2: shaded selection color
 * @BT_UI_RES_COLOR_PLAYLINE: playback line
 * @BT_UI_RES_COLOR_LOOPLINE: loop line
 * @BT_UI_RES_COLOR_ENDLINE: song end line
 * @BT_UI_RES_COLOR_SOURCE_MACHINE_BASE: source machine base color
 * @BT_UI_RES_COLOR_SOURCE_MACHINE_BRIGHT1: source machine lighter variant
 * @BT_UI_RES_COLOR_SOURCE_MACHINE_BRIGHT2: source machine even lighter variant
 * @BT_UI_RES_COLOR_SOURCE_MACHINE_DARK1: source machine darker variant
 * @BT_UI_RES_COLOR_SOURCE_MACHINE_DARK2: source machine even darker variant
 * @BT_UI_RES_COLOR_PROCESSOR_MACHINE_BASE: processor machine base color
 * @BT_UI_RES_COLOR_PROCESSOR_MACHINE_BRIGHT1: processor machine lighter variant
 * @BT_UI_RES_COLOR_PROCESSOR_MACHINE_BRIGHT2: processor machine even lighter variant
 * @BT_UI_RES_COLOR_PROCESSOR_MACHINE_DARK1: processor machine darker variant
 * @BT_UI_RES_COLOR_PROCESSOR_MACHINE_DARK2: processor machine even darker variant
 * @BT_UI_RES_COLOR_SINK_MACHINE_BASE: sink machine base color
 * @BT_UI_RES_COLOR_SINK_MACHINE_BRIGHT1: sink machine lighter variant
 * @BT_UI_RES_COLOR_SINK_MACHINE_BRIGHT2: sink machine even lighter variant
 * @BT_UI_RES_COLOR_SINK_MACHINE_DARK1: sink machine darker variant
 * @BT_UI_RES_COLOR_SINK_MACHINE_DARK2: sink machine even darker variant
 * @BT_UI_RES_COLOR_ANALYZER_DECAY: analyzer peak lines
 * @BT_UI_RES_COLOR_GRID_LINES: grid lines
 * @BT_UI_RES_COLOR_COUNT: symbolic color count
 *
 * Symbolic color names for the UI.
 */
typedef enum {
  BT_UI_RES_COLOR_CURSOR=0,
  BT_UI_RES_COLOR_SELECTION1,
  BT_UI_RES_COLOR_SELECTION2,
  BT_UI_RES_COLOR_PLAYLINE,
  BT_UI_RES_COLOR_LOOPLINE,
  BT_UI_RES_COLOR_ENDLINE,
  BT_UI_RES_COLOR_SOURCE_MACHINE_BASE,       /* machine view normal */
  BT_UI_RES_COLOR_SOURCE_MACHINE_BRIGHT1,    /* list view odd */
  BT_UI_RES_COLOR_SOURCE_MACHINE_BRIGHT2,    /* list view even */
  BT_UI_RES_COLOR_SOURCE_MACHINE_DARK1,      /* machine title */
  BT_UI_RES_COLOR_SOURCE_MACHINE_DARK2,      /* --- */
  BT_UI_RES_COLOR_PROCESSOR_MACHINE_BASE,    /* machine view normal */
  BT_UI_RES_COLOR_PROCESSOR_MACHINE_BRIGHT1, /* list view odd */
  BT_UI_RES_COLOR_PROCESSOR_MACHINE_BRIGHT2, /* list view even */
  BT_UI_RES_COLOR_PROCESSOR_MACHINE_DARK1,   /* machine title */
  BT_UI_RES_COLOR_PROCESSOR_MACHINE_DARK2,   /* --- */
  BT_UI_RES_COLOR_SINK_MACHINE_BASE,         /* machine view normal */
  BT_UI_RES_COLOR_SINK_MACHINE_BRIGHT1,      /* list view odd */
  BT_UI_RES_COLOR_SINK_MACHINE_BRIGHT2,      /* list view even */
  BT_UI_RES_COLOR_SINK_MACHINE_DARK1,        /* --- */
  BT_UI_RES_COLOR_SINK_MACHINE_DARK2,        /* --- */
  BT_UI_RES_COLOR_ANALYZER_DECAY,            /* analyzer window decay marks */
  BT_UI_RES_COLOR_GRID_LINES,                /* grid lines */
  BT_UI_RES_COLOR_COUNT
} BtUIResourcesColors;

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

BtUIResources *bt_ui_resources_new(void);

GdkPixbuf *bt_ui_resources_get_icon_pixbuf_by_machine(const BtMachine *machine);
GdkPixbuf *bt_ui_resources_get_machine_graphics_pixbuf_by_machine(const BtMachine *machine, gdouble zoom);
GtkWidget *bt_ui_resources_get_icon_image_by_machine(const BtMachine *machine);
GtkWidget *bt_ui_resources_get_icon_image_by_machine_type(GType machine_type);

GdkPixbuf *bt_ui_resources_get_wire_graphics_pixbuf_by_wire(const BtWire *wire, gdouble zoom);

GdkColor *bt_ui_resources_get_gdk_color(BtUIResourcesColors color_type);
void bt_ui_resources_get_rgb_color(BtUIResourcesColors color_type, GdkRGBA *color);
guint32 bt_ui_resources_get_color_by_machine(const BtMachine *machine, BtUIResourcesMachineColors color_type);

GtkAccelGroup *bt_ui_resources_get_accel_group(void);

#endif // BT_UI_RESOURCES_H
