/* $Id: main-page-machines.h,v 1.12 2006-08-31 19:57:57 ensonic Exp $
 *
 * Buzztard
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
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
/* structure of the main-page-machines class */
struct _BtMainPageMachinesClass {
  GtkVBoxClass parent;
  
};

/* used by MAIN_PAGE_MACHINES_TYPE */
GType bt_main_page_machines_get_type(void) G_GNUC_CONST;


// machine view area
#define MACHINE_VIEW_ZOOM_X 400.0
#define MACHINE_VIEW_ZOOM_Y 300.0
#define MACHINE_VIEW_ZOOM_FC  1.0

#define MACHINE_VIEW_GRID_FC  4.0

#define MACHINE_VIEW_MACHINE_SIZE_X 35.0
#define MACHINE_VIEW_MACHINE_SIZE_Y 23.0

#define MACHINE_VIEW_WIRE_PAD_SIZE 6.0

#endif // BT_MAIN_PAGE_MACHINES_H
