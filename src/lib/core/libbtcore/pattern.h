/* $Id: pattern.h,v 1.9 2004-07-20 18:24:18 ensonic Exp $
 * class for the pattern pattern
 *
 */

#ifndef BT_PATTERN_H
#define BT_PATTERN_H

#include <glib.h>
#include <glib-object.h>

/**
 * BT_TYPE_PATTERN:
 *
 * #GType for BtPattern instances
 */
#define BT_TYPE_PATTERN		         (bt_pattern_get_type ())
#define BT_PATTERN(obj)		         (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PATTERN, BtPattern))
#define BT_PATTERN_CLASS(klass)	   (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_PATTERN, BtPatternClass))
#define BT_IS_PATTERN(obj)	       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_PATTERN))
#define BT_IS_PATTERN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BT_TYPE_PATTERN))
#define BT_PATTERN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BT_TYPE_PATTERN, BtPatternClass))

/* type macros */

typedef struct _BtPattern BtPattern;
typedef struct _BtPatternClass BtPatternClass;
typedef struct _BtPatternPrivate BtPatternPrivate;

/**
 * BtPattern:
 *
 * Class that holds a sequence of events for a #BtMachine.
 * A #BtTimeLineTrack denotes which pattern will be played at which time..
 */
struct _BtPattern {
  GObject parent;
  
  /* private */
  BtPatternPrivate *private;
};
/* structure of the pattern class */
struct _BtPatternClass {
  GObjectClass parent_class;
  
};

/* used by PATTERN_TYPE */
GType bt_pattern_get_type(void);

#endif /* BT_PATTERN_H */

