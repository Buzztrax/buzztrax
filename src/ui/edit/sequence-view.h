/* $Id: sequence-view.h,v 1.2 2005-08-05 09:36:18 ensonic Exp $
 * class for the sequence view widget
 */

#ifndef BT_SEQUENCE_VIEW_H
#define BT_SEQUENCE_VIEW_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_SEQUENCE_VIEW             (bt_sequence_view_get_type ())
#define BT_SEQUENCE_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_SEQUENCE_VIEW, BtSequenceView))
#define BT_SEQUENCE_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_SEQUENCE_VIEW, BtSequenceViewClass))
#define BT_IS_SEQUENCE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_SEQUENCE_VIEW))
#define BT_IS_SEQUENCE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_SEQUENCE_VIEW))
#define BT_SEQUENCE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_SEQUENCE_VIEW, BtSequenceViewClass))

/* type macros */

typedef struct _BtSequenceView BtSequenceView;
typedef struct _BtSequenceViewClass BtSequenceViewClass;
typedef struct _BtSequenceViewPrivate BtSequenceViewPrivate;

/**
 * BtSequenceView:
 *
 * the root window for the editor application
 */
struct _BtSequenceView {
  GtkTreeView parent;
  
  /*< private >*/
  BtSequenceViewPrivate *priv;
};
/* structure of the sequence-view class */
struct _BtSequenceViewClass {
  GtkTreeViewClass parent;
  
};

/* used by SEQUENCE_VIEW_TYPE */
GType bt_sequence_view_get_type(void);

#endif // BT_SEQUENCE_VIEW_H
