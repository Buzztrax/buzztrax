/* $Id: main-page-info.h,v 1.3 2004-09-02 17:43:30 ensonic Exp $
 * class for the editor main info page
 */

#ifndef BT_MAIN_PAGE_INFO_H
#define BT_MAIN_PAGE_INFO_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_MAIN_PAGE_INFO		         (bt_main_page_info_get_type ())
#define BT_MAIN_PAGE_INFO(obj)		         (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_MAIN_PAGE_INFO, BtMainPageInfo))
#define BT_MAIN_PAGE_INFO_CLASS(klass)	   (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_MAIN_PAGE_INFO, BtMainPageInfoClass))
#define BT_IS_MAIN_PAGE_INFO(obj)	       (G_TYPE_CHECK_TYPE ((obj), BT_TYPE_MAIN_PAGE_INFO))
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
  GtkVBox parent;
  
  /* private */
  BtMainPageInfoPrivate *private;
};
/* structure of the main-page-info class */
struct _BtMainPageInfoClass {
  GtkVBoxClass parent;
  
};

/* used by MAIN_PAGE_INFO_TYPE */
GType bt_main_page_info_get_type(void);

#endif // BT_MAIN_PAGE_INFO_H

