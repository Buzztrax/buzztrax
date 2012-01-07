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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef BT_MAIN_STATUSBAR_H
#define BT_MAIN_STATUSBAR_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_MAIN_STATUSBAR            (bt_main_statusbar_get_type ())
#define BT_MAIN_STATUSBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_MAIN_STATUSBAR, BtMainStatusbar))
#define BT_MAIN_STATUSBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_MAIN_STATUSBAR, BtMainStatusbarClass))
#define BT_IS_MAIN_STATUSBAR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_MAIN_STATUSBAR))
#define BT_IS_MAIN_STATUSBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_MAIN_STATUSBAR))
#define BT_MAIN_STATUSBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_MAIN_STATUSBAR, BtMainStatusbarClass))

/* type macros */

typedef struct _BtMainStatusbar BtMainStatusbar;
typedef struct _BtMainStatusbarClass BtMainStatusbarClass;
typedef struct _BtMainStatusbarPrivate BtMainStatusbarPrivate;

/**
 * BtMainStatusbar:
 *
 * the root window for the editor application
 */
struct _BtMainStatusbar {
  GtkHBox parent;
  
  /*< private >*/
  BtMainStatusbarPrivate *priv;
};

struct _BtMainStatusbarClass {
  GtkHBoxClass parent;
  
};

/**
 * BT_MAIN_STATUSBAR_DEFAULT:
 *
 * Default text to display when idle.
 */
#define BT_MAIN_STATUSBAR_DEFAULT _("Ready to rock!")

GType bt_main_statusbar_get_type(void) G_GNUC_CONST;

BtMainStatusbar *bt_main_statusbar_new(void);

#endif // BT_MAIN_STATUSBAR_H
