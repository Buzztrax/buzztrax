/* $Id: wire-canvas-item.h,v 1.1 2004-10-15 15:39:33 ensonic Exp $
 * class for the editor wire views wire canvas item
 */

#ifndef BT_WIRE_CANVAS_ITEM_H
#define BT_WIRE_CANVAS_ITEM_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_WIRE_CANVAS_ITEM		        (bt_wire_canvas_item_get_type ())
#define BT_WIRE_CANVAS_ITEM(obj)		        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_WIRE_CANVAS_ITEM, BtWireCanvasItem))
#define BT_WIRE_CANVAS_ITEM_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_WIRE_CANVAS_ITEM, BtWireCanvasItemClass))
#define BT_IS_WIRE_CANVAS_ITEM(obj)	        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_WIRE_CANVAS_ITEM))
#define BT_IS_WIRE_CANVAS_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_WIRE_CANVAS_ITEM))
#define BT_WIRE_CANVAS_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_WIRE_CANVAS_ITEM, BtWireCanvasItemClass))

/* type macros */

typedef struct _BtWireCanvasItem BtWireCanvasItem;
typedef struct _BtWireCanvasItemClass BtWireCanvasItemClass;
typedef struct _BtWireCanvasItemPrivate BtWireCanvasItemPrivate;

/**
 * BtWireCanvasItem:
 *
 * the root window for the editor application
 */
struct _BtWireCanvasItem {
  GnomeCanvasGroup parent;
  
  /* private */
  BtWireCanvasItemPrivate *priv;
};
/* structure of the main-pages class */
struct _BtWireCanvasItemClass {
  GnomeCanvasGroupClass parent;
  
};

/* used by MAIN_PAGES_TYPE */
GType bt_wire_canvas_item_get_type(void);

#endif // BT_WIRE_CANVAS_ITEM_H

