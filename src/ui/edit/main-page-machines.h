/* $Id: main-page-machines.h,v 1.4 2004-10-08 13:50:04 ensonic Exp $
 * class for the editor main machines page
 */

#ifndef BT_MAIN_PAGE_MACHINES_H
#define BT_MAIN_PAGE_MACHINES_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_MAIN_PAGE_MACHINES		        (bt_main_page_machines_get_type ())
#define BT_MAIN_PAGE_MACHINES(obj)		        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_MAIN_PAGE_MACHINES, BtMainPageMachines))
#define BT_MAIN_PAGE_MACHINES_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_MAIN_PAGE_MACHINES, BtMainPageMachinesClass))
#define BT_IS_MAIN_PAGE_MACHINES(obj)	        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_MAIN_PAGE_MACHINES))
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
  
  /* private */
  BtMainPageMachinesPrivate *priv;
};
/* structure of the main-page-machines class */
struct _BtMainPageMachinesClass {
  GtkVBoxClass parent;
  
};

/* used by MAIN_PAGE_MACHINES_TYPE */
GType bt_main_page_machines_get_type(void);

#endif // BT_MAIN_PAGE_MACHINES_H

