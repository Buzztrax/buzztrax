/* $Id: machine-canvas-item.h,v 1.2 2004-10-15 15:39:33 ensonic Exp $
 * class for the editor machine views machine canvas item
 */

#ifndef BT_MACHINE_CANVAS_ITEM_H
#define BT_MACHINE_CANVAS_ITEM_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_MACHINE_CANVAS_ITEM		         (bt_machine_canvas_item_get_type ())
#define BT_MACHINE_CANVAS_ITEM(obj)		         (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_MACHINE_CANVAS_ITEM, BtMachineCanvasItem))
#define BT_MACHINE_CANVAS_ITEM_CLASS(klass)	   (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_MACHINE_CANVAS_ITEM, BtMachineCanvasItemClass))
#define BT_IS_MACHINE_CANVAS_ITEM(obj)	       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_MACHINE_CANVAS_ITEM))
#define BT_IS_MACHINE_CANVAS_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_MACHINE_CANVAS_ITEM))
#define BT_MACHINE_CANVAS_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_MACHINE_CANVAS_ITEM, BtMachineCanvasItemClass))

/* type macros */

typedef struct _BtMachineCanvasItem BtMachineCanvasItem;
typedef struct _BtMachineCanvasItemClass BtMachineCanvasItemClass;
typedef struct _BtMachineCanvasItemPrivate BtMachineCanvasItemPrivate;

/**
 * BtMachineCanvasItem:
 *
 * the root window for the editor application
 */
struct _BtMachineCanvasItem {
  GnomeCanvasGroup parent;
  
  /* private */
  BtMachineCanvasItemPrivate *priv;
};
/* structure of the main-pages class */
struct _BtMachineCanvasItemClass {
  GnomeCanvasGroupClass parent;

  void (*position_changed)(const BtMachineCanvasItem *citem, gpointer user_data);
};

/* used by MAIN_PAGES_TYPE */
GType bt_machine_canvas_item_get_type(void);

#endif // BT_MACHINE_CANVAS_ITEM_H

