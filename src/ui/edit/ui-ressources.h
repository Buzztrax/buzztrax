/* $Id: ui-ressources.h,v 1.5 2006-02-17 08:19:57 ensonic Exp $
 * singleton class that hold shared ui ressources
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

typedef enum {
  BT_UI_RES_COLOR_CURSOR=0,
  BT_UI_RES_COLOR_SELECTION1,
  BT_UI_RES_COLOR_SELECTION2,
  BT_UI_RES_COLOR_PLAYLINE,
  BT_UI_RES_COLOR_LOOPLINE,
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
  BT_UI_RES_COLOR_SINK_MACHINE_DARK1,        /* machine title */
  BT_UI_RES_COLOR_SINK_MACHINE_DARK2,        /* --- */
  BT_UI_RES_COLOR_COUNT
} BtUIRessourcesColors;

typedef enum {
  BT_UI_RES_COLOR_MACHINE_BASE=0,     /* machine view normal */
  BT_UI_RES_COLOR_MACHINE_BRIGHT1,    /* list view odd */
  BT_UI_RES_COLOR_MACHINE_BRIGHT2,    /* list view even */
  BT_UI_RES_COLOR_MACHINE_DARK1,      /* machine title */
  BT_UI_RES_COLOR_MACHINE_DARK2       /* --- */  
} BtUIRessourcesMachineColors;

/* used by UI_RESSOURCES_TYPE */
GType bt_ui_ressources_get_type(void) G_GNUC_CONST;

#endif // BT_UI_RESSOURCES_H
