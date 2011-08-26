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

#ifndef BT_MAIN_PAGE_WAVES_H
#define BT_MAIN_PAGE_WAVES_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_MAIN_PAGE_WAVES             (bt_main_page_waves_get_type ())
#define BT_MAIN_PAGE_WAVES(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_MAIN_PAGE_WAVES, BtMainPageWaves))
#define BT_MAIN_PAGE_WAVES_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_MAIN_PAGE_WAVES, BtMainPageWavesClass))
#define BT_IS_MAIN_PAGE_WAVES(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_MAIN_PAGE_WAVES))
#define BT_IS_MAIN_PAGE_WAVES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_MAIN_PAGE_WAVES))
#define BT_MAIN_PAGE_WAVES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_MAIN_PAGE_WAVES, BtMainPageWavesClass))

/* type macros */

typedef struct _BtMainPageWaves BtMainPageWaves;
typedef struct _BtMainPageWavesClass BtMainPageWavesClass;
typedef struct _BtMainPageWavesPrivate BtMainPageWavesPrivate;

/**
 * BtMainPageWaves:
 *
 * the pattern page for the editor application
 */
struct _BtMainPageWaves {
  GtkVBox parent;
  
  /*< private >*/
  BtMainPageWavesPrivate *priv;
};

struct _BtMainPageWavesClass {
  GtkVBoxClass parent;
  
};

GType bt_main_page_waves_get_type(void) G_GNUC_CONST;

#include "main-pages.h"

BtMainPageWaves *bt_main_page_waves_new(const BtMainPages *pages);

#endif // BT_MAIN_PAGE_WAVES_H
