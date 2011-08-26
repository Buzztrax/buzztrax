/* $Id$
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

#ifndef BT_MAIN_TOOLBAR_H
#define BT_MAIN_TOOLBAR_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_MAIN_TOOLBAR            (bt_main_toolbar_get_type ())
#define BT_MAIN_TOOLBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_MAIN_TOOLBAR, BtMainToolbar))
#define BT_MAIN_TOOLBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_MAIN_TOOLBAR, BtMainToolbarClass))
#define BT_IS_MAIN_TOOLBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_MAIN_TOOLBAR))
#define BT_IS_MAIN_TOOLBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_MAIN_TOOLBAR))
#define BT_MAIN_TOOLBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_MAIN_TOOLBAR, BtMainToolbarClass))

/* type macros */

typedef struct _BtMainToolbar BtMainToolbar;
typedef struct _BtMainToolbarClass BtMainToolbarClass;
typedef struct _BtMainToolbarPrivate BtMainToolbarPrivate;

/**
 * BtMainToolbar:
 *
 * the main toolbar for the editor application
 */
struct _BtMainToolbar {
  GtkToolbar parent;
  
  /*< private >*/
  BtMainToolbarPrivate *priv;
};

struct _BtMainToolbarClass {
  GtkToolbarClass parent;
  
};

GType bt_main_toolbar_get_type(void) G_GNUC_CONST;

BtMainToolbar *bt_main_toolbar_new(void);

#endif // BT_MAIN_TOOLBAR_H
