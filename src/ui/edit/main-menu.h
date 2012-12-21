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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BT_MAIN_MENU_H
#define BT_MAIN_MENU_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_MAIN_MENU             (bt_main_menu_get_type ())
#define BT_MAIN_MENU(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_MAIN_MENU, BtMainMenu))
#define BT_MAIN_MENU_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_MAIN_MENU, BtMainMenuClass))
#define BT_IS_MAIN_MENU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_MAIN_MENU))
#define BT_IS_MAIN_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_MAIN_MENU))
#define BT_MAIN_MENU_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_MAIN_MENU, BtMainMenuClass))

/* type macros */

typedef struct _BtMainMenu BtMainMenu;
typedef struct _BtMainMenuClass BtMainMenuClass;
typedef struct _BtMainMenuPrivate BtMainMenuPrivate;

/**
 * BtMainMenu:
 *
 * the main menu inside the #BtMainWindow
 */
struct _BtMainMenu {
  GtkMenuBar parent;
  
  /*< private >*/
  BtMainMenuPrivate *priv;
};

struct _BtMainMenuClass {
  GtkMenuBarClass parent;
};

GType bt_main_menu_get_type(void) G_GNUC_CONST;

BtMainMenu *bt_main_menu_new(void);

#endif // BT_MAIN_MENU_H
