/* $Id: main-page-sequence.h,v 1.5 2004-10-15 15:39:33 ensonic Exp $
 * class for the editor main sequence page
 */

#ifndef BT_MAIN_PAGE_SEQUENCE_H
#define BT_MAIN_PAGE_SEQUENCE_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_MAIN_PAGE_SEQUENCE		        (bt_main_page_sequence_get_type ())
#define BT_MAIN_PAGE_SEQUENCE(obj)		        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_MAIN_PAGE_SEQUENCE, BtMainPageSequence))
#define BT_MAIN_PAGE_SEQUENCE_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_MAIN_PAGE_SEQUENCE, BtMainPageSequenceClass))
#define BT_IS_MAIN_PAGE_SEQUENCE(obj)	        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_MAIN_PAGE_SEQUENCE))
#define BT_IS_MAIN_PAGE_SEQUENCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_MAIN_PAGE_SEQUENCE))
#define BT_MAIN_PAGE_SEQUENCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_MAIN_PAGE_SEQUENCE, BtMainPageSequenceClass))

/* type macros */

typedef struct _BtMainPageSequence BtMainPageSequence;
typedef struct _BtMainPageSequenceClass BtMainPageSequenceClass;
typedef struct _BtMainPageSequencePrivate BtMainPageSequencePrivate;

/**
 * BtMainPageSequence:
 *
 * the sequence page for the editor application
 */
struct _BtMainPageSequence {
  GtkVBox parent;
  
  /* private */
  BtMainPageSequencePrivate *priv;
};
/* structure of the main-page-sequence class */
struct _BtMainPageSequenceClass {
  GtkVBoxClass parent;
  
};

/* used by MAIN_PAGE_SEQUENCE_TYPE */
GType bt_main_page_sequence_get_type(void);

#endif // BT_MAIN_PAGE_SEQUENCE_H

