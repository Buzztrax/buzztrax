/* $Id: main-pages.h,v 1.5 2004-10-13 16:05:15 ensonic Exp $
 * class for the editor main pages
 */

#ifndef BT_MAIN_PAGES_H
#define BT_MAIN_PAGES_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_MAIN_PAGES		        (bt_main_pages_get_type ())
#define BT_MAIN_PAGES(obj)		        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_MAIN_PAGES, BtMainPages))
#define BT_MAIN_PAGES_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_MAIN_PAGES, BtMainPagesClass))
#define BT_IS_MAIN_PAGES(obj)	        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_MAIN_PAGES))
#define BT_IS_MAIN_PAGES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_MAIN_PAGES))
#define BT_MAIN_PAGES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_MAIN_PAGES, BtMainPagesClass))

/* type macros */

typedef struct _BtMainPages BtMainPages;
typedef struct _BtMainPagesClass BtMainPagesClass;
typedef struct _BtMainPagesPrivate BtMainPagesPrivate;

/**
 * BtMainPages:
 *
 * the root window for the editor application
 */
struct _BtMainPages {
  GtkNotebook parent;
  
  /* private */
  BtMainPagesPrivate *priv;
};
/* structure of the main-pages class */
struct _BtMainPagesClass {
  GtkNotebookClass parent;
  
};

/* used by MAIN_PAGES_TYPE */
GType bt_main_pages_get_type(void);

#endif // BT_MAIN_PAGES_H

