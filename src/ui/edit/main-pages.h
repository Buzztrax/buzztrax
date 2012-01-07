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

#ifndef BT_MAIN_PAGES_H
#define BT_MAIN_PAGES_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_MAIN_PAGES            (bt_main_pages_get_type ())
#define BT_MAIN_PAGES(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_MAIN_PAGES, BtMainPages))
#define BT_MAIN_PAGES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_MAIN_PAGES, BtMainPagesClass))
#define BT_IS_MAIN_PAGES(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_MAIN_PAGES))
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
  
  /*< private >*/
  BtMainPagesPrivate *priv;
};

struct _BtMainPagesClass {
  GtkNotebookClass parent;
  
};

enum {
  BT_MAIN_PAGES_MACHINES_PAGE=0,
  BT_MAIN_PAGES_PATTERNS_PAGE,
  BT_MAIN_PAGES_SEQUENCE_PAGE,
  BT_MAIN_PAGES_WAVES_PAGE,
  BT_MAIN_PAGES_INFO_PAGE
};

GType bt_main_pages_get_type(void) G_GNUC_CONST;

BtMainPages *bt_main_pages_new(void);

#endif // BT_MAIN_PAGES_H
