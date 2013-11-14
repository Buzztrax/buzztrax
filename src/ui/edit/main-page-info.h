/* Buzztrax
 * Copyright (C) 2006 Buzztrax team <buzztrax-devel@buzztrax.org>
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

#ifndef BT_MAIN_PAGE_INFO_H
#define BT_MAIN_PAGE_INFO_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_MAIN_PAGE_INFO             (bt_main_page_info_get_type ())
#define BT_MAIN_PAGE_INFO(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_MAIN_PAGE_INFO, BtMainPageInfo))
#define BT_MAIN_PAGE_INFO_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_MAIN_PAGE_INFO, BtMainPageInfoClass))
#define BT_IS_MAIN_PAGE_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_MAIN_PAGE_INFO))
#define BT_IS_MAIN_PAGE_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_MAIN_PAGE_INFO))
#define BT_MAIN_PAGE_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_MAIN_PAGE_INFO, BtMainPageInfoClass))

/* type macros */

typedef struct _BtMainPageInfo BtMainPageInfo;
typedef struct _BtMainPageInfoClass BtMainPageInfoClass;
typedef struct _BtMainPageInfoPrivate BtMainPageInfoPrivate;

/**
 * BtMainPageInfo:
 *
 * the info page for the editor application
 */
struct _BtMainPageInfo {
  GtkBox parent;
  
  /*< private >*/
  BtMainPageInfoPrivate *priv;
};

struct _BtMainPageInfoClass {
  GtkBoxClass parent;
  
};

GType bt_main_page_info_get_type(void) G_GNUC_CONST;

#include "main-pages.h"

BtMainPageInfo *bt_main_page_info_new(const BtMainPages *pages);

#endif // BT_MAIN_PAGE_INFO_H
