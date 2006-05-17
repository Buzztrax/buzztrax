/* $Id: pattern-view.h,v 1.1 2006-05-17 18:32:54 ensonic Exp $
 * class for the pattern view widget
 */

#ifndef BT_PATTERN_VIEW_H
#define BT_PATTERN_VIEW_H

#include <glib.h>
#include <glib-object.h>

#define BT_TYPE_PATTERN_VIEW            (bt_pattern_view_get_type ())
#define BT_PATTERN_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PATTERN_VIEW, BtPatternView))
#define BT_PATTERN_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_PATTERN_VIEW, BtPatternViewClass))
#define BT_IS_PATTERN_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_PATTERN_VIEW))
#define BT_IS_PATTERN_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_PATTERN_VIEW))
#define BT_PATTERN_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_PATTERN_VIEW, BtPatternViewClass))

/* type macros */

typedef struct _BtPatternView BtPatternView;
typedef struct _BtPatternViewClass BtPatternViewClass;
typedef struct _BtPatternViewPrivate BtPatternViewPrivate;

/**
 * BtPatternView:
 *
 * the root window for the editor application
 */
struct _BtPatternView {
  GtkTreeView parent;
  
  /*< private >*/
  BtPatternViewPrivate *priv;
};
/* structure of the sequence-view class */
struct _BtPatternViewClass {
  GtkTreeViewClass parent;
  
};

/* used by PATTERN_VIEW_TYPE */
GType bt_pattern_view_get_type(void) G_GNUC_CONST;

#endif // BT_PATTERN_VIEW_H
