/* $Id: ui-ressources.h,v 1.2 2005-08-05 09:36:19 ensonic Exp $
 * singleton class that hold shared ui ressources
 */

#ifndef BT_UI_RESSOURCES_H
#define BT_UI_RESSOURCES_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_UI_RESSOURCES             (bt_ui_ressources_get_type ())
#define BT_UI_RESSOURCES(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_UI_RESSOURCES, BtUIRessources))
#define BT_UI_RESSOURCES_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_UI_RESSOURCES, BtUIRessourcesClass))
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

/* used by UI_RESSOURCES_TYPE */
GType bt_ui_ressources_get_type(void);

/* shared singleton class instance */
extern BtUIRessources *ui_ressources;

#endif // BT_UI_RESSOURCES_H
