/* $Id: main-window.h,v 1.3 2004-09-02 17:43:30 ensonic Exp $
 * class for the editor main window
 */

#ifndef BT_MAIN_WINDOW_H
#define BT_MAIN_WINDOW_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_MAIN_WINDOW		         (bt_main_window_get_type ())
#define BT_MAIN_WINDOW(obj)		         (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_MAIN_WINDOW, BtMainWindow))
#define BT_MAIN_WINDOW_CLASS(klass)	   (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_MAIN_WINDOW, BtMainWindowClass))
#define BT_IS_MAIN_WINDOW(obj)	       (G_TYPE_CHECK_TYPE ((obj), BT_TYPE_MAIN_WINDOW))
#define BT_IS_MAIN_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_MAIN_WINDOW))
#define BT_MAIN_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_MAIN_WINDOW, BtMainWindowClass))

/* type macros */

typedef struct _BtMainWindow BtMainWindow;
typedef struct _BtMainWindowClass BtMainWindowClass;
typedef struct _BtMainWindowPrivate BtMainWindowPrivate;

/**
 * BtMainWindow:
 *
 * the root window for the editor application
 */
struct _BtMainWindow {
  GtkWindow parent;
  
  /* private */
  BtMainWindowPrivate *private;
};
/* structure of the main-window class */
struct _BtMainWindowClass {
  GtkWindowClass parent;
  
};

/* used by MAIN_WINDOW_TYPE */
GType bt_main_window_get_type(void);

#endif // BT_MAIN_WINDOW_H

