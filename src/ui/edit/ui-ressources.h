/* $Id: ui-ressources.h,v 1.10 2007-03-11 20:19:20 ensonic Exp $
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

#ifndef BT_UI_RESSOURCES_H
#define BT_UI_RESSOURCES_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_UI_RESSOURCES            (bt_ui_ressources_get_type ())
#define BT_UI_RESSOURCES(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_UI_RESSOURCES, BtUIRessources))
#define BT_UI_RESSOURCES_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_UI_RESSOURCES, BtUIRessourcesClass))
#define BT_IS_UI_RESSOURCES(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_UI_RESSOURCES))
#define BT_IS_UI_RESSOURCES_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_UI_RESSOURCES))
#define BT_UI_RESSOURCES_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_UI_RESSOURCES, BtUIRessourcesClass))

/* type macros */

typedef struct _BtUIRessources BtUIRessources;
typedef struct _BtUIRessourcesClass BtUIRessourcesClass;
typedef struct _BtUIRessourcesPrivate BtUIRessourcesPrivate;

/**
 * BtUIRessources:
 *
 * a collection of shared ui ressources
 */
struct _BtUIRessources {
  GObject parent;

  /*< private >*/
  BtUIRessourcesPrivate *priv;
};
/* structure of the ui-ressources class */
struct _BtUIRessourcesClass {
  GObjectClass parent; 
};

/**
 * BtUIRessourcesColors:
 *
 * Symbolic color names for the UI. 
 */
typedef enum {
  BT_UI_RES_COLOR_CURSOR=0,
  BT_UI_RES_COLOR_SELECTION1,
  BT_UI_RES_COLOR_SELECTION2,
  BT_UI_RES_COLOR_PLAYLINE,
  BT_UI_RES_COLOR_LOOPLINE,
  BT_UI_RES_COLOR_ENDLINE,
  BT_UI_RES_COLOR_SOURCE_MACHINE_BASE,       /* machine view normal */
  BT_UI_RES_COLOR_SOURCE_MACHINE_BRIGHT1,    /* list view odd */
  BT_UI_RES_COLOR_SOURCE_MACHINE_BRIGHT2,    /* list view even */
  BT_UI_RES_COLOR_SOURCE_MACHINE_DARK1,      /* machine title */
  BT_UI_RES_COLOR_SOURCE_MACHINE_DARK2,      /* --- */
  BT_UI_RES_COLOR_PROCESSOR_MACHINE_BASE,    /* machine view normal */
  BT_UI_RES_COLOR_PROCESSOR_MACHINE_BRIGHT1, /* list view odd */
  BT_UI_RES_COLOR_PROCESSOR_MACHINE_BRIGHT2, /* list view even */
  BT_UI_RES_COLOR_PROCESSOR_MACHINE_DARK1,   /* machine title */
  BT_UI_RES_COLOR_PROCESSOR_MACHINE_DARK2,   /* --- */
  BT_UI_RES_COLOR_SINK_MACHINE_BASE,         /* machine view normal */
  BT_UI_RES_COLOR_SINK_MACHINE_BRIGHT1,      /* list view odd */
  BT_UI_RES_COLOR_SINK_MACHINE_BRIGHT2,      /* list view even */
  BT_UI_RES_COLOR_SINK_MACHINE_DARK1,        /* --- */
  BT_UI_RES_COLOR_SINK_MACHINE_DARK2,        /* --- */
  BT_UI_RES_COLOR_ANALYZER_PEAK,             /* analyzer widnow peak marks */
  BT_UI_RES_COLOR_COUNT
} BtUIRessourcesColors;

/**
 * BtUIRessourcesMachineColors:
 *
 * Symbolic color names for machines. 
 */
typedef enum {
  BT_UI_RES_COLOR_MACHINE_BASE=0,     /* machine view normal */
  BT_UI_RES_COLOR_MACHINE_BRIGHT1,    /* list view odd */
  BT_UI_RES_COLOR_MACHINE_BRIGHT2,    /* list view even */
  BT_UI_RES_COLOR_MACHINE_DARK1,      /* --- */
  BT_UI_RES_COLOR_MACHINE_DARK2       /* --- */  
} BtUIRessourcesMachineColors;

/* used by UI_RESSOURCES_TYPE */
GType bt_ui_ressources_get_type(void) G_GNUC_CONST;

#endif // BT_UI_RESSOURCES_H
