/* $Id: main-page-waves.h,v 1.1 2004-12-02 10:45:33 ensonic Exp $
 * class for the editor main waves page
 */

#ifndef BT_MAIN_PAGE_WAVES_H
#define BT_MAIN_PAGE_WAVES_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_MAIN_PAGE_WAVES		         (bt_main_page_waves_get_type ())
#define BT_MAIN_PAGE_WAVES(obj)		         (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_MAIN_PAGE_WAVES, BtMainPageWaves))
#define BT_MAIN_PAGE_WAVES_CLASS(klass)	   (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_MAIN_PAGE_WAVES, BtMainPageWavesClass))
#define BT_IS_MAIN_PAGE_WAVES(obj)	       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_MAIN_PAGE_WAVES))
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
  
  /* private */
  BtMainPageWavesPrivate *priv;
};
/* structure of the main-page-waves class */
struct _BtMainPageWavesClass {
  GtkVBoxClass parent;
  
};

/* used by MAIN_PAGE_WAVES_TYPE */
GType bt_main_page_waves_get_type(void);

#endif // BT_MAIN_PAGE_WAVES_H
