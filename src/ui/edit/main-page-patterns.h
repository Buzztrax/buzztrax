/* $Id: main-page-patterns.h,v 1.4 2004-10-08 13:50:04 ensonic Exp $
 * class for the editor main machines page
 */

#ifndef BT_MAIN_PAGE_PATTERNS_H
#define BT_MAIN_PAGE_PATTERNS_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_MAIN_PAGE_PATTERNS		        (bt_main_page_patterns_get_type ())
#define BT_MAIN_PAGE_PATTERNS(obj)		        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_MAIN_PAGE_PATTERNS, BtMainPagePatterns))
#define BT_MAIN_PAGE_PATTERNS_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_MAIN_PAGE_PATTERNS, BtMainPagePatternsClass))
#define BT_IS_MAIN_PAGE_PATTERNS(obj)	        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_MAIN_PAGE_PATTERNS))
#define BT_IS_MAIN_PAGE_PATTERNS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_MAIN_PAGE_PATTERNS))
#define BT_MAIN_PAGE_PATTERNS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_MAIN_PAGE_PATTERNS, BtMainPagePatternsClass))

/* type macros */

typedef struct _BtMainPagePatterns BtMainPagePatterns;
typedef struct _BtMainPagePatternsClass BtMainPagePatternsClass;
typedef struct _BtMainPagePatternsPrivate BtMainPagePatternsPrivate;

/**
 * BtMainPagePatterns:
 *
 * the pattern page for the editor application
 */
struct _BtMainPagePatterns {
  GtkVBox parent;
  
  /* private */
  BtMainPagePatternsPrivate *priv;
};
/* structure of the main-page-patterns class */
struct _BtMainPagePatternsClass {
  GtkVBoxClass parent;
  
};

/* used by MAIN_PAGE_PATTERNS_TYPE */
GType bt_main_page_patterns_get_type(void);

#endif // BT_MAIN_PAGE_PATTERNS_H

